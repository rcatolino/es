//this file contains the implementation of each command
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/sem.h>

#include "ripd.h"
#include "io_tools.h"
#include "panic.h"
#include "if.h"

using namespace std;

static map<string,item> items;
static unsigned int pending_op = 0;
static map<string,string> help_list;
static int msgid = 0;
static fstream pipe_out;

void set_msgid(int new_msgid) {msgid = new_msgid;}

void ini_help(){
	string help = "help [command] : Display help about the required command\n";
	help += "\tIf no 'command' is passed to help, a list of all comands is displayed";

	string quit = "quit : end rip interface\n";
	quit += "\tN.B : this doesn't end the rip daemon, to do so please use the\n";
	quit += "\t'stop' command";

	string start = "start : start the share-daemon\n";
	start += "\tThe daemon has to be running in order to get and send files.\n";
	start += "\tOnce it's running you can stop it with the 'stop' command.\n";
	start += "\tYou can ask rip to start the daemon at boot time using the 'boot' command";

	string stop = "stop : stop the share-daemon\n";
	stop += "\tThe daemon has to be running in order to get and send files.\n";
	stop += "\tTo get it running use the 'start' command.";

	string addh = "add file -n <name> [-t <type>] [-m <comment>] : add file to the index of available files,\n";
	addh += "\twhere file is the path to the file you want to add\n";
	addh += "\tThe options are :\n";
	addh += "\t -n (recomended) : name of the file content, in one word.\n";
	addh += "\t -t (optional) : type of the content, in one word.\n";
	addh += "\t -m (optional) : leave a short message to describe the content.";
	
	string displayh = "\tdisplay [name]+ : display the list of files currently in your index.\n";
	displayh += "\tIf one or more name are precised, display only informations about the index\n";
	displayh += "\tcorresponding entries";

	string removeh = "\tremove [name]+ : remove the corresponding entries from the index.\n";

	string searchh = "\tsearch [-t <type>] [-n <name>]+ : looks for files on remote ends corresponding to given names and types.";

	help_list["help"]=help;
	help_list["quit"]=quit;
	help_list["start"]=start;
	help_list["stop"]=stop;
	help_list["add"]=addh;
	help_list["display"]=displayh;
	help_list["remove"]=removeh;
	help_list["search"]=searchh;
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
		cout << "\tdisplay : display index" << endl;
		cout << "\tremove : remove one or more files" << endl;
		cout << "\tsearch : search a file on remote ends" << endl;
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
	pid_t pid;
	get_pid(&pid,NULL);
	if(kill (pid, SIGUSR2)==0) {
		cout << "Daemon stoped successfully" << endl;	
		write_pid(0,0);
		return false;
	} else {
		cout << "Error while stopping daemon" << endl;
		return true;
	}
}

bool start_daemon(){
	struct sembuf take = {0,-1,0};
	int semid = semget(IPC_PRIVATE,1,IPC_CREAT | 0666);
	if (semid == -1) {
		perror("semget failed ");
	}	
	if ( semctl(semid,0,SETVAL,0) ==-1 ) {
		perror("semctl failed");
	}	//set the sem to 0;
	pid_t pid;
	pid = fork();
	if(pid < 0){
		cout << "Error while starting daemon" << endl;
		return false;
	} else if (pid == 0) {
		//child
		init_ripd(semid);
		return true;
	} else {
		//parent
		if (semop(semid,&take,1) == -1) {
			perror("semtake failed "); // block parent until child put a token in the semaphore
			cout << "semid = " << semid << endl;
		}
		semctl(semid,0,IPC_RMID);
		cout << "Daemon started successfully!" << endl;
		pipe_out.open("rip.com",ios::out);
		if (pipe_out.fail()){
			cout << "rip : Failed to open pipe" << endl;
		}
/*		int write_end;
		get_pid(NULL,&write_end);
		cout << "With write_end = " << hex << key << endl;
		cout << "And msgid = " << dec << msgid << endl;
		if (msgid == -1) {
			cout << "Failed to contact daemon" << endl;
		}
*/		return true;
	}
	return true;
}

unsigned int search(vector<string> * names, vector<item> * elements, string name) {
	map<string,item>::iterator it;
	if (!name.empty()) { //we want something specific
		it = items.find(name);
		if (it != items.end()) { //it exists
			elements->push_back(it->second);
			names->push_back(name);
			return 1;
		} else { //it doesn't
			return 0;
		}
	} else { //we want everything
		for (it = items.begin() ; it != items.end() ; it++) {
			elements->push_back(it->second);
			names->push_back(it->first);
		}
		return elements->size();
	}
	return 0;
}

