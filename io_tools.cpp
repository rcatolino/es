
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <string>
#include <vector>
#include <map>


#include "io_tools.h"

using namespace std;


int write_pid(pid_t pid, key_t key){
	ofstream pid_file(ripd_pid,ios::trunc | ios::binary | ios::out);
	if (pid_file.fail()){
		pid_file.clear(ios::failbit);
		//failed to open file
		return -1;
	}
	pid_file.write((char*) &pid, sizeof(pid));
	pid_file.write((char*) &key, sizeof(key));
	pid_file.close();
	return 0;
}

pid_t get_pid(pid_t* pid, key_t* key){
	ifstream pid_file(ripd_pid,ios::in | ios::binary);
	if (pid_file.fail()){
		pid_file.clear(ios::failbit);
		return -1;
	}
	pid_file.read((char*) pid, sizeof(pid_t));
	pid_file.read((char*) key, sizeof(key_t));
	return *pid;
}

int dump(map<string,item> * items) {
	ofstream index(ripd_index,ios::trunc | ios::out);
	if (index.fail()){
		index.clear(ios::failbit);
		return -1;
	}
	for (map<string,item>::iterator it = items->begin() ; it != items->end() ; it++) {
		index << it->first << " " << it->second.type << " " << it->second.path;
		index << " " << it->second.message << endl;
	}
	index.close();
	return 0;
}

int restore(map<string,item> * items) {
	ifstream index(ripd_index,ios::in);
	if (index.fail()){
		index.clear(ios::failbit);
		return -1;
	}
	while (!index.eof()){
		char buff[255];
		index.getline(buff,255,' ');
		string name(buff);
		index.getline(buff,255,' ');
		string type(buff);
		index.getline(buff,255,' ');
		string path(buff);
		index.getline(buff,255);
		string message(buff);
		struct item new_item = {type,path,message};
		if (!name.empty()){
			(*items)[name]=new_item;
		}
	}
	return 0;
}

void display_element(string name, item * element) {
		cout << name << " :" << endl;
		if (!element->type.empty()){
			cout << "\ttype : " << element->type << endl; 
		}
		if (!element->message.empty()){
			cout << "\tcomment : " << element->message << endl;
		}
		cout << "\tfile : " << element->path << endl;
}

