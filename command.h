#if ! defined (COMMAND)
#define COMMAND

#include <vector>
#include <string>
#include "io_tools.h"

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


#endif
