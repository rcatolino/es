#if ! defined (COMMAND)
#define COMMAND

#include <vector>
#include <string>
#include "if.h"

void help(vector<string> command);

void set_msgid(int msgid);

void ini_help();

bool start_daemon();

bool stop_daemon();

void add_file(string name, item to_add);

void parse_params(vector<string> command, void (*callback)(string, item));

void display(vector<string> command);

void remove(vector<string> command);

void write_index();

void read_index();

void search(string name, item to_search);

unsigned int search(vector<string> * names, vector<item> * elements, string name);


#endif
