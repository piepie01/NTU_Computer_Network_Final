#include "command.h"
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include "json_include/cJSON/cJSON.h"
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sys/time.h>
struct winsize w;
char space[10000];
void Command_var_init(){
    memset(space, ' ', sizeof(space)-1);
    space[10000-1] = '\0';
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return;
}
void Flush_term(){
    printf("\033[%d;%dH", 0, 0);
    printf("%s", &space[10000 - w.ws_row * w.ws_col]);
    printf("\033[%d;%dH", 0, 0);
    return;
}

void Flush_line(){
    printf("\033[%d;%dH", 0, 0);
    printf("%s", &space[10000 - w.ws_row]);
    printf("\033[%d;%dH", 0, 0);
    return;
}

void Flush_term_size(int row, int col){
    printf("\033[%d;%dH", 0, 0);
    for(int i=0;i<row;i++) printf("%s", &space[10000 - w.ws_col]);
    printf("\033[%d;%dH", 0, 0);
    return;
}

void enable_raw_mode(){
    termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO); // Disable echo as well
    tcsetattr(0, TCSANOW, &term);
    return;
}

void disable_raw_mode(){
    termios term;
    tcgetattr(0, &term);
    term.c_lflag |= ICANON | ECHO;
    tcsetattr(0, TCSANOW, &term);
    return;
}

int getch() {
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}
int Get_waiting_bytes(){
    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);
    return byteswaiting;
}
int Cmd_quit(struct User *user_info, int sockfd){
    printf("Bye bye %s~\n", user_info->username);
    return 127;
}
int Parse_users(char users[][64], int user_status[], int sockfd){
    cJSON *send_get_user = cJSON_CreateObject();
    cJSON_AddStringToObject(send_get_user, "cmd", "Get_users");
    char *send_get_user_json  = cJSON_PrintUnformatted(send_get_user);
    send(sockfd, send_get_user_json, strlen(send_get_user_json), 0);
    send(sockfd,"\n",1,0);

    char ret[4096];
    recv(sockfd, &ret, 4095, 0);
    cJSON *root = cJSON_Parse(ret);
    cJSON *data = cJSON_GetObjectItem(root,"data");
    int len = cJSON_GetArraySize(data);
    for (int i=0;i<len;i++){
        cJSON *subitem = cJSON_GetArrayItem(data, i);
        cJSON *username = cJSON_GetObjectItem(subitem, "username");
        cJSON *status = cJSON_GetObjectItem(subitem, "online");
        strcpy(users[i], username->valuestring);
        user_status[i] = status->valueint;
    }
    cJSON_Delete(root);
    return len;

}
int Print_users(char users[][64], int user_status[], int users_len, int mode, int choose_mode, int choose, int *choose_ind){
    char space[20] = "\0";
    memset(space, ' ', sizeof(space)-1);
    int ret = 0;
    if(mode == USERS_STATUS_ALL){
        for(int i=0;i<users_len;i++){
            if(user_status[i] == 1 && (choose_mode == 1 && choose == i)) printf("%c[32;47m%s[0m%s", " \n"[(i)%3==0], users[i], &space[strlen(users[i])]), *choose_ind = i;
            else if(user_status[i] == 1) printf("%c[32m%s[0m%s", " \n"[(i)%3==0], users[i], &space[strlen(users[i])]);
            else if(user_status[i] == 0 && (choose_mode == 1 && choose == i)) printf("%c[47m%s[0m%s", " \n"[(i)%3==0], users[i], &space[strlen(users[i])]), *choose_ind = i;
            else if(user_status[i] == 0) printf("%c%s%s", " \n"[(i)%3==0], users[i], &space[strlen(users[i])]);
        }
        ret = users_len;
    }
    if(mode == USERS_STATUS_ON){
        int num = 0;
        for(int i=0;i<users_len;i++){
            if(user_status[i] == 1 && (choose_mode == 1 && choose == num)) printf("%c[32;47m%s[0m%s", " \n"[(num++)%3==0], users[i], &space[strlen(users[i])]), *choose_ind = i;
            else if(user_status[i] == 1) printf("%c[32m%s[0m%s", " \n"[(num++)%3==0], users[i], &space[strlen(users[i])]);
        }
        ret = num;
    }
    if(mode == USERS_STATUS_OFF){
        int num = 0;
        for(int i=0;i<users_len;i++){
            if(user_status[i] == 0 && (choose_mode == 1 && choose == num)) printf("%c[47m%s[0m%s", " \n"[(num++)%3==0], users[i], &space[strlen(users[i])]), *choose_ind = i;
            else if(user_status[i] == 0) printf("%c%s%s", " \n"[(num++)%3==0], users[i], &space[strlen(users[i])]);
        }
        ret = num;
    }
    printf("\n");
    fflush(stdout);
    return ret;
}
int Print_mode(int mode){
    char space[20] = "\0";
    memset(space, ' ', sizeof(space)-1);
    char all[5] = "ALL\0", online[8] = "ONLINE\0", offline[9] = "OFFLINE\0", exit[6] = "EXIT\0";
    if(mode == USERS_STATUS_ALL) printf("[46m%s%s[0m", all, &space[strlen(all)]);
    else printf("%s%s", all, &space[strlen(all)]);
    if(mode == USERS_STATUS_ON) printf("[46m%s%s[0m", online, &space[strlen(online)]);
    else printf("%s%s", online, &space[strlen(online)]);
    if(mode == USERS_STATUS_OFF) printf("[46m%s%s[0m", offline, &space[strlen(offline)]);
    else printf("%s%s", offline, &space[strlen(offline)]);
    if(mode == USERS_STATUS_EXIT) printf("[46m%s%s[0m", exit, &space[strlen(exit)]);
    else printf("%s%s", exit, &space[strlen(exit)]);
    printf("\n");
    return 0;
}

