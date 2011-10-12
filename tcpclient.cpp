
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
#include "if.h"
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

int tcpclient::send_results(int socketd, item result) {

	data << "Sending result" << endl;
	string ret = "ret";
	string end = "end";
	if (send(socketd,ret.c_str(),ret.length()+1,0) == -1) {
		perror("send ");
		return -1;
	}
	if (result.name.length() != 0){
		if (send(socketd,result.name.c_str(),result.name.length()+1,0) == -1) {
			perror("send ");
			return -1;
		}
	} else {
		if (send(socketd,end.c_str(),end.length()+1,0) == -1) {
			perror("send ");
			return -1;
		}
		return 0;
	}
	if (result.name.length() != 0){
		if (result.type.empty()) {result.type = "empty";}
		if (send(socketd,result.type.c_str(),result.type.length()+1,0) == -1) {
			perror("send ");
			return -1;
		}
	}
	if (result.name.length() != 0){
		if (result.message.empty()) {result.message = "empty";}
		if (send(socketd,result.message.c_str(),result.message.length()+1,0) == -1) {
			perror("send ");
			return -1;
		}
	}
	unsigned long size[2];
	size[0] = result.size;
	data << "size : " << size[0] << endl;
	((char*)size)[sizeof(long)]='\0';
	if (send(socketd,(void*)&size, sizeof(long)+1,0) == -1) {
		perror("send ");
		return -1;
	}
	if (send(socketd,address,strlen(address)+1,0) == -1) {
		perror("send address ");
		return -1;
	}
	if (send(socketd,end.c_str(),4,0) == -1) {
		perror("send end ");
		return -1;
	}
	return 0;
}

int tcpclient::send_request(int socketd, string name, string action) {

	//name += '\n';
	data << "Sending request" << endl;
	const char* msg = name.c_str();
	if (send(socketd,action.c_str(),action.length()+1,0) == -1) {
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
	return 0;
}
void tcpclient::send_file(int socketd, string path) {
	data << "Sending file" << endl;
	string msg;
	string end = "OK\0end";
	if (path.empty()){
		msg = "POK";
		if (send(socketd,msg.c_str(),msg.length()+1,0) == -1) {
			perror("send POK");
			return;
		}
		if (send(socketd,end.c_str(),end.length()+1,0) == -1) {
			perror("send POK");
			return;
		}
		return;
	} else {
		sleep(1);
		if (send(socketd,msg.c_str(),msg.length()+1,0) == -1) {
			perror("send OK");
		}
		if (send(socketd,msg.c_str(),msg.length()+1,0) == -1) {
			perror("send OK");
			return;
		}
		if (send(socketd,end.c_str(),end.length()+1,0) == -1) {
			perror("send OK");
			return;
		}
	}
	struct stat stats;
	stat(path.c_str(),&stats);
	data << "file size = "<< stats.st_size << endl;

	char co[3];
	ssize_t ret = recv(socketd, co, 3, 0);
	data << "Remote end ready, sending file" << endl;
	if ( ret == -1 ) {
		data << "Failed to read from socket" << endl;
		switch (errno) {
			case EBADF : 
				data << "\tWrong file descriptor " << socketd << endl;
				break;
			case ECONNREFUSED :
				data << "\tConnection refused" << endl;
				break;
			case EINTR :
				data << "\tReception interupted by signal" << endl;
				break;
			case EINVAL :
				data << "\tInvalid argument" << endl;
				break;
			case ENOTSOCK :
				data << "\tThe descriptor isn't a socket" << endl;
				break;
		}
		return;
	} else if ( ret == 0) {
		data << "Connection closed by peer" << endl;
		return;
	}
	string resp = co;
	if (resp == "NO") {
		data << "The fuckin' bastards refused the file!!" << endl;
		return;
	}
	if (send(socketd,(void*)&stats.st_size,sizeof(off_t),0) == -1) {
		perror("send size");
		return;
	}
	data << "Sending file size" << endl;
	ifstream file(path.c_str(),ios::in);
	char block[BLOCK_SIZE];
	long br;
	for (br = 0; br < stats.st_size; br += BLOCK_SIZE) {
		if (file.fail()){
			data << "Can't read file" << endl;
			return;
		}
		file.read(block,BLOCK_SIZE);
		if (send(socketd,block,BLOCK_SIZE,0) == -1) {
			file.clear(ios::failbit);
			perror("send file");
			return;
		}
	}
	file.read(block,stats.st_size-br);
	if (send(socketd,block,stats.st_size,0) == -1) {
		perror("send file");
		return;
	}
	data << "File sent" << endl;
	shutdown(socketd,SHUT_RDWR);
	close(socketd);
}
