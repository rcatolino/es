#if ! defined ( RIPD )
#define RIPD

int init_ripd(int semid);

int request_received();

void* wait_search(void * params);


#endif
