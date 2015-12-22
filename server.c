#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG_MODE

#define MAX_LISTEN_QUEUE 128

#define BUF_LEN 1028
#define SERVER_PORT 8080
#define COMMAND_LEN 1028

#define CMD_EXEC     1
#define CMD_LOGIN    2
#define CMD_NONE    -1
#define CMD_EXIT    -2

const char *cmd_exec = "cmd";
const char *cmd_login = "login";

const char *login_user = "user";
const char *login_passwd = "passwd";
const char *not_proper_msg = "not proper message";
const char *cmd_exit = "exit";



int ifUser(const char *user, const char *passwd);

int get_command(char *request, char *command, int *type);

int init_server(int * fd, int type, const struct sockaddr *addr, socklen_t alen,int qlen);

void serve(int sockfd);


int main(int argc, char*argv[]){

    if(argc != 2)
    {
        printf("usage : %s [port_number]\n", argv[0]);
        return -1;
    }

    int sockfd;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    if(init_server(&sockfd,SOCK_STREAM,(struct sockaddr*)&addr,sizeof(addr),MAX_LISTEN_QUEUE)<0)
    {
        perror("init_server error\n");
        return -1;
    }

    // should be multi process
    while(1){

        int newfd = accept(sockfd, NULL, NULL);
        if(newfd==-1){
            perror("accept error");
        }

        serve(newfd);
        close(newfd);
    }
}









int ifUser(const char *user, const char *passwd)
{
        if((strcmp(user,"root")==0) && (strcmp(passwd,"123")==0))
            return 1;
        else
            return 0;
}

int get_command(char *request, char *command, int *type)
{
        printf("http_request %s\n",request );

        int firstWordLen = (int)(strchr(request,' ')-request);

        if(strncmp(request,cmd_exec,firstWordLen)==0)
        {
            //to execute a command
            strcpy(command,strchr(request,' ')+1);
            if(type != NULL)
                *type = CMD_EXEC;
            return 0;
        }
        else if(strncmp(request,cmd_login,firstWordLen)==0)
        {
            //to login
            char *itr = strchr(request,' ');

            #ifdef DEBUG_MODE
            printf("request : %s\n", request);
            #endif


            while(1)
            {
                if(*itr != ' ')
                    break;
                itr++;
            }

            if(strncmp(itr,login_user,strlen(login_user)) == 0)
            {
                itr = itr + strlen(login_user);
            }
            else
            {
                printf("login formation error : no username\n");
                return -1;
            }


            while(1)
            {
                if(*itr != ' ')
                    break;
                itr++;
            }

            //now itr points to username

            int userLen = (int)(strchr(itr,' ') - itr);
            char * user = (char*)malloc((size_t)(userLen+1));
            strncpy(user,itr,userLen);
            user[userLen] = '\0';


            itr = itr + userLen;
            #ifdef DEBUG_MODE
            printf("username : set\n");
            #endif

            while(1)
            {
                if(*itr != ' ')
                    break;
                itr++;
            }


            if(strncmp(itr,login_passwd,strlen(login_passwd)) == 0)
            {
                itr = itr + strlen(login_passwd);
            }
            else
            {
                printf("login formation error : no passwd\n");
                return -1;
            }

            while(1)
            {
                if(*itr != ' ')
                    break;
                itr++;
            }

            int passwdLen = 0;
            char * temp = itr;
            for (;*temp!=' ' && *temp != '\0' && *temp !='\n';temp++)
            {
                passwdLen++;
            }
            char *passwd = (char*)malloc(passwdLen + 1);
            strncpy(passwd,itr,passwdLen);
            passwd[passwdLen] = '\0';

            #ifdef DEBUG_MODE
            printf("password : set\n");
            #endif


            #ifdef DEBUG_MODE
            printf("user : %s\n", user);
            printf("passwd : %s\n", passwd);
            #endif
            if(ifUser(user,passwd) == 1)
            {
                if(type != NULL)
                    *type = CMD_LOGIN;
                strcpy(command,"success");
                return 0;
            }

            else
                return -1;

        }
        else if(strncmp(request,cmd_exit,firstWordLen)==0)
        {
            *type = CMD_EXIT;
            return 0;
        }
        else
        {
            strcpy(command,not_proper_msg);
            return -1;
        }

}


int init_server(int * fd, int type, const struct sockaddr *addr, socklen_t alen,int qlen)
{

        if ((*fd = socket(addr->sa_family, type, 0)) < 0)
        {
            perror("socket error\n");
            return -1;
        }

        if (bind(*fd, addr, alen) < 0) {
            perror("bind error\n");
            close(*fd);
            return -1;
        }
        if (type == SOCK_STREAM || type == SOCK_SEQPACKET) {
            if (listen(*fd, qlen) < 0) {
                perror("listen error\n");
                close(*fd);
                return -1;
            }
        }
        return(*fd);
}

void serve(int sockfd)
{
    char buf[BUF_LEN];
    int login =0;
    while(1)
    {
        memset(buf,0,BUF_LEN);
        int recvStat=recv( sockfd,buf,BUF_LEN,0);
        if(recvStat==0){
            printf("complete \n");
            continue;
        }else if(recvStat==-1){
            perror("recv error");
        }else{

        printf("recv %s \n",buf);
        char *command = (char*)malloc(COMMAND_LEN);
        memset(command,0,COMMAND_LEN);

        int type = CMD_NONE;
        get_command(buf,command,&type);


        if(type == CMD_LOGIN)
        {
            login = 1;
            write(sockfd,"success",7);
            continue;
        }

        else if(type == CMD_EXEC)
        {
            //printf("111\n");
            int stdInCopy = dup(STDOUT_FILENO);
            dup2(sockfd,STDOUT_FILENO);
            //printf("222\n");
            system(command);
            fflush(stdout);
            dup2(stdInCopy,STDOUT_FILENO);
            close(stdInCopy);
            write(sockfd," ",1);
            //printf("333\n");
        }
        else if(type == CMD_NONE)
        {
            write(sockfd,not_proper_msg,strlen(not_proper_msg)+1);
        }
        else if(type == CMD_EXIT)
        {
            break;
        }
        }

    }
}


