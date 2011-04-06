#if ! defined (COMMAND)
#define COMMAND

#include <vector>
#include <string>

void help(vector<string> command);

void ini_help();

bool start_daemon();

bool stop_daemon();

void add(vector<string> command);

void remove(vector<string> command);


#endif
