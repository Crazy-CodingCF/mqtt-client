//
// Created by Win10 on 2019/11/18.
//

#ifndef MQADAPTER_H_
#define MQADAPTER_H_
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <cstdint>
#include <cerrno>
#include <cstdio>
#include <iostream>

using namespace std;

#define TCP_TO_FCS_NAME         "/tcp_to_fcs"
#define FCS_TO_TCP_NAME         "/fcs_to_tcp"
#define MQTT_TO_FCS_NAME        "/mqtt_to_fcs"
#define FCS_TO_MQTT_NAME        "/fcs_to_mqtt"
#define H212_TO_FCS_NAME        "/h212_to_fcs"
#define FCS_TO_H212_NAME        "/fcs_to_h212"

class MqAdapter {
public:
    MqAdapter(string name, long msg_num = 20, long msg_size = 4096);
    ~MqAdapter();

    bool CreateMq();
    bool DeleteMq();
    bool MqSend(char msg[], size_t len, uint32_t msg_prio);
    long MqRecv(char *msg, uint32_t &msg_prio);
    long GetMqAttr(struct mq_attr &addr);
    long GetCurrMsgNum();
    long GetMsgSize();
    long GetMaxMsgNum();
    long GetMqFlags();
    mqd_t GetMqId();
    bool IsOpen();

private:
    string name_;
    mqd_t mq_id_;
    long mq_max_msg_num_;
    long mq_msg_size_;
};


#endif //MQADAPTER_H_
