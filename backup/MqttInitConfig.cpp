/*
 * Copyright (c) 2010-2020, FlexEm Technologies Inc
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided that
 * the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of FlexEm Technologies Inc nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS
 * LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <glog/logging.h>
#include <algorithm>
#include "GlogInit.h"
#include "MqttInitConfig.h"
#include "MqttEncryptData.h"
#include "MqttPictureToBase64.h"
#include "OneNetPlatAuth.h"
#include "AliPlatAuth.h"
#include "HuaWeiPlatAuth.h"

string MQTT_CONNECT_CONFIG_FILE;

using namespace std;

MqttConfigInfoType g_server_info = SERVERINFO_INITIALIZER;
vector<string> g_topic_list;
Json::Value g_function_config = Json::nullValue;

MqttInitConfig::MqttInitConfig()
{
	sn.clear();
}

MqttInitConfig::~MqttInitConfig()
{

}

int MqttInitConfig::InitFboxSN(string SN)
{
	if(!SN.empty()) {
		sn = SN;
	} else {
		LOG(ERROR) << "ERROR : Fbox sn is null";
		return ERROR;
	}
}

/**
 * Store the information in the configuration file of "mqttconfig.json" in a global variable of "g_server_info"
 * Gets the fbox serial number stored in the global variable
 * Store box connection configuration information
 * Create and store key certificate file information
 *
 * @return Returns the status of whether the initialization was successful
 * @retval SUCCESS definition of 0
 * @retval ERROR definition of -1
 */
int MqttInitConfig::InitServerConfigFile()
{
	Json::CharReaderBuilder readerBuilder;
	readerBuilder["collectComments"] = false;
	JSONCPP_STRING errs;
	Json::Value obj;
	ifstream ifs;
	ifs.open(MQTT_CONNECT_CONFIG_FILE, ios::in);
	if(!ifs.is_open()) {
		LOG(ERROR) << "Open json file error !";
	}

	bool parsesuccessful = Json::parseFromStream(readerBuilder, ifs, &obj, &errs);
	if( !parsesuccessful || !errs.empty()) {
		LOG(ERROR) << "Faild to parase configuration";
		LOG(ERROR) << "ERROR : " << errs;
		ifs.close();
		return ERROR;
	}
	ifs.close();

	Json::Value serverConfig = obj["server_config"];
	if( !serverConfig.isNull() ) {
		g_server_info.configName = serverConfig["config_name"].asString();
		g_server_info.platform = serverConfig["platform"].asString();
		g_server_info.serverUrl = serverConfig["server_url"].asString();
		g_server_info.clientId = serverConfig["client_id"].asString();
		g_server_info.mqttVersion = serverConfig["mqtt_version"].asString();
		g_server_info.connectType = serverConfig["connection_type"].asString();
		g_server_info.store = serverConfig["store"].asString();
		g_server_info.pub_mode = serverConfig["pub_mode"].asString();

		g_server_info.serverPort = serverConfig["server_port"].asInt();
		g_server_info.keepAlive = serverConfig["keepalive"].asInt();
		g_server_info.offlineCaching = serverConfig["offline_caching"].asBool();
		g_server_info.withAuthen = serverConfig["with_authen"].asBool();
		g_server_info.pub_interval = serverConfig["pub_interval"].asInt();

		g_server_info.userName = serverConfig["username"].asString();
		g_server_info.passWord = serverConfig["password"].asString();

		if(!serverConfig["ca_crt"].isNull()) {
			FILE *file;
			file = fopen(MQTT_CONFIG_CA_CRT_FILE,"w");
			if(file == NULL) {
				LOG(ERROR) << "fopen error";
				return ERROR;
			}
			fprintf(file, "%s", serverConfig["ca_crt"].asString().c_str());
			fclose(file);
		}

		if(!serverConfig["client_crt"].isNull()) {
			FILE *file;
			file = fopen(MQTT_CONFIG_CLIENT_CTR_FILE,"w");
			if(file == NULL) {
				LOG(ERROR) << "fopen error";
				return ERROR;
			}
			fprintf(file, "%s", serverConfig["client_crt"].asString().c_str());
			fclose(file);
		}

		if(!serverConfig["client_key"].isNull()) {
			FILE *file;
			file = fopen(MQTT_CONFIG_CLIENT_KEY_FILE,"w");
			if(file == NULL) {
				LOG(ERROR) << "fopen error";
				return ERROR;
			}
			fprintf(file, "%s", serverConfig["client_key"].asString().c_str());
			fclose(file);
		}

		MultiPlatform plat;
		MultiPlatform *plat_ptr = &plat;
		if(g_server_info.platform == "ali_plat") {
			AliPlat ali;
			plat_ptr = &ali;
		} else if(g_server_info.platform == "huawei_plat") {
			HuaWeiPlat huawei;
			plat_ptr = &huawei;
		} else if( g_server_info.platform == "onenet_plat") {
			OneNetPlat onenet;
			plat_ptr = &onenet;
		}
		plat_ptr->AuthPlatform(g_server_info, serverConfig);

	} else {
		LOG(ERROR) << "serverConfig error";
		return ERROR;
	}

	LOG(INFO) << "SN : " << sn;
	LOG(INFO) << "config_name : " << g_server_info.configName;
	LOG(INFO) << "platform : " << g_server_info.platform;
	LOG(INFO) << "server_url : " << g_server_info.serverUrl;
	LOG(INFO) << "server_port : " << g_server_info.serverPort;
	LOG(INFO) << "client_id : " << g_server_info.clientId;
	LOG(INFO) << "keepalive : " << g_server_info.keepAlive;
	LOG(INFO) << "mqtt_version : " << g_server_info.mqttVersion;
	LOG(INFO) << "connection_type : " << g_server_info.connectType;
	if(!serverConfig["ca_crt"].isNull() && !serverConfig["client_crt"].isNull() && !serverConfig["client_key"].isNull()) {
		LOG(INFO) << "ca_crt : " << MQTT_CONFIG_CA_CRT_FILE;
		LOG(INFO) << "client_crt : " << MQTT_CONFIG_CLIENT_CTR_FILE;
		LOG(INFO) << "client_key : " << MQTT_CONFIG_CLIENT_KEY_FILE;
	} else {
		LOG(INFO) << "ca_crt : " << "NULL";
		LOG(INFO) << "client_crt : " << "NULL";
		LOG(INFO) << "client_key : " << "NULL";
	}
	LOG(INFO) << "offlineCaching : " << g_server_info.offlineCaching;
	LOG(INFO) << "store : " << g_server_info.store;
	LOG(INFO) << "with_authen : " << g_server_info.withAuthen;
	LOG(INFO) << "username : " << g_server_info.userName;
	LOG(INFO) << "passWord : " << g_server_info.passWord;
	LOG(INFO) << "pub_mode : " << g_server_info.pub_mode;
	LOG(INFO) << "pub_interval : " << g_server_info.pub_interval;

	return SUCCESS;
}

