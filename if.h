#ifndef __IF__
#define __IF__

#include <string>
#include <sys/sem.h>

#define LISTENING_PORT 8080
#define DATA_PORT 8081
#define BLOCK_SIZE 2048
using namespace std;
struct item {
	string name;
	string type;
	string path;
	unsigned long size;
	string message;
};
struct mess {
	const char * name;
	const char * type;
	const char * path;
	long size;
	const char * message;
};
struct msgfile {
	long mtype;
	struct mess test;
};	

#endif
