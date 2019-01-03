//#include "client.h"
typedef enum{
	USERS_STATUS_ALL = 7122,
	USERS_STATUS_ON = 7123,
	USERS_STATUS_OFF = 7124,
	USERS_STATUS_EXIT = 7125,
} users_status;
typedef enum{
	BLACKLIST_SHOW = 7126,
	BLACKLIST_ADD = 7127,
	BLACKLIST_EXIT = 7128,
} blacklist_status;
typedef enum{
	GROUP_SHOW = 7126,
	GROUP_MAKE = 7127,
	GROUP_EXIT = 7128,
} group_status;
void Command_var_init();
void Flush_term();
void Flush_line();
void enable_raw_mode();
void disable_raw_mode();
int kbhit();
int getch();
int Cmd_quit(struct User*, int sockfd);
int Cmd_users(struct User*, int sockfd);
int Cmd_friend(struct User*, int sockfd);
int Cmd_group(struct User*, int sockfd);
int Cmd_file(struct User*, int sockfd);
int Cmd_help(struct User*, int sockfd);
int Cmd_blacklist(struct User*, int sockfd);
