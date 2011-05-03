//client.cpp : a simple comand-line interface with following commands :
//	add -n name -t type path-to-file : add a file from your hard-drive to the 
//			index of available files for download
//	remove : remove file from the index (without removing it from hd
//	get [file name] : download the file corresponding to "file name" or display 
//	search -t type -n name : display a list of all the availables files of
//				the given type and/or name (name could be a regexp)

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <string>
#include <sys/msg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>

#include "client.h"
#include "command.h"
#include "io_tools.h"

using namespace std;

static bool daemon_started = false;
static struct sigaction end_daemon; 
static int sh_semid = 0;
static void* sh_mem;
static void dead_child(int signum) {
	int status;
	waitpid(-1,&status,WNOHANG);
}	

static int ini_handler() {
	sigfillset(&end_daemon.sa_mask);//mask any other signals when handling SIGUSR2 
	end_daemon.sa_handler = dead_child;//call to dead_child() function when receiving SIGUSR2 
	end_daemon.sa_flags = 0;
	int ret1 = sigaction(SIGCHLD,&end_daemon,0);
	return ret1;
}

int write(vector<string> data) {
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
			cout << "Splitting command" << endl;
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

int read(vector<string> * data) {
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

static void parse(vector<string> command){
	string body = command[0];
	if(body == "help"){
		help(command);
		return;
	} else if(body == "start") {
		if (!daemon_started){
			daemon_started = start_daemon(&sh_semid,&sh_mem);
		} else {
			cout << "The daemon is already running" << endl;
		}
	} else if(body == "stop") {
		if (daemon_started) {
			daemon_started = stop_daemon();
		} else {
			cout << "The daemon isn't running" << endl;
		}
	} else if(body == "add" || body == "display" || body == "remove" || body == "search" ) {
		if ( get_pid(NULL) <= 0 ) {
			//couldn't read from file, or no daemon running. Maybe it hasn't been created yet.
			cout << "The daemon doesn't seem to be running, you have to start it before searching files" << endl;
			return;
		}
		write(command);
		vector<string> response;
		read(&response);
		for (unsigned int i = 0; i < response.size() ; i++) {
			cout << response[i] ;
		}
	} else { 
		cout << "unknown command : " <<command[0]<<endl;
		cout << "Try 'help' for a list of available commands" <<endl;
	}
}

int start(){
	bool quit = false;
	key_t sem_key;
	key_t shm_key;
	pid_t pid;
	get_pid(&pid,&shm_key,&sem_key); //get pid of running daemon, and keys. 0 if no daemon running

	cout << "Welcome to rip! Use help to get a list of available commands";
	cout << endl;

	ini_help(); //initialize help
	if (ini_handler() == -1) {
		cout << "Couldn't declare SIGCHLD handler" << endl;
	}

	read_index(); //restore index from hard drive
	int ret = kill(pid,0); //Verify whether the daemon is really running
	if (ret == -1) {
		pid = 0;
		write_pid(0,0);
	}
	if (pid) {
		daemon_started = true;
	
		sh_semid = semget(sem_key,1,0);
		if (sh_semid == -1) {
			perror("client semget ");
		}
		if (semctl(sh_semid,0,SETVAL,0) == -1) {
			perror("client sh_semid semctl ");
		}
		int shmid = shmget(shm_key,100,0);
		if (shmid == -1) {
			perror("client shmid ");
		}
		sh_mem = shmat(shmid,NULL,0);
		if ((int)shmid == -1) {
			perror("client shmat ");
		}

	} else {
		cout << "Starting daemon..." << endl;
		pid = start_daemon(&sh_semid,&sh_mem);
		daemon_started = pid;
	}
	cout << "The daemon is running under the pid " << pid << " !" << endl;


	while (!quit){
		cout << ">>";
		string buff;
		char b='a';
		vector<string> command;

		while (b != '\n'){
			buff = "";
			for (cin.get(b) ; b != '\n' && b != ' ' ; cin.get(b)){
				if (cin.fail() || cin.eof()) {
					cin.clear();
					buff = "";
					break;
				}
				buff += b;//append next char to buf
			}
			if (!buff.empty()){
				command.push_back(buff);
			}
		}

		if (command.empty()){
			continue;
		}else if (command[0] == "quit"){
			quit = true; //determine wether we have to quit or continue
		}else{
			parse(command);
		}

	}
	write_index();
	return 1; //the user stopped the ihm
}

int main(int argc, char ** argv){
	return start();
}
