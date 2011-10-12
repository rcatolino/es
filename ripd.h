#if ! defined ( RIPD )
#define RIPD
#include <string>
using namespace std;
int init_ripd(int semid);

int request_received(string name, string address);

void ret_received(string name, string type, string message, string ssize, string address);

void get_received(string name, string address);

void resp_received(string message);

void* wait_search(void * params);


#endif
