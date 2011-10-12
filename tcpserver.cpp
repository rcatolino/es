
//lol

#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <cstdlib>
#include <netinet/in.h>
#include <errno.h>
#include <cstring>
#include <fstream>
#include "ripd.h"
#include "if.h"
#include "tcpserver.h"

#define MAX_CON 6
using namespace std;
tcpserver::tcpserver(string file){
	data.open(file.c_str(),ios::out);
	data << "Creating server" << endl;
	pthread_mutex_init(&join_mutex, NULL);
	pthread_cond_init(&join_cond, NULL);
	for (int i = 0; i<= max_size ; requests_socketd[i++] = 0 ); // fill requests_socketd with 0
	if (pthread_create(&collector, NULL, tcpserver::start_thread_collector, this)) {
		collector = 0;
		return;
	}
	sleep(1);
}

tcpserver::~tcpserver(){
	data << "Closing sockets desc" << endl;
	shutdown(listen_socketd,SHUT_RDWR);
	close(listen_socketd);
	for (int  index = 0 ; index < max_size ; index++) {
		shutdown(requests_socketd[index],SHUT_RDWR);
		close(requests_socketd[index]);
	}
	data << "Destroying join_condition" << endl;
	pthread_cond_destroy(&join_cond);
	data << "Freeing mutex" << endl;
	pthread_mutex_destroy(&join_mutex);
	data << "Canceling collector" << endl;
	pthread_cancel(collector);
	data << "Waiting to join with collector..." << endl;
	pthread_join(collector,NULL);
	data << "Exiting server" << endl;
}

int tcpserver::ini(uint16_t listen_port){

	port = htons(listen_port);

	data << "Server initializing" << endl;

	listen_socketd = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_socketd == -1) {
		data << "Couldn't create socket" << endl;
		return errno; //fail to create new socket
	}
	int opt = 1;
	if (setsockopt(listen_socketd,SOL_SOCKET,SO_REUSEADDR,&opt,4)) {
		data << "Failed to set options on socket" << endl;
		perror("setsockopt ");
	}
	
	memset(&own_address,0,sizeof(struct sockaddr_in)); //clear struct
	own_address.sin_family = AF_INET; //ipv4
	own_address.sin_port = port; //listening port
	own_address.sin_addr.s_addr = INADDR_ANY; //own address... 

	if (bind(listen_socketd, (struct sockaddr *) &own_address, sizeof(own_address)) < 0) {
		data << "Couldn't bind socket" << endl;
		//fail to name socket;
		return errno;
	}
	data << "Server Initialized" << endl;
	return 0;
}

file_details tcpserver::wait(int file_number) { 
	struct file_details details;
	pthread_t connection_thread;
	listen(listen_socketd,MAX_CON);
	data << "Server listening on port " << ntohs(port) << endl;
	socklen_t client_size = sizeof(client_address);
	thread_params params; 
	int index = 0;

	for (;;) {

		for (index = 0; requests_socketd[index] != 0; index++); //find next free space in array

		requests_socketd[index] = accept(listen_socketd,(struct sockaddr *) &client_address, &client_size);

		if (index >= max_size-1) {
			data << "Too many connections" << endl;
			shutdown(requests_socketd[index],SHUT_RDWR);
			close(requests_socketd[index]);
			requests_socketd[max_size-1] = 0;
			break;
		} //Check whether the array is full

		data << "New pending connection" << endl;
		if ( requests_socketd[index] < 0 ) {
			details.file_length = errno;
			data << "Failed to accept new connection" << endl;
			return details;
			//fail to accept incoming connection
		}
		data << "New client connected on socket " << requests_socketd[index] << endl;
		data << "\tremote address : " << inet_ntoa(client_address.sin_addr) << endl;
		data << "\tremote port : " << ntohs(client_address.sin_port) << endl;

		//new connection 
		
		params.obj = this;
		params.it = index;
		if (pthread_create(&connection_thread, NULL, start_thread_request, &params)) {
			details.file_length = errno;
			data << "error while creating connection thread" << endl;
			return details;
		}
		sleep(1);
	}
	return details;
	
/*	if (read_fd(&details) < 0) {
		//reading error
		details.file_length = errno;
		return details;
	} else {
		return details;
	}*/
}
int tcpserver::read_fd(struct file_details* details, int file_socket){
	int flength;
	ssize_t ret = recv(file_socket, (void*)&flength, sizeof(flength), 0);
	if ( ret < (int) sizeof(flength)) {
		//failed to read the socket or not enough bytes read
		return -1;
	} else { 
		//we manage to read the correct value
		details->file_length = flength;
	}
	int plength;
	ret = recv(file_socket, (void*)&plength, sizeof(plength), 0);
	if ( ret < (int) sizeof(plength)) {
		return -1;
	} else { 
		details->packet_length = plength;
	}
	int pnumber;
	ret = recv(file_socket, (void*)&pnumber, sizeof(pnumber), 0);
	if ( ret < (int) sizeof(pnumber)) {
		return -1;
	} else { 
		details->packet_number = pnumber;
	}
	char name[50]; 
	ret = recv(file_socket, (void*) name, 50, 0);
	if ( ret < 0 ) {
		//failed to read the socket
		return -1;
	} else { 
		//we managed to read a value
		details->file_name = name;
	}
	return 0;

}

