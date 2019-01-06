#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>    
#include <sys/stat.h>    
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <dlfcn.h>
#include "json_include/cJSON/cJSON.h"
#include "server.h"

struct fd_connection user_data[CONNECT_MAX];
char user_buf[CONNECT_MAX][JSON_LEN];
int user_buf_index[CONNECT_MAX];
char user_file[CONNECT_MAX][CONNECT_MAX][100000];
int user_file_ind[CONNECT_MAX];
int friend_matrix[CONNECT_MAX][CONNECT_MAX], name_len;
char names[CONNECT_MAX][40];
struct Group groups[CONNECT_MAX];

void printline(){
	printf("-----------------------------\n");
	return;
}
int new_user(int sockfd){
	printline();
	printf("new connect\n");
    struct sockaddr_in client_addr;
   	socklen_t addrlen = sizeof(client_addr);
    int client_fd = accept(sockfd, (struct sockaddr*) &client_addr, &addrlen);
    assert(client_fd >= 0);
	printf("connect succeed, fd : %d\n",client_fd);
	user_data[client_fd].status = DONGDONG_STATUS_IDLE;
	return client_fd;
}
void fd_quit(int fd, fd_set *readset){
	printline();
	memset(&user_data[fd],0,sizeof(user_data[fd]));
	user_data[fd].status = DONGDONG_STATUS_DISCONNECT;
	memset(user_buf[fd],0,sizeof(user_buf[fd]));
	user_buf_index[fd] = 0;

	close(fd);
	printf("close : %d\n",fd);
	FD_CLR(fd,readset);
	return;
}
void buf_init(){
	for(int i=0;i<CONNECT_MAX;i++) user_buf_index[i] = 0;
	memset(user_buf,0,sizeof(user_buf));
	memset(user_data,0,sizeof(user_data));
    memset(groups, 0, sizeof(groups));
	memset(names,0,sizeof(names));
	memset(friend_matrix,0,sizeof(friend_matrix));
    name_len = 0;
	for(int i=0;i<CONNECT_MAX;i++) user_data[i].status = DONGDONG_STATUS_DISCONNECT;
	for(int i=0;i<CONNECT_MAX;i++) user_file_ind[i] = 0;
    
    char user_dir[6] = "User\0";
    if(access(user_dir, F_OK) != 0) mkdir(user_dir, 0755);
    char messages_dir[10] = "Messages\0";
    if(access(messages_dir, F_OK) != 0) mkdir(messages_dir, 0755);


    FILE *pFile;
    pFile = fopen( "User/account.json","r" );
    if(pFile == NULL) return;
    char buf[4096] = {0};
    fread(buf, 4095, 1, pFile);
    fclose(pFile);

    int start = 0, len = strlen(buf);
    char tmp[4096] = {0};
    for(int i = 0;i<len;i++){
        tmp[i-start] = buf[i];
        if(buf[i] == '\n'){
            cJSON *root = cJSON_Parse(tmp);
            memset(tmp, 0, 4096);
            start = i+1;
            cJSON *user = cJSON_GetObjectItemCaseSensitive(root, "username");
            strcpy(names[name_len++], user->valuestring);
        }
    }
    
	return;
}

