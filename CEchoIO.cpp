#include "CEchoIO.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>

ssize_t 
CEchoIO::Read(int fd, void * ptr, size_t nbytes)
{
	ssize_t n;
	if ( (n = read(fd, ptr, nbytes)) == -1&&errno !=EAGAIN)
		perror("read error");
	return(n);
}

void
CEchoIO::Writen(int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
		perror("write error");
}

ssize_t	
CEchoIO::writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = (const char	*)vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}

