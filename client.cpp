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
#include <string>
#include <sys/msg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#include "client.h"
#include "command.h"
#include "io_tools.h"

using namespace std;

static bool daemon_started = false;

static void parse(vector<string> command){
	string body = command[0];
	if(body == "help"){
		help(command);
		return;
	} else if(body == "start") {
		if (!daemon_started){
			daemon_started = start_daemon();
		} else {
			cout << "The daemon is already running" << endl;
		}
	} else if(body == "stop") {
		if (daemon_started) {
			daemon_started = stop_daemon();
		} else {
			cout << "The daemon isn't running" << endl;
		}
	} else if(body == "add") {
		parse_params(command,&add_file);
	} else if(body == "display") {
		display(command);
	} else if(body == "remove") {
		remove(command);
	} else if(body == "search") {
		parse_params(command,&search);
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

	read_index(); //restore index from hard drive
	int ret = kill(pid,0); //Verify whether the daemon is really running
	if (ret == -1) {
		pid = 0;
		write_pid(0,0);
	}
	if (pid) {
		daemon_started = true;
		cout << "The daemon is up and running under the pid " << pid << " !" << endl;
	
		int sh_semid = semget(sem_key,1,0);
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
		void* sh_mem = shmat(shmid,NULL,0);
		if ((int)shmid == -1) {
			perror("client shmat ");
		}
		set_ids(sh_semid, sh_mem);
	}

	while (!quit){
		cout << ">>";
		string buff;
		char b=' ';
		vector<string> command;

		while (b != '\n'){
			buff = "";
			for (cin.get(b) ; b != '\n' && b != ' ' ; cin.get(b)){
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
