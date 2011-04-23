#if ! defined (COMMAND)
#define COMMAND

#include <vector>
#include <string>
#include "if.h"

void help(vector<string> command);

void ini_help();

bool start_daemon();

bool stop_daemon();

void add_file(string name, item to_add);

void add(vector<string> command);

void display(vector<string> command);

void remove(vector<string> command);

void write_index();

void read_index();

void search(vector<string command, int msgid);

unsigned int search(vector<string> * names, vector<item> * elements, string name);


#endif