void tcpserver::request(int index) {
	int it = requests_socketd[index];
	data << "New request thread launched on socket : " << it << " (as seen by args)" << endl; 
	char req[BUFF];
	for (;;) {
		ssize_t ret = recv(it, (void*)req, BUFF, 0);
		if ( ret == -1 ) {
			data << "Failed to read from socket" << endl;
			switch (errno) {
				case EBADF : 
					data << "\tWrong file descriptor " << it << endl;
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
			break;
		} else if ( ret == 0) {
			data << "Connection closed by peer" << endl;
			break;
		}
		string command[10];
		string disp;
		data << "Char received : " << ret << "on socket : " << it << endl;
		int j = 0;
		int i = 0;
		int u = 0;
		do {
			for (; i<ret && req[i]!='\0' ; i++) {
			command[j] += req[i];
			}
			i++;
			if (command[j].empty()) {
			} else {
				data << command[j] << endl;
				u = j;
				j++;
			}
		} while (command[u] != "end" && i<ret);
		if ( command[0] == "search" ) {
			request_received( command[1], command[2]);
		} else if ( command[0] == "ret") {
			char size[100];
			unsigned long lsize = ((unsigned long*)(command[4].c_str()))[0];
			data << lsize << endl;
			double dsize = (double)lsize;
			data << dsize << endl;
			string sf;
			if (lsize > 1024*1024*1024) {
				dsize /= 1024*1024*1024;
				sf = " GB";
			} else if (lsize > 1024*1024) {
				dsize /= 1024*1024;
				sf = " MB";
			} else if (lsize > 1024) {
				dsize /= 1024;
				sf = " kB";
			} else {
				sf = " B";
			}
			sprintf(size,"%.1f",dsize);
			ret_received( command[1], command[2], command[3], size + sf, command[5]);
		} else if ( command[0] == "get") {
			get_received( command[1], command[2]);
		} else if ( command[0] == "POK" ) {
			resp_received ( "File doesn't exist\n" );
		} else if ( command[0] == "OK" ) {
			resp_received ( "Downloading file\n") ;
			dl_file(it);
		}

	}
	sleep(1);
	data << "Connection closed, closing socket..." << it << endl;
	shutdown(it,SHUT_RDWR);
	close(it);
	data << "removing connection from list" << endl;
	requests_socketd[index] = 0;
	data << "Signaling end of connection to collector" << endl;
	pthread_mutex_lock(&join_mutex);
	to_join = pthread_self();
	data << "Thread " << to_join << " dying" << endl;
	pthread_cond_signal(&join_cond);
	pthread_mutex_unlock(&join_mutex);
	data << "Signal sent" << endl;
	pthread_exit(NULL);
}
void tcpserver::dl_file(int it) {
	string msg = "GO";
	if (send(it,msg.c_str(),msg.length()+1,0) == -1) {
		perror("send OK");
		return;
	}
	off_t file_size;
	ssize_t ret = recv(it, (void*)&file_size, sizeof(off_t), 0);
	if (ret < (int)sizeof(off_t)){
		shutdown(it,SHUT_RDWR);
		return;
	}
	long bytes_read = 0;
	char buff[BLOCK_SIZE];
	ofstream file("lol", ios::out | ios::trunc);
	for(;;) {
		if (bytes_read>=file_size){
			break;
		}
		ret = recv(it,buff,BLOCK_SIZE, 0);
		if ( ret == -1 ) {
			data << "Failed to read from socket" << endl;
			switch (errno) {
				case EBADF : 
					data << "\tWrong file descriptor " << it << endl;
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
			break;
		} else if ( ret == 0) {
			data << "Connection closed by peer" << endl;
			break;
		}
		bytes_read += ret;
		file.write(buff,ret);
	}
	shutdown(it,SHUT_RDWR);
	close(it);
	
}
void* tcpserver::start_thread_request(void* args) {
	struct thread_params * arguments = (thread_params*)args;
	tcpserver * obj = arguments->obj;
	int it = arguments->it;
	obj->request(it);
	return NULL;
}
	
void tcpserver::thread_collector() {
	data << "Thread collector started" << endl;
	pthread_mutex_lock(&join_mutex);
	for (;;) {
		pthread_cond_wait(&join_cond,&join_mutex);
		data << "New dead thread to collect" << endl;
		pthread_testcancel();
		pthread_join(to_join,NULL);
		data << "Thread " << to_join << " joined" << endl;
	}
}
void * tcpserver::start_thread_collector(void* param){
	tcpserver * obj = (tcpserver*)param;
	obj->thread_collector();
	return NULL;
}