void search(string name, item to_search) {
	//Unlike display, this command search on remote ends, therefore the daemon should be up and running
	cout << "coucou 0" << endl;
	if ( get_pid(NULL,NULL) <= 0 ) {
		//couldn't read from file, or no daemon running. Maybe it hasn't been created yet.
		cout << "The daemon doesn't seem to be running, you have to start it before searching files" << endl;
		return;
	}
	cout << "coucou 7" << endl;
	if (!to_search.message.empty()){
		cout << "Discarding wrong option '-m'!" << endl;
	}
/*	if (msgid == 0) {
		//we don't have the message box id, not supposed to be possible...
		cout << "Error while contacting daemon" << endl;
		return;
	}
	struct mess test = {to_search.name.c_str(),to_search.type.c_str(),to_search.path.c_str(),0,to_search.message.c_str()};
	struct msgfile mes = {1,test};
	if (msgsnd(msgid, &mes, sizeof(to_search),0) == -1) {
		perror("msgsnd");
	} else {
		cout << "Message send" << endl;
	}*/
	cout << name << endl;
	cout << to_search.type << endl;
	
	if (!name.empty()) {
		pipe_out << 1 << endl;
		pipe_out << name << endl;
	}
	if (pipe_out.fail()) {
		cout << "Failed to write in pipe 1" << endl;
	}
	if (!to_search.type.empty()) {
		pipe_out << 2 << endl;
		pipe_out << to_search.type << endl;
	}
	if (pipe_out.fail()) {
		cout << "Failed to write in pipe 2" << endl;
	}
	pipe_out << 0 << endl;
}
/*
	//Unlike display, this command search on remote ends, therefore the daemon should be up and running
	key_t key;
	if ( get_pid(NULL,&key) <= 0 ) {
		//couldn't read from file, or no daemon running. Maybe it hasn't been created yet.
		cout << "The daemon doesn't seem to be running, you have to start it before searching files" << endl;
		return;
	} else {
		msg
*/

void add_file(string name, item to_add) {
	if (to_add.path.empty()) {
		cout << "You must specify a file" << endl;
		return; 
	}
	string new_name;
	if (name.empty()){
		cout << "Warning : no name indicated, using file name" << endl;
		int pos = to_add.path.rfind('/');
		new_name = to_add.path.substr(pos+1);//use only last part of path
	} else {
		new_name = name;
	}
	ifstream test(to_add.path.c_str(),ios::in);//test wether the file to add exists or not
	if (test.fail()){
		test.clear(ios::failbit);
		cout << new_name << " doesn't exist or you don't have read permission on it." << endl;
		return;
	} else {
		struct stat stats;
		stat(to_add.path.c_str(),&stats);
		to_add.size = stats.st_size; 
		for (;;){
			map<string,item>::iterator it = items.find(new_name); //does a file with 
																  //the same name exists?
			if (it == items.end()) {
				 items[new_name] = to_add;
				 pending_op++;
				 break;
			} else {
				cout << "The following element already has the same name in your index :" << endl;
				cout << it->first << " : ";
				cout << it->second.path << endl;
				cout << "Do you want to rename the new element? (y/n)" << endl;
				char a = cin.get();
				if (a == 'y' || a == 'Y') {
					cout << "New name :" << endl;
					cin >> new_name;
				} else {
					cout << "The file has not been added" << endl;
					break;
				} 
			}
		}
	}
}

int parse_params(vector<string> command, void (*callback)(string, item)) {
	unsigned int i = 1;
	item to_add = {"","","",0,""};
	if (command.size() == 1) {
		callback("",to_add);
		return -1;
	}
	if (command[i][0] == '-' && callback == &add_file){
		cout << "You must specify a file" << endl;
		return 1;
	} else if (callback == &add_file){
		to_add.path = command[i];
		i++;
	}
		string name;
		bool option = true; //are we expecting an option?
		bool message = false; //are we expecting a message?

		for (; i < command.size() ; i++) {
			if (command[i][0] == '-' && option == false) {
				//this is an option and we expected something else :
				command[1] = command[0];
				cout << "Wrong option" << endl;
				help(command); 
				return 1;
			} else if (command[i][0] == '-' && option == true) {
				option = false;
				message = false;
				//this is an option, what kind?
				if (command[i] == "-n") {
					//the following element should be a name
					//store it already, the loop will return at next iteration if
					//it wasn't :
					name = command[i+1];
				} else if (command[i] == "-t") {
					//the following element should be a type
					to_add.type = command[i+1];
				} else if (command[i] == "-m") {
					message = true;
				} else {
					cout << "Unknown option" << endl; 
					command[1] = command[0];
					help(command);
					return 1;
				}
			} else if (message == true) {
				to_add.message += command[i];
				to_add.message += " ";
				option = true; //next element might be an option
			} else if (option == false) {
				option = true;
			}
		}	
		cout << name << endl;
		cout << to_add.type << endl;
		cout << to_add.path << endl;
		cout << to_add.message << endl;
		callback(name, to_add);
	return 0;
}

void display_index(string name) {
	vector<item> elements;
	vector<string> names;
	unsigned int ret = search(&names, &elements, name);
	if (ret == 0) {
		cout << "No entry corresponding to " << name << endl;
	} else {
		for (unsigned int i = 0 ; i < ret ; i++) {
			display_element(names[i],&elements[i]);
		}
	}
}

void display(vector<string> command) {
	if (command.size() < 2) {
		//no name precised
		display_index("");
	} else {
		for (unsigned int i = 1 ; i < command.size() ; i++) {
			display_index(command[i]);
		}
	}
}

void remove_file(string name) {
	if (items.find(name)==items.end()){
		cout << "No match for '" << name << "' in index" << endl;
	} else {
		items.erase(name);
		pending_op++;
		cout << name << " was removed." << endl;
	}
}

void remove(vector<string> command) {
	if (command.size() < 2) {
		cout << "You must specify the name of the element to remove" << endl;
	} else {
		for (unsigned int i = 1 ; i < command.size() ; i++) {
			remove_file(command[i]);
		}
	}
}

void write_index() {
	if (pending_op == 0) {
		return;
	}
	dump(&items);
	pending_op = 0;
}

void read_index() {
	restore(&items);
}
