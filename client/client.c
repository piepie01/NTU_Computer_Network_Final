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
#include<sys/time.h>
#include <arpa/inet.h>
#include "json_include/cJSON/cJSON.h"
#define CONNECT_MAX 99

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <string>
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

struct connection{
    int port;
    char ip[20];
    struct timeval timeout;
    int times;
};

char trash[1000000];
int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    if ( (he = gethostbyname( hostname ) ) == NULL)
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    for(i = 0; addr_list[i] != NULL; i++)
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }

    return 1;
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
int Connect(struct connection *connect_info){
    int sockfd = 0;
    sockfd = socket(AF_INET , SOCK_STREAM , 0);

    if (sockfd == -1){
        printf("Fail to create a socket.");
    }

    struct sockaddr_in info;
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    info.sin_addr.s_addr = inet_addr(connect_info->ip);
    info.sin_port = htons(connect_info->port);

    int err = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
    if(err==-1){
        printf("Connection error\n");
        pthread_exit(NULL);    
    }
    return sockfd;

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
int Login_Register(int sockfd){
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
    if(ret == '1') return 1;
    else return 0;
    return 0;
}
int main(int argc, char *argv[])
{
    struct connection connect_info;
    parse_arg(argc, argv, &connect_info);

    //print_logo();
    int sockfd = Connect(&connect_info);
    int ret = 2, cnt = 0;
    cout << "\U0001F680 [32;1mPress ESC to register[0m" << endl;
    while(ret == 2 || (ret == 0 && cnt != 3)){      //ret = 0(fail), 1(success), 2(register)
        cout << endl << "\U0001F340 [47;34;1mLogin[0m \U0001F340" << endl;
        ret = Login_Register(sockfd);
        if(ret == 0) printf("\U0001F6AB [1;31mWrong Username or Passwd, remain [0m[1;35m%d[0m [1;31mtimes[0m\n", 3-(++cnt));
    }
    printf("\U0001F197 [1;32mLogin succeed[0m \U0001F197\n");
    sleep(1);
    print_logo();
    return 0;
}
