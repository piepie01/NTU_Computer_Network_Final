#include "command.h"
#include "client.h"
#include <stdio.h>
#include "json_include/cJSON/cJSON.h"

int Cmd_quit(struct User *user_info){
    printf("Bye bye %s~", user_info->username);
    return -1;
}
int Cmd_users(struct User *user_info){
    printf("In users, username:%s\n",user_info->username);
    return 0;
}
int Cmd_friend(struct User *user_info){
    return 0;
}
int Cmd_whoami(struct User *user_info){
    return 0;
}
int Cmd_file(struct User *user_info){
    return 0;
}
