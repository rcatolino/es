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
#include <cstring>
#include <pthread.h>
#include <iostream>
#include <errno.h>
#include "ripd.h"
#include "io_tools.h"
#include "tcpserver.h"

using namespace std;
static struct sigaction end; 
static pid_t sid;
static pthread_t listening_thread;
//static fstream pipe_in;
static int sh_semid;
static int shmid;
static ofstream out;
static tcpserver data_server;
static tcpserver request_server;

static void end_daemon(int signum){
	out << "endind daemon" << endl;
	write_pid(0,0);
	out << "Deleting shm" << endl;
	shmctl(shmid,IPC_RMID,NULL);
	out << "Deleting sem" << endl;
	semctl(sh_semid,0,IPC_RMID);
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

void* thread_action(void* out) {
	for (unsigned int i = 0 ; i < 5 ; i++) {
		//(*(ofstream*) out) << "thread running" << endl;
		sleep(1);
	}
	pthread_exit(NULL);
	return NULL;

}
void* send_search(void* params) {
	tcpserver send_server;
	int ret = send_server.ini(8081);
	if ( ret != 0) {
		perror("server ini ");
		pthread_exit(NULL);
		return NULL;
	}
	//send_server.send((string *)params);

	pthread_exit(NULL);
	return NULL;

}
void* wait_search(void * params) {
	out << "listening thread launched" << endl;
	request_server.wait(1);
	pthread_exit(NULL);
	return NULL;
}

int search() {
	int desc = 1;
	string name;
	string type;

	while (desc != 0) {
		switch (desc) {
			case 0 :  //nothing to read left;
				break;
			case 1 :  // name :
				out << "name : " << name << endl;
				break;
			case 2 :
				break;
			default :
				return -1;
		}
	}
//	string params[] = {name,type};
	out << name << endl;
	out << type << endl;

/*
	if (pthread_create(&thread, NULL, send_search, (void*) params)) {
		out << "error while creating thread" << endl;
		return -1;
	}*/
	return 0;
}
void * read(void* shm, string * data) {
	int size = ((int*)shm)[0];
	char * src = &((char*)shm)[4];
	char new_data[size];
	strncpy(new_data,src,size);
	*data = new_data;

	return &((char*)shm)[4+size];
}

int init_ripd(int semid) {

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
	struct sembuf take = {0,-1,0};
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
	void * sh_mem = shmat(shmid,NULL,0);
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
	if (semop(semid,&give,1) == -1) {  //continue parent
		perror("semgive failed "); 
	}
	if (ini_handler() == -1) {
		out << "Failed to declare end handler" << endl;
		end_daemon(-4);//fail to declare end signal handler
	}
	data_server.ini(8080);
	request_server.ini(8081);
	
	if (pthread_create(&listening_thread, NULL, wait_search, NULL)) {
		out << "error while creating thread" << endl;
		return -1;
	}
	out << "ripd working" << endl;

	string name;
	string type;
	for(;;){
		//int type;
		out << "Now waiting for message" << endl;
		if (semop(sh_semid,&take,1) == -1) {
			perror("semtake from sh_semid failed ");
			sleep(1); //So that ripd doesn't go mad on failure
		}
//		pipe_in >> type;
		out << "Message received" << endl;
		void * src = sh_mem;
		src = read(src,&name);
		src = read(src,&type);
		out << name << endl;
		out << type << endl;

		/*
			switch (type) {
				case 1 :
					search();
				break;

				default :
					out << "Wrong frame type" << endl;
		*/
	}
	out << "daemon terminated" << endl;
	end_daemon(0);
	return 0;
}
