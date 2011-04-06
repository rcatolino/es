#include <signal.h>
#include <sys/types.h>
#include <cstdlib>


#include "panic.h"
#include "io_tools.h"

using namespace std;

void panic() {
	pid_t pid = get_pid();	
	kill(pid,SIGKILL);
	exit(5);
}

