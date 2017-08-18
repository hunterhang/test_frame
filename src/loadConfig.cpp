#include "loadConfig.h"
#include "../tcinclude/tc_file.h"

LoadTestConfig::LoadTestConfig()
{
};

int LoadTestConfig::Init(const std::string &filename)
{
	taf::TC_File file;
	if (file.isFileExist(filename) == false)
	{
		return -1;
	}
	std::string content = file.load2str(filename);
	return parse(content);
}

int LoadTestConfig::parse(std::string &content)
{
	enum json_tokener_error jerr;
	json_object *root = json_tokener_parse_verbose(content.c_str(), &jerr);
	if (jerr != json_tokener_success)
	{
		printf("parse json str fail!\n");
		return -1;
	}

	//判断svr 是否是数组类型
	if (!json_object_is_type(root, json_type_array))
	{
		printf("json str not array!\n");
		json_object_put(root);
		return -2;
	}

	int size = json_object_array_length(root);
	for (int i = 0; i < size; i++)
	{
		json_object *_cmd_info = json_object_array_get_idx(root, i);
		if (_cmd_info == NULL)
		{
			continue;
		}
		CmdInfo cmd_info;
		std::string err_info;
		//cmd
		if (get(_cmd_info, "cmd", cmd_info.cmd, err_info) != 0)
		{
			printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
			continue;
		}
		if (cmd_info.cmd <= 0)
		{
			printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
			continue;
		}
		if (get(_cmd_info, "ip", cmd_info.ip, err_info) != 0)
		{
			printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
			continue;
		}
		int port = 0;
		if (get(_cmd_info, "port", port, err_info) != 0)
		{
			printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
			continue;
		}
		cmd_info.port = (unsigned short)port;
		
		if (get(_cmd_info, "pre_req", cmd_info.pre_req, err_info) != 0)
		{
			printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
			continue;
		}
		
		if (get(_cmd_info, "req", cmd_info.req, err_info) != 0)
		{
			printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
			continue;
		}
		//非必填
		if (get(_cmd_info, "pre_assert", cmd_info.pre_assert, err_info) != 0)
		{
			//printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
		}
		if (get(_cmd_info, "assert", cmd_info.assert, err_info) != 0)
		{
			//printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
		}
		if (get(_cmd_info, "thread_num", cmd_info.thread_num, err_info) != 0)
		{
			printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
			continue;
		}


		if (get(_cmd_info, "load_test_time", cmd_info.load_test_time, err_info) != 0)
		{
			printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
			continue;
		}

		if (get(_cmd_info, "times_limit", cmd_info.times_limit, err_info) != 0)
		{
			printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
			continue;
		}

		if (get(_cmd_info, "frequence", cmd_info.frequence, err_info) != 0)
		{
			printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
			continue;
		}

		if (get(_cmd_info, "log_file", cmd_info.log_file, err_info) != 0)
		{
			printf("config error!index:%d,err_info:%s\n", i, err_info.c_str());
			continue;
		}
		_config_list.insert(make_pair(cmd_info.cmd, cmd_info));
	}
	json_object_put(root);
	return 0;
}

int LoadTestConfig::get_config(unsigned int cmd, CmdInfo &cmd_info)
{
	std::map<unsigned int, CmdInfo>::iterator it = _config_list.find(cmd);
	if (it != _config_list.end())
	{
		cmd_info = it->second;
		return 0;
	}
	return -1;
}


int LoadTestConfig::get(const json_object *root, const std::string &name, std::string &value, std::string &err_info) {
	//method
	json_object *_obj = NULL;
	if (!json_object_object_get_ex(root, name.c_str(), &_obj))
	{
		err_info = "it is invalid req, no " + name;
		return -1;
	}
	if (!json_object_is_type(_obj, json_type_string))
	{
		err_info = "it is invalid req, " + name + " isn't string.";
		return -1;

	}
	value = json_object_get_string(_obj);
	return 0;
}

int LoadTestConfig::get(const json_object *root, const std::string &name, unsigned long long &value, std::string &err_info) {
	//method
	json_object *_obj = NULL;
	if (!json_object_object_get_ex(root, name.c_str(), &_obj))
	{
		err_info = "it is invalid req, no " + name;
		return -1;
	}
	if (!json_object_is_type(_obj, json_type_int))
	{
		err_info = "it is invalid req, " + name + " isn't string.";
		return -2;

	}
	value = (unsigned long long)json_object_get_int64(_obj);
	return 0;
}

int LoadTestConfig::get(const json_object *root, const std::string &name, unsigned int &value, std::string &err_info) {
	//method
	json_object *_obj = NULL;
	if (!json_object_object_get_ex(root, name.c_str(), &_obj))
	{
		err_info = "it is invalid req, no " + name;
		return -1;
	}
	if (!json_object_is_type(_obj, json_type_int))
	{
		err_info = "it is invalid req, " + name + " isn't string.";
		return -1;

	}
	value = (unsigned int)json_object_get_int(_obj);
	return 0;
}
int LoadTestConfig::get(const json_object *root, const std::string &name, int &value, std::string &err_info) {
	//method
	json_object *_obj = NULL;
	if (!json_object_object_get_ex(root, name.c_str(), &_obj))
	{
		err_info = "it is invalid req, no " + name;
		return -1;
	}
	if (!json_object_is_type(_obj, json_type_int))
	{
		err_info = "it is invalid req, " + name + " isn't string.";
		return -1;

	}
	value = (int)json_object_get_int(_obj);
	return 0;
}

