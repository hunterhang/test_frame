#ifndef _LOG__H_
#define _LOG__H_
#include <stdio.h>
#include <string>
#include "../tcinclude/tc_common.h"
#include "../tcinclude/tc_singleton.h"
#include "testsynch.h"
static unsigned long thread_id(void)
{
	unsigned long ret;
	ret = (unsigned long)pthread_self();
	return(ret);
}

class log_file {
public:
	int Init(const std::string &filename,bool is_debug)
	{
		_is_debug = is_debug;
		if (filename != "")
		{
			_handle = fopen(filename.c_str(), "a+");
			if (_handle == NULL)
			{
				return -1;
			}
		}
		return 0;
	}
	void log_debug(const std::string &log)
	{
		if (_handle != NULL && _is_debug)
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
	}
	void log_error(const std::string &log)
	{
		if (_handle != NULL)
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

	}
	log_file() :_handle(NULL) {};
	~log_file() {
		SmartLock smart_log(_lock);
		fclose(_handle);
	}
private:
	bool _is_debug;
	FILE *_handle;
	MutexLock _lock;
};
#endif
