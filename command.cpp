//this file contains the implementation of each command
//

#include <iostream>
#include <vector>
#include <string>
#include <map>

using namespace std;

static map<string,string> help_list;

void ini_help(){
	string help = "help [command] : Display help about the required command\n";
	help += "\tIf no 'command' is passed to help, a list of all comands is displayed";

	string quit = "quit : end rip interface\n";
	quit += "\tN.B : this doesn't end the rip daemon, to do so please use the\n";
	quit += "\t'stop' command";

	string start = "start : start the share-daemon\n";
	start += "\tThe daemon has to be running in order to get and send any file.\n";
	start += "\tOnce it's running you can stop it with the 'stop' command.\n";
	start += "\tYou can ask rip to start the daemon at boot time using the 'boo' command";

	help_list["help"]=help;
	help_list["quit"]=quit;
}
			

void help(vector<string> command) {
	if (command.size() == 1) {
	//the help doesn't have any options : let's display a list of all commands
		cout << "Type 'help command' to obtain hep about a particular command";
		cout << endl << endl;
		cout << "\tquit : end rip interface" << endl;
		cout << "\tstart : start the share-daemon" <<endl;
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
int start_daemon(){
	return 0;
}
