//implementation of the sharing daemon :

#include <unistd.h>
#include <fstream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <signal.h>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <pthread.h>
#include <errno.h>
#include "command.h"
#include "ripd.h"
#include "io_tools.h"
#include "if.h"

using namespace std;
static pid_t sid;
static pthread_t listening_thread;
static int sh_semid;
static int shmid;
static ofstream out;
static struct sigaction end; 
static tcpserver * data;
static tcpserver * request;
static tcpclient * client;
static void * sh_mem;

static void end_daemon(int signum){
	out << "endind daemon" << endl;
	write_pid(0,0);
	out << "saving index" << endl;
	write_index();
	out << "Deleting shm" << endl;
	shmctl(shmid,IPC_RMID,NULL);
	out << "Deleting sem" << endl;
	semctl(sh_semid,0,IPC_RMID);
	out << "Killing data server" << endl;
	data->~tcpserver();
	out << "Killing request server" << endl;
	request->~tcpserver();
	out << "Canceling listening thread" << endl;
	pthread_cancel(listening_thread);
	out << "Joining with listening thread" << endl;
	pthread_join(listening_thread,NULL);
	out << "All clear!" << endl;
	exit (EXIT_SUCCESS);
}
static int ini_handler() {
	sigfillset(&end.sa_mask);//mask any other signals when handling SIGUSR2 
	end.sa_handler = end_daemon;//call to end() function when receiving SIGUSR2 
	end.sa_flags = 0;
	int ret1 = sigaction(SIGUSR2,&end,0);
	int ret2 = sigaction(SIGTERM,&end,0);
	int ret3 = sigaction(SIGINT,&end,0);
	if (ret1 == -1 || ret2 == -1 || ret3 == -1) {
		return -1;
	}
	return 0;
}

vector<string> parse(vector<string> command){
	string body = command[0];
	
	if(body == "add") {
		return parse_params(command,&add_file);
	} else if(body == "display") {
		return display(command);
	} else if(body == "remove") {
		return remove(command);
	} else if(body == "search" || body == "get") {
		return parse_params(command,&search);
	}
	vector<string> ret;
	ret.push_back("unknown command : ");
	ret.push_back(command[0]);
	ret.push_back("\nTry 'help' for a list of available commands");
	return ret;
}

void* thread_action(void* out) {
	for (unsigned int i = 0 ; i < 5 ; i++) {
		//(*(ofstream*) out) << "thread running" << endl;
		sleep(1);
	}
	pthread_exit(NULL);
	return NULL;

}
void* wait_search(void * params) {
	tcpserver * request_server = (tcpserver*) params; 
	out << "listening thread launched" << endl;
	if(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == -1){
		out << "bad arg for setcanceltype" << endl;
	}
	request_server->wait(1);
	pthread_exit(NULL);
	return NULL;
}
int request_received(string name, string address) {
	vector<string> names;
	vector<item> items;
	int socket = client->join(address,LISTENING_PORT);
	if (search(&names,&items,name) > 0) {
		for (unsigned int i = 0; i<items.size(); i++) {
			client->send_results(socket,items[i]);
		}
	} else {
		struct item nothing;
		client->send_results(socket,nothing);
	}
	client->leave(socket);
	return 0;
}

void get_received(string name, string address) {
	vector<string> names;
	vector<item> items;
	int socket = client->join(address,LISTENING_PORT);
	if (search(&names,&items,name) >0) {
		for (unsigned int i = 0; i<items.size(); i++) {
			client->send_file(socket,items[i].path);
		}
	} else {
		client->send_file(socket,"");
	}
	client->leave(socket);
	return;
}

static int write(vector<string> data) {
	void * shm = sh_mem;
	struct sembuf give = {0,1,0};
	struct sembuf take = {0,-1,0};
	int sum = 0;
	int data_size;

	for (unsigned int i = 0; i<data.size(); i++) {
		data_size = data[i].length()+1;
		if (data_size == 0) { break; } // this string is empty, ignore it
		sum += data_size + 4;
		if (sum >= 95) {
			out << "Splitting command" << endl;
			((int*)shm)[0] = -1; //notice remote end that there is more to read 
			semop(sh_semid,&give,1); //unblock remote end
			shm = sh_mem; //move cursor to the begining of memory block
			sum = data_size +4; // reinit the number of bytes written
			semop(sh_semid,&take,1); //block itself until it is safe to write again
		}
		((int*)shm)[0] = data_size;
		char * dest = &((char*)shm)[4];
		strcpy(dest,data[i].c_str());	
		shm = &((char*)shm)[4+data_size];
	}
	sum += 4;
	if (sum >= 100) {
		return sum-4;
	} else {
		((int*)shm)[0] = 0; // notice end of transmission 
	}

	if (semop(sh_semid,&give,1) == -1) {
		perror("semgive to sh_semid failed ");
		return -1;
	} //we wrote somthing -> wake up daemon
	return sum;

}

