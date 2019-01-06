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
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
struct winsize w;
char space[10000];
struct File_obj file_obj[1000];
int file_obj_ind;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
void Command_var_init(){
    memset(space, ' ', sizeof(space)-1);
    memset(file_obj, 0, sizeof(file_obj));
    file_obj_ind = 0;
    space[10000-1] = '\0';
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    char file_dir[8] = "files\0";
    if(access(file_dir, F_OK) != 0) mkdir(file_dir, 0755);
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
int Find_in_file(char targetstr[], char file[], int deletemode, int sockfd, struct User* user_info){
    char blacklist[10][64];
    memset(blacklist, 0, sizeof(blacklist));
    int blacklist_len = 0;
    FILE *pFile = fopen(file, "r");
    if(pFile == NULL){
        pFile = fopen(file,"a");         
    }
    else{
        size_t read, len=0;
        char * line = NULL;
        while((read = getline(&line, &len, pFile))!= -1){
            strcpy(blacklist[blacklist_len], line); 
            blacklist[blacklist_len][strlen(blacklist[blacklist_len])-1] = '\0';
            blacklist_len++;                
        }
    }
    fclose(pFile);
    for(int i =0; i<blacklist_len; i++){
        //printf("blacklist, target --%s--%s--\n", blacklist[i], targetstr);
        if(strcmp(blacklist[i], targetstr) == 0){
            return 1;
        }
    }
    return 0;
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

int Print_blacklist(int mode, char blacklist[][64], int blacklist_len, struct User *user_info, int sockfd, int choose_mode, int choose, int choose_ind){
    char space[20] = "\0";
    memset(space, ' ', sizeof(space)-1);    
    char showblack[11] = "BLACKLIST\0", addblack[10] = "ADDBLACK\0", exitblack[6] = "EXIT\0";
    if(mode == BLACKLIST_SHOW) printf("[46m%s%s[0m", showblack, &space[strlen(showblack)]);
    else printf("%s%s", showblack, &space[strlen(showblack)]);    
    if(mode == BLACKLIST_ADD) printf("[46m%s%s[0m", addblack, &space[strlen(addblack)]);
    else printf("%s%s", addblack, &space[strlen(addblack)]);    
    if(mode == BLACKLIST_EXIT) printf("[46m%s%s[0m", exitblack, &space[strlen(exitblack)]);
    else printf("%s%s", exitblack, &space[strlen(exitblack)]);    
    printf("\n");

    memset(space, ' ', sizeof(space)-1);

    if(mode == BLACKLIST_SHOW){
        for(int i=0;i<blacklist_len;i++){
            blacklist[i][strlen(blacklist[i])-1] = '\0';
            if((choose_mode == 1 && choose == i)){
                printf("%c[32;47m%s[0m%s", " \n"[(i)%3==0], blacklist[i], &space[strlen(blacklist[i])]);
//                *choose_ind = i;
            }
            else
                printf("%c%s%s", " \n"[(i)%3==0], blacklist[i], &space[strlen(blacklist[i])]);
        }  
    }
    else if(mode == BLACKLIST_ADD){
        char file[64] = "./Blacklist_\0";
        strcat(file, user_info->username);
        strcat(file, ".txt");                
        FILE *pFile = fopen(file, "a");     
        char black[64] = "\0";
        printf("Who do you want to add to blacklist? (press any key to add)\n");
        unsigned char ch=getch(), DIR=27;
        if(ch == DIR){
            ch = getch();
            ch = getch();
            if(ch == 68) return BLACKLIST_SHOW;
            else if(ch == 67) return BLACKLIST_EXIT;
        }
        printf("his/her username: ");
        scanf("%s", black);

        printf("\n %s added to blacklist\n", black);   
        sleep(1);       
        black[strlen(black)] = '\n';
        black[strlen(black)] = '\0';
        int a = fwrite(black, sizeof(char), strlen(black), pFile);         
        fclose(pFile);       
    } 
    printf("\n");
    fflush(stdout);
    return mode;
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
int Chatroom(char target[], char blacklistfile[], int sockfd, struct User *user_info){
    Flush_term();
    if(Find_in_file(target, blacklistfile, 0, sockfd, user_info)){
        printf("This user is in the blacklist!!! You don't have to receive his/her messages. Yaa!\n");
        sleep(3);
        return 0;
    }    
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
            char file[64] = "./Blacklist_\0";
            strcat(file, user_info->username);
            strcat(file, ".txt");          
            Chatroom(users[choose_ind], file, sockfd, user_info);
        }
        else if(ch == RETURN && mode == USERS_STATUS_EXIT){
            Flush_term();
            break;
        }
    }
    
    return 0;
}
int Cmd_blacklist(struct User *user_info, int sockfd){
    //fprintf(stderr,"Inblacklist\n");
    char blacklist[10][64];
    memset(blacklist, 0, sizeof(blacklist));

    int mode = BLACKLIST_SHOW, choose_mode = 0, choose = 0, choose_ind = 0;
    const char RETURN=10;
    const char DIR=27;

    while(1){        
        Flush_term();
        char file[64] = "./Blacklist_\0";
        strcat(file, user_info->username);
        strcat(file, ".txt");        
        int blacklist_len = 0;
        FILE *pFile = fopen(file, "r");
        if(pFile == NULL){
            pFile = fopen(file,"a");         
        }
        else{
            size_t read, len=0;
            char * line = NULL;
            while((read = getline(&line, &len, pFile))!= -1){
                strcpy(blacklist[blacklist_len], line); 
                //printf("getline blacklist = %s\n", blacklist[blacklist_len]);
                blacklist_len++;                
            }
        }
        fclose(pFile);  
        int mode2 = Print_blacklist(mode, blacklist, blacklist_len, user_info, sockfd, choose_mode, choose, choose_ind);    
        if(mode2 != mode){
            mode = mode2;
            continue;
        }
        unsigned char ch = getch();
        if(ch == DIR){
            ch = getch();
            ch = getch();
            if(choose_mode == 0){
                if(ch == 67 && mode < BLACKLIST_EXIT) mode++; //right
                else if(ch == 68 && mode > BLACKLIST_SHOW) mode--; //left
                else if(ch == 66 && mode != BLACKLIST_EXIT)  choose_mode = 1, choose = 0;//down
            }else{
                if(ch == 67) choose++;
                else if(ch == 68) choose--;
                else if(ch == 66) choose+=3;
                else if(ch == 65) choose-=3;
                if(choose >= blacklist_len) choose-=3;
                else if(choose < 0) choose_mode = 0;
            }
        }
        else if(ch == RETURN && choose_mode == 1){ //to delete from blacklist
            int deletemode = 1;
            Find_in_file(blacklist[choose_ind], file, deletemode, sockfd, user_info);
        }
        else if(ch == RETURN && mode == BLACKLIST_EXIT){
            Flush_term();
            break;
        }
    }

    return 0;
}
int Print_mode_friend(int mode){
    char space[20] = "\0";
    memset(space, ' ', sizeof(space)-1);
    char all[8] = "Friend\0", online[8] = "ALL\0", offline[9] = "Request\0", exit[6] = "EXIT\0";
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
int Parse_friend(char users[][64], int user_status[], int friends[], int requests[], int sockfd){
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
        cJSON *f_status = cJSON_GetObjectItem(subitem, "friend");
        cJSON *req = cJSON_GetObjectItem(subitem, "req");
        strcpy(users[i], username->valuestring);
        user_status[i] = status->valueint;
        friends[i] = f_status->valueint;
        requests[i] = req->valueint;
    }
    cJSON_Delete(root);
    return len;

}
int Print_friend(char users[][64], int user_status[], int friends[], int requests[], int users_len, int mode, int choose_mode, int choose, int *choose_ind){
    char space[20] = "\0";
    memset(space, ' ', sizeof(space)-1);
    int ret = 0;
    if(mode == USERS_STATUS_ON){
        for(int i=0;i<users_len;i++){
            if(user_status[i] == 1 && (choose_mode == 1 && choose == i)) printf("%c[32;47m%s[0m%s", " \n"[(i)%3==0], users[i], &space[strlen(users[i])]), *choose_ind = i;
            else if(user_status[i] == 1) printf("%c[32m%s[0m%s", " \n"[(i)%3==0], users[i], &space[strlen(users[i])]);
            else if(user_status[i] == 0 && (choose_mode == 1 && choose == i)) printf("%c[47m%s[0m%s", " \n"[(i)%3==0], users[i], &space[strlen(users[i])]), *choose_ind = i;
            else if(user_status[i] == 0) printf("%c%s%s", " \n"[(i)%3==0], users[i], &space[strlen(users[i])]);
        }
        ret = users_len;
    }
    if(mode == USERS_STATUS_ALL){
        int num = 0;
        for(int i=0;i<users_len;i++){
            if(friends[i] == 1 && (choose_mode == 1 && choose == num)) printf("%c[32;47m%s[0m%s", " \n"[(num++)%3==0], users[i], &space[strlen(users[i])]), *choose_ind = i;
            else if(friends[i] == 1) printf("%c[32m%s[0m%s", " \n"[(num++)%3==0], users[i], &space[strlen(users[i])]);
        }
        ret = num;
    }
    if(mode == USERS_STATUS_OFF){
        int num = 0;
        for(int i=0;i<users_len;i++){
            if(requests[i] == 1 && friends[i] == 0 && (choose_mode == 1 && choose == num)) printf("%c[47m%s[0m%s", " \n"[(num++)%3==0], users[i], &space[strlen(users[i])]), *choose_ind = i;
            else if(requests[i] == 1 && friends[i] == 0) printf("%c%s%s", " \n"[(num++)%3==0], users[i], &space[strlen(users[i])]);
        }
        ret = num;
    }
    printf("\n");
    fflush(stdout);

    return ret;
}
int Send_friend_request(char target[], int sockfd, struct User *user_info){
    printf("Add %s as friend? (y/n)", target);
    char input = getch();
    if(input == 'y'){
        cJSON *send_friend_request = cJSON_CreateObject();
        cJSON_AddStringToObject(send_friend_request, "cmd", "Friend_request");
        cJSON_AddStringToObject(send_friend_request, "me", user_info->username);
        cJSON_AddStringToObject(send_friend_request, "you", target);   
        char *send_friend_request_json  = cJSON_PrintUnformatted(send_friend_request);
        send(sockfd, send_friend_request_json, strlen(send_friend_request_json), 0);
        send(sockfd,"\n",1,0);
    
    }
    return 0;
}
int Send_friend_accept(char target[], int sockfd, struct User *user_info){
    printf("Accept %s as friend? (y/n)", target);
    char input = getch();
    if(input == 'y'){
        cJSON *send_friend_request = cJSON_CreateObject();
        cJSON_AddStringToObject(send_friend_request, "cmd", "Friend_accept");
        cJSON_AddStringToObject(send_friend_request, "me", user_info->username);
        cJSON_AddStringToObject(send_friend_request, "you", target);   
        char *send_friend_request_json  = cJSON_PrintUnformatted(send_friend_request);
        send(sockfd, send_friend_request_json, strlen(send_friend_request_json), 0);
        send(sockfd,"\n",1,0);
    
    }
    return 0;
}


int Cmd_friend(struct User *user_info, int sockfd){
    char users[100][64];
    int status[100] = {0}, friends[100] = {0}, requests[100] = {0};
    memset(users, 0, sizeof(users));

    int mode = USERS_STATUS_ALL, choose_mode = 0, choose = 0, choose_ind = 0;
    const char RETURN=10;
    const char DIR=27;
    
    while(1){
        Flush_term();
        //fprintf(stderr, "In Cmd_users\n");
        Print_mode_friend(mode);
        int users_len = Parse_friend(users, status, friends, requests, sockfd);
        int term_users = Print_friend(users, status, friends, requests, users_len, mode, choose_mode, choose, &choose_ind);
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
            char file[64] = "./Blacklist_\0";
            strcat(file, user_info->username);
            strcat(file, ".txt");          
            if(mode == USERS_STATUS_ALL) Chatroom(users[choose_ind], file, sockfd, user_info);
            else if(mode == USERS_STATUS_ON) Send_friend_request(users[choose_ind], sockfd, user_info);
            else if(mode == USERS_STATUS_OFF) Send_friend_accept(users[choose_ind], sockfd, user_info);
        }
        else if(ch == RETURN && mode == USERS_STATUS_EXIT){
            Flush_term();
            break;
        }
    }
    
    return 0;
}
int Print_mode_group(int mode){
    char space[20] = "\0";
    memset(space, ' ', sizeof(space)-1);
    char all[8] = "ALL\0", online[8] = "Create\0", exit[6] = "EXIT\0";
    if(mode == GROUP_SHOW) printf("[46m%s%s[0m", all, &space[strlen(all)]);
    else printf("%s%s", all, &space[strlen(all)]);
    if(mode == GROUP_MAKE) printf("[46m%s%s[0m", online, &space[strlen(online)]);
    else printf("%s%s", online, &space[strlen(online)]);
    if(mode == GROUP_EXIT) printf("[46m%s%s[0m", exit, &space[strlen(exit)]);
    else printf("%s%s", exit, &space[strlen(exit)]);
    printf("\n");
    return 0;
}
int Parse_group(char groups[][64], char owners[][64], int sockfd){
    cJSON *send_get_user = cJSON_CreateObject();
    cJSON_AddStringToObject(send_get_user, "cmd", "Get_group");
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
        cJSON *name = cJSON_GetObjectItem(subitem, "name");
        cJSON *owner = cJSON_GetObjectItem(subitem, "owner");
        strcpy(groups[i], name->valuestring);
        strcpy(owners[i], owner->valuestring);
    }
    cJSON_Delete(root);
    return len;

}
int Print_group(char users[][64], int users_len, int mode, int choose_mode, int choose, int *choose_ind){
    char space[20] = "\0";
    memset(space, ' ', sizeof(space)-1);
    int ret = 0;
    if(mode == GROUP_SHOW){
        for(int i=0;i<users_len;i++){
            if((choose_mode == 1 && choose == i)) printf("%c[47m%s[0m%s", " \n"[(i)%3==0], users[i], &space[strlen(users[i])]), *choose_ind = i;
            else printf("%c%s%s", " \n"[(i)%3==0], users[i], &space[strlen(users[i])]);
        }
        ret = users_len;
    }
    printf("\n");
    fflush(stdout);

    return ret;
}
int Print_Group(char group_name[], char owner_name[], char history[], struct User *user_info){
    int width = 40, height = 20, len = strlen(history), g_len = strlen(group_name) + strlen(owner_name);
    if(len == 1) history[0] = '\0', len = 0;
    char tmp[64] = {0}, space[44] = {0};
    memset(space, ' ', sizeof(space)-1);
    printf("%s%sOwner:%s\n", group_name, &space[g_len+9], owner_name);
    for(int i=0;i<width;i++) printf("█");
    printf("\n");
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
int Grouproom(char group_name[], char owner_name[], int sockfd, struct User *user_info){
    Flush_term();
    enable_raw_mode();
    
    cJSON *send_chatroom = cJSON_CreateObject();
    cJSON_AddStringToObject(send_chatroom, "cmd", "Grouproom");
    cJSON_AddStringToObject(send_chatroom, "me", user_info->username);
    cJSON_AddStringToObject(send_chatroom, "name", group_name);
    char *send_chatroom_json  = cJSON_PrintUnformatted(send_chatroom);
    send(sockfd, send_chatroom_json, strlen(send_chatroom_json), 0);
    send(sockfd,"\n",1,0);

    char history[60000], ch;
    memset(history, 0, sizeof(history));
    Print_Group(group_name, owner_name, history, user_info);
    char input[64] = {0};
    int input_ind = 0;
    while(1){
        if (kbhit()){
            ch = getch();
            if(ch == 10){ //RETURN
                cJSON *send_chat = cJSON_CreateObject();
                cJSON_AddStringToObject(send_chat, "cmd", "Chat");
                cJSON_AddStringToObject(send_chat, "me", user_info->username);
                cJSON_AddStringToObject(send_chat, "name", group_name);
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
                    cJSON_AddStringToObject(send_quit, "me", user_info->username);
                    cJSON_AddStringToObject(send_quit, "name", group_name);
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
            if(strcmp(message->valuestring, "enter\0") == 0 || strcmp(message->valuestring, "quit\0") == 0)
                strcat(history, "~");
            else
                strcat(history, ">");
            strcat(history, message->valuestring);
            strcat(history, "\n\0");
            Flush_term();

            Print_Group(group_name, owner_name, history, user_info);
            printf("%s", input);
            fflush(stdout);
        }
    }
    disable_raw_mode();
    return 0;
}
int Cmd_group(struct User *user_info, int sockfd){
    char groups[100][64], owners[100][64];
    memset(groups, 0, sizeof(groups));
    memset(owners, 0, sizeof(owners));

    int mode = GROUP_SHOW, choose_mode = 0, choose = 0, choose_ind = 0;
    const char RETURN=10;
    const char DIR=27;
    
    while(1){
        Flush_term();
        //fprintf(stderr, "In Cmd_users\n");
        Print_mode_group(mode);
        int groups_len = Parse_group(groups, owners, sockfd);
        int term_groups = Print_group(groups, groups_len, mode, choose_mode, choose, &choose_ind);
        unsigned char ch = getch();
        if(ch == DIR){
            ch = getch();
            ch = getch();
            if(choose_mode == 0){
                if(ch == 67 && mode < GROUP_EXIT) mode++; //right
                else if(ch == 68 && mode > GROUP_SHOW) mode--; //left
                else if(ch == 66 && mode == GROUP_SHOW)  choose_mode = 1, choose = 0;//down
            }else{
                if(ch == 67) choose++;
                else if(ch == 68) choose--;
                else if(ch == 66) choose+=3;
                else if(ch == 65) choose-=3;
                if(choose >= term_groups) choose-=3;
                else if(choose < 0) choose_mode = 0;
            }
        }
        else if(ch == RETURN && choose_mode == 1) Grouproom(groups[choose_ind], owners[choose_ind], sockfd, user_info);
        else if(ch == RETURN && mode == GROUP_MAKE){
            printf("Group name:");
            fflush(stdout);
            char group_name[30];
            scanf("%s", group_name);
            cJSON *send_group_make = cJSON_CreateObject();
            cJSON_AddStringToObject(send_group_make, "cmd", "Group_make");
            cJSON_AddStringToObject(send_group_make, "me", user_info->username);
            cJSON_AddStringToObject(send_group_make, "name", group_name);   
            char *send_group_make_json  = cJSON_PrintUnformatted(send_group_make);
            send(sockfd, send_group_make_json, strlen(send_group_make_json), 0);
            send(sockfd,"\n",1,0);

            getch();

        }
        else if(ch == RETURN && mode == GROUP_EXIT){
            Flush_term();
            break;
        }
    }
    
    return 0;
    return 0;
}
int File_Search_Command(char command[], int *command_ind, char Commands[][20], int Commands_num, int all){
    int match = 0;
    char tmp_cmd[20] = "\0";
    char space[10] = "\0";
    memset(space, ' ', sizeof(space)-1);
    for(int i=0;i<Commands_num;i++){
        if(strncmp(command, Commands[i], strlen(command)) == 0){
            match++;
            strcpy(tmp_cmd, Commands[i]);
            if(all == 1) printf("%c%s%s", " \n"[(match-1)%3==0], tmp_cmd, &space[strlen(tmp_cmd)]);
        }
    }
    if(all == 0 && match == 1){

        strcpy(command, tmp_cmd);
        printf("%s", &command[*command_ind]);
        *command_ind = strlen(command);
    }
    return match;
    
}
int File_Get_Command(char command[], struct User *user_info, char Commands[][20], int Commands_num, char History[][20], int History_ind){
    printf("%s(file)> ", user_info->username);
    const char BACKSPACE=127;
    const char TAB=9;
    const char RETURN=10;
    const char DIR=27;

    int command_ind = 0;
    char ch = 0;
    int tab_num = 0;
    int now_history = History_ind;
    char space[50] = "\0";
    memset(space, ' ', sizeof(space)-1);
    while((ch=getch())!=RETURN){
        //cout << (int)ch << endl;
        if(ch == TAB) tab_num++;
        else if(ch == BACKSPACE){
            tab_num = 0;
            if(strlen(command) != 0){
                printf("\b \b");
                command[--command_ind] = '\0';
            }
        }
        else if(ch == DIR){
            ch = getch();
            ch = getch();
            if(ch == 65 && History_ind!=0){
                strcpy(History[History_ind--], command);
                memset(command, 0, 20);
                strcpy(command, History[History_ind]);
                command_ind = strlen(command);
                printf("\r%s\r%s(file)> %s", space, user_info->username, command);
            };
            if(ch == 66 && History_ind!=now_history+1){
                strcpy(History[History_ind++], command);
                if(strlen(command) == 0 && History_ind == now_history+1) History_ind--;
                memset(command, 0, 20);
                strcpy(command, History[History_ind]);
                command_ind = strlen(command);
                printf("\r%s\r%s(file)> %s", space, user_info->username, command);
            };
            if(ch == 67 && command_ind != (int)strlen(command)){ //right
                //printf("%d %d\n",command_ind, strlen(command));
                printf("%c", command[command_ind++]);
            }
            if(ch == 68 && command_ind != 0){ //left
                printf("\b");
                command_ind--;
            }
        }
        else{
            if((int)strlen(command) <= 19){ 
                for(int i=20-2;i>=command_ind;i--) command[i+1] = command[i];
                command[command_ind++] = ch;
                tab_num = 0;
                printf("%s",&command[command_ind-1]);
                for(int i=command_ind;i<(int)strlen(command);i++) printf("\b");
            }
        }
        if(tab_num == 1){
            if(File_Search_Command(command, &command_ind, Commands, Commands_num, 0) == 1) tab_num--; 
        }
        else if(tab_num == 2){
            if(File_Search_Command(command, &command_ind, Commands, Commands_num, 1)) printf("\n%s(file)> %s", user_info->username, command);
            tab_num = 0;
            
        }
    }
    printf("\n");
    return 0;
}
int Print_files(char filenames[][100], int file_num){
    char space[30] = "\0";
    memset(space, ' ', sizeof(space)-1);
    for(int i=0;i<file_num;i++){
        printf("%c[32m%s[0m%s", " \n"[(i)%3==0], filenames[i], &space[strlen(filenames[i])]);
    }
    printf("\n");
    return 0;
}
void* File_send(void *arg){
    int *fd = (int *)arg;
    int ind = 0;
    struct File_obj tmp_obj;
    memset(&tmp_obj, 0, sizeof(tmp_obj));
    while(1){
        pthread_mutex_lock( &mutex1 );
        if(file_obj[ind].status == 1){
            memcpy(&tmp_obj, &file_obj[ind], sizeof(tmp_obj));
            memset(&file_obj[ind], 0, sizeof(tmp_obj));
            ind = (ind+1) % 1000;
        }
        pthread_mutex_unlock( &mutex1 );
        if(tmp_obj.status == 1){
            //printf("handle %s\n", tmp_obj.filename);
            FILE *f = fopen(tmp_obj.filename, "r");
            char buf[1001] = "\0";
            while(!feof(f)){
                cJSON *send_file = cJSON_CreateObject();
                cJSON_AddStringToObject(send_file, "cmd", "file");
                cJSON_AddStringToObject(send_file, "me", tmp_obj.from);
                cJSON_AddStringToObject(send_file, "you", tmp_obj.target);
                cJSON_AddStringToObject(send_file, "filename", tmp_obj.filename);
                int res = fread(buf, 1, 1000, f);
                buf[res] = 0;
                cJSON_AddStringToObject(send_file, "content", buf);
                char *send_file_json  = cJSON_PrintUnformatted(send_file);
                send(*fd, send_file_json, strlen(send_file_json), 0);
                send(*fd,"\n",1,0);
                cJSON_Delete(send_file);
                //sleep(1);

            }
            fclose(f);
            memset(&tmp_obj, 0, sizeof(tmp_obj));
        }

    }
    pthread_exit(NULL);
}
void* File_receive(void *arg){
    int *fd = (int *)arg;
    char ret[40960] = "\0";
    int ind = 0;
    while(1){
        if(server_message(*fd)){
            int sz = recv(*fd, &ret[ind], 4096, 0);
            ind+=sz;
            
            while(1){
            cJSON *root = NULL;
            char tmp[4096] = {0};
            char copy[4096] = {0};
            for(int i=0;i<ind;i++){
                if(ret[i] == '\n'){
                    strncpy(tmp,ret,i);
                    strcpy(copy, &ret[i+1]);
                    memset(ret, 0, sizeof(ret));
                    strcpy(ret, copy);
                    ind = strlen(ret);
                    root = cJSON_Parse(tmp);
                    break;
                }
            }
            if(root == NULL) break;
            //printf("%s\n", cJSON_PrintUnformatted(root));
            cJSON *filename = cJSON_GetObjectItemCaseSensitive(root, "filename");
            cJSON *content = cJSON_GetObjectItemCaseSensitive(root, "content");
            char store[64] = "files/\0";
            int len = strlen(filename->valuestring);
            for(int i=len-1;i>=0;i--){
                if(filename->valuestring[i] == '/'){
                    strcat(store, &filename->valuestring[i+1]);
                    break;
                }
            }
            //printf("%s\n", store);
            FILE *new_f = fopen(store, "a");
            fwrite(content->valuestring, 1, strlen(content->valuestring), new_f);
            fclose(new_f);
            cJSON_Delete(root);
            }
        }
    }
    pthread_exit(NULL);
}
int listdir(char path[]){
    DIR *dp;
    struct dirent *ep;     
    dp = opendir (path);

    if (dp != NULL)
    {
      while (ep = readdir (dp))
          if(strcmp(ep->d_name, ".\0") && strcmp(ep->d_name, "..\0"))
            puts (ep->d_name);

      (void) closedir (dp);
    }
    else
      perror ("Couldn't open the directory");

  return 0;
}
int Cmd_file(struct User *user_info, int sockfd){
    char Commands[8][20] = {"add\0", "ls\0", "cd\0", "files\0", "users\0", "send\0", "quit\0", "get\0"};
    char History[10000][20];
    int History_ind = 0, Commands_num = 8;
    memset(History, 0, sizeof(History));
    int cmd_ret;
    char filenames[100][100];
    int file_num = 0;
    memset(filenames, 0, sizeof(filenames));
    char path[6000] = "./\0";
    char users[100][64];
    int status[100] = {0}, friends[100] = {0}, requests[100] = {0};
    memset(users, 0, sizeof(users));
    while(1){
        char command[20] = "\0";
        int ret = File_Get_Command(command, user_info, Commands, Commands_num, History, History_ind);
        int match_cmd = 0;
        cmd_ret = 0;
        for(int i=0;i<Commands_num;i++){
            //fprintf(stdout, "---%s---%s---\n", Commands[i], command);
            if(strncmp(Commands[i], command, strlen(Commands[i])) == 0){
                match_cmd = 1;
                strcpy(History[History_ind++], command);
                if(i == 0){ // add
                    if(command[strlen(Commands[i])] != ' ') printf("Specify a file\n");
                    else{
                        char tmp_path[6000] = "\0";
                        strcpy(tmp_path, path);
                        strcat(tmp_path, &command[strlen(Commands[i])+1]);
                        if(access(tmp_path, F_OK ) == -1) printf("%s not exist\n", &command[strlen(Commands[i])+1]);
                        else strcpy(filenames[file_num++], tmp_path);
                        
                    }
                }
                else if(i == 1){ // ls
                    listdir(path);
                }
                else if(i == 2){ // cd
                    if(command[strlen(Commands[i])] != ' ') printf("Specify a path\n");
                    else{

                        char tmp_path[6000] = "\0";
                        strcpy(tmp_path, path);
                        strcat(tmp_path, &command[strlen(Commands[i])+1]);
                        struct stat statbuf;
                        if(strcmp(&command[strlen(Commands[i])+1], "~\0") == 0) {memset(path, 0, sizeof(path)); path[0] = '.'; path[1] = '/';}
                        else if(strcmp(&command[strlen(Commands[i])+1], "/\0") == 0) {memset(path, 0, sizeof(path)); path[0] = '/';}
                        else if (stat(tmp_path, &statbuf) != 0) printf("%s not exist\n", &command[strlen(Commands[i])+1]);
                        else if (S_ISDIR(statbuf.st_mode) == 0) printf("%s is not a dir\n", &command[strlen(Commands[i])+1]);
                        else{
                            strcat(path, &command[strlen(Commands[i])+1]);
                            strcat(path, "/");
                        }
                        
                    }
                
                }
                else if(i == 3){ // files
                    Print_files(filenames, file_num);
                }
                else if(i == 4){ // users
                    int users_len = Parse_friend(users, status, friends, requests, sockfd);
                    for(int j=0;j<users_len;j++){
                        if(friends[j] == 1 && status[j] == 1)
                            printf("[5;7;1;44;36m%s[0m\n", users[j]);
                        else if(status[j] == 1)
                            printf("%s\n", users[j]);
                    }
                }
                else if(i == 5){ // send
                    if(command[strlen(Commands[i])] != ' ') printf("Specify a user\n");
                    int users_len = Parse_friend(users, status, friends, requests, sockfd);
                    for(int j=0;j<users_len;j++){
                        if(strcmp(users[j], &command[strlen(Commands[i])+1]) == 0 && status[j] == 1){
                            pthread_mutex_lock( &mutex1 );
                            for(int k=0;k<file_num;k++){
                                strcpy(file_obj[file_obj_ind].filename, filenames[k]);
                                strcpy(file_obj[file_obj_ind].target, users[j]);
                                strcpy(file_obj[file_obj_ind].from, user_info->username);
                                file_obj[file_obj_ind].status = 1;
                                file_obj_ind = ( file_obj_ind + 1 ) %1000;
                            }
                            pthread_mutex_unlock( &mutex1 );
                            memset(filenames, 0, sizeof(filenames));
                            file_num = 0;

                            break;
                        }
                        else if(j == users_len - 1) printf("users:%s not found or offline, try command:>users\n");
                    }
                
                }
                else if(i == 6){ // quit
                    cmd_ret = 127;
                }
                else if(i == 7){ // get
                }
            }
            else if(strlen(command) == 0) match_cmd = 1;
        }
        if(!match_cmd){
            printf("%s command not found~\n",command);
            strcpy(History[History_ind++], command);
        }
        if(cmd_ret == 127) return 0;
    }
    return 0;



/*
    while(1){
        Flush_term();
        //fprintf(stderr, "In Cmd_users\n");
        int users_len = Parse_users(users, status, sockfd);
        int term_users = Print_users(users, status, users_len, mode, choose_mode, choose, &choose_ind);
        if(term_users == 0){
            printf("No online user~\n");
            sleep(2);
            break;
        }
        unsigned char ch = getch();
        if(ch == DIR){
            ch = getch();
            ch = getch();
            if(ch == 67) choose++;
            else if(ch == 68) choose--;
            else if(ch == 66) choose+=3;
            else if(ch == 65) choose-=3;
            if(choose >= term_users) choose-=3;
            else if(choose < 0) choose = 0;
        }
        else if(ch == RETURN && choose_mode == 1){
            Flush_term();
            printf("Send to %s\n", users[choose_ind]);
            printf("Add file path:");
            while(scanf("%s", filenames[file_num++])){
                if(strlen(filenames[file_num-1]) == 0 || strcmp(filenames[file_num-1], "send\0") == 0) break;
                Flush_term();
                printf("Send to %s\n", users[choose_ind]);
                printf("Add file path:\n");
                Print_files(filenames, file_num);
                printf("\033[2;0H");
                printf("Add file path:");
            }
            file_num--;
            break;
        }
        else if(ch == 'g'){
            cJSON *send_check_file = cJSON_CreateObject();
            cJSON_AddStringToObject(send_check_file, "cmd", "check_file");
            cJSON_AddStringToObject(send_check_file, "me", user_info->username);
            char *send_check_file_json  = cJSON_PrintUnformatted(send_check_file);
            send(sockfd, send_check_file_json, strlen(send_check_file_json), 0);
            send(sockfd,"\n",1,0);
            int ret;
            recv(sockfd, &ret, sizeof(int), 0);
            if(ret == 0){
                printf("No file for you~\n");
                return 0;
            }
            printf("%d file(s) for you, receive or not?(y/n)", ret);
            char input;
            scanf("%c", &input);
            if(input == 'y'){
                cJSON *send_get_file = cJSON_CreateObject();
                cJSON_AddStringToObject(send_get_file, "cmd", "get_file");
                cJSON_AddStringToObject(send_get_file, "me", user_info->username);
                char *send_get_file_json  = cJSON_PrintUnformatted(send_get_file);
                send(sockfd, send_get_file_json, strlen(send_get_file_json), 0);
                send(sockfd,"\n",1,0);
                char get_file_buf[100000] = {0};
                int len = 0;
                while(1){
                    int ret1 = recv(sockfd, &get_file_buf[len], 1000, 0);
                    len+=ret1;
                    int num = 0;
                    for(int i=0;i<len;i++) if(get_file_buf[i] == '\n') num++;
                    if(num == ret) break;
                }
                Save_files(get_file_buf);
            }
            return 0;
            
        }
    }
    Flush_term();
    for(int i=0;i<file_num;i++){
        printf("Sending %s\n", filenames[i]);
        Send_file(user_info, sockfd, filenames[i], users[choose_ind]);
    }
    //Flush_term();
    return 0;
    */
}

int Cmd_help(struct User *user_info, int sockfd){
    printf(">users\n");
    printf(">friend\n");
    printf(">group\n");
    printf(">file\n");
    printf(">blacklist\n");
    printf(">quit\n");
    return 0;
}
