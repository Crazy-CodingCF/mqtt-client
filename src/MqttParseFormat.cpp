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
extern MqttConfigInfoType g_server_info;

CMqttFormatParse::CMqttFormatParse()
{

}

CMqttFormatParse::~CMqttFormatParse()
{
    
}

static int num = 0;
/**
 * Group of json packages
 */
string CMqttFormatParse::GetMyPostMessageFalse()
{
    Json::StreamWriterBuilder writerBuilder;
    ostringstream os;

    Json::Value root;
    Json::Value params;
    Json::Value switchs;
    Json::Value add;

    switchs["value"] = 0;
    switchs["time"] = 1591842032000 + num;

    add["value"] = num;
    add["time"] = 1591842032000 + num+1;

    params["switch"] = switchs;   
    params["add"] = add;

    root["params"] = params;
    root["id"] = "123";
    root["version"] = "1.0";
    root["method"] = "thing.event.property.post";

    num++;
    if(num > 10000000) {
        num = 0;
    }
    writerBuilder.settings_["indentation"] = "";
    writerBuilder.settings_["commentStyle"] = "None";
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());
    jsonWriter->write(root, &os);

    return os.str();
}

/**
 * Group of json packages
 */
string CMqttFormatParse::GetMyPostMessageTrue()
{
    Json::StreamWriterBuilder writerBuilder;
    ostringstream os;

    Json::Value root;
    Json::Value params;
    Json::Value switchs;
    Json::Value add;

    switchs["value"] = 1;
    switchs["time"] = 1591842032000 + num;

    add["value"] = num;
    add["time"] = 1591842032000 + num + 1;

    params["switch"] = switchs;   
    params["add"] = add;

    root["params"] = params;
    root["id"] = "123";
    root["version"] = "1.0";
    root["method"] = "thing.event.property.post";

    num++;
    if(num > 10000000) {
        num = 0;
    }
    writerBuilder.settings_["indentation"] = "";
    writerBuilder.settings_["commentStyle"] = "None";
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());
    jsonWriter->write(root, &os);

    return os.str();
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

    while(1) {
        string out_f = GetMyPostMessageFalse();
        connect_service.PublishMessage(out_f,(char *)topic_pub, online);
        sleep(g_server_info.pub_interval);

        string out_t = GetMyPostMessageTrue();
        connect_service.PublishMessage(out_t,(char *)topic_pub, online);
        sleep(g_server_info.pub_interval);
    }

    return SUCCESS;
}

