//client.cpp : a simple comand-line interface with following commands :
//	add -n name -t type path-to-file : add a file from your hard-drive to the 
//			index of available files for download
//	remove : remove file from the index (without removing it from hd
//	get [file name] : download the file corresponding to "file name" or display 
//	search -t type -n name : display a list of all the availables files of
//				the given type and/or name (name can be a regexp)

#include <iostream>
#include <vector>
#include <string>

#include "client.h"
#include "command.h"
#include "io_tools.h"

using namespace std;

static bool ini = false; //Has help been initialized yet?
static bool daemon_started = false;

static void parse(vector<string> command){
	string body = command[0];
	if(body == "help"){
		if (! ini){
			ini_help();
			ini = true;
		}
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
		add(command);
	} else if(body == "remove") {
		remove(command);
	} else { 
		cout << "unknown command : " <<command[0]<<endl;
		cout << "Try 'help' for a list of available commands" <<endl;
	}
}

int start(){
	bool quit = false;
	pid_t pid = get_pid();

	cout << "Welcome to rip! Use help to get a list of available commands";
	cout << endl;
	if (pid) {
		daemon_started = true;
		cout << "The daemon is up and running under the pid " << pid << " !" << endl;
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
	return 1; //the user stopped the ihm
}

int main(int argc, char ** argv){
	return start();
}
