
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <string>
#include <map>


#include "io_tools.h"

using namespace std;
static map<string,item> items;

int write_pid(pid_t pid){
	ofstream pid_file(ripd_pid,ios::trunc | ios::binary | ios::out);
	if (pid_file.fail()){
		pid_file.clear(ios::failbit);
		//failed to open file
		return 1;
	}
	pid_file.write((char*) &pid, sizeof(pid));
	pid_file.close();
	return 0;
}

pid_t get_pid(){
	ifstream pid_file(ripd_pid,ios::in | ios::binary);
	if (pid_file.fail()){
		pid_file.clear(ios::failbit);
		return -1;
	}
	pid_t pid;
	pid_file.read((char*) &pid, sizeof(pid));
	return pid;
}

int write_file(string file_name) {
	ofstream index(ripd_index,ios::app | ios::out);
	if (index.fail()){
		index.clear(ios::failbit);
		return -1;
	}
	ifstream test(file_name.c_str(),ios::in);
	if (test.fail()){
		test.clear(ios::failbit);
		cout << file_name << " doesn't exist or you don't have read permission on it." << endl;
		return -2;
	} else {
		index << file_name << "\n";
		index.close();
		return 0;
	}
}
