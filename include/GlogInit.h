/*************************************************************************
	> File Name: GlogInit.h
	> Author: caofei
	> Mail: caofei_lexue@163.com
	> Created Time: 2019年10月09日 星期三 20时59分20秒
 ************************************************************************/
#ifndef __GLOG_H__
#define __GLOG_H__

#include <string>

#define RESET       "\033[0m"
#define BLACK       "\033[30m"             /* Black */
#define RED         "\033[31m"             /* Red */
#define GREEN       "\033[32m"             /* Green */
#define YELLOW      "\033[33m"             /* Yellow */
#define BLUE        "\033[34m"             /* Blue */
#define MAGENTA     "\033[35m"             /* Magenta */
#define CYAN        "\033[36m"             /* Cyan */
#define WHITE       "\033[37m"             /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */
#define BCBLACK     "\033[7m\033[30m"      /* Bold Black */
#define BCRED       "\033[7m\033[31m"      /* Bold Red */
#define BCGREEN     "\033[7m\033[32m"      /* Bold Green */
#define BCYELLOW    "\033[7m\033[33m"      /* Bold Yellow */
#define BCBLUE      "\033[7m\033[34m"      /* Bold Blue */
#define BCMAGENTA   "\033[7m\033[35m"      /* Bold Magenta */
#define BCCYAN      "\033[7m\033[36m"      /* Bold Cyan */
#define BCWHITE     "\033[7m\033[37m"      /* Bold White */


int32_t GlogInit(const char* tag, const char* log_dir, int32_t log_level);

int32_t GlogStop();

enum SeverityLevel {
	FLOG_INFO = 0,
	FLOG_WARNING = 1,
	FLOG_ERROR = 2,
	FLOG_FATAL = 3
};

enum ReturnInfo {
	ERROR = -1,
	SUCCESS = 0
};

#endif
