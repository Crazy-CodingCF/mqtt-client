/*************************************************************************
	> File Name: GlogInit.cpp
	> Author: caofei
	> Mail: caofei_lexue@163.com
	> Created Time: 2019年10月09日 星期三 20时54分19秒
 ************************************************************************/
#include <glog/logging.h>
#include "GlogInit.h"

int32_t GlogInit(const char* tag, const char* log_dir, int32_t log_level)
{
	CHECK_NOTNULL(log_dir);
	google::InitGoogleLogging(tag);
	FLAGS_stderrthreshold = log_level;
	FLAGS_minloglevel = log_level;
	FLAGS_logbufsecs = 0;             // Print log int real time
	FLAGS_max_log_size = 1;           // Max log size 1MB
	FLAGS_log_dir = log_dir;
	FLAGS_colorlogtostderr = true;
	FLAGS_alsologtostderr = true;

	return 0;
}

int32_t GlogStop()
{
	google::ShutdownGoogleLogging();
}
