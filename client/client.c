#include <stdio.h>
#include <ctype.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "json_include/cJSON/cJSON.h"

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "client.h"
#include "connect.h"
#include "command.h"
using namespace std;

void print_logo(){
    FILE *pFile;
    char buffer[5000];

    pFile = fopen( "logo", "r" );
    if ( NULL == pFile ){
        printf( "Open LOGO failure" );
        return;
    }else{
        fread( buffer, 5000, 1, pFile );
        printf( "%s", buffer );
    }

    fclose(pFile);
    return;
}


void parse_arg(int argc, char *argv[], struct connection *connect_info){
    if(argc < 2){
        printf("./client ip:port\n");
        return;
    }

    int goal = 0, len = strlen(argv[1]), judge = 0;
    for(int i=0;i<len;i++){
        if(argv[1][i] == ':') goal = i;
        else if(argv[1][i] != '.' && !isdigit(argv[1][i])) judge = 1;
    }
    argv[1][goal] = '\0';
    connect_info->port = atoi(&argv[1][goal+1]);
    if(judge) hostname_to_ip(argv[1],connect_info->ip);
    else strcpy(connect_info->ip,argv[1]);
    
    //printf("===SERVER===\n");
    //printf("[IP]:%d\n[PORT]%s\n", connect_info->port, connect_info->ip);
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





string get_stdin(string prompt, bool show_asterisk=true, bool visable=true){
    char space[17] = {0}, backspace[17] = {0};
    memset(space, ' ', 16);
    memset(backspace, '\b', 16);
    const char BACKSPACE=127;
    const char ESC=27;
    const char RETURN=10;
    
    cout << prompt;
    printf(":[46m%s[0m%s", space, backspace); //16 space

    string password;
    unsigned char ch=0;

    while((ch=getch())!=RETURN){
        if(ch == ESC){
              return "Register";
        }
        if(ch == BACKSPACE){
            if(password.length()!=0){
                if(show_asterisk)
                    cout <<"\b[46m [0m\b";
                password.resize(password.length()-1);
            }
        }
        else{
            password+=ch;
            if(visable)
                cout << "[46m" << ch << "[0m";
            else if(show_asterisk)
                cout << "[46m*[0m";
        }
    }
    cout << endl;
    return password;
}
int Register(int sockfd){
    cout << endl << "\U0001F680 [44;32;1mRegister[0m \U0001F680" << endl;
    string username, passwd;
    username = get_stdin("Username", true, true);
    passwd = get_stdin("Password", true, false);

    cJSON *send_reg = cJSON_CreateObject();
    cJSON_AddStringToObject(send_reg, "cmd", "Register");
    cJSON_AddStringToObject(send_reg, "username", username.c_str());
    cJSON_AddStringToObject(send_reg, "password", passwd.c_str());
    char *send_reg_json  = cJSON_PrintUnformatted(send_reg);
    send(sockfd, send_reg_json, strlen(send_reg_json), 0);
    send(sockfd,"\n",1,0);

    char ret;
    recv(sockfd, &ret, 1, 0);
    if(ret == '1') printf("\U0001F192 [1;32mRegister succeed[0m \U0001F192\n");
    else printf("\U0001F196 [1;31mUsername has been used[0m \U0001F196\n");
    return 0;
}
int Login_Register(int sockfd, struct User *user_info){
    string username, passwd;

    username = get_stdin("Username", true, true);
    if(username == "Register"){
        cout << endl;
        Register(sockfd);
        return 2;
    }

    passwd = get_stdin("Password", true, false);
    if(passwd == "Register"){
        cout << endl;
        Register(sockfd);
        return 2;
    }

    cJSON *send_login = cJSON_CreateObject();
    cJSON_AddStringToObject(send_login, "cmd", "Login");
    cJSON_AddStringToObject(send_login, "username", username.c_str());
    cJSON_AddStringToObject(send_login, "password", passwd.c_str());
    char *send_login_json  = cJSON_PrintUnformatted(send_login);
    send(sockfd, send_login_json, strlen(send_login_json), 0);
    send(sockfd,"\n",1,0);

    char ret;
    recv(sockfd, &ret, 1, 0);
    if(ret == '1'){
        strcpy(user_info->username, username.c_str());
        strcpy(user_info->passwd, passwd.c_str());
        return 1;
    }
    else return 0;
    return 0;
}
int Search_Command(char command[], int *command_ind, char Commands[][20], int Commands_num, int all){
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
int Get_Command(char command[], struct User *user_info, char Commands[][20], int Commands_num, char History[][20], int History_ind){
    printf("%s> ", user_info->username);
    const char BACKSPACE=127;
    const char TAB=9;
    const char RETURN=10;
    const char DIR=27;

    int command_ind = 0;
    char ch = 0;
    int tab_num = 0;
    int now_history = History_ind;
    char space[100] = "\0";
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
                printf("\r%s\r%s> %s", space, user_info->username, command);
            };
            if(ch == 66 && History_ind!=now_history+1){
                strcpy(History[History_ind++], command);
                if(strlen(command) == 0 && History_ind == now_history+1) History_ind--;
                memset(command, 0, 20);
                strcpy(command, History[History_ind]);
                command_ind = strlen(command);
                printf("\r%s\r%s> %s", space, user_info->username, command);
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
            for(int i=20-2;i>=command_ind;i--) command[i+1] = command[i];
            command[command_ind++] = ch;
            tab_num = 0;
            printf("%s",&command[command_ind-1]);
            for(int i=command_ind;i<(int)strlen(command);i++) printf("\b");
        }
        if(tab_num == 1){
            if(Search_Command(command, &command_ind, Commands, Commands_num, 0) == 1) tab_num--; 
        }
        else if(tab_num == 2){
            if(Search_Command(command, &command_ind, Commands, Commands_num, 1)) printf("\n%s> %s", user_info->username, command);
            tab_num = 0;
            
        }
    }
    printf("\n");
    return 0;
}
int Parse_Commands(char Commands[][20]){
    FILE *pFile;
    char buffer[5000];
    pFile = fopen( "command.txt", "r" );
    if ( NULL == pFile ){
        printf( "Open command.txt failure" );
        return 0;
    }else fread( buffer, 5000, 1, pFile );
    fclose(pFile);

    int num = 0, len = strlen(buffer), ind = 0;
    for(int i=0;i<len;i++){
        if(buffer[i] == '\n'){
            Commands[++num][ind] = '\0';
            ind = 0;
        }else Commands[num][ind++] = buffer[i];
    }
    return num;
    
}
int Parse_func(int (*Commands_func[])(struct User*)){
    Commands_func[0] = Cmd_quit;
    Commands_func[1] = Cmd_users;
    Commands_func[2] = Cmd_friend;
    Commands_func[3] = Cmd_whoami;
    Commands_func[4] = Cmd_file;
    return 0;
}
int Command_Interface(struct User *user_info){
    char Commands[50][20];
    char History[10000][20];
    int History_ind = 0;
    memset(Commands, 0, sizeof(Commands));
    memset(History, 0, sizeof(History));
    int Commands_num = Parse_Commands(Commands);
    int (*Commands_func[50])(struct User*);
    Parse_func(Commands_func);
    while(1){
        char command[20] = "\0";
        int ret = Get_Command(command, user_info, Commands, Commands_num, History, History_ind);
        int match_cmd = 0, cmd_ret = 0;
        for(int i=0;i<Commands_num;i++){
            if(strcmp(Commands[i], command) == 0){
                match_cmd = 1;
                cmd_ret = (*Commands_func[i])(user_info);
                strcpy(History[History_ind++], command);
            }
            else if(strlen(command) == 0) match_cmd = 1;
        }
        if(!match_cmd){
            printf("%s command not found~\n",command);
            strcpy(History[History_ind++], command);
        }
        if(cmd_ret == -1) break;
        
    }
    return 0;
}
int main(int argc, char *argv[]){
    struct connection connect_info;
    struct User user_info;
    parse_arg(argc, argv, &connect_info);

    int sockfd = Connect(&connect_info);
    int ret = 2, cnt = 0;
    cout << "\U0001F680 [32;1mPress ESC to register[0m" << endl;
    while(ret == 2 || (ret == 0 && cnt != 3)){      //ret = 0(fail), 1(success), 2(register)
        cout << endl << "\U0001F340 [47;34;1mLogin[0m \U0001F340" << endl;
        ret = Login_Register(sockfd, &user_info);
        if(ret == 0) printf("\U0001F6AB [1;31mWrong Username or Passwd, remain [0m[1;35m%d[0m [1;31mtimes[0m\n", 3-(++cnt));
    }
    printf("\U0001F197 [1;32mLogin succeed[0m \U0001F197\n");
    sleep(1);
    print_logo();

    Command_Interface(&user_info);
    close(sockfd);

    return 0;
}
