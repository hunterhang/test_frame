#include <stdio.h>
#include <string>

// 包含测试框架头文件
#include "testframe.h"

#include "Client.h"
#include "../tcinclude/tc_http.h"
#include "loadConfig.h"

//static pthread_mutex_t *lockarray;

#define MAX_SDK_BUFFER_SIZE 1024
#define QUERY_SERVICE(cmd) client->RPC<cmd>(req, rsp)
#define QUERY_HTTP() client->http(httpBuffer,rspBuffer)

using namespace std;
using namespace taf;

string param1;
string param2;
string param3;
// 定义测试案例
#if 0
static unsigned long thread_id(void)
{
	unsigned long ret;
	ret = (unsigned long)pthread_self();
	return(ret);
}

static void lock_callback(int mode, int type, const char *file, int line)
{
	(void)file;
	(void)line;
	if (mode & CRYPTO_LOCK) {
		pthread_mutex_lock(&(lockarray[type]));
	}
	else {
		pthread_mutex_unlock(&(lockarray[type]));
	}
}

static void init_locks(void)
{
	int i;
	lockarray = (pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() *
		sizeof(pthread_mutex_t));
	for (i = 0; i < CRYPTO_num_locks(); i++) {
		pthread_mutex_init(&(lockarray[i]), NULL);
	}
	CRYPTO_set_id_callback((unsigned long(*)())thread_id);
	CRYPTO_set_locking_callback(lock_callback);
}
static void kill_locks(void)
{
	int i;
	CRYPTO_set_locking_callback(NULL);
	for (i = 0; i < CRYPTO_num_locks(); i++)
		pthread_mutex_destroy(&(lockarray[i]));
	OPENSSL_free(lockarray);
}

class TestCase_Https : public TestCase
{
public:
	TestCase_Https()
	{

		// 在这里编写初始化代码
		// SDK初始化，数据库连接初始化，打开文件 等等
	};
	virtual ~TestCase_Https()
	{
		// 在这里编写销毁代码
		// SDK销毁，断开数据库连接，关闭文件 等等
	};

	virtual int Execute(void*arg)
	{
		const char *ip = "10.217.252.221";
		int port = 443;
		std::string err_msg;

		HttpsClient  https_client;
		if (0 != https_client.Init(ip, port, err_msg))
		{
			printf("Init err[%s]\n", err_msg.c_str());
			return -1;
		}

		char buffer[4096] = { '\0' };
		snprintf(buffer, 4096 - 1,
			"GET /share/qq?uid=UUUU&token=JJJJ&appkey_id=7879797798&encode=1 HTTP/1.1\r\n"
			"Host: %s:%d \r\n"
			"Connection: close\r\n"             //"Connection: Keep-Alive\r\n"
			"Cache-Control: no-cache\r\n"
			"Content-Length: 0\r\n"
			"Content-Type: application/x-www-form-urlencoded\r\n\r\n",
			"10.217.252.221", port);

		std::string req = buffer;
		std::string rsp_body;


		if (0 != https_client.SendAndRecvHttp(req, rsp_body, err_msg))
		{
			//printf("SendAndRecvHttp err[%s]\n", err_msg.c_str() );
			return -1;
		}
		https_client.Close();
		//printf("Http Send:\n%s \n", req.c_str() );
		//printf("Http Recv:\n%s \n", rsp_body.c_str() );
		return 0;
	}

};
#endif


class TestCase_IOT : public TestCase
{
public:
	TestCase_IOT(std::string &ip,unsigned short port,std::string &pre_req,std::string &req,std::string &log_file)
	{
		_ip = ip;
		_port = port;
		_pre_req = pre_req;
		_req = req;
		_log_file = log_file;
	};
	virtual int Init(void*arg)
	{
		Client* client = NULL;
		client = (Client*)arg;
		client->AddServer(_ip, _port);
		return 0;
	};
	virtual ~TestCase_IOT()
	{
	};
	virtual int Execute(void*arg)
	{
		Client* client = NULL;
		client = (Client*)arg;
		std::string rsp;
		int ret = client->tcp_iot(_req, rsp);
		if (ret != 0)
		{
			printf("SendAndRcv fail,ret:%d\n", ret);
			return -1;
		}
		if (!_log_file.empty())
		{

			taf::TC_File file;
			std::string old_log = file.load2str(_log_file);
			old_log.append("\n");
			old_log.append(rsp);
			file.save2file(_log_file, old_log);
		}
		//判断成功失败
		std::string succ_flag = "code\":0";
		if (rsp.find(succ_flag) != std::string::npos)
		{
			return 0;
		}
		else {
			return -2;
		}
	}
	virtual int PreExecute(void *arg)
	{
		if (_pre_req.empty())
		{
			return 0;
		}
		Client* client = NULL;
		client = (Client*)arg;
		std::string rsp;
		int ret = client->tcp_iot(_pre_req, rsp);
		if (ret != 0)
		{
			printf("SendAndRcv1 fail,ret:%d\n", ret);
			return -1;
		}
		if (!_log_file.empty())
		{

			taf::TC_File file;
			std::string old_log = file.load2str(_log_file);
			old_log.append(_pre_req);
			old_log.append("\n");
			old_log.append(rsp);
			old_log.append("\n");
			file.save2file(_log_file, old_log);
		}
		//判断成功失败
		std::string succ_flag = "code\":0";
		if (rsp.find(succ_flag) != std::string::npos)
		{
			return 0;
		}
		else {
			return -2;
		}
		return 0;
	}

private:
	// 在这里定义需要使用的成员变量
	std::string _ip;
	unsigned short _port;
	std::string _req;
	std::string _pre_req;
	std::string _log_file;
};

int main(int argc, char* argv[])
{

	if (argc < 3)
	{
		printf("Usage: %s file cmd\n", argv[0]);
		return 0;
	}
	std::string file = argv[1];
	unsigned int cmd = (unsigned int)atoi(argv[2]);
	
	LoadTestConfig config;
	int ret = config.Init(file);
	if (ret != 0)
	{
		printf("file not exist!%s\n", file.c_str());
		return -1;
	}
	CmdInfo cmd_info;
	ret = config.get_config(cmd,cmd_info);
	if (ret != 0)
	{
		printf("cmd not in config file,cmd:%d\n",cmd);
		return -2;
	}

	printf("============================================\n");
	std::string s = cmd_info.to_str();
	printf("%s\n", s.c_str());
	printf("============================================\n");

	// 测试框架实例
	TestFrame tf;
	TestCase_IOT case1(cmd_info.ip,cmd_info.port,cmd_info.pre_req,cmd_info.req,cmd_info.log_file);
	tf.SetTestCase(&case1);
	/**
	switch (cmd)
	{
	case 10000:
		tf.SetTestCase(&case1);
		break;
	default:
		printf("ERROR CMD!\n");
		break;
	}
	**/
	// 测试案例实例
	//TestCase001 tc001;

	// 设置测试案例
	//tf.SetTestCase(&tc001);

	// 设置测试重复次数，默认不限次数
	//tf.SetRepeatCount(100000);

	// 设置测试并发线程数
	//tf.SetThreadNum(700);
	tf.SetThreadNum(cmd_info.thread_num);
	// 设置测试持续时间，默认不限时间
	//tf.SetDurationTime(30);
	tf.SetDurationTime(cmd_info.load_test_time);
	// 设置频率限制，默认不限频率
	//tf.SetFrequence(1000);

	// 设置时延阀值，用于统计时延健康度指标
	//tf.SetDelayThreshold(300);

	// 阻塞运行测试案例，并且输出测试结果
	tf.Run();

	return 0;
}


