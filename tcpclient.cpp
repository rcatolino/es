
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <cstdlib>
#include <errno.h>
#include <cstring>
#include <fstream>
#include <set>
#include "tcpclient.h"
using namespace std;

tcpclient::tcpclient(string file) {
	data.open(file.c_str(),ios::out);
	data << "Creating client" << endl;

	data << "Initializing" << endl;
	struct ifaddrs * iflist;
	if (getifaddrs(&iflist) == -1) {
		data << "Failed to get iflist" << endl;
	}
	data << "Available interfaces :" << endl;
	for (ifaddrs * interface = iflist; interface != NULL; interface = interface->ifa_next){
		if (interface->ifa_addr == NULL || interface->ifa_addr->sa_family != AF_INET) {
			continue;
		}
		struct sockaddr_in* addr;
		addr = (sockaddr_in*)interface->ifa_addr;
	   	inet_ntop(AF_INET, &addr->sin_addr,address,ADDR_SIZE);
		data << "\t" << interface->ifa_name << " : " << address << endl;
		string ad = address;
		string loopback = "127.0.0.1";
		if (ad != loopback) {
			//this is a good address, stop looking any further
			break;
		}
		
	}
	freeifaddrs(iflist);
	data << "Client Initialized" << endl;
}
int tcpclient::join(string remote_address, uint16_t remote_port) {

	uint16_t port = htons(remote_port);
	int socketd;
	socketd = socket(AF_INET, SOCK_STREAM, 0);
	if(socketd == -1) {
		data << "Couldn't create socket" << endl;
		perror("socket "); //fail to create new socket
	}
	int opt = 1;
	if (setsockopt(socketd,SOL_SOCKET,SO_REUSEADDR,&opt,4)) {
		data << "Failed to set options on socket" << endl;
		perror("setsockopt ");
	}
	
	memset(&server_address,0,sizeof(struct sockaddr_in)); //clear struct
	server_address.sin_family = AF_INET; //ipv4

	if (inet_pton(AF_INET,remote_address.c_str(),&server_address.sin_addr) == -1) {
		perror("inet_pton ");
		return -1;
	}
	server_address.sin_port = port; 
	if (connect(socketd,(sockaddr *)&server_address,sizeof(server_address))) {
		perror("connect ");
		return -1;
	}
	sockets.insert(socketd);
	return socketd;

}
void tcpclient::leave(int socketd) {
	shutdown(socketd,SHUT_RDWR);
	close(socketd);
	sockets.erase(sockets.find(socketd));
}

tcpclient::~tcpclient() {

}

int tcpclient::send_request(int socketd, string name) {

	//name += '\n';
	const char* msg = name.c_str();
	string search = "search";
	if (send(socketd,search.c_str(),search.length()+1,0) == -1) {
		perror("send ");
		return -1;
	}
	if (send(socketd,msg,strlen(msg)+1,0) == -1) {
		perror("send ");
		return -1;
	}
	if (send(socketd,address,strlen(address)+1,0) == -1) {
		perror("send address ");
		return -1;
	}
	string end = "end";
	if (send(socketd,end.c_str(),4,0) == -1) {
		perror("send end ");
		return -1;
	}
	sleep(1);
	return 0;
}
