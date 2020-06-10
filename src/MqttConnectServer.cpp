/*************************************************************************
  > File Name: MqttConnectServer.cpp
  > Author: caofei
  > Mail: caofei_lexue@163.com
  > Created Time: 2019年10月10日 星期四 09时36分33秒
 ************************************************************************/
#include <glog/logging.h>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <string>
#include <vector>
#include <iostream>
#include "GlogInit.h"
#include "MqttConnectServer.h"

MQTTAsync client = NULL;

MqttConfigInfoType g_server_info;

using namespace std;

CMqttService::CMqttService()
{

}

CMqttService::~CMqttService()
{

}

void CMqttService::Connlost(void *context, char *cause)
{
    LOG(ERROR) << "Connection lost";
    LOG(ERROR) << "Reconnecting";
}

int CMqttService::SslErrorCb(const char *str, size_t len, void *u)
{
    LOG(ERROR) << "SslErrorCb : " << str;
    return 0;
}

void CMqttService::MqTraceCallback(enum MQTTASYNC_TRACE_LEVELS level, char *message)
{
    LOG(ERROR) << "trace : " << level << ", " << message;
}

void CMqttService::OnSend(void *context, MQTTAsync_successData *response)
{
    LOG(INFO) << "Message with token value " << response->token << " delivery successed";
}

void CMqttService::OnLost(void *context, MQTTAsync_failureData *response)
{
    LOG(ERROR) << "Message send Failed";
}

int CMqttService::Msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    string payload_str;
    payload_str = (char *)message->payload;
    payload_str = payload_str.substr(0, message->payloadlen);

    LOG(INFO) << YELLOW << "message : " << BOLDYELLOW << "server -->  mqtt" << RESET;
    LOG(INFO) << YELLOW << "topic : " << topicName << RESET;
    LOG(INFO) << YELLOW << "messageLen :" << message->payloadlen << RESET;
    LOG(INFO) << YELLOW << "payload : " << payload_str << RESET;

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);

    return 1;
}

void CMqttService::OnSubscribe(void *context, MQTTAsync_successData *response)
{
    LOG(INFO) << "Subscribe Successful " << response->token;
}

void CMqttService::OnSubscribeFailure(void *context, MQTTAsync_failureData *response)
{
    LOG(ERROR) << "Subscribe failed, rc " << response->code;
    LOG(ERROR) << "Subscribe failed info :  " << MQTTAsync_strerror(response->code);
}

/**
 * The callback function of connect successful
 */
void CMqttService::OnConnect(void *context, MQTTAsync_successData *response)
{
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    char **topics = NULL;
    int *topicsQos = NULL;
    int rc;

    opts.onSuccess = OnSubscribe;
    opts.onFailure = OnSubscribeFailure;
    opts.context = client;

    topics = (char **)calloc(MQTT_TOPIC_SUB_NUM, sizeof(char *));
    topicsQos = (int *)calloc(MQTT_TOPIC_SUB_NUM, sizeof(int));

    topics[0] = (char *)topic_sub;

    LOG(INFO) << "Subscribe topic." << 1 << "|Qos" << Qos1 << " : "<< topics[0]; 
    if((rc = MQTTAsync_subscribe(client, topics[0], Qos1, &opts)) != MQTTASYNC_SUCCESS ){
        LOG(ERROR) << "Failed to start subscribe, return code " << rc;
        exit(EXIT_FAILURE);
    }

    LOG(INFO) << "Successful Connection";

    free((void*)topics);
    free(topicsQos);
}

/**
 * Checks to see if the specified path exists
 *
 * @param path specified path
 * @retval SUCCESS definition of 0
 * @retval ERROR definition of -1
 */
int CMqttService::CheckStorePath(const char *path)
{
    if(access(path, F_OK) < 0){
        LOG(ERROR) << path << " is not exist";
        return ERROR;
    }

    return SUCCESS;
}

void CMqttService::OnConnectFailure(void *context, MQTTAsync_failureData* response)
{
    LOG(ERROR) << "Connect failed, rc " << response->code;
    LOG(ERROR) << "Connect failed info : " << MQTTAsync_strerror(response->code);
}

/**
 * Select the link mode for MQTT,which is provice three ways
 *
 * @param ADDRESS  store link address
 */
void CMqttService::InitConnectType(char *ADDRESS)
{
    if(ADDRESS == NULL){
        LOG(INFO) << "ADDRESS error" << endl;
        return;
    }

    if( g_server_info.connectType == "raw_tcp" ) {
        /*TCP connect*/
        memset(ADDRESS, 0, sizeof(ADDRESS));
        sprintf(ADDRESS, "tcp://%s:%d", g_server_info.serverUrl.c_str(), g_server_info.serverPort);
    } else if( g_server_info.connectType == "secure_tcp" ) {
        /*TCP with SSL connet*/
        memset(ADDRESS, 0, sizeof(ADDRESS));
        sprintf(ADDRESS, "ssl://%s:%d", g_server_info.serverUrl.c_str(), g_server_info.serverPort);
    } else if( g_server_info.connectType == "websocket" ) {
        /*WEBSOCKET connect*/
        memset(ADDRESS, 0, sizeof(ADDRESS));
    }

    LOG(INFO) << "CONNECT TYPE : " << g_server_info.connectType;
    LOG(INFO) << "ADDRESS : " << ADDRESS;
}

