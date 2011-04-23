#include <signal.h>
#include <sys/types.h>
#include <cstdlib>


#include "panic.h"
#include "io_tools.h"

using namespace std;

void panic() {
	pid_t pid;
	key_t key;
	get_pid(&pid,&key);	
	kill(pid,SIGKILL);
	exit(5);
}