void MultiPlatform::AuthPlatform(MqttConfigInfoType &server_info, Json::Value &server_config)
{
    ;//Do not anything
}

/**
 * Store all subs and pub_subs in the configuration file of "mqttconfig.json" in
 * global variables of "g_topic_list"
 *
 * Store "function_config" to "g_function_config"
 *
 * @return Returns the status of whether the initialization was successful
 * @retval SUCCESS definition of 0
 * @retval ERROR definition of -1
 */
int MqttInitConfig::InitTopicConfigInfo()
{
	Json::CharReaderBuilder readerBuilder;
	readerBuilder["collectComments"] = false;
	JSONCPP_STRING errs;
	Json::Value obj;
	char function[20] = {0};
	ifstream ifs;

	ifs.open(MQTT_CONNECT_CONFIG_FILE, ios::in);
	if(!ifs.is_open()) {
		LOG(ERROR) << "Open json file error !";
	}

	bool parsesuccessful = Json::parseFromStream(readerBuilder, ifs, &obj, &errs);
	if(!parsesuccessful || !errs.empty()) {
		LOG(ERROR) << "Faild to parse configuration";
		LOG(ERROR) << "ERROR : " << errs;
		return ERROR;
	}
	ifs.close();

	g_function_config = obj["function_config"];
	if( !g_function_config.isNull() ) {
		int num = g_function_config.size();
		for(int i = 0; i < num; i++) {
			memset(function, 0, sizeof(function));
			sprintf(function, "function_%d", i+1);
			Json::Value root = g_function_config[function];

			Json::Value sub = root["sub"];
			if( !sub.isNull() ) {
				Json::Value topic = sub["topic"];
				g_topic_list.push_back(topic.asString().c_str());
				continue;
			}

			Json::Value pub_sub = root["pub_sub"];
			if( !pub_sub.isNull() ) {
				Json::Value topic = pub_sub["topic"];
				g_topic_list.push_back(topic.asString().c_str());
			}
		}
	} else {
		LOG(ERROR) << "g_function_config error";
		return ERROR;
	}

	return SUCCESS;
}
