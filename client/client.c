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
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "client.h"
#include "connect.h"
#include "command.h"
using namespace std;
struct winsize w1;
pthread_t t_send, t_receive;
int fd_send, fd_receive;
/*
void Flush_term(){
    printf("\033[%d;%dH", 0, 0);
    for(int i=0;i<100;i++)
        for(int j=0;j<200;j++) printf("%c", " \n"[j==199]);
    printf("\033[%d;%dH", 0, 0);
    return;
}
*/
void print_logo_line(char buffer[], int times){
    Flush_term();
    int len = strlen(buffer), ret = 0;
    char space[60] = "\0";
    memset(space, ' ', sizeof(space)-1);
    for(int i=0;i<len;i++){
        if(ret >= 16 && buffer[i-1] == '\n') printf("%s", &space[times]);
        printf("%c", buffer[i]);
        if(buffer[i] == '\n') ret++;
    }

    usleep(200000);
    return;
}
void print_logo(){
    FILE *pFile;
    char buffer[5000], buffer2[5000];

    pFile = fopen( "logo", "r" );
    if ( NULL == pFile ){
        printf( "Open LOGO failure" );
        return;
    }else fread( buffer, 5000, 1, pFile );
    fclose(pFile);

    pFile = fopen( "logo3", "r" );
    if ( NULL == pFile ){
        printf( "Open LOGO failure" );
        return;
    }else fread( buffer2, 5000, 1, pFile );
    fclose(pFile);
    int times = 0;
    for(int i=0;i<7;i++){
        print_logo_line(buffer, times+=2);
        print_logo_line(buffer2, times+=2);
    }
    Flush_term();
    return;
}

int flash(char filename[]){
    srand(time(NULL));
    int color[7] = {31, 32, 33, 34, 35, 36, 37};
    FILE *f = fopen(filename, "r");
    char pic[4000] = "\0";
    int pic_len = fread(pic, 1, sizeof(pic), f);
    FILE *f1 = fopen("meow", "r");
    char pic1[4000] = "\0";
    int pic1_len = fread(pic1, 1, sizeof(pic1), f1);
    int wid = 0, hei = 0;
    int tmp_wid = 0;
    for(int i=0;i<pic_len;i++){
        tmp_wid++;
        if(pic[i] == '\n'){
            hei++;
            wid = (tmp_wid > wid)?tmp_wid:wid;
            tmp_wid = 0;
        }
    }
    int wid1 = 0, hei1 = 0;
    int tmp_wid1 = 0;
    for(int i=0;i<pic1_len;i++){
        tmp_wid1++;
        if(pic1[i] == '\n'){
            hei1++;
            wid1 = (tmp_wid1 > wid1)?tmp_wid1:wid1;
            tmp_wid1 = 0;
        }
    }
    wid = 18;
    wid1 = 18;
    int lt[2] = {0,0}, ld[2] = {hei, 0}, rt[2] = {0, wid}, rd[2] = {hei, wid}, d[2] = {1, 1};
    int lt1[2] = {20,50}, ld1[2] = {20+hei1, 50}, rt1[2] = {20, wid1+50}, rd1[2] = {20+hei1, wid1+50}, d1[2] = {1, -1};
    int color_ind = rand()%7;
    while(1){
        if(kbhit()){
            getch();
            Flush_term();
            break;
        }
        lt[0]+=d[0], lt[1]+=d[1];
        ld[0]+=d[0], ld[1]+=d[1];
        rt[0]+=d[0], rt[1]+=d[1];
        rd[0]+=d[0], rd[1]+=d[1];
        lt1[0]+=d1[0], lt1[1]+=d1[1];
        ld1[0]+=d1[0], ld1[1]+=d1[1];
        rt1[0]+=d1[0], rt1[1]+=d1[1];
        rd1[0]+=d1[0], rd1[1]+=d1[1];
        Flush_term();

        if(lt[1] > lt1[1]){
        for(int i=0;i<=lt[0];i++) printf("\n");
        for(int i=0;i<=lt[1];i++) printf(" ");

        printf("[%dm", color[color_ind]);
        for(int i=0;i<pic_len;i++){
            if(pic[i] == '\n'){
                printf("\n");
                for(int j=0;j<=lt[1];j++) printf(" ");
            }else printf("%c", pic[i]);

        }
        printf("[0m");

        printf("\033[%d;%dH", 0, 0);

        printf("\033[%d;%dH", 0, 0);
        for(int i=0;i<=lt1[0];i++) printf("\n");
        for(int i=0;i<=lt1[1];i++) printf(" ");
        printf("[%dm", color[color_ind]);
        for(int i=0;i<pic1_len;i++){
            if(pic1[i] == '\n'){
                    printf("\n");
                for(int j=0;j<=lt1[1];j++) printf(" ");
            }else printf("%c", pic1[i]);

        }
        printf("[0m");
        }
        else{

        for(int i=0;i<=lt1[0];i++) printf("\n");
        for(int i=0;i<=lt1[1];i++) printf(" ");
        printf("[%dm", color[color_ind]);
        for(int i=0;i<pic1_len;i++){
            if(pic1[i] == '\n'){
                    printf("\n");
                for(int j=0;j<=lt1[1];j++) printf(" ");
            }else printf("%c", pic1[i]);

        }
        printf("[0m");
        printf("\033[%d;%dH", 0, 0);

        for(int i=0;i<=lt[0];i++) printf("\n");
        for(int i=0;i<=lt[1];i++) printf(" ");

        printf("[%dm", color[color_ind]);
        for(int i=0;i<pic_len;i++){
            if(pic[i] == '\n'){
                printf("\n");
                for(int j=0;j<=lt[1];j++) printf(" ");
            }else printf("%c", pic[i]);

        }
        printf("[0m");
        }

        printf("\033[%d;%dH", 2, 0);
        printf("[%dm", color[color_ind]);
        for(int i=0;i<w1.ws_col;i++) printf("█");
        printf("[0m");

        printf("\033[%d;%dH", w1.ws_row-2, 0);
        printf("[%dm", color[color_ind]);
        for(int i=0;i<w1.ws_col;i++) printf("█");
        printf("[0m");


        printf("\033[%d;%dH", w1.ws_row-1, 0);
        printf("[%dm", color[color_ind]);
        for(int i=0;i<w1.ws_col;i++) printf("█");
        printf("[0m");
        printf("\033[%d;%dH", 0, 0);
        usleep((int)3e+5);

        if(rt[1] == w1.ws_col){
            d[1] = -1;
            color_ind = rand()%7;
        }
        if(lt[1] == 0){
            d[1] = 1;
            color_ind = rand()%7;
        }
        if(rt[0] == 0){
            d[0] = 1;
            color_ind = rand()%7;
        }
        if(ld[0] == w1.ws_row-4){
            d[0] = -1;
            color_ind = rand()%7;
        }

        if(rt1[1] == w1.ws_col){
            d1[1] = -1;
            color_ind = rand()%7;
        }
        if(lt1[1] == 0){
            d1[1] = 1;
            color_ind = rand()%7;
        }
        if(rt1[0] == 0){
            d1[0] = 1;
            color_ind = rand()%7;
        }
        if(ld1[0] == w1.ws_row-4){
            d1[0] = -1;
            color_ind = rand()%7;
        }
    }
    return 0;
}

