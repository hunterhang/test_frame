#ifndef __LOAD_CONFIG_H_
#define __LOAD_CONFIG_H_
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <map>
#include "json-c/json.h"
#include "../tcinclude/tc_file.h"

struct CmdInfo {
public:
	CmdInfo():times_limit(0),frequence(0) {
		pre_assert = "code\":0";
		assert = "code\":0";
	};
	std::string to_str()
	{
		std::stringstream ss;
		ss
			<< "cmd:" << cmd << "\n"
			<< "ip:" << ip << "\n"
			<< "port:" << port << "\n"
			<< "pre_req:" << pre_req << "\n"
			<< "pre_assert:" <<pre_assert << "\n"
			<< "req:" << req << "\n"
			<< "assert:" << assert << "\n"
			<< "thread_num:" << thread_num << "\n"
			<< "load_test_time:" << load_test_time << "\n"
			<< "times_limit:" << times_limit << "\n"
			<< "frequence:" << frequence << "\n"
			<< "log_file:" << log_file << "\n"
			<< endl;
		return ss.str();
	}
	unsigned int cmd;
	std::string ip;
	unsigned short port;
	std::string pre_req;
	std::string pre_assert;
	std::string req;
	std::string assert;
	unsigned int thread_num;
	unsigned int load_test_time;
	unsigned long long times_limit;
	unsigned long long frequence;
	std::string log_file;
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