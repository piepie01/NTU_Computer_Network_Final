
#define USER_NAME_LEN 33
#define INTRO_LEN 1025
#define FILTER_LEN 4097
#define JSON_LEN 7000
#define CONNECT_MAX 2000


typedef enum{
	DONGDONG_STATUS_DISCONNECT = 0x88,
	DONGDONG_STATUS_IDLE = 0x89,
	DONGDONG_STATUS_LOGIN = 0x90,
} dongdong_status;

struct User{
	char name[USER_NAME_LEN];
	unsigned int age;
	char gender[7];
	char introduction[INTRO_LEN];
};

struct fd_connection{
	struct User user;
	int status;
	int fd;
};
