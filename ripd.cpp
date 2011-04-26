//implementation of the sharing daemon :

#include <unistd.h>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <cstdlib>
#include <pthread.h>
#include <iostream>
#include <errno.h>
#include "ripd.h"
#include "io_tools.h"

using namespace std;
static struct sigaction end; 
static pid_t sid;
static pthread_t thread;

static void end_daemon(int signum){
	write_pid(0,0);
	msgctl(msgid,IPC_RMID,NULL);
	pthread_exit(0);
	if (signum < 0) {
		exit (-signum);
	}
	exit (0);
}

static int ini_handler() {
	sigfillset(&end.sa_mask);//mask any other signals when handling SIGUSR2 
	end.sa_handler = end_daemon;//call to end() function when receiving SIGUSR2 
	end.sa_flags = 0;
	return(sigaction(SIGUSR2,&end,0));
}

void* thread_action(void* out) {
	for (unsigned int i = 0 ; i < 5 ; i++) {
		//(*(ofstream*) out) << "thread running" << endl;
		sleep(1);
	}
	pthread_exit(NULL);
	return NULL;

}
int init_ripd(int semid) {
	struct sembuf give = {0,1,0};
	sid = setsid();
	if (sid == -1) {
	//fail to obtain a session id => die
		end_daemon(-2);
	} 

	umask (0000);//changing file permission mask
	ofstream out("out", ios::out | ios::trunc);// log
	if (out.fail()){
		out.clear(ios::failbit);
		//failed to open log file
		end_daemon (-3);
	}

	pid_t pid = getpid(); //get it's pid
	key_t key = ftok("rip",'r'); //create a key for the message queue
	out << "Msg queue key = " << key << endl;
	msgid = msgget(key,IPC_CREAT | 0777); //create the message queue
	if (msgid == -1) {
		//fail to create msg queue : die
		out << "Failed to create message queue" << endl;
		end_daemon(-5);
	}
	int ret = write_pid(pid,key);
	if (semop(semid,&give,1) == -1) {
		perror("semgive failed "); //restart parent
	}
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
	
	if (pthread_create(&thread, NULL, thread_action, (void*) &out)) {
		out << "error while creating thread" << endl;
	}
	out << "ripd working" << endl;

	for(;;){
		struct msgfile mes;
		if (msgrcv(msgid,&mes,40,0,0) == -1) {
			perror("msg");
		} else {
			cout << "Message received" << endl;
			cout << "name : " << sizeof(mes.test.name) << endl;
			cout << "type : " << sizeof(*(mes.test.type)) << endl;
			cout << "path : " << mes.test.path << endl;
			cout << "size : " << mes.test.size << endl;
		}
		sleep(2);
	}
	out << "daemon terminated" << endl;
	end_daemon(0);
	return 0;
}
