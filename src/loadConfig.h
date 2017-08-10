#ifndef __LOAD_CONFIG_H_
#define __LOAD_CONFIG_H_
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <map>
#include "json-c/json.h"
#include "../tcinclude/tc_file.h"

struct CmdInfo {
	std::string to_str()
	{
		std::stringstream ss;
		ss
			<< "cmd:" << cmd << "\n"
			<< "ip:" << ip << "\n"
			<< "port:" << port << "\n"
			<< "pre_req:" << pre_req << "\n"
			<< "req:" << req << "\n"
			<< "thread_num:" << thread_num << "\n"
			<< "load_test_time:" << load_test_time << "\n"
			<< "log_file:" << log_file << "\n"
			<< endl;
		return ss.str();
	}
	unsigned int cmd;
	std::string ip;
	unsigned short port;
	std::string pre_req;
	std::string req;
	unsigned int thread_num;
	unsigned int load_test_time;
	std::string log_file;
};
class LoadTestConfig {
public:
	LoadTestConfig();
	int Init(const std::string &filename);
	int get_config(unsigned int cmd,CmdInfo &cmd_info);
private:
	int parse(std::string &content);
	std::map<unsigned int, CmdInfo> _config_list;

};
#endif