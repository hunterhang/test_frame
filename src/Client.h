#ifndef __SHARE_CLIENT_H_
#define __SHARE_CLIENT_H_

#include <string>
#include <vector>
using namespace std;

#define MAX_JCE_BUFFER_SIZE 65536

#define SDK_SUCCESS                 0                       //成功
#define SDK_ERR_API                 -1000                   //-1000到-1999为客户端返回的错误
#define SDK_ERR_RSP_LEN             SDK_ERR_API - 1         //未接收到完整消息包
#define SDK_ERR_RSP_CMD             SDK_ERR_API - 2         //回包包头cmd错误，传入buf的起始位置可能不是消息头的起始位置
#define SDK_ERR_RSP_NOBODY          SDK_ERR_API - 3         //回包缺少包体
#define SDK_ERR_JCE_ENCODE          SDK_ERR_API - 4         //JCE编码异常
#define SDK_ERR_JCE_DECODE          SDK_ERR_API - 5         //JCE解码异常
#define SDK_ERR_BUFFER_SIZE         SDK_ERR_API - 6         //输入输出Buffer大小不足
#define SDK_ERR_ADDRESS             SDK_ERR_API - 7         //未配置后台目标IP和端口
#define SDK_ERR_SOCKET              SDK_ERR_API - 8         //TCP通信错误，查看errno
#define SDK_ERR_SEQ                 SDK_ERR_API - 9         //seq不匹配
#define SDK_ERR_NULLPT              SDK_ERR_API - 10        //空指针错误
#define SDK_ERR_SERVER              -2000                   //-2000~-9999以下为服务器返回的错误
#define SDK_ERR_SERVER_UNPACK       SDK_ERR_SERVER - 1      //消息解析失败
#define SDK_ERR_SERVER_VER          SDK_ERR_SERVER - 2      //协议版本不匹配
#define SDK_ERR_SERVER_CMD          SDK_ERR_SERVER - 3      //命令不存在
#define SDK_ERR_SERVER_RSP_PACKAGE  SDK_ERR_SERVER - 4      //http 响应包错误
#define SDK_ERR_SERVER_EMPTY_BODY   SDK_ERR_SERVER - 5      //http empty body
#define SDK_ERR_SERVER_JSON         SDK_ERR_SERVER - 6      //json响应包解析失败
#define SDK_ERR_SERVER_RET          SDK_ERR_SERVER - 7      //响应包返回值为空
#define SDK_ERR_SERVER_ST_DECRYPT   SDK_ERR_SERVER - 8      //ST解码失败
#define SDK_ERR_SERVER_A8_DECRYPT   SDK_ERR_SERVER - 9      //A8解码失败
#define SDK_ERROR_SERVER_CONFIG	    SDK_ERR_SERVER - 10     //配置文件没有该配置或配置错误


class Client {
public:
	Client();
	//添加和获取配置server地址接口
	int AddServer(const string &ip, unsigned short port);
	template<int Cmd, class Req, class Rsp>
	int http(const string &httpBuffer, string &rspBuffer) {
		int res = SendHttp(httpBuffer, rspBuffer);
		return res;
	}
	int tcp_iot(const string &send_buf, string &rsp_buf);


protected:
	int GetServer(string& ip, int& port);
	int SendNRecv(const char *reqbuf, unsigned int reqlen, char *rspbuf, unsigned int& rsplen);
	int SendHttp(const string &httpBuffer, string &rspBuffer);
	int Rand();
	/*
	   SDK处理TCP链接
	   @param  req    请求结构体
	   @param  rsp    返回的消息结构体
	   @return  请求状态,=0成功，其他值失败。成功时rsp才是有效的。rsp.ret和rsp.msg可以确认业务消息是否处理成功。
	 */
	 //int GetRoute(QOSREQUEST& req, string& ip, int& port, string& err_msg);

private:
	vector< pair<string, int> >  addrs;
	int							sockfd;
};

#endif
