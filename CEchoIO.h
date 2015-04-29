#ifndef CECHOIO_H
#define CECHOIO_H
/*
Echo Server 和 Client的IO操作类，实际上copy了UNP的代码，简单的封装
*/
#include <sys/types.h>

class CEchoIO
{
	public:
		//从文件描述符fd读数据
		ssize_t Read(int fd, void *ptr, size_t nbytes);
		//从文件描述符fd写数据
		void Writen(int fd, void *ptr, size_t nbytes);
	private:
		ssize_t writen(int fd, const void *vptr, size_t n);
};
#endif