/**
 * Select the location of the offline cache data store
 *
 * @param cachPath the location of cache data
 * @param create_opts sets the number of cached data
 */
void CMqttService::InitStoreType(char *cachPath, MQTTAsync_createOptions *create_opts)
{
    memset(cachPath, 0, sizeof(cachPath));

    if( g_server_info.store == "local" ) {
        /*offlineCaching data store temp*/
        sprintf(cachPath, "%s", MQTT_CACH_DATA_SAVA_TEMP);
        create_opts->maxBufferedMessages = 1000;
    } else if( g_server_info.store == "sd" ) {
        /*offlineCaching data store SD-CARD*/
        if(CheckStorePath(MQTT_CACH_DATA_SAVA_SD) == SUCCESS) {
            sprintf(cachPath, "%s", MQTT_CACH_DATA_SAVA_SD);
            create_opts->maxBufferedMessages = INT_MAX;
        } else {
            sprintf(cachPath, "%s", MQTT_CACH_DATA_SAVA_TEMP);
            create_opts->maxBufferedMessages = 1000;
        }
    } else if( g_server_info.store == "usb" ) {
        /*offlineCaching data store USB*/
        if(CheckStorePath(MQTT_CACH_DATA_SAVA_USB) == SUCCESS) {
            sprintf(cachPath, "%s", MQTT_CACH_DATA_SAVA_USB);
            create_opts->maxBufferedMessages = INT_MAX;
        } else {
            sprintf(cachPath, "%s", MQTT_CACH_DATA_SAVA_TEMP);
            create_opts->maxBufferedMessages = 1000;
        }
    }

    LOG(INFO) << "Caching Path : " << cachPath;
}

/**
 * If it is encrypted link, you need to configure the key information
 *
 * @param  sslopts  Configure key options
 */
void CMqttService::InitSSLOpts(MQTTAsync_SSLOptions *sslopts)
{
    if( g_server_info.connectType == "secure_tcp" )
    {
        /*STL connect need certificate*/
        sslopts->trustStore = MQTT_CONFIG_CA_CRT_FILE;            //ca.crt(PM): official certificate
        sslopts->keyStore = MQTT_CONFIG_CLIENT_CTR_FILE;          //client.crt(PM): client public key certificate(mutual authentication)
        sslopts->privateKey = MQTT_CONFIG_CLIENT_KEY_FILE;        //client.key(PM): client private key certificate(mutual authentication)

        sslopts->privateKeyPassword = NULL;                       //if encrypted,this is the client private key password
        sslopts->enabledCipherSuites = NULL;                      //Whether password suite is enabled
        sslopts->enableServerCertAuth = true;                     //Whether server certificate authentication is enabled
        sslopts->sslVersion = MQTT_SSL_VERSION_TLS_1_2;           //SSL version
        sslopts->verify = true;                                   //Whether check the connection,including the certificate matches the host name
        sslopts->CApath = NULL;                                   //Folder containing CA certificates in PEM format
        sslopts->ssl_error_cb = &SslErrorCb;                      //SSL connect callback function
        sslopts->ssl_error_context = client;                      //connect handle
    }
}

/**
 * Select the MQTT protocol version
 *
 * @param conn_opts Configure version options
 */
void CMqttService::InitMqttVersion(MQTTAsync_connectOptions *conn_opts)
{
    if( g_server_info.mqttVersion == "3.1" ) {
        conn_opts->MQTTVersion = MQTTVERSION_3_1;
    } else if( g_server_info.mqttVersion == "3.1.1" ) {
        conn_opts->MQTTVersion = MQTTVERSION_3_1_1;
    } else if( g_server_info.mqttVersion == "5.0" ) {
        conn_opts->MQTTVersion = MQTTVERSION_5;
    }
}

/**
 * MQTT enters the main entrance to the link server
 *
 * @return Returns the status of whether the initialization was successful
 * @retval SUCCESS definition of 0
 * @retval ERROR definition of -1
 */