int sock_init(char port[]){
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(sockfd >= 0);
	struct sockaddr_in tmp;
	memset(&tmp,0,sizeof(tmp));
	tmp.sin_family = PF_INET;
	tmp.sin_addr.s_addr = INADDR_ANY;
	tmp.sin_port = htons(atoi(port));
	//printf("port : %d\n",atoi(port));
	int retval = bind(sockfd, (struct sockaddr*) &tmp, sizeof(tmp));
	if(retval){
		printf("socket fail\n");
		exit(0);
	}
	assert(!retval);
	retval = listen(sockfd, 1001);
	assert(!retval);
	return sockfd;
}
int find_name_ind(char *r){
    for(int i=0;i<name_len;i++) if(strcmp(names[i], r) == 0) return i;
    return -1;
}
cJSON* GET_JSON(int fd){
	cJSON *root = NULL;
	char tmp[6000];
	char copy[6000];
	memset(tmp,0,sizeof(tmp));
	memset(copy,0,sizeof(tmp));
    //printline();
    //printf("%s\n", user_buf[fd]);
    //printline();
	for(int i=0;i<user_buf_index[fd];i++){
		if(user_buf[fd][i] == '\n'){
			strncpy(tmp,user_buf[fd],i);
            //printf("-=-=-=-=-=\n");
            //printf("%s\n", tmp);
            //printf("-=-=-=-=-=\n");
			strcpy(copy,&user_buf[fd][i+1]);
			memset(user_buf[fd],0,sizeof(user_buf[fd]));
			strcpy(user_buf[fd],copy);
			user_buf_index[fd] = strlen(user_buf[fd]);
			root = cJSON_Parse(tmp);
            break;
		}
	}
	return root;
}
int check_account(cJSON *username, cJSON *passwd){
    FILE *pFile;
    pFile = fopen( "User/account.json","r" );
    if(pFile == NULL) return 0;
    char buf[4096] = {0};
    fread(buf, 4095, 1, pFile);
    fclose(pFile);
    printf("%s\n", buf);

    int start = 0, len = strlen(buf);
    char tmp[4096] = {0};
    int same = 0;
    for(int i = 0;i<len;i++){
        tmp[i-start] = buf[i];
        if(buf[i] == '\n'){
            cJSON *root = cJSON_Parse(tmp);
            memset(tmp, 0, 4096);
            start = i+1;
            cJSON *user = cJSON_GetObjectItemCaseSensitive(root, "username");
            cJSON *pass = cJSON_GetObjectItemCaseSensitive(root, "password");
            if(strcmp(user->valuestring, username->valuestring) == 0 && same == 0){
                same++;
                if(strcmp(pass->valuestring, passwd->valuestring) == 0) same++;
            }
        }
    }
    printf("Same value:%d\n",same);
    return same;
    
}
int handle_idle(int fd, cJSON *root){
	cJSON *cmd = cJSON_GetObjectItemCaseSensitive(root, "cmd");
	cJSON *username = cJSON_GetObjectItemCaseSensitive(root, "username");
	cJSON *passwd = cJSON_GetObjectItemCaseSensitive(root, "password");
    if(strcmp("Register\0", cmd->valuestring) == 0){
        printf("Register\n");
        int ret = check_account(username, passwd);  //ret = 0(OK), 1(same user), 2(same user and passwd)
        char ret_char[4] = "100";
        send(fd, &ret_char[ret], 1, 0);
        
        if(ret == 0){
            FILE *pFile;
            pFile = fopen( "User/account.json","a" );
            fwrite(cJSON_PrintUnformatted(root), strlen(cJSON_PrintUnformatted(root)), 1, pFile);
            fwrite("\n", 1, 1, pFile);
            fclose(pFile);
        }

    }
    else if(strcmp("Login\0", cmd->valuestring) == 0){
        printf("Login\n");
        int ret = check_account(username, passwd);  //ret = 0(No user), 1(wrong passwd), 2(OK)
        char ret_char[4] = "001";
        send(fd, &ret_char[ret], 1, 0);
        if(ret == 2){
            printf("Login succeed!\n");
            strcpy(user_data[fd].user.name, username->valuestring);
            return DONGDONG_STATUS_ONLINE;
        }

    }
    else if(strcmp("Login2\0", cmd->valuestring) == 0){
        strcpy(user_data[fd].user.name, username->valuestring);
        strcat(user_data[fd].user.name, "_send\0");
        return DONGDONG_STATUS_SEND;
    }
    else if(strcmp("Login3\0", cmd->valuestring) == 0){
        strcpy(user_data[fd].user.name, username->valuestring);
        strcat(user_data[fd].user.name, "_recv\0");
        return DONGDONG_STATUS_RECV;
    }
    return DONGDONG_STATUS_IDLE;
}
cJSON* Get_users_list(int mode, int fd){
    FILE *pFile;
    pFile = fopen( "User/account.json","r" );
    if(pFile == NULL) return 0;
    char buf[4096] = {0};
    fread(buf, 4095, 1, pFile);
    fclose(pFile);

    char friend_file[20] = "User/\0";
    strcat(friend_file, user_data[fd].user.name);
    pFile = fopen(friend_file, "r");
    if(pFile == NULL){
        pFile = fopen(friend_file, "w");
        fclose(pFile);
        pFile = fopen(friend_file, "r");
    }
    char buf2[4096] = {0};
    fread(buf2, 4095, 1, pFile);
    fclose(pFile);
    int buf2_len = strlen(buf2);
    printf("buf2 : %s\n", buf2);

    int start = 0, len = strlen(buf);
    char tmp[4096] = {0};
    char tmp_users[CONNECT_MAX][64];
    int tmp_status[CONNECT_MAX] = {0}, tmp_len = 0;
    memset(tmp_users, 0, sizeof(tmp_users));
    int f_status[CONNECT_MAX] = {0}, f_len = 0;
    int req_status[CONNECT_MAX] = {0}, req_len = 0;
    for(int i = 0;i<len;i++){
        tmp[i-start] = buf[i];
        if(buf[i] == '\n'){
            cJSON *root = cJSON_Parse(tmp);
            memset(tmp, 0, 4096);
            start = i+1;
            cJSON *user = cJSON_GetObjectItemCaseSensitive(root, "username");
            for(int j=0;j<CONNECT_MAX;j++){
                //if(j == fd) continue;  //user himself/herself
                if(strcmp(user->valuestring, user_data[j].user.name) == 0){
                    strcpy(tmp_users[tmp_len], user->valuestring);
                    tmp_status[tmp_len++] = 1;
                    break;
                }
                else if(j == CONNECT_MAX - 1)
                    strcpy(tmp_users[tmp_len++], user->valuestring);
            }
            int start = 0;
            for(int j=0;j<buf2_len;j++){
                if(buf2[j] == '\n'){
                    buf2[j] = '\0';
                    if(strcmp(&buf2[start], user->valuestring) == 0) f_status[f_len] = 1;
                    buf2[j] = '\n';
                    start = j+1;
                }
            }
            f_len++;

            int ind2 = find_name_ind(user_data[fd].user.name), ind1 = find_name_ind(user->valuestring);
            req_status[req_len++] = friend_matrix[ind1][ind2];
        }
    }
	cJSON *ret = cJSON_CreateObject();
    cJSON_AddStringToObject(ret, "cmd", "Get_users");
    cJSON *arr = cJSON_CreateArray();
    for(int i=0;i<tmp_len;i++){
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "username", tmp_users[i]);
        cJSON_AddNumberToObject(obj, "online", tmp_status[i]);
        cJSON_AddNumberToObject(obj, "friend", f_status[i]);
        cJSON_AddNumberToObject(obj, "req", req_status[i]);
        cJSON_AddItemToArray(arr, obj);
    }
    cJSON_AddItemToObject(ret, "data", arr);
	printf("%s\n",cJSON_Print(ret));
    
	return ret;
}
cJSON* Get_groups_list(int mode, int fd){
	cJSON *ret = cJSON_CreateObject();
    cJSON_AddStringToObject(ret, "cmd", "Get_group");
    cJSON *arr = cJSON_CreateArray();
    for(int i=0;i<CONNECT_MAX;i++){
        if(groups[i].status == 1){
            cJSON *obj = cJSON_CreateObject();
            cJSON_AddStringToObject(obj, "name", groups[i].name);
            cJSON_AddStringToObject(obj, "owner", groups[i].owner);
            cJSON_AddItemToArray(arr, obj);
        }
    }
    cJSON_AddItemToObject(ret, "data", arr);
	printf("%s\n",cJSON_Print(ret));
    
	return ret;
}
int Get_chat_history(char history[], cJSON *me, cJSON *you){
    char file1[64] = "Messages/\0";
    strcat(file1, me->valuestring);
    strcat(file1, you->valuestring);
    FILE *pFile = fopen( file1,"r" );
    if(pFile != NULL){
        fread(history, 60000-1, 1, pFile);
        if(strlen(history) == 0) history[0] = '\n';
        fclose(pFile);
        return 0;
    }
    char file2[64] = "Messages/\0";
    strcat(file2, you->valuestring);
    strcat(file2, me->valuestring);
    pFile = fopen( file2,"r" );
    if(pFile != NULL){
        fread(history, 60000-1, 1, pFile);
        if(strlen(history) == 0) history[0] = '\n';
        fclose(pFile);
        return 0;
    }
    pFile = fopen( file1,"w" );
    fclose(pFile);
    history[0] = '\n';
    return 0;


}
int Write_chat_history(cJSON *me, cJSON *you, cJSON *message){
    char file1[64] = "Messages/\0";
    strcat(file1, me->valuestring);
    strcat(file1, you->valuestring);
    FILE *pFile = fopen( file1,"r" );
    if(pFile != NULL){
        fclose(pFile);
        FILE *pFile = fopen( file1,"a" );
        fwrite(me->valuestring, strlen(me->valuestring), 1, pFile);
        fwrite(">", 1, 1, pFile);
        fwrite(message->valuestring, strlen(message->valuestring), 1, pFile);
        fwrite("\n", 1, 1, pFile);
        fclose(pFile);
        return 0;
    }
    char file2[64] = "Messages/\0";
    strcat(file2, you->valuestring);
    strcat(file2, me->valuestring);
    pFile = fopen( file2,"r" );
    if(pFile != NULL){
        fclose(pFile);
        FILE *pFile = fopen( file2,"a" );
        fwrite(me->valuestring, strlen(me->valuestring), 1, pFile);
        fwrite(">", 1, 1, pFile);
        fwrite(message->valuestring, strlen(message->valuestring), 1, pFile);
        fwrite("\n", 1, 1, pFile);
        fclose(pFile);
        return 0;
    }
    return 0;
}

