#if ! defined (COMMAND)
#define COMMAND

#include <vector>
#include <string>
#include <fstream>
#include "tcpserver.h"
#include "tcpclient.h"
#include "if.h"

void help(vector<string> command);

void set_ids(int sh_semid, void* sh_mem);

void set_io(ofstream * out, tcpserver * data, tcpserver * requests, tcpclient * client, int right, int left);

void ini_help();

int start_daemon(int * sh_semid, void** sh_mem);

bool stop_daemon();

vector<string> add_file(string name, item to_add);

vector<string> parse_params(vector<string> command, vector<string> (*callback)(string, item));

vector<string> display(vector<string> command);

vector<string> remove(vector<string> command);

void write_index();

int write(vector<string> data);

void read_index();

vector<string> search(string name, item to_search);

unsigned int search(vector<string> * names, vector<item> * elements, string name);


#endif
