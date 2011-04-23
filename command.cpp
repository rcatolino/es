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

#include "ripd.h"
#include "io_tools.h"
#include "panic.h"
#include "if.h"

using namespace std;

static map<string,item> items;
static unsigned int pending_op = 0;
static map<string,string> help_list;
static int msgid = 0;

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

	help_list["help"]=help;
	help_list["quit"]=quit;
	help_list["start"]=start;
	help_list["stop"]=stop;
	help_list["add"]=addh;
	help_list["display"]=displayh;
	help_list["remove"]=removeh;
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
	get_pid(&pid,&key);
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
	int semid = semget(IPC_PRIVATE,1,IPC_CREAT);
	semctl(semid,0,0); //set the sem to 0;
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
		semop(semid,&take,1); // block parent until child put a token in the semaphore
		cout << "Daemon started successfully!" << endl;
		key_t key;
		get_pid(NULL,&key);
		msgid = msgget(key,0); 
		if (msgid == -1) {
			cout << "Failed to contact daemon" << endl;
		}
		return true;
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

void search(vector<string> command, int client_msgid) {
	if (msg_id == 0) {
		//we don't have the message box id, let's use the one frome the client
		if (client_msgid == 0) {
			//the client doesn't have any good value either, that's not supose to be possible..
			cout << "Error while contacting daemon" << endl;
		} else {
			msgid = client_msgid;
		}
	}
	//contrary to display this command search on remote ends, therefore the daemon should be up and running
	pid_t pid;
	key_t key;
	if ( get_pid(&pid,&key) <= 0 ) {
		//couldn't read from file, maybe it hasn't been created yet.
		cout << "The daemon doesn't seem to be running, you have to start it before searching files" << endl;
		return;
	}
	/* Have to choos between the two solutions !!!*/
}	

	

void add_file(string name, item to_add) {
	string new_name = name;
	ifstream test(to_add.path.c_str(),ios::in);//test wether the file to add exists or not
	if (test.fail()){
		test.clear(ios::failbit);
		cout << name << " doesn't exist or you don't have read permission on it." << endl;
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

int add(vector<string> command) {
	unsigned int i = 1;
	if (command[i][0] == '-'){
		cout << "You must specify a file" << endl;
		return 1;
	} else {
		string name;
		item to_add;
		bool option = true; //are we expecting an option?
		bool message = false; //are we expecting a message?
		to_add.path = command[i];

		for (i++ ; i < command.size() ; i++) {
			if (command[i][0] == '-' && option == false) {
				//this is an option and we expected something else :
				command[1] = "add";
				cout << "Wrong option" << endl;
				help(command); 
				return 1;
			} else if (command[i][0] == '-' && option == true) {
				option = false;
				message = false;
				//this is an option, what kind?
				if (command[i] == "-n") {
					//the following element should be a name
					//store it already, the command will return at next iteration if
					//it wasn't :
					name = command[i+1];
				} else if (command[i] == "-t") {
					//the following element should be a type
					to_add.type = command[i+1];
				} else if (command[i] == "-m") {
					message = true;
				} else {
					cout << "Unknown option" << endl; 
					command[1] = "add";
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
		if (name.empty()){
			cout << "Warning : no name indicated, using file name" << endl;
			int pos = to_add.path.rfind('/');
			name = to_add.path.substr(pos+1);//use only last part of path
		}
		cerr << name << endl;
		cerr << to_add.path << endl;
		cerr << to_add.type << endl;
		cerr << to_add.message << endl;
		add_file(name, to_add);
	}
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
