#include <stdio.h>
#include <string>

// 包含测试框架头文件
#include "testframe.h"

#include "Client.h"
#include "../tcinclude/tc_http.h"
#include "loadConfig.h"
#include "testsynch.h"
#include "../tcinclude/tc_singleton.h"
#include "../tcinclude/tc_common.h"
#include <sys/time.h>
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
static unsigned long thread_id(void)
{
	unsigned long ret;
	ret = (unsigned long)pthread_self();
	return(ret);
}
#if 0
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

class UserAccountPool {
public:
	UserAccountPool(){};
	void Init(unsigned long long num)
	{
		_num = num;
	}
	std::string getIncre() {
		SmartLock lock(_lock);
		_num++;
		std::stringstream ss;
		ss
			<< _num;
		return ss.str();
	}
private:
	MutexLock _lock;
	unsigned long long _num;
};



class ReqPool {
public:
	void Init(const std::vector<ReqInfo> &req_list)
	{
		_req_list = req_list;
	}
	ReqInfo getReq(std::string &user_id, std::string &family_id)
	{
		struct timeval tv;
		gettimeofday(&tv, 0);
		srand(tv.tv_usec + atoi(user_id.c_str()));
		unsigned int index = rand() % _req_list.size();
		if (index == 0)
		{
			index = 1;
		}
		ReqInfo req_info;
		req_info = _req_list[index - 1];
		req_info._req = TC_Common::replace(req_info._req, "$user_id", user_id);
		req_info._req = TC_Common::replace(req_info._req, "$family_id", family_id);
		return req_info;
	}
	
	static int getVal(const std::string &rsp, const std::string &name,const size_t pos,std::string &val)
	{
		size_t p1 = rsp.find(name);
		if (p1 == std::string::npos)
		{
			printf("get user_id from rsp fail!\n");
			return -2;
		}
		size_t p2 = rsp.find_first_of(",", p1 + 1);
		if (p2 == std::string::npos)
		{
			printf("get user_id from rsp fail!\n");
			return -3;
		}
		std::string s_rsp = rsp;
		size_t name_len = name.size();
		val = s_rsp.substr(p1 + name_len+pos, p2 - p1 - name_len -pos);
		return 0;
	}
private:
	std::vector<ReqInfo> _req_list;
};

class log_file {
public:
	int Init(const std::string &filename)
	{
		_handle = fopen(filename.c_str(), "a+");
		if (_handle == NULL)
		{
			return -1;
		}
		return 0;
	}

	void log(const std::string &log)
	{
		SmartLock smart_lock(_lock);
		unsigned long t_id = thread_id();
		std::string now = taf::TC_Common::now2str("%Y-%m-%d %H:%M:%S");
		std::stringstream ss;
		ss
			<< "[" << now << "]"
			<< "[" << t_id << "]"
			<< log;
		fputs(ss.str().c_str(), _handle);
	}
	log_file() :_handle(NULL) {};
	~log_file() {
		SmartLock smart_log(_lock);
		fclose(_handle);
	}
private:
	FILE *_handle;
	MutexLock _lock;
};

