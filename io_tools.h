//this part implement tools used for logging, used both by the client
//and the daemon.
//
#if ! defined (IO_TOOLS)
#define IO_TOOLS

#include <sys/types.h>
#include <string>

#define ripd_pid "./ripd_pid"
//file where is stored the daemon pid when it's running
#define ripd_log "./ripd_log"//file used to write execiton information
#define ripd_index "./index"//file containing the available files

struct item {
	std::string type;
	std::string path;
	unsigned long offset;
};
int write_pid(pid_t pid);

pid_t get_pid();

int write_file(std::string file_name);

#endif
