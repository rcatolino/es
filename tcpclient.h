#ifndef __TCPCLIENT__
#define __TCPCLIENT__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <pthread.h>
#include <set>
#include "if.h"

using namespace std;
const int ADDR_SIZE = 15;
class tcpclient {
	public :
		tcpclient(string file);
		~tcpclient();
		int join(string address, uint16_t port);
		void leave(int socketd);
		int send_request(int socketd, string name, string action);
		int send_results(int socketd, item result);
		void send_file(int socketd, string path);


	private :
		set<int> sockets;
		fstream data;
		struct sockaddr_in server_address;
		char address[ADDR_SIZE];

};

#endif
