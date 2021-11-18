#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#include "server.h"
#include "utility.h"
#include "request.h"
#include "response.h"
#include "kv.h"
#include "handler.h"

static Response *execHandler(Server *server, Request *request)
{
    ListCell *current = (server->handlers)->head;
    Response *response = NULL;
    while(current != NULL) {
        Handler *handler = (current->value);
        response = (*handler)(request);
        current = current->next;
        if (response) return response;
    }
    return response403(request->path);
}

static void printConnectInfo(struct sockaddr_in *sin) 
{
    changePrintColor("bold-yellow");
    printf("[info] connected from %s:%d\n",
            inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
    changePrintColor("white");
}

static void handlePacket(Server *server, int fd, struct sockaddr_in *sin) {
    printConnectInfo(sin);
    char reqPacket[20480];
    int MAX_LEN_PACK=20470;
    int total = 0;
    int num = recv(fd, reqPacket, sizeof(reqPacket), 0);
    total += num;
    printf("QAQQQQ %d",num);
    printf("\n==========%s\n=========\n",reqPacket);

    if(strstr(reqPacket,"boundary")){
	char* boundary = "-----------------------------";
	int b_len = strlen(boundary);
	int cnt = 0;
	int req_len;
	char * len_pos_f = strstr(reqPacket,"Content-Length");
	char * len_pos_e = strstr(len_pos_f,"\r\n");
	len_pos_f+=16;
	char len[20];
	strncpy(len,len_pos_f,len_pos_e-len_pos_f);
	int len_n = atoi(len);
	char* OwO = strstr(reqPacket,boundary);
	char* TAT = strstr(OwO,"\r\n");
	int foot_len = TAT-OwO+2+4;
	char type[30]="NULL";
	if(strstr(OwO,"text/plain")) strcpy(type, ".txt");
	else if(strstr(OwO,"image/jpeg")) strcpy(type,".jpg");
	else if(strstr(OwO,"image/png"))  strcpy(type,".png");
	char* QAQ = strstr(OwO,"\r\n\r\n");
	num -= QAQ-reqPacket;
	QAQ += 4;
	b_len = QAQ-OwO;
	len_n = len_n - b_len - foot_len;
	int MAX_LEN = len_n;
	printf("\nlen = %d\n",len_n);
	FILE* f = fopen("test.png","wb+");
	if(num>len_n){//len_n+footer
	    fwrite(QAQ, sizeof(char), len_n, f);
	}
	fwrite(QAQ, sizeof(char), num, f);//need another read
	len_n -= num;
	int debug_cnt = 0;
	total = num;
	while(total <= MAX_LEN){
	    //debug_cnt++;
	    if(len_n<MAX_LEN_PACK) MAX_LEN_PACK = len_n;
   	    int num = recv(fd, reqPacket, MAX_LEN_PACK , 0);
	    fwrite(reqPacket, sizeof(char), num, f);
	    //if(debug_cnt==1)  break;
	    printf("len_n -> %d\ntotal -> %d\nnum -> %d\nMAX_INPUT -> %d\nMAX_LEN -> %d\n------\n",len_n,total,num,MAX_LEN_PACK,MAX_LEN);
	    len_n -= num;
	    total += num;
	}
	
	fclose(f);
    }
    Request *request = requestNew(reqPacket, sin);
    Response *response = execHandler(server, request);

    char *resPacket = responsePacket(response);
    size_t packetLength = (response->statusLength) + (response->headerLength) + (response->contentLength);
    send(fd, resPacket, packetLength, 0);
    close(fd);
    printRequest(request);
    printResponse(response);
    freeRequest(request);
    freeResponse(response);
}

void serverUse(Server *server, Handler handler)
{
    Handler *hp = malloc(sizeof(Handler));
    *(hp) = handler;
    listAppend(server->handlers, hp, sizeof(Handler));
}

Server *serverNew(char *path, char *port)
{
    int fd;
    unsigned val = 1;
    struct sockaddr_in sin;
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(atoi(port));

    if (bind(fd, (struct sockaddr*) &sin, sizeof(sin)) < 0) { perror("bind"); exit(-1); }
    if (listen(fd, SOMAXCONN) < 0) { perror("listen"); exit(-1); }

    Server *server = malloc(sizeof(Server));
    server->path = malloc((sizeof(char))* strlen(path)+1);
    server->port = malloc((sizeof(char))* strlen(port)+1);
    server->handlers = listNew();
    server->fd = fd;

    memcpy(server->path, path, (sizeof(char))* strlen(path)+1);
    memcpy(server->port, port, (sizeof(char))* strlen(port)+1);

    return server;
}

void serverServe(Server *server)
{
    pid_t pid;
    int pfd;
    struct sockaddr_in psin;
    printf("server is listening on port %s.\n", server->port);
    signal(SIGCHLD, SIG_IGN); // prevent child zombie
    chdir(server->path);
    while(1) {
        int val = sizeof(psin);
        bzero(&psin, sizeof(psin));
        if((pfd = accept(server->fd, (struct sockaddr*) &psin, &val))<0) perror("accept");
	//int file_MAX = 10000;
	//char buff[file_MAX+5];
        //if(read(pfd, buff, file_MAX) == -1){
	//    printf("=======read error=======\n");
//	}
	//printf("=======================\n\n%s\n\n==================\n",buff);
	if((pid = fork()) < 0) perror("fork");
        else if(pid == 0) {
            close(server->fd);
            handlePacket(server, pfd, &psin);
            exit(0);
        }
        close(pfd);
    }
}
