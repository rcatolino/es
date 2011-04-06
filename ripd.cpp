//implementation of the sharing daemon :

#include <unistd.h>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <cstdlib>

#include "ripd.h"

static pid_t sid;
static struct sigaction end; 

static void end_daemon(int signum){
	exit (0);
}

static int ini_handler() {
	sigfillset(&end.sa_mask);//mask any other signals when handling SIGUSR2 
	end.sa_handler = end_daemon;//call to end() function when receiving SIGUSR2 
	end.sa_flags = 0;
	return(sigaction(SIGUSR2,&end,0));
}

int init_ripd() {
	sid = setsid();
	if (sid == -1) {
	//fail to obtain a session id => die
		exit(2);
	} 
	umask (0666);//changing file permission mask

	if (ini_handler() == -1) {
		exit(3);//fail to declare end signal handler
	}

	for(;;){
		sleep(10);
		//event loop
	}
	return 0;
}