int Print_blacklist(){
    char blacklist[11] = "BLACKLIST\0";
    printf("[46m%s%s[0m", all, &space[strlen(all)]);

}

int Print_Chat(char history[], struct User *user_info){
    int width = 40, height = 20, len = strlen(history);
    if(len == 1) history[0] = '\0', len = 0;
    for(int i=0;i<width;i++) printf("█");
    printf("\n");
    char tmp[64] = {0}, space[44] = {0};
    memset(space, ' ', sizeof(space)-1);
    int message_num = 0;
    for(int i=0;i<len;i++) if(history[i] == '\n') message_num++;
    message_num-=height;
    int times = 0;
    int tmp_ind = 0;
    for(int i=message_num;i<0;i++) printf("█%s█\n", &space[5]);
    for(int i=0;i<len;i++){
        if(history[i] != '\n') tmp[tmp_ind++] = history[i];
        else{
            tmp[tmp_ind] = '\0';
            if((times++) >= message_num){
                if(strncmp(user_info->username, tmp, strlen(user_info->username)) == 0)
                    printf("█[1;36m%s%s[0m█\n", &space[tmp_ind+5], tmp);
                else
                    printf("█[1;33m%s%s[0m█\n", tmp, &space[tmp_ind+5]);
            }
            tmp_ind = 0;
        }
    }
    for(int i=0;i<width;i++) printf("█");
    printf("\n>");
    fflush(stdout);
    return 0;
}
int kbhit(){
    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);
    return byteswaiting>0;
}
int server_message(int sockfd){
    struct timeval time;
	time.tv_sec = 0;
	time.tv_usec = 1;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);
    select(sockfd+1, &fds, NULL, NULL, &time);
    return FD_ISSET(sockfd,&fds);
}
int Chatroom(char target[], int sockfd, struct User *user_info){
    Flush_term();
    enable_raw_mode();
    
    cJSON *send_chatroom = cJSON_CreateObject();
    cJSON_AddStringToObject(send_chatroom, "cmd", "Chatroom");
    cJSON_AddStringToObject(send_chatroom, "me", user_info->username);
    cJSON_AddStringToObject(send_chatroom, "you", target);
    char *send_chatroom_json  = cJSON_PrintUnformatted(send_chatroom);
    send(sockfd, send_chatroom_json, strlen(send_chatroom_json), 0);
    send(sockfd,"\n",1,0);

    char history[60000], ch;
    memset(history, 0, sizeof(history));
    recv(sockfd, &history, 60000, 0);
    Print_Chat(history, user_info);
    char input[64] = {0};
    int input_ind = 0;
    while(1){
        if (kbhit()){
            ch = getch();
            if(ch == 10){ //RETURN
                strcat(history, user_info->username);
                strcat(history, ">");
                strcat(history, input);
                strcat(history, "\n\0");
                Flush_term();
                Print_Chat(history, user_info);
                cJSON *send_chat = cJSON_CreateObject();
                cJSON_AddStringToObject(send_chat, "cmd", "Chat");
                cJSON_AddStringToObject(send_chat, "me", user_info->username);
                cJSON_AddStringToObject(send_chat, "you", target);
                cJSON_AddStringToObject(send_chat, "message", input);
                char *send_chat_json  = cJSON_PrintUnformatted(send_chat);
                send(sockfd, send_chat_json, strlen(send_chat_json), 0);
                send(sockfd,"\n",1,0);
                memset(input, 0, sizeof(input));
                input_ind = 0;
            }
            else if(ch == 127){
                if(input_ind > 0){
                    input[--input_ind] = '\0';
                    printf("\b \b");
                }
            }
            else if(ch == 27){
                if(kbhit() == 0){
                    cJSON *send_quit = cJSON_CreateObject();
                    cJSON_AddStringToObject(send_quit, "cmd", "Quit");
                    char *send_quit_json  = cJSON_PrintUnformatted(send_quit);
                    send(sockfd, send_quit_json, strlen(send_quit_json), 0);
                    send(sockfd,"\n",1,0);
                    usleep(2e+5);
                    break;
                }
            }
            else{
                input[input_ind++] = ch;
                printf("%c", ch);
            }
            fflush(stdout);
        }
        if(server_message(sockfd)){
            char messages[6000];
            memset(messages, 0, sizeof(messages));
            recv(sockfd, &messages, 6000, 0);
            cJSON *root = cJSON_Parse(messages);
            cJSON *me = cJSON_GetObjectItem(root,"me");
            cJSON *message = cJSON_GetObjectItem(root,"message");
            strcat(history, me->valuestring);
            strcat(history, ">");
            strcat(history, message->valuestring);
            strcat(history, "\n\0");
            Flush_term();

            Print_Chat(history, user_info);
            printf("%s", input);
            fflush(stdout);
        }
    }
    disable_raw_mode();
    return 0;
}
int Cmd_users(struct User *user_info, int sockfd){
    char users[100][64];
    int status[100] = {0};
    memset(users, 0, sizeof(users));

    int mode = USERS_STATUS_ALL, choose_mode = 0, choose = 0, choose_ind = 0;
    const char RETURN=10;
    const char DIR=27;
    
    while(1){
        Flush_term();
        //fprintf(stderr, "In Cmd_users\n");
        Print_mode(mode);
        int users_len = Parse_users(users, status, sockfd);
        int term_users = Print_users(users, status, users_len, mode, choose_mode, choose, &choose_ind);
        unsigned char ch = getch();
        if(ch == DIR){
            ch = getch();
            ch = getch();
            if(choose_mode == 0){
                if(ch == 67 && mode < USERS_STATUS_EXIT) mode++; //right
                else if(ch == 68 && mode > USERS_STATUS_ALL) mode--; //left
                else if(ch == 66 && mode != USERS_STATUS_EXIT)  choose_mode = 1, choose = 0;//down
            }else{
                if(ch == 67) choose++;
                else if(ch == 68) choose--;
                else if(ch == 66) choose+=3;
                else if(ch == 65) choose-=3;
                if(choose >= term_users) choose-=3;
                else if(choose < 0) choose_mode = 0;
            }
        }
        else if(ch == RETURN && choose_mode == 1){
            Chatroom(users[choose_ind], sockfd, user_info);
        }
        else if(ch == RETURN && mode == USERS_STATUS_EXIT){
            Flush_term();
            break;
        }
    }
    
    return 0;
}
int Cmd_friend(struct User *user_info, int sockfd){
    return 0;
}
int Cmd_whoami(struct User *user_info, int sockfd){
    return 0;
}
int Cmd_file(struct User *user_info, int sockfd){
    return 0;
}

int Cmd_blacklist(struct User *user_info, int sockfd){

    return 0;
}
int Cmd_help(struct User *user_info, int sockfd){
    printf(">users\n    start chatting!\n");
    printf(">friend\n    add friend\n");
    printf(">whoami\n    WHOAMI?\n");
    printf(">file\n    file\n");
    printf(">quit\n");
    return 0;
}