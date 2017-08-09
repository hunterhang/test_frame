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
		//ip
		json_object *_cmd = NULL;
		if (!json_object_object_get_ex(_cmd_info, "cmd", &_cmd))
		{
			printf("config error!index:%d\n", i);
			continue;
		}
		if (!json_object_is_type(_cmd, json_type_int))
		{
			printf("config error!cmd should be int,index:%d\n",i);
			continue;
		}
		cmd_info.cmd = json_object_get_int(_cmd);

		//type
		json_object *_type = NULL;
		if (!json_object_object_get_ex(_cmd_info, "type", &_type))
		{
			printf("config error!index:%d\n", i);
			continue;
		}
		if (!json_object_is_type(_type, json_type_int))
		{
			printf("config error!type should be int,index:%d\n", i);
			continue;
		}
		cmd_info.type = json_object_get_int(_type);

		//ip
		json_object *_ip = NULL;
		if (!json_object_object_get_ex(_cmd_info, "ip", &_ip))
		{
			printf("config error!index:%d\n", i);
			continue;
		}
		if (!json_object_is_type(_ip, json_type_string))
		{
			printf("config error!ip should be int,index:%d\n", i);
			continue;
		}
		cmd_info.ip = json_object_get_string(_ip);

		//port
		json_object *_port = NULL;
		if (!json_object_object_get_ex(_cmd_info, "port", &_port))
		{
			printf("config error!index:%d\n", i);
			continue;
		}
		if (!json_object_is_type(_port, json_type_int))
		{
			return -1;
		}
		cmd_info.port = (unsigned short)json_object_get_int(_port);

		if (cmd_info.type == 2)
		{
			//pre_req
			json_object *_pre_req = NULL;
			if (!json_object_object_get_ex(_cmd_info, "pre_req", &_pre_req))
			{
				printf("config error!index:%d\n", i);
				continue;
			}
			if (!json_object_is_type(_pre_req, json_type_string))
			{
				printf("config error!_pre_req should be int,index:%d\n", i);
				continue;
			}
			cmd_info.pre_req = json_object_get_string(_pre_req);
		}
		//req
		json_object *_req = NULL;
		if (!json_object_object_get_ex(_cmd_info, "req", &_req))
		{
			printf("config error!index:%d\n", i);
			continue;
		}
		if (!json_object_is_type(_req, json_type_string))
		{
			printf("config error!_pre_req should be int,index:%d\n", i);
			continue;
		}
		cmd_info.req = json_object_get_string(_req);

		//thread_num
		json_object *_thread_num = NULL;
		if (!json_object_object_get_ex(_cmd_info, "thread_num", &_thread_num))
		{
			printf("config error!index:%d\n", i);
			continue;
		}
		if (!json_object_is_type(_thread_num, json_type_int))
		{
			return -1;
		}
		cmd_info.thread_num = (unsigned int)json_object_get_int(_thread_num);

		//load_test_time
		json_object *_load_test_time = NULL;
		if (!json_object_object_get_ex(_cmd_info, "load_test_time", &_load_test_time))
		{
			printf("config error!index:%d\n", i);
			continue;
		}
		if (!json_object_is_type(_load_test_time, json_type_int))
		{
			return -1;
		}
		cmd_info.load_test_time = (unsigned int)json_object_get_int(_load_test_time);

		//log_file
		json_object *_log_file = NULL;
		if (!json_object_object_get_ex(_cmd_info, "log_file", &_log_file))
		{
			printf("config error!index:%d\n", i);
			continue;
		}
		if (!json_object_is_type(_log_file, json_type_string))
		{
			printf("config error!_log_file should be int,index:%d\n", i);
			continue;
		}
		cmd_info.log_file = json_object_get_string(_log_file);
		if (cmd_info.cmd == 0)
		{
			printf("config error!cmd error,index:%d\n", i);
			continue;
		}
		_config_list.insert(make_pair(cmd_info.cmd, cmd_info));
	}
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
