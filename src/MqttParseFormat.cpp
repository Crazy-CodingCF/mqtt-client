/*************************************************************************
  > File Name: MqttFormatParse.cpp
  > Author: caofei
  > Mail: caofei_lexue@163.com
  > Created Time: 2019年12月16日 星期一 19时47分50秒
 ************************************************************************/
#include <glog/logging.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <cstdio>
#include <memory>
#include "GlogInit.h"
#include "MqttParseFormat.h"
#include "MqttConnectServer.h"

extern CMqttService connect_service;

CMqttFormatParse::CMqttFormatParse()
{

}

CMqttFormatParse::~CMqttFormatParse()
{
    
}

/**
 * Initialize the message queue function
 */
int CMqttFormatParse::InitQueue()
{
    bool online;
    while(1) {
        online = MQTTAsync_isConnected(client);
        if( online == true ) {
            break;
        }
        usleep(1000 * 500);
    }
    string out = "{\"caofei\":\"1234\"}";

    while(1) {
        connect_service.PublishMessage(out,(char *)topic_pub, online);
        sleep(5);
    }

    return SUCCESS;
}