static int read(vector<string> * data) {
	void * shm = sh_mem;
	struct sembuf take = {0,-1,0};
	struct sembuf give = {0,1,0};
	if (semop(sh_semid,&take,1) == -1) {
		perror("semtake from sh_semid failed ");
	}
	int size = ((int*)shm)[0];
	int sum = 4;
	while (size != 0) {
		if (size == -1) {
			semop(sh_semid,&give,1);//remote end has more to write, unblock it
			shm = sh_mem; //move the cursor
			sum = 0; //reinit sum
			semop(sh_semid,&take,1); //wait until the next memory block has been written
			size = ((int*)shm)[0];
		}
		char * src = &((char*)shm)[4];
		char new_data[size];
		strncpy(new_data,src,size);
		data->push_back(new_data);

		shm = &((char*)shm)[4+size];
		sum += size + 4;
		if (sum >= 100) {
			return sum-4;
		}
		size = ((int*)shm)[0];
	}
	return sum;
}
void resp_received(string message){
	vector<string> data;
	data.push_back(message);
	write(data);
}
void ret_received(string name, string type, string message, string ssize, string address){
	vector<string> data;
	if (name == "end" && ssize == "0.0 B") {
		data.push_back("Sorry, no file was found corresponding to your description\n");
	} else {
		data.push_back(name + " : \n");
		if (type != "empty") {
			data.push_back("\ttype : " + type + "\n");
		}
		data.push_back("\tsize : " + ssize + "\n");
		if (message != "empty") {
			data.push_back("\tnote : " + message + "\n");
		}

		out << name << endl;
		out << ssize << endl;
		out << type << endl;
		out << message << endl;
	}
	write (data);
}

int init_ripd(int semid) {

	read_index(); //restore index from hard drive
	umask (0000);//changing file permission mask
	out.open("ripd.out", ios::out | ios::trunc);// log
	if (out.fail()){
		out.clear(ios::failbit);
		//failed to open log file
		end_daemon (-3);
	}
	sid = setsid();
	if (sid == -1) {
	//fail to obtain a session id => die
		end_daemon(-2);
	} 
	pid_t pid = getpid(); //get it's pid

	struct sembuf give = {0,1,0};
	out << "coucou, comment ca va?" << endl;
	key_t shm_key = ftok("rip",'B');
	key_t sem_key = ftok("rip",'C');

	sh_semid = semget(sem_key,1, IPC_CREAT | IPC_EXCL | 0660);
	if (sh_semid == -1) {
		perror("daemon semget ");
	}
	if (semctl(sh_semid,0,SETVAL,0) == -1) {
		perror("daemon sh_semid semctl ");
	}
	shmid = shmget(shm_key,100,IPC_CREAT | IPC_EXCL | 0660);
	if (shmid == -1) {
		perror("daemon shmid ");
	}
	sh_mem = shmat(shmid,NULL,0);
	if ((int)shmid == -1) {
		perror("daemon shmat ");
	}
	
	out << "bien et toi?" << endl;

	int ret = write_pid(pid,shm_key,sem_key); //write keys
	if(ret){
		//We couldn't notify that we started the daemon successfuly =>
		out << "Cannot write to ripd_pid file, stoping the daemon," << endl;
		out << "please ensure you have write permission and try again" << endl;
		end_daemon(-6);
	}
	if (ini_handler() == -1) {
		out << "Failed to declare end handler" << endl;
		end_daemon(-4);//fail to declare end signal handler
	}
	tcpserver data_server("data.out");
	data = &data_server;
	data_server.ini(DATA_PORT);
	if (semop(semid,&give,1) == -1) {  //continue parent
		perror("semgive failed "); 
	}
	tcpserver request_server("request.out");
	request = &request_server;
	request_server.ini(LISTENING_PORT);
	
	if (pthread_create(&listening_thread, NULL, wait_search, (void*)&request_server)) {
		out << "error while creating thread" << endl;
	}
	sleep(1);
	tcpclient request_client("client.out");
	client = &request_client;
	int right_client = request_client.join("127.0.0.1",LISTENING_PORT);
	set_io(&out,data,request,&request_client,right_client,0);

	out << "ripd working" << endl;

	for(;;){
		out << "Now waiting for message" << endl;
		vector<string> data;
		read(&data);
		out << "Message received" << endl;
		for (unsigned int i = 0; i<data.size(); i++) {
			out << data[i] << endl;
		}
		
		vector<string> response = parse(data);
		if (!response.empty() && response[0]=="WAIT") {
			continue;
		}
		write(response);
	}
	out << "daemon terminated" << endl;
	end_daemon(0);
	return 0;
}

