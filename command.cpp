//this file contains the implementation of each command
//

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#include "tcpserver.h"
#include "tcpclient.h"
#include "ripd.h"
#include "io_tools.h"
#include "panic.h"
#include "if.h"

using namespace std;

static map<string,item> items;
static unsigned int pending_op = 0;
static map<string,string> help_list;
static ofstream * out;
static tcpserver * data;
static tcpserver * request;
static tcpclient * client;
static int right_client;
static int left_client;

void set_io(ofstream * new_out, tcpserver * new_data, tcpserver * new_request, tcpclient * new_client, int new_right, int new_left) {
	data = new_data;
	request = new_request;
	client = new_client;
	out = new_out;
	left_client = new_left;
	right_client = new_right;
}

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

int start_daemon(int * sh_semid, void** sh_mem){
	struct sembuf take = {0,-1,0};
	int semid = semget(IPC_PRIVATE,1,IPC_CREAT | 0666);
	if (semid == -1) {
		perror("semget failed ");
	}	
	if (semctl(semid,0,SETVAL,0) ==-1 ) {
		perror("semctl failed");
	}	//set the sem to 0;
	pid_t pid;
	pid = fork();
	if(pid < 0){
		cout << "Error while starting daemon" << endl;
		return 0;
	} else if (pid == 0) {
		//child
		init_ripd(semid);
	} else {
		//parent
		if (semop(semid,&take,1) == -1) {
			perror("semtake failed "); // block itself until child put a token in the semaphore : wait for child to write in sync file
			cout << "semid = " << semid << endl;
		}
		semctl(semid,0,IPC_RMID);
		cout << "Daemon started successfully!" << endl;
		key_t shm_key;
		key_t sem_key;
		if (get_pid(NULL,&shm_key,&sem_key) == 0){
			cout << "Error while getting keys" << endl;
		}
		*sh_semid = semget(sem_key,1,0);
		if (*sh_semid == -1) {
			perror("client semget ");
		}
		if (semctl(*sh_semid,0,SETVAL,0) == -1) {
			perror("client sh_semid semctl ");
		}
		int shmid = shmget(shm_key,100,0);
		if (shmid == -1) {
			perror("client shmid ");
		}
		*sh_mem = shmat(shmid,NULL,0);
		if (*(int*)sh_mem == -1) {
			perror("client shmat ");
		}
		return pid;
	}
	return pid;
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

vector<string> search(string name, item to_search) {
	vector<string> ret;
	//Unlike display, this command search on remote ends, therefore the daemon should be up and running
	
	if (!to_search.message.empty()){
		ret.push_back("Discarding wrong option '-m'!\n");
	}
	int name_size = name.length()+1;
	int type_size = to_search.type.length()+1;
	if (name_size + type_size + 8 >= 256) {
		//not enough room in shm
		ret.push_back("Name and/or type too long\n");
		return ret;
	}
	if (client->send_request(right_client, name) == -1){
		ret.push_back("Failed to send request to right peer\n");
	} else {
		ret.push_back("Searching...\n");
	}
	return ret;
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

vector<string> add_file(string name, item to_add) {
	vector<string> ret;
	if (to_add.path.empty()) {
		ret.push_back("You must specify a file\n");
		return ret; 
	}
	string new_name;
	if (name.empty()){
		ret.push_back("Warning : no name indicated, using file name\n");
		int pos = to_add.path.rfind('/');
		new_name = to_add.path.substr(pos+1);//use only last part of path
	} else {
		new_name = name;
	}
	ifstream test(to_add.path.c_str(),ios::in);//test wether the file to add exists or not
	if (test.fail()){
		test.clear(ios::failbit);
		ret.push_back(new_name);
		ret.push_back(" doesn't exist or you don't have read permission on it.\n");
		return ret;
	} else {
		struct stat stats;
		stat(to_add.path.c_str(),&stats);
		to_add.size = stats.st_size; 
		for (int i = 2 ;; i++){
			map<string,item>::iterator it = items.find(new_name); //does a file with 
																  //the same name exists?
			if (it == items.end()) {
				items[new_name] = to_add;
				pending_op++;
				break;
			} else {
				ret.push_back("The following element already has the same name in your index :\n");
				ret.push_back(it->first); 
				ret.push_back(" : ");
				ret.push_back(it->second.path);
				ret.push_back("\nRenaming entry\n");
				new_name += "(1)";
			}
		}
	}
	return ret;
}

vector<string> parse_params(vector<string> command, vector<string> (*callback)(string, item)) {
	unsigned int i = 1;
	vector<string> ret;
	item to_add = {"","","",0,""};
	if (command.size() == 1) {
		return callback("",to_add);
	}
	if (command[i][0] == '-' && callback == &add_file){
		ret.push_back("You must specify a file\n");
		return ret;
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
			ret.push_back("Wrong option\n");
			help(command); 
			return ret;
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
				ret.push_back("Unknown option\n"); 
				command[1] = command[0];
				help(command);
				return ret;
			}
		} else if (message == true) {
			to_add.message += command[i];
			to_add.message += " ";
			option = true; //next element might be an option
		} else if (option == false) {
			option = true;
		}
	}	
		
	return callback(name, to_add);
}

vector<string> display_index(string name) {
	vector<item> elements;
	vector<string> names;
	vector<string> disp;
	unsigned int ret = search(&names, &elements, name);
	if (ret == 0) {
		disp.push_back("No entry corresponding to " + name + "\n");
	} else {
		for (unsigned int i = 0 ; i < ret ; i++) {
			disp.push_back(names[i] + " :\n");
			if (!elements[i].type.empty()){
				disp.push_back("\ttype : " + elements[i].type + "\n"); 
			}
			if (!elements[i].message.empty()){
				disp.push_back("\tnote : " + elements[i].message + "\n");
			}
			disp.push_back("\tfile : " + elements[i].path + "\n");
		}
	}
	return disp;
}

vector<string> display(vector<string> command) {
	vector<string> ret;
	if (command.size() < 2) {
		//no name precised
		return display_index("");
	} else {
		for (unsigned int i = 1 ; i < command.size() ; i++) {
			vector<string> disp = display_index(command[i]);
			for (unsigned int j = 0; j < disp.size(); j++) {
				ret.push_back(disp[j]);
			}
		}
	}
	return ret;
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

vector<string> remove(vector<string> command) {
	vector<string> ret;
	if (command.size() < 2) {
		ret.push_back("You must specify the name of the element to remove\n");
	} else {
		for (unsigned int i = 1 ; i < command.size() ; i++) {
			if (items.find(command[i])==items.end()){
				ret.push_back("No match for '" + command[i] + "' in index\n");
			} else {
				items.erase(command[i]);
				pending_op++;
				ret.push_back(command[i] + " was removed.\n");
			}
		}
	}
	return ret;
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

