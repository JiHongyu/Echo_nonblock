#ifndef CECHOIO_H
#define CECHOIO_H
/*
Echo Server �� Client��IO�����࣬ʵ����copy��UNP�Ĵ��룬�򵥵ķ�װ
*/
#include <sys/types.h>

class CEchoIO
{
	public:
		//���ļ�������fd������
		ssize_t Read(int fd, void *ptr, size_t nbytes);
		//���ļ�������fdд����
		void Writen(int fd, void *ptr, size_t nbytes);
	private:
		ssize_t writen(int fd, const void *vptr, size_t n);
};
#endif
