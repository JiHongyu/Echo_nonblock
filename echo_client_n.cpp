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

#define MAXEVENT 4
#define ECHOPORT 9999
#define BUFFERSIZE 4096
// Echo Client 主函数

char to[BUFFERSIZE], fr[BUFFERSIZE];
int	clientfd, epollfd;

void setEpollEvent();
void setNonblock();
void echoClientHandler();
void Epoll_CTL(int epfd, int op, int fd, struct epoll_event *event);
int main(int argc, char **argv)
{

	if (argc < 2){
		perror("usage: echo_client <IP-multicast-address> [<port#>]");
		exit(0);
	}

	int nfds;
	struct sockaddr_in	clientaddr;

	//获取socket套接字描述符
	if((clientfd = socket(AF_INET, SOCK_STREAM, 0))<0){
		perror("socket error");
		exit(0);
	}

	int reuse = 1;
	if(setsockopt(clientfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))==-1)
		perror("setsockopt error");

		//填充 sockaddr_in 地址信息
		bzero(&clientaddr, sizeof(clientaddr));
		clientaddr.sin_family = AF_INET;
		if (inet_pton(AF_INET,argv[1], &clientaddr.sin_addr) <= 0){
			perror("IP address error");
			exit(-1);
		}
		if(argc == 3 )
			clientaddr.sin_port = htons(atoi(argv[2]));
		else
			clientaddr.sin_port = htons(ECHOPORT);

		if(connect(clientfd, (struct sockaddr*) &clientaddr
			, sizeof(clientaddr))!=0)
		{
			perror("connect error");
			exit(errno);
		}

		setNonblock();
		echoClientHandler();

		return 0;
}

void setNonblock()
{
	int flags;
	//设置stdin非阻塞
	flags = fcntl(fileno(stdin), F_GETFL, 0);
	fcntl(fileno(stdin), F_SETFL, flags | O_NONBLOCK);

	//设置stdout非阻塞
	flags = fcntl(fileno(stdout), F_GETFL, 0);
	fcntl(fileno(stdout), F_SETFL, flags | O_NONBLOCK);

	//设置socket非阻塞
	flags = fcntl(clientfd, F_GETFL, 0);
	fcntl(clientfd, F_SETFL, flags | O_NONBLOCK);
}
void setEpollEvent()
{
	struct epoll_event ev;

	//配置client socket的读事件，边沿出发
	ev.events = EPOLLIN|EPOLLET;
	ev.data.fd = clientfd;
	//注册事件
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev) < 0) {
		perror("epoll_ctl: clientfd");
		exit(errno);
	}

	//配置client stdin 的读事件，边沿出发
	ev.events = EPOLLIN|EPOLLET;
	ev.data.fd = fileno(stdin);
	//注册事件
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fileno(stdin), &ev) < 0) {
		perror("epoll_ctl: stdin");
		exit(errno);
	}

	//配置client stdout 的写事件，边沿出发
	ev.events = EPOLLOUT|EPOLLET;
	ev.data.fd = fileno(stdout);
	//注册事件
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fileno(stdout), &ev) < 0) {
		perror("epoll_ctl: stdout");
		exit(errno);
	}

}

void echoClientHandler()
{
	int			maxfdp1, val, stdineof;
	ssize_t		n, nwritten;
	int nfds;

	struct epoll_event ev,events[MAXEVENT];
	char  *toiptr, *tooptr, *friptr, *froptr;

	toiptr = tooptr = to;	/* initialize buffer pointers */
	friptr = froptr = fr;
	stdineof = 0;

	if ((epollfd = epoll_create1(0))< 0) {
		perror("epoll_create error");
		exit(errno);
	}
	setEpollEvent();
	for ( ; ; ) {
		//epoll等待事件发生
		nfds = epoll_wait(epollfd, events, MAXEVENT, -1);
		if (nfds == -1) {
			perror("epoll_pwait");
			exit(EXIT_FAILURE);
		}
		for (int i=0;i<nfds;i++)
		{
			if (events[i].data.fd==STDIN_FILENO&&(events[i].events&EPOLLIN)) {
				if ( (n = read(STDIN_FILENO, toiptr, &to[BUFFERSIZE] - toiptr)) < 0) {
					if (errno != EWOULDBLOCK)
						perror("read error on stdin");

				} else if (n == 0) {
					stdineof = 1;			/* all done with stdin */
					if (tooptr == toiptr)
						shutdown(clientfd, SHUT_WR);/* send FIN */
				} else {
					toiptr += n;			/* # just read */
					ev.events = EPOLLOUT|EPOLLET;
					ev.data.fd = clientfd;
					if (epoll_ctl(epollfd, EPOLL_CTL_MOD,clientfd, &ev) < 0) {
						perror("epoll_ctl: stdout");
						//exit(errno);
					}
				}
			}

			if (events[i].data.fd==clientfd&&(events[i].events&EPOLLIN)) {
				if ( (n = read(clientfd, friptr, &fr[BUFFERSIZE] - friptr)) < 0) {
					if (errno != EWOULDBLOCK)
						perror("read error on socket");

				} else if (n == 0) {
				if (stdineof)
					return;		/* normal termination */
				else
					perror("echoClientHandler: server terminated prematurely"),exit(0);

				} else {
					friptr += n;		/* # just read */
				}
			}

			if (events[i].data.fd==STDOUT_FILENO&&(events[i].events&EPOLLOUT) && ( (n = friptr - froptr) > 0)) {
				if ( (nwritten = write(STDOUT_FILENO, froptr, n)) < 0) {
					if (errno != EWOULDBLOCK)
						perror("write error to stdout");
					} else {
						froptr += nwritten;		/* # just written */
						if (froptr == friptr)
							froptr = friptr = fr;	/* back to beginning of buffer */
					}
			}
			
  		if (events[i].data.fd==clientfd&&(events[i].events&EPOLLOUT) && ( (n = toiptr - tooptr) > 0)) {
  			if ( (nwritten = write(clientfd, tooptr, n)) < 0) {
  				if (errno != EWOULDBLOCK)
  					perror("write error to socket");
  			} else {
  				tooptr += nwritten;	/* # just written */
  				if (tooptr == toiptr) {
  					toiptr = tooptr = to;	/* back to beginning of buffer */
  					if (stdineof)
  						shutdown(clientfd, SHUT_WR);	/* send FIN */
  				}
  			}
  			ev.events = EPOLLIN|EPOLLET;
  			ev.data.fd = clientfd;
  			if (epoll_ctl(epollfd, EPOLL_CTL_MOD,clientfd, &ev) < 0) {
  				perror("epoll_ctl: stdout");
  			}
  		}
		}
	}
}