int handle_online(int fd, cJSON *root){
	cJSON *cmd = cJSON_GetObjectItemCaseSensitive(root, "cmd");
    if(strcmp("Get_users\0", cmd->valuestring) == 0){
        printf("In Get_users\n");
        cJSON *root = Get_users_list(0, fd);
        char *send_root  = cJSON_PrintUnformatted(root);
        send(fd, send_root, strlen(send_root), 0);
        cJSON_Delete(root);
        return DONGDONG_STATUS_ONLINE;
    }
    if(strcmp("Chatroom\0", cmd->valuestring) == 0){
	    cJSON *me = cJSON_GetObjectItemCaseSensitive(root, "me");
	    cJSON *you = cJSON_GetObjectItemCaseSensitive(root, "you");
        char history[60000];
        memset(history, 0, sizeof(history));
        Get_chat_history(history, me, you);
        printf("history len : %d\n", strlen(history));
        send(fd, history, strlen(history), 0);
        return DONGDONG_STATUS_CHATROOM;
        
    }
    if(strcmp("Friend_request\0", cmd->valuestring) == 0){
	    cJSON *me = cJSON_GetObjectItemCaseSensitive(root, "me");
	    cJSON *you = cJSON_GetObjectItemCaseSensitive(root, "you");
        int ind1 = find_name_ind(me->valuestring), ind2 = find_name_ind(you->valuestring);
        printf("%s:%d %s:%d\n", me->valuestring, ind1, you->valuestring, ind2);
        friend_matrix[ind1][ind2] = 1;
        return DONGDONG_STATUS_ONLINE;
        
    }
    if(strcmp("Friend_accept\0", cmd->valuestring) == 0){
	    cJSON *me = cJSON_GetObjectItemCaseSensitive(root, "me");
	    cJSON *you = cJSON_GetObjectItemCaseSensitive(root, "you");
        char filename[30] = "User/\0";
        strcat(filename, me->valuestring);
        FILE *f = fopen(filename, "a");
        fwrite(you->valuestring, 1, strlen(you->valuestring), f);
        fwrite("\n", 1, 1, f);
        fclose(f);
        char filename1[30] = "User/\0";
        strcat(filename1, you->valuestring);
        f = fopen(filename1, "a");
        fwrite(me->valuestring, 1, strlen(me->valuestring), f);
        fwrite("\n", 1, 1, f);
        fclose(f);
        return DONGDONG_STATUS_ONLINE;
        
    }
    if(strcmp("Group_make\0", cmd->valuestring) == 0){
	    cJSON *me = cJSON_GetObjectItemCaseSensitive(root, "me");
	    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
        for(int i=0;i<CONNECT_MAX;i++){
            if(groups[i].status == 0){
                strcpy(groups[i].name, name->valuestring);
                strcpy(groups[i].owner, me->valuestring);
                groups[i].status = 1;
                break;
            }
        }
        return DONGDONG_STATUS_ONLINE;
    }
    if(strcmp("Get_group\0", cmd->valuestring) == 0){
        cJSON *root = Get_groups_list(0, fd);
        char *send_root  = cJSON_PrintUnformatted(root);
        send(fd, send_root, strlen(send_root), 0);
        cJSON_Delete(root);
        return DONGDONG_STATUS_ONLINE;
        
    }
    if(strcmp("Grouproom\0", cmd->valuestring) == 0){
	    cJSON *me = cJSON_GetObjectItemCaseSensitive(root, "me");
	    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
        for(int i=0;i<CONNECT_MAX;i++){
            if(strcmp(groups[i].name, name->valuestring) == 0){
	            cJSON *ret = cJSON_CreateObject();
                cJSON_AddStringToObject(ret, "me", me->valuestring);
                cJSON_AddStringToObject(ret, "message", "enter");
                char *send_root  = cJSON_PrintUnformatted(ret);
                printf("%s\n", send_root);
                for(int j=0;j<CONNECT_MAX;j++){
                    if(groups[i].fds[j] == 1) send(j, send_root, strlen(send_root), 0);
                }
                cJSON_Delete(ret);
                groups[i].fds[fd] = 1;
                break;
            }
        }
        
        return DONGDONG_STATUS_GROUPROOM;
        
    }
    return DONGDONG_STATUS_ONLINE;
}
int handle_chatroom(int fd, cJSON *root){
	cJSON *cmd = cJSON_GetObjectItemCaseSensitive(root, "cmd");
    if(strcmp("Chat\0", cmd->valuestring) == 0){
        printf("In Chat\n");
	    cJSON *me = cJSON_GetObjectItemCaseSensitive(root, "me");
	    cJSON *you = cJSON_GetObjectItemCaseSensitive(root, "you");
	    cJSON *message = cJSON_GetObjectItemCaseSensitive(root, "message");
        for(int i=0;i<CONNECT_MAX;i++){
            if(strcmp(you->valuestring, user_data[i].user.name) == 0 && user_data[i].status == DONGDONG_STATUS_CHATROOM){
	            send(i, cJSON_Print(root), strlen(cJSON_Print(root)), 0);            	
            }
        }
        Write_chat_history(me, you, message);
        return DONGDONG_STATUS_CHATROOM;
    }
    if(strcmp("Quit\0", cmd->valuestring) == 0){
        return DONGDONG_STATUS_ONLINE;
    }
    return DONGDONG_STATUS_CHATROOM;
}
int handle_grouproom(int fd, cJSON *root){
	cJSON *cmd = cJSON_GetObjectItemCaseSensitive(root, "cmd");
    if(strcmp("Chat\0", cmd->valuestring) == 0){
        printf("In Chat\n");
	    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
        for(int i=0;i<CONNECT_MAX;i++){
            if(strcmp(groups[i].name, name->valuestring) == 0){
                char *send_root  = cJSON_PrintUnformatted(root);
                printf("%s\n", send_root);
                for(int j=0;j<CONNECT_MAX;j++){
                    if(groups[i].fds[j] == 1) send(j, send_root, strlen(send_root), 0);
                }
                break;
            }
        }
        return DONGDONG_STATUS_GROUPROOM;
    }
    if(strcmp("Quit\0", cmd->valuestring) == 0){
	    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
	    cJSON *me = cJSON_GetObjectItemCaseSensitive(root, "me");
        for(int i=0;i<CONNECT_MAX;i++){
            if(strcmp(groups[i].name, name->valuestring) == 0){
                groups[i].fds[fd] = 0;
	            cJSON *ret = cJSON_CreateObject();
                cJSON_AddStringToObject(ret, "me", me->valuestring);
                cJSON_AddStringToObject(ret, "message", "quit");
                char *send_root  = cJSON_PrintUnformatted(ret);
                printf("%s\n", send_root);
                for(int j=0;j<CONNECT_MAX;j++){
                    if(groups[i].fds[j] == 1) send(j, send_root, strlen(send_root), 0);
                }
                cJSON_Delete(ret);
                break;
            }
        }
        return DONGDONG_STATUS_ONLINE;
    }
    return DONGDONG_STATUS_GROUPROOM;
}
int handle_send(int fd, cJSON *root){
	cJSON *cmd = cJSON_GetObjectItemCaseSensitive(root, "cmd");
    if(strcmp("file\0", cmd->valuestring) == 0){
	    cJSON *target = cJSON_GetObjectItemCaseSensitive(root, "you");
        char *sends = cJSON_PrintUnformatted(root);
        for(int i=0;i<CONNECT_MAX;i++){
            if(user_data[i].status == DONGDONG_STATUS_RECV && strncmp(target->valuestring, user_data[i].user.name, strlen(target->valuestring)) == 0){
                send(i, sends, strlen(sends), 0);
                send(i, "\n", 1, 0);
            }
        }
    }
    return DONGDONG_STATUS_SEND;
}
int handle_command(int fd,cJSON *root){
	//cJSON *cmd = cJSON_GetObjectItemCaseSensitive(root, "cmd");
	//printline();
	//printf("%s\n",cJSON_Print(root));
    if(user_data[fd].status == DONGDONG_STATUS_IDLE){
        int next_status = handle_idle(fd, root);
        user_data[fd].status = next_status;
    }
    else if(user_data[fd].status == DONGDONG_STATUS_ONLINE){
        user_data[fd].status = handle_online(fd, root);
    }
    else if(user_data[fd].status == DONGDONG_STATUS_CHATROOM){
        user_data[fd].status = handle_chatroom(fd, root);
    }
    else if(user_data[fd].status == DONGDONG_STATUS_GROUPROOM){
        user_data[fd].status = handle_grouproom(fd, root);
    }
    else if(user_data[fd].status == DONGDONG_STATUS_SEND){
        user_data[fd].status = handle_send(fd, root);
    }
	return 0;
}
int main(int argc, char **argv){
	if(argc != 2){
		fprintf(stderr,"without appointed port\n");
		exit(1);
	}
	buf_init(); //initial buffer & mkdir User
	int sockfd = sock_init(argv[1]);
	int retval;

	fd_set readset,workerset;
	fd_set working_readset;
	FD_ZERO(&readset);
	FD_ZERO(&workerset);
	FD_SET(sockfd,&readset);
	FD_SET(STDIN_FILENO,&readset);
	struct timeval time;
	time.tv_sec = 0;
	time.tv_usec = 1;

	while (1){
		FD_ZERO(&working_readset);
		memcpy(&working_readset, &readset, sizeof(fd_set));
		retval = select(FD_SETSIZE, &working_readset, NULL, NULL, &time);
		if(retval < 0){
			fprintf(stderr,"select error\n");
			return 0;
		}
		else if(retval != 0){
			if(FD_ISSET(STDIN_FILENO,&working_readset)){
	            for(int i=0;i<CONNECT_MAX;i++){
                    if(user_data[i].status != DONGDONG_STATUS_DISCONNECT) close(i);
                }
				break;
			}
			for(int fd=3;fd<FD_SETSIZE;fd++){
				if(!FD_ISSET(fd,&working_readset)) continue;
				if(fd == sockfd){
					int client_fd = new_user(sockfd);
					FD_SET(client_fd,&readset);
				}
				else{
					ssize_t sz;
					int buffer_size = 1000;
					sz = recv(fd,&user_buf[fd][user_buf_index[fd]],buffer_size,0);
					user_buf_index[fd]+=(int)sz;

					if(sz == 0) fd_quit(fd,&readset);
					else{
                        while(1){
						    cJSON *root = GET_JSON(fd);
						    if(root != NULL){
							    handle_command(fd, root);
                                cJSON_Delete(root);
						    }else break;
                        }
					}
				}
			}
		}

	}

	close(sockfd);
	return 0;
}
