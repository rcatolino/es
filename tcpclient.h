#ifndef __TCPCLIENT__
#define __TCPCLIENT__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <pthread.h>
#include <set>

using namespace std;
const int ADDR_SIZE = 15;
class tcpclient {
	public :
		tcpclient(string);
		~tcpclient();
		int join(string address, uint16_t port);
		void leave(int socketd);
		int send_request(int socketd, string name);


	private :
		set<int> sockets;
		fstream data;
		struct sockaddr_in server_address;
		char address[ADDR_SIZE];

};

#endif
