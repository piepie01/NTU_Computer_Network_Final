//#include "client.h"
typedef enum{
	USERS_STATUS_ALL = 7122,
	USERS_STATUS_ON = 7123,
	USERS_STATUS_OFF = 7124,
	USERS_STATUS_EXIT = 7125,
} users_status;
void Command_var_init();
void Flush_term();
void Flush_line();
int getch();
int Cmd_quit(struct User*, int sockfd);
int Cmd_users(struct User*, int sockfd);
int Cmd_friend(struct User*, int sockfd);
int Cmd_whoami(struct User*, int sockfd);
int Cmd_file(struct User*, int sockfd);
int Cmd_help(struct User*, int sockfd);
