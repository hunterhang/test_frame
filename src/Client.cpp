#include "Client.h"

#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/time.h>
#include "../tcinclude/tc_http.h"
#include "../tcinclude/tc_epoller.h"
#include "../tcinclude/tc_singleton.h"

using namespace std;

extern taf::TC_Epoller g_epoll;

#define TIME_OUT 3

Client::Client() :sockfd(-1),_is_first_connect(true)
{
}

int Client::Rand()
{
	srand(time(NULL));
	return rand();
}

/*
   发送http请求

   @ return 成功返回SDK_SUCCESS，失败返回小于0错误码
 */
int Client::SendHttp(const string &httpBuffer, string &rspBuffer)
{
	if (sockfd == -1) {
		//printf("instance...\n");
		sockfd = socket(AF_INET, SOCK_STREAM, 0);//创建socket
		if (sockfd < 0) {
			printf("create socket fail!:%d\n", sockfd);
			return SDK_ERR_SOCKET;
		}

		string ip, err_msg;
		int port = 0;
		int ret = -1;

		if (GetServer(ip, port) < 0)
		{
			return SDK_ERR_ADDRESS;
		}
		struct sockaddr_in addr;
		/* 设置连接目的地址 */
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(ip.c_str());
		bzero(&(addr.sin_zero), 8);

		/* 发送连接请求 */
		ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
		if (ret < 0)
		{
			close(sockfd);
			sockfd = -1;
			printf("connect sock fail!%d\n",ret);
			return SDK_ERR_SOCKET;
		}
	}

	/*设置TCP接收超时*/
	struct timeval timeout = { TIME_OUT,0 };
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	unsigned int size = 0;
	/*发送*/
	size = send(sockfd, httpBuffer.c_str(), httpBuffer.length(), 0);
	if (size != httpBuffer.length())
	{
		close(sockfd);
		sockfd = -1;
		printf("send buff fail!source length:%zu,sended length:%d\n", httpBuffer.length(), size);
		return SDK_ERR_SOCKET;
	}

	//收包
	bool  bHttp = false;

	int size_recv, total_size = 0;
	char chunk[1024];
	memset(chunk, 0, 1024); //clear the variable
	while (1)
	{
		memset(chunk, 0, 1024); //clear the variable
		if ((size_recv = recv(sockfd, chunk, 1024, 0)) == -1)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				printf("recv timeout ...\n");
				break;
			}
			else if (errno == EINTR)
			{
				printf("interrupt by signal...\n");
				continue;
			}
			else if (errno == ENOENT)
			{
				printf("recv RST segement...\n");
				break;
			}
			else
			{
				printf("unknown error!error:%d\n", errno);
				//exit(1);
			}
		}
		else if (size_recv == 0) {
			//如果收到空包，则继续收
			continue;
		}
		//printf("received buff:%s\n",chunk);
		total_size += size_recv;
		taf::TC_HttpResponse httpRsponse;
		try {
			bHttp = httpRsponse.decode(chunk, total_size);
			if (bHttp) {
				//继续收包
				int bodyLen = httpRsponse.getContent().length();
				if (bodyLen == 0) {
					continue;
				}
				//如果对方关闭了链接，则这边也要关闭链接
				if (httpRsponse.getHeader("Connection").compare("close") == 0)
				{
					close(sockfd);
					sockfd = -1;
				}
				break;
			}
			else {
				continue;
			}
		}
		catch (...) {

		}

	}
	//printf("received ok\n");
	rspBuffer.assign(chunk, sizeof(chunk));
	return SDK_SUCCESS;
}

