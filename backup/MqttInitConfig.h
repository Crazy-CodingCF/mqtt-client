/*************************************************************************
	> File Name: MqttInitConfig.h
	> Author: caofei
	> Mail: caofei_lexue@163.com
	> Created Time: 2019年12月12日 星期四 10时50分21秒
 ************************************************************************/
#ifndef __MQTT_CONNECT_CONFIG___
#define __MQTT_CONNECT_CONFIG___

#include "json/json.h"

using namespace std;

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

extern string MQTT_CONNECT_CONFIG_FILE;

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

/** Methods used primarily to process MQTT messages */
class MqttInitConfig
{
	public:
		MqttInitConfig();
		~MqttInitConfig();

		int InitFboxSN(string SN);
		int InitServerConfigFile();
		int InitTopicConfigInfo();

	private:
		string sn;
};

/** The Base Class */
class MultiPlatform
{
	public:
		virtual void AuthPlatform(MqttConfigInfoType &server_info, Json::Value &server_config);
	protected:
};

#endif