class TestCase_IOT : public TestCase
{
public:
	TestCase_IOT(std::vector<TcpInfo> &ip_port,std::string &log_file)
	{
		_ip_port.assign(ip_port.begin(), ip_port.end());
		_log_file = log_file;
	};
	virtual int Init(void*arg)
	{
		Client* client = NULL;
		client = (Client*)arg;
		
		for (size_t i = 0; i < _ip_port.size(); i++)
		{
			client->AddServer(_ip_port[i]._ip,_ip_port[i]._port);
		}
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
		ReqInfo req_info = taf::TC_Singleton<ReqPool>::getInstance()->getReq(_user_id,_family_id);
		
		int ret = client->tcp_iot(req_info._req, rsp);
		if (ret != 0)
		{
			printf("SendAndRcv fail,ret:%d\n", ret);
			return -1;
		}
		if (!_log_file.empty())
		{
			log_file * log = taf::TC_Singleton<log_file>::getInstance();
			stringstream ss;
			ss
				<< "req:"
				<< req_info._req << "\n"
				<< "rsp:" << rsp
				<< std::endl;
			log->log(ss.str());
		}
		//判断成功失败
		//std::string succ_flag = "code\":0";
		if (rsp.find(req_info._assert) != std::string::npos || rsp.find("code\":0") != std::string::npos)
		{
			return 0;
		}
		else {
			if (!_log_file.empty())
			{
				log_file * log = taf::TC_Singleton<log_file>::getInstance();
				stringstream ss;
				ss
					<< "assert fail!\nreq:"
					<< req_info._req << "\n"
					<< "rsp:" << rsp << "\n"
					<< "assert:" << req_info._assert
					<< std::endl;
				log->log(ss.str());
			}
			return -2;
		}
	}
	virtual int PreExecute(void *arg)
	{
		std::string login_req = "{\"uuid\":\"111\", \"encry\":\"false\", \"content\":{\"method\":\"um_login_pwd\",\"timestamp\":12345667,\"req_id\":123,\"params\":{\"phone\":\"$phone\",\"pwd\":\"96e79218965eb72c92a549dd5a330112\",\"os_type\":\"Android\"}}}\n";
		std::string family_info_req = "{\"uuid\":\"111\", \"encry\":\"false\", \"content\":{\"method\":\"fm_get_family_list\",\"timestamp\":12345667,\"req_id\":123}}\n";

		Client* client = NULL;
		client = (Client*)arg;
		std::string rsp;
		//替换变量
		UserAccountPool * pool = taf::TC_Singleton<UserAccountPool>::getInstance();
		std::string req = TC_Common::replace(login_req, "$phone", pool->getIncre());
		int ret = client->tcp_iot(req, rsp);
		if (ret != 0)
		{
			printf("SendAndRcv1 fail,ret:%d\n", ret);
			return -1;
		}
		if (!_log_file.empty())
		{
			log_file * log = taf::TC_Singleton<log_file>::getInstance();
			stringstream ss;
			ss
				<< "req:" << req << "\n"
				<< "rsp:" << rsp << "\n";
			log->log(ss.str());
		}
		//判断成功失败
		
		std::string succ_flag = "code\":0";
		if (rsp.find(succ_flag) != std::string::npos)
		{
			std::string val;
			int ret = ReqPool::getVal(rsp, "user_id", 2, val);
			if (ret != 0)
			{
				printf("getVal fail!ret:%d\n", ret);
				return -2;
			}
			_user_id = val;
			ret = client->tcp_iot(family_info_req, rsp);
			if (ret != 0)
			{
				printf("SendAndRcv1 fail,ret:%d\n", ret);
				return -3;
			}
			if (rsp.find(succ_flag) == std::string::npos)
			{
				printf("api fail!req:%s\n rsp:%s\n", family_info_req.c_str(),rsp.c_str());
				return -5;
			}
			ret = ReqPool::getVal(rsp, "family_id", 2, val);
			if (ret != 0)
			{
				printf("getVal fail!ret:%d\n", ret);
				return -6;
			}
			_family_id = val;
			printf("prepare req success!user_id:%s,family_id:%s\n",_user_id.c_str(),_family_id.c_str());
			return 0;
		}
		else {
			printf("rsp:%s\n", rsp.c_str());
			return -2;
		}
		return 0;
	}

private:
	// 在这里定义需要使用的成员变量
	std::vector<TcpInfo> _ip_port;
	std::string _log_file;
	std::string _user_id;
	std::string _family_id;
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

	if (!cmd_info.log_file.empty())
	{
		log_file * log = taf::TC_Singleton<log_file>::getInstance();
		log->Init(cmd_info.log_file);
		log->log(cmd_info.to_str());
	}
	//初始化预设参数

	taf::TC_Singleton<UserAccountPool>::getInstance()->Init(atoll(cmd_info._first_user_phone.c_str()));
	taf::TC_Singleton<ReqPool>::getInstance()->Init(cmd_info._req_list);
	// 测试框架实例
	TestFrame tf;
	TestCase_IOT case1(cmd_info._tcp_info,cmd_info.log_file);
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
	if (cmd_info.times_limit != 0)
	{
		tf.SetRepeatCount(cmd_info.times_limit);
	}
	
	// 设置测试并发线程数
	tf.SetThreadNum(cmd_info.thread_num);
	// 设置测试持续时间，默认不限时间
	tf.SetDurationTime(cmd_info.load_test_time);
	// 设置频率限制，默认不限频率
	if (cmd_info.frequence != 0)
	{
		tf.SetFrequence(cmd_info.frequence);
	}

	// 设置时延阀值，用于统计时延健康度指标
	//tf.SetDelayThreshold(300);

	// 阻塞运行测试案例，并且输出测试结果
	tf.Run();

	return 0;
}


