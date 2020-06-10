/*************************************************************************
  > File Name: MqttParseFormat.h
  > Author: caofei
  > Mail: caofei_lexue@163.com
  > Created Time: 2019年12月16日 星期一 19时48分45秒
 ************************************************************************/
#ifndef __MQTT_PARSE_FORMAT__
#define __MQTT_PARSE_FORMAT__

#include <iostream>
#include <vector>
#include <set>
#include "json/json.h"
#include "MqttConnectServer.h"


using namespace std;

/**
 * @brief json format parse\n
 *
 *  */
class CMqttFormatParse
{
	public:
		CMqttFormatParse();
		~CMqttFormatParse();
		int InitQueue();

	private:
};

#endif