int Client::tcp_iot(const string &send_buf, string &rsp_buf,unsigned short &sock)
{
	if (sockfd == -1) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);//创建socket
		if (sockfd < 0) {
			printf("create sock fail!%d\n", sockfd);
			return SDK_ERR_SOCKET;
		}

		string ip, err_msg;
		int port = 0;
		int ret = -1;

		if (GetServer(ip, port) < 0)
		{
			printf("GetServer fail!\n");
			return SDK_ERR_ADDRESS;
		}

		struct sockaddr_in addr;
		/* 设置连接目的地址 */
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(ip.c_str());
		bzero(&(addr.sin_zero), 8);


		/* 发送连接请求 */
		ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
		if (ret < 0)
		{
			close(sockfd);
			sockfd = -1;
			printf("connect sockfd fail!%d\n", ret);
			return SDK_ERR_SOCKET;
		}
		g_epoll.add(sockfd, sockfd, EPOLLIN);
		sock = sockfd;
	}

	/*设置TCP接收超时*/
	struct timeval timeout = { TIME_OUT,0 };
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	unsigned int size = 0;
	/*发送*/
	size = send(sockfd, send_buf.c_str(), send_buf.length(), 0);
	if (size != send_buf.length())
	{
		close(sockfd);
		sockfd = -1;
		printf("send fail!%d\n", size);
		return SDK_ERR_SOCKET;
	}

	int size_recv, total_size = 0;
	char chunk[1024];
	memset(chunk, 0, 1024); //clear the variable
	while (1)
	{
		memset(chunk, 0, 1024); //clear the variable
		if ((size_recv = recv(sockfd, chunk, 1024, 0)) == -1)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				printf("recv timeout ...\n");
				break;
			}
			else if (errno == EINTR)
			{
				printf("interrupt by signal...\n");
				continue;
			}
			else if (errno == ENOENT)
			{
				printf("recv RST segement...\n");
				break;
			}
			else
			{
				printf("unknown error!error:%d\n", errno);
				//exit(1);
			}
		}
		else if (size_recv == 0) {
			//如果收到空包，则继续收
			continue;
		}
		//printf("received buff:%s\n",chunk);
		total_size += size_recv;
		std::string tmp_rsp = chunk;
		if (tmp_rsp.find("\n") != std::string::npos)
		{
			rsp_buf = chunk;
			break;
		}
	}
	//printf("rsp:%s\n", chunk);
	rsp_buf.assign(chunk, sizeof(chunk));
	return SDK_SUCCESS;

}



int Client::sync_tcp_iot(const string &send_buf)
{
	if (sockfd == -1) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);//创建socket
		if (sockfd < 0) {
			printf("create sock fail!%d\n", sockfd);
			return SDK_ERR_SOCKET;
		}

		string ip, err_msg;
		int port = 0;
		int ret = -1;

		if (GetServer(ip, port) < 0)
		{
			printf("GetServer fail!\n");
			return SDK_ERR_ADDRESS;
		}

		struct sockaddr_in addr;
		/* 设置连接目的地址 */
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(ip.c_str());
		bzero(&(addr.sin_zero), 8);

		/* 发送连接请求 */
		ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
		if (ret < 0)
		{
			close(sockfd);
			sockfd = -1;
			printf("connect sockfd fail!%d\n", ret);
			return SDK_ERR_SOCKET;
		}
		int flags = fcntl(sockfd, F_GETFL, 0);
		fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
	}

	/*设置TCP接收超时*/
	struct timeval timeout = { 0,3000 };
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	unsigned int size = 0;
	/*发送*/
	size = send(sockfd, send_buf.c_str(), send_buf.length(), 0);
	if (size != send_buf.length())
	{
		close(sockfd);
		sockfd = -1;
		printf("send fail!%d\n", size);
		return SDK_ERR_SOCKET;
	}

	return SDK_SUCCESS;

}



int Client::GetServer(string& ip, int& port)
{
	if (addrs.empty())
		return SDK_ERR_ADDRESS;
	int i = Rand() % addrs.size();
	ip = addrs[i].first;
	port = addrs[i].second;
	return SDK_SUCCESS;
}

int Client::AddServer(const string &ip, unsigned short port)
{
	addrs.push_back(make_pair(ip, port));
	return SDK_SUCCESS;
}