int CMqttService::MqttInit()
{
    int ret = 0;
    char ADDRESS[100] = {"0"};
    char cachPath[50] = "./";

    g_server_info.configName = "demo";
    g_server_info.platform = "ali";
    g_server_info.serverUrl = "39.99.57.4";
    g_server_info.clientId = "demo_test_caofei";
    g_server_info.mqttVersion = "3.1.1";
    g_server_info.connectType = "raw_tcp";
    g_server_info.store = "local";
    g_server_info.pub_mode = "interval";

    g_server_info.serverPort = 10001;
    g_server_info.keepAlive = 60;
    g_server_info.offlineCaching = false;
    g_server_info.withAuthen = true;
    g_server_info.pub_interval = 15*1000;

    g_server_info.userName = "caofei";
    g_server_info.passWord = "123456789";

    MQTTAsync_createOptions create_opts = MQTTAsync_createOptions_initializer;
    MQTTAsync_SSLOptions sslopts = MQTTAsync_SSLOptions_initializer;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

    LOG(INFO) << "configName : " << g_server_info.configName;
    LOG(INFO) << "platform : " << g_server_info.platform;
    LOG(INFO) << "clientId : " << g_server_info.clientId;
    LOG(INFO) << "mqttVersion : " << g_server_info.mqttVersion;
    LOG(INFO) << "pub_mode : " << g_server_info.pub_mode;
    LOG(INFO) << "serverPort : " << g_server_info.serverPort;
    LOG(INFO) << "keepAlive : " << g_server_info.keepAlive;
    LOG(INFO) << "withAuthen : " << g_server_info.withAuthen;
    LOG(INFO) << "pub_interval : " << g_server_info.pub_interval;
    
    CMqttService::InitConnectType(ADDRESS);
    CMqttService::InitStoreType(cachPath, &create_opts);

    /*Select whether to use offline caching*/
    if( g_server_info.offlineCaching == true ){
        create_opts.sendWhileDisconnected = true;
        if( MQTTAsync_createWithOptions(&client, ADDRESS, g_server_info.clientId.c_str(), MQTTCLIENT_PERSISTENCE_DEFAULT, cachPath, &create_opts) != MQTTASYNC_SUCCESS){
            LOG(ERROR) << "MQTTAsync_connectOptions failure";
            return ERROR;
        }
        LOG(INFO) << "offlineCaching : " << g_server_info.offlineCaching;
    }else{
        if( MQTTAsync_create(&client, ADDRESS, g_server_info.clientId.c_str(), MQTTCLIENT_PERSISTENCE_DEFAULT, cachPath) != MQTTASYNC_SUCCESS ){
            LOG(ERROR) << "MQTTAsync_create failure";
            return ERROR;
        }
        LOG(INFO) << "offlineCaching : " << g_server_info.offlineCaching;
    }

    MQTTAsync_setCallbacks(client, NULL, Connlost, Msgarrvd, NULL);
    MQTTAsync_setTraceCallback(&MqTraceCallback);
    MQTTAsync_setTraceLevel(MQTTASYNC_TRACE_ERROR);
    CMqttService::InitSSLOpts(&sslopts);
    CMqttService::InitMqttVersion(&conn_opts);

    conn_opts.ssl = &sslopts;

    if( g_server_info.withAuthen == true ){
        conn_opts.username = g_server_info.userName.c_str();
        conn_opts.password = g_server_info.passWord.c_str();
        LOG(INFO) << "userName : " << g_server_info.userName.c_str();
        LOG(INFO) << "password : " << g_server_info.passWord.c_str();
    }

    conn_opts.keepAliveInterval = g_server_info.keepAlive;
    LOG(INFO) << "keepAlive : " << g_server_info.keepAlive;
    LOG(INFO) << "offlineCaching : " << g_server_info.offlineCaching;
    if(g_server_info.offlineCaching == true) {
        conn_opts.cleansession = false;
    } else {
        conn_opts.cleansession = true;
    }
    conn_opts.onSuccess = OnConnect;
    conn_opts.onFailure = OnConnectFailure;
    conn_opts.context = client;
    conn_opts.automaticReconnect = true;

    if( (ret = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS ){
        LOG(ERROR) << "Failed to start connect, return code " << ret;
        LOG(ERROR) << "Failed info : " << MQTTAsync_strerror(ret);
    }

    return SUCCESS;
}

/**
 * Publish the data content required by the MQTT server
 *
 * @param out the data of content
 * @param topic the data of need topic
 * @param online The current client is online
 */
int CMqttService::PublishMessage(string &out, char *topic, bool online)
{
    int ret;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    opts.onSuccess = OnSend;
    opts.onFailure = OnLost;
    opts.context = client;

    pubmsg.payload = (char *)out.c_str();
    pubmsg.payloadlen = out.length();
    pubmsg.qos = Qos1;
    pubmsg.retained = 0;

    LOG(INFO) << BLUE << "message : " << BOLDBLUE << "mqtt --> server " << RESET;
    LOG(INFO) << BLUE << "topic : " << topic << RESET;
    if( online == true ) {
        //Because of the glog library, anything larger than 20000 bytes is not
        //stored in the log file
        if(out.length() > 20000) {
            cout << out << endl;
        } else {
            LOG(INFO) << BLUE << "payload(online): " << out << RESET;
        }
    } else {
        LOG(INFO) << BLUE << "payload(offline): " << out << RESET;
    }

    if( (ret = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS ) {
        LOG(ERROR) << "Failed to start sendMessage, return code " << ret;
        LOG(ERROR) << "Failed info : " << MQTTAsync_strerror(ret);
        return ERROR;
    }

    return SUCCESS;
}
