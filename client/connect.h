
#include <time.h>
#include <sys/time.h>
struct connection{
    int port;
    char ip[20];
    struct timeval timeout;
    int times;
};
int hostname_to_ip(char * hostname , char* ip);
int Connect(struct connection *connect_info);
