//this part implement tools used for logging, used both by the client
//and the daemon.
//
#if ! defined (IO_TOOLS)
#define IO_TOOLS

#include <sys/types.h>
#include <string>
#include <map>

#define ripd_pid "./ripd_sync"
//file where is stored the daemon pid when it's running
#define ripd_log "./ripd_log"//file used to write execution information
#define ripd_index "./index"//file containing the available files

using namespace std;

struct item {
	string type;
	string path;
	string message;
};


int write_pid(pid_t pid, key_t key);

pid_t get_pid(pid_t* pid, key_t* key);

void display_element(string name, item * element);

int dump(map<string,item> * items);
int restore(map<string,item> * items);

#endif