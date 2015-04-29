#include "CEchoIO.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAXEVENT 20
#define ECHOPORT 9999
#define BUFFERSIZE 4096
//Echo Server ҵ����������
ssize_t echoHandler(int sockfd);

//�����д�ӿڶ���
CEchoIO *io = new CEchoIO;
char buffer[4096];
int main(int argc, char **argv)
{
	int	serverfd, connfd, epollfd;
	int nfds;
	struct sockaddr_in	servaddr;
	struct epoll_event ev;
	struct epoll_event *events;

	//��ȡsocket�׽���������
	if((serverfd = socket(AF_INET, SOCK_STREAM, 0))<0){
		perror("socket error");
		exit(0);
	}
	
	int reuse = 1;  
 	if(setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))==-1)
		perror("setsockopt error"); 
		
	//����socket������
	int flags = fcntl(serverfd, F_GETFL, 0); 
	fcntl(serverfd, F_SETFL, flags | O_NONBLOCK);
	
	//��� sockaddr_in ��ַ��Ϣ
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(ECHOPORT);

	//��ȡepoll������
	if ((epollfd = epoll_create1(0))< 0) {
   		perror("epoll_create");
			exit(errno);
	}
	
	//����Server socket�Ķ��¼������س���
	ev.events = EPOLLIN|EPOLLET;
	ev.data.fd = serverfd;
	
	//ע���¼�
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serverfd, &ev) < 0) {
		perror("epoll_ctl: serverfd");
		exit(EXIT_FAILURE);
	}
	events = new struct epoll_event[MAXEVENT];
	
	//���׽���
	if (bind(serverfd, (struct sockaddr *) &servaddr
		, sizeof(servaddr)) <0){
			perror("bind error");
			exit(0);
	}
	//��������
	if (listen(serverfd, 64)<0){	
			perror("Listen error");
			exit(0);
	}
	
	
	for (;;) {
		//epoll�ȴ��¼�����
		nfds = epoll_wait(epollfd, events, MAXEVENT, -1);
		if (nfds == -1) {
			perror("epoll_pwait");
			exit(EXIT_FAILURE);
		}

		for (int n = 0; n < nfds; ++n) {
			if (events[n].data.fd == serverfd) {
				//���Կͻ��˵������¼�
				//��ȡ�ͻ����׽���������
				if((connfd= accept(serverfd,NULL,NULL))<0)
				{
					perror("accept error");
					exit(0);
				}
				//���ÿͻ����׽���Ϊ������ģʽ
				flags = fcntl(connfd, F_GETFL, 0); 
				fcntl(connfd, F_SETFL, flags | O_NONBLOCK);
				//���ÿͻ���ҵ�����¼�
				ev.events = EPOLLIN|EPOLLET ;
				ev.data.fd = connfd;
				//ע���¼�
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd,
					&ev) == -1) {
					perror("epoll_ctl: connfd");
					exit(EXIT_FAILURE);
				}
				printf("A new user connected!!\n");
			}else if(events[n].events&EPOLLIN){
				//���Կͻ��˵�ҵ���߼��¼�
		       if(echoHandler(events[n].data.fd)>0){
		       		printf("deal with job\n");//debug information
		       	}
		       	else{
		       		if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd,
								events+n) == -1) {
									perror("epoll_ctl: conn_sock");
									exit(EXIT_FAILURE);
							}
							printf("connection termined\n");//debug information
		       	}
		  }
		}		
	}
	
	delete []events;
	return 0;
}

//Echo server ҵ������
ssize_t echoHandler(int sockfd)
{
	
	ssize_t		n;
	ssize_t 	count;
	while ( (n = io->Read(sockfd, buffer, BUFFERSIZE)) > 0)
	{

		io->Writen(sockfd, buffer, n);
		count +=n;
	}
	if (n == -1 && errno != EAGAIN)
		perror("echoHandler: read error");
	return count;	
}