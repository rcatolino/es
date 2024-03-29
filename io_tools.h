//this part implement tools used for logging, used both by the client
//and the daemon.
//
#if ! defined (IO_TOOLS)
#define IO_TOOLS

#include <sys/types.h>
#include <string>
#include <vector>
#include <map>
#include "if.h"

#define ripd_pid "./ripd.sync"
//file where is stored the daemon pid when it's running
#define ripd_log "./ripd.log"//file used to write execution information
#define ripd_index "./index"//file containing the available files

using namespace std;

int write_pid(pid_t pid, key_t shm_key = 0, key_t sem_key = 0 );

pid_t get_pid(pid_t* pid, key_t* shm_key = NULL, key_t* sem_key = NULL );

void display_element(string name, item * element);

int dump(map<string,item> * items);

int restore(map<string,item> * items);

#endif