void parse_arg(int argc, char *argv[], struct connection *connect_info){
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w1);
    if(argc != 2){
        //printf("./client ip:port\n");
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
/*
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
*/




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
    Flush_term();
    cout << endl << "\U0001F680 [44;32;1mRegister[0m \U0001F680" << endl;
    string username, passwd;
    username = get_stdin("Username", true, true);
    passwd = get_stdin("Password", true, false);

    int lenpass = strlen(passwd.c_str());
    char xorkey = '?', key = passwd[1];
    for(int i=0; i<lenpass; i++){
        passwd[i] = passwd[i] ^ xorkey;
        passwd[i] = passwd[i] + key;
    }
    srand(passwd[0]);    
    if(strlen(passwd.c_str())<32){
        for(int i=lenpass; i<32; i++){
            char a = rand()%128;
            passwd += a;
        }
    } 

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

int Login_Register(int sockfd, struct User *user_info, struct connection *connect_info){
    string username, passwd;

    username = get_stdin("Username", true, true);
    if(username == "Register"){
        Register(sockfd);
        return 2;
    }

    passwd = get_stdin("Password", true, false);
    if(passwd == "Register"){
        Register(sockfd);
        return 2;
    }

    
    int lenpass = strlen(passwd.c_str());
    char xorkey = '?', key = passwd[1];
    for(int i=0; i<lenpass; i++){
        passwd[i] = passwd[i] ^ xorkey;
        passwd[i] = passwd[i] + key;
    }
    srand(passwd[0]);    
    if(strlen(passwd.c_str())<32){
        for(int i=lenpass; i<32; i++){
            char a = rand()%128;
            passwd += a;
        }
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
        fd_send = Connect(connect_info);
        cJSON *send_login2 = cJSON_CreateObject();
        cJSON_AddStringToObject(send_login2, "cmd", "Login2");
        cJSON_AddStringToObject(send_login2, "username", username.c_str());
        char *send_login2_json  = cJSON_PrintUnformatted(send_login2);
        send(fd_send, send_login2_json, strlen(send_login2_json), 0);
        send(fd_send,"\n",1,0);
        pthread_create(&t_send, NULL, File_send, (void*) &fd_send);

        fd_receive = Connect(connect_info);
        cJSON *send_login3 = cJSON_CreateObject();
        cJSON_AddStringToObject(send_login3, "cmd", "Login3");
        cJSON_AddStringToObject(send_login3, "username", username.c_str());
        char *send_login3_json  = cJSON_PrintUnformatted(send_login3);
        send(fd_receive, send_login3_json, strlen(send_login3_json), 0);
        send(fd_receive,"\n",1,0);
        pthread_create(&t_receive, NULL, File_receive, (void*) &fd_receive);
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
    enable_raw_mode();
    printf("%s> ", user_info->username);
    fflush(stdout);
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
    char protect[20] = "reconnect.txt\0";
    clock_t start = clock();
    while(1){
        //cout << (int)ch << endl;
        if((float)(clock() - start)/CLOCKS_PER_SEC > 5.0){
            flash(protect);
            start = clock();
            printf("\r%s\r%s> %s", space, user_info->username, command);
        }
        if(!kbhit()){
            fflush(stdout);
            continue;
        }
        start = clock();
        ch=getch();
        if(ch == RETURN) break;
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
            if((int)strlen(command) <= 19){
                for(int i=20-2;i>=command_ind;i--) command[i+1] = command[i];
                command[command_ind++] = ch;
                tab_num = 0;
                printf("%s",&command[command_ind-1]);
                for(int i=command_ind;i<(int)strlen(command);i++) printf("\b");
            }
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
    disable_raw_mode();
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
int Parse_func(int (*Commands_func[])(struct User*, int)){
    Commands_func[0] = Cmd_quit;
    Commands_func[1] = Cmd_users;
    Commands_func[2] = Cmd_friend;
    Commands_func[3] = Cmd_group;
    Commands_func[4] = Cmd_file;
    Commands_func[5] = Cmd_help;    
    Commands_func[6] = Cmd_blacklist;
    return 0;
}
int Command_Interface(struct User *user_info, int sockfd){
    char Commands[50][20];
    char History[10000][20];
    int History_ind = 0;
    memset(Commands, 0, sizeof(Commands));
    memset(History, 0, sizeof(History));
    int Commands_num = Parse_Commands(Commands);
    int (*Commands_func[50])(struct User*, int);
    Parse_func(Commands_func);
    int cmd_ret;
    while(1){
        char command[20] = "\0";
        int ret = Get_Command(command, user_info, Commands, Commands_num, History, History_ind);
        int match_cmd = 0;
        cmd_ret = 0;
        for(int i=0;i<Commands_num;i++){
            //fprintf(stdout, "---%s---%s---\n", Commands[i], command);
            if(strcmp(Commands[i], command) == 0){
                match_cmd = 1;
                cmd_ret = (*Commands_func[i])(user_info, sockfd);//Cmd_xxxx
                strcpy(History[History_ind++], command);
            }
            else if(strlen(command) == 0) match_cmd = 1;
        }
        if(!match_cmd){
            printf("%s command not found~ type the help command for more information~ \n",command);
            strcpy(History[History_ind++], command);
        }
        if(cmd_ret == 127) return cmd_ret;
    }
    return 1;
}
int main(int argc, char *argv[]){
    Command_var_init();
    struct connection connect_info;
    struct User user_info;
    parse_arg(argc, argv, &connect_info);

    while(1){
        pid_t pid = fork();
        if(pid == 0){
            int sockfd = Connect(&connect_info);
            if(sockfd == 0){
                printf("Connection error\n");
                break;
            }
            int ret = 2, cnt = 0;
            while(ret == 2 || (ret == 0 && cnt != 3)){      //ret = 0(fail), 1(success), 2(register)
                Flush_term();
                cout << "\U0001F680 [32;1mPress ESC to register[0m" << endl;
                if(cnt != 0) printf("\U0001F6AB [1;31mWrong Username or Passwd, remain [0m[1;35m%d[0m [1;31mtimes[0m\n", 3-cnt);

                cout << endl << "\U0001F340 [47;34;1mLogin[0m \U0001F340" << endl;
                ret = Login_Register(sockfd, &user_info, &connect_info);
                if(ret == 0) cnt++;
            }
            printf("\U0001F197 [1;32mLogin succeed[0m \U0001F197\n");
            sleep(1);
            print_logo();
    
            Command_Interface(&user_info, sockfd);
            close(sockfd);
            break;
        }else{
            int detect_fd = Connect(&connect_info);
            if(detect_fd == 0){
                printf("Connection error\n");
                break;
            }
            struct timeval time = {0, 100000};
            int status;
            pid_t ret_pid;
            while(1){
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(detect_fd, &fds);
                select(detect_fd+1, &fds, NULL, NULL, &time);
                ret_pid = waitpid(pid, &status, WNOHANG);
                if(ret_pid == pid) return 0;
                if(FD_ISSET(detect_fd,&fds)){
                    kill(pid, SIGKILL);
                    break;
                }
            }
        }
        Flush_term();
        printf("Reconnecting...\n");

    }

    return 0;
}
