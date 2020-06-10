/*************************************************************************
	> File Name: MqttConnectServer.h
	> Author: caofei
	> Mail: caofei_lexue@163.com
	> Created Time: 2019年10月10日 星期四 10时58分58秒
 ************************************************************************/
#ifndef __MQTT_FUNC_H__
#define __MQTT_FUNC_H__

#include <iostream>

using namespace std;

extern "C" {
#include "MQTTAsync.h"
}

extern MQTTAsync client;

/** Version number of the current program */
#define Qos0                          0
#define Qos1                          1
#define Qos2                          2

#define MQTT_TOPIC_SUB_NUM            100

#define MQTT_CONFIG_CA_CRT_FILE       "/tmp/ca.crt"
#define MQTT_CONFIG_CLIENT_CTR_FILE   "/tmp/client.crt"
#define MQTT_CONFIG_CLIENT_KEY_FILE   "/tmp/client.key"
#define MQTT_VERSION_FILE             "/tmp/mqtt.ver"

#define MQTT_CACH_DATA_SAVA_TEMP      "/tmp/"
#define MQTT_CACH_DATA_SAVA_SD        "/disk/sd/"
#define MQTT_CACH_DATA_SAVA_USB       "/disk/usb1/"

#define SERVERINFO_INITIALIZER {"FlexemMqtt", "FLEXEM",    "127.0.0.1",\
                                 1883,        "123456789", 60,\
                                 "3.1.1",     "raw_tcp",   true,\
                                 "local",     false,       " ",\
                                 " ",         "interval",  20}

#define topic_sub                        "ali/get/data"
#define topic_pub                        "ali/pub/data"


/** Handles all functions related to MQTT links */
class CMqttService
{
	public:
		CMqttService();
		~CMqttService();

		int MqttInit();
		int PublishMessage(string &out, char *topic, bool online);
		int CheckStorePath(const char *path);
		void InitConnectType(char *ADDRESS);
		void InitStoreType(char *cachPath, MQTTAsync_createOptions *create_opts);
		void InitSSLOpts(MQTTAsync_SSLOptions *sslopts);
		void InitMqttVersion(MQTTAsync_connectOptions *conn_opts);

	private:
		static void Connlost(void *context, char *cause);
		static int SslErrorCb(const char *str, size_t len, void *u);
		static void MqTraceCallback(enum MQTTASYNC_TRACE_LEVELS level, char *message);
		static int Msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message);
		static void OnSubscribe(void *context, MQTTAsync_successData *response);
		static void OnSubscribeFailure(void *context, MQTTAsync_failureData *response);
		static void OnConnect(void *context, MQTTAsync_successData *response);
		static void OnConnectFailure(void *context, MQTTAsync_failureData* response);
		static void OnSend(void *context, MQTTAsync_successData *response);
		static void OnLost(void *context, MQTTAsync_failureData *response);
};

/** This type is used to strore server configuration information */
class MqttConfigInfoType
{
	public:
	string configName;             /**< The name of the protocol */
	string platform;               /**< Supported cloud platform */
	string serverUrl;              /**< Server IP address */

	uint32_t serverPort;           /**< Server port number */
	string clientId;               /**< Client ID */
	uint32_t keepAlive;            /**< Hold connection time */

	string mqttVersion;            /**< MQTT protocol version("3.1", "3.1.1" and "5.0") */
	string connectType;            /**< Connect type("raw_tcp", "secure_tcp", "websocket") */
	bool offlineCaching;           /**< Enbled offlineCaching */

	string store;                  /**< Caching file location("local", "sd" "usb") */
	bool withAuthen;               /**< Authentication flag */
	string userName;               /**< Username for authentication */

	string passWord;               /**< passWord for authentication */
	string pub_mode;               /**< Item publish mode*/
	uint32_t pub_interval;         /**< Item publish interval*/
};


#endif

