#ifndef __TCPSERVER__
#define __TCPSERVER__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <pthread.h>

using namespace std;
const int max_size = 5;
const int BUFF = 200;
struct file_details {
	int file_length;//bytes
	int	packet_length;//bytes
	int packet_number;
	string file_name;
};

class tcpserver {
	public :
		tcpserver(string file);
		~tcpserver();

		int ini(uint16_t port); //initialize the server and return errno if an error has occured or 0 on succes
		file_details wait(int file_number); //wait for someone to indicate he's got the requested file
		int receive(int file_number, ostream& file); //receive data for the specified file
		int progress(int file_number); //return the percentage of transfer completion for the specified file or -1 if no file corresponding
		static void* start_thread_collector(void* params);
		static void* start_thread_request(void* params);

	private :
		int read_fd(struct file_details* details, int file_socket);
		void thread_collector();
		void request(int index);

		fstream data;
		
		pthread_mutex_t join_mutex;
		pthread_cond_t join_cond;
		int listen_socketd;
		uint16_t port;
		int requests_socketd[max_size];
		pthread_t to_join;
		pthread_t collector;
		struct sockaddr_in own_address;
		struct sockaddr_in client_address;


};

struct thread_params {
	int it;
	tcpserver * obj;
};

#endif
