//this file contains the implementation of each command
//

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "ripd.h"
#include "io_tools.h"
#include "panic.h"

using namespace std;

static map<string,string> help_list;
static pid_t pid;

void ini_help(){
	string help = "help [command] : Display help about the required command\n";
	help += "\tIf no 'command' is passed to help, a list of all comands is displayed";

	string quit = "quit : end rip interface\n";
	quit += "\tN.B : this doesn't end the rip daemon, to do so please use the\n";
	quit += "\t'stop' command";

	string start = "start : start the share-daemon\n";
	start += "\tThe daemon has to be running in order to get and send files.\n";
	start += "\tOnce it's running you can stop it with the 'stop' command.\n";
	start += "\tYou can ask rip to start the daemon at boot time using the 'boo' command";

	string stop = "stop : stop the share-daemon\n";
	stop += "\tThe daemon has to be running in order to get and send files.\n";
	stop += "\tTo get it running use the 'start' command.\n";

	string addh = "add file -n <name> [-t <type>] [-m <comment>] : add file to the index of available files,\n";
	addh += "\twhere file is the path to the file you want to add\n";
	addh += "\tThe options are :\n";
	addh += "\t -n : name of the file content.\n";
	addh += "\t -t (optional) : type of the content.\n";
	addh += "\t -m (optional) : leave a short message to describe the content.\t";

	help_list["help"]=help;
	help_list["quit"]=quit;
	help_list["start"]=start;
	help_list["stop"]=stop;
	help_list["add"]=addh;
}
			

void help(vector<string> command) {
	if (command.size() == 1) {
	//the help doesn't have any options : let's display a list of all commands
		cout << "Type 'help command' to obtain hep about a particular command";
		cout << endl << endl;
		cout << "\tquit : end rip interface" << endl;
		cout << "\tstart : start the share-daemon" << endl;
		cout << "\tstop : stop the share-daemon" << endl;
		cout << "\tadd : add a file to the index" << endl;
		cout << endl;
	} else {
		if(command.size() > 2) {
			cout << "Displaying help for the '" << command[1] <<"' command" << endl;
		}
		cerr << command[1] <<endl;
		map<string,string>::iterator it = help_list.find(command[1]);
		if (it != help_list.end()){
			cout << it->second <<endl;
		} else {
			cout << "Unknown command, "<< command[1] << endl;
			cout << "Try help for a list of available commands" << endl;
		}
	}
}

bool stop_daemon(){
	pid = get_pid();
	if(kill (pid, SIGUSR2)==0) {
		cout << "Daemon stoped successfully" << endl;	
		write_pid(0);
		return false;
	} else {
		cout << "Error while stopping daemon" << endl;
		return true;
	}
}

bool start_daemon(){
	pid = fork();
	if(pid < 0){
		cout << "Error while starting daemon" << endl;
		if(write_pid(0)){
			panic();//We couldn't start the daemon and we couldn't write to the file
						//something is wrong => abort
		}
		return false;
	} else if (pid == 0) {
		init_ripd();
		return true;
	} else {
		cout << "Daemon started successfully!" << endl;
		if(write_pid(pid)){
			//We couldn't notify that we started the daemon successfuly =>
			cout << "Cannot write to ripd_pid file, stoping the daemon," << endl;
			cout << "please ensure you have write permission and try again" << endl;
			if(!stop_daemon()){
				//nothing works : clean everything we can clean and abort
				panic();
			}
			return false;
		} else { 
			return true;
		}
	}
}

void add(vector<string> command) {
	if (command.size() < 2) {
		cout << "You must specify some file name!" << endl;
	} else {
		for (unsigned int i = 1 ; i < command.size() ; i++) {
			if (write_file(command[i]) == -1){
					cout << "Failed to write to the index" << endl;
					break;//we can't write on index, no use to keep tryin'
			}	
		}
	}
}

void remove(vector<string> command) {
	/*
	if (command.size() < 2) {
		cout << "You must specify the file name to remove" << endl;
	} else {
		for i
		*/
}
