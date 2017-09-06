#ifndef __LOAD_CONFIG_H_
#define __LOAD_CONFIG_H_
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <map>
#include "json-c/json.h"
#include "../tcinclude/tc_file.h"

struct TcpInfo {

	std::string _ip;
	unsigned short _port;
};
struct ReqInfo {
	ReqInfo() :_weight(0) {};
	std::string _req;
	std::string _assert;
	unsigned int _weight;
};
struct CmdInfo {
public:
	CmdInfo():times_limit(0),frequence(0),is_debug(true) {
	};
	std::string to_str()
	{
		std::stringstream ss;
		ss
			<< "cmd:" << cmd << "\n"
			<< "tcp_info.size():" <<_tcp_info.size()<<"\n"
			<< "req_list:" << _req_list.size() << "\n"
			<< "first_user_phone:" << _first_user_phone << "\n"
			<< "conn_num:" << conn_num << "\n"
			<< "load_test_time:" << load_test_time << "\n"
			<< "times_limit:" << times_limit << "\n"
			<< "frequence:" << frequence << "\n"
			<< "log_file:" << log_file << "\n"
			<< "is_debug:" << is_debug<< "\n"
			<< endl;
		return ss.str();
	}
	unsigned int cmd;
	std::vector<TcpInfo> _tcp_info;
	std::vector<ReqInfo> _req_list;
	std::string _first_user_phone;
	unsigned int conn_num;
	unsigned int load_test_time;
	unsigned long long times_limit;
	unsigned long long frequence;
	std::string log_file;
	bool is_debug;
};
class LoadTestConfig {
public:
	LoadTestConfig();
	int Init(const std::string &filename);
	int get_config(unsigned int cmd,CmdInfo &cmd_info);
private:
	int parse(std::string &content);
	static int get(const json_object *root, const std::string &name, std::string &value, std::string &err_info);
	static int get(const json_object *root, const std::string &name, unsigned long long &value, std::string &err_info);
	static int get(const json_object *root, const std::string &name, unsigned int &value, std::string &err_info);
	static int get(const json_object *root, const std::string &name, int &value, std::string &err_info);

	std::map<unsigned int, CmdInfo> _config_list;


};



#endif