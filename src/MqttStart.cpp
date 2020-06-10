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

#include <glog/logging.h>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include "GlogInit.h"
#include "MqttConnectServer.h"
#include "MqttParseFormat.h"
#include "config.h"


/** Defines an object for the connection to the MQTT server */
CMqttService connect_service;
CMqttFormatParse format_parse;

static int start_mqtt_flag = 0;

typedef struct _GET_PARAM { 
    string mqttconfig_path;
    string fbox_sn;         
}GET_PARAM;                 

/**
 * Initializes the connection to the server,including IP , PORT, ADDRESS, TOPIC,
 * CALLBACK and so on;
 *
 * @return Returns the status of whether the initialization was successful
 * @retval SUCCESS definition of 0
 * @retval ERROR definition of -1
 */
int InitServerConnect()
{
    if( connect_service.MqttInit() == ERROR ) {
        LOG(ERROR) << "Mqtt Init failure!";
        return ERROR;
    }

    return SUCCESS;
}

/**
 * The main entrance to the program of mqtt
 *
 * @param void *p The path of mqtt config file
 */
extern "C" {
    void *MqttStart(void *p)
    {
        printf("\033[1;34m*******************************************************************************\033[0m\n");
        printf("\033[1;34m*******************************     MQTT    ***********************************\033[0m\n");
        printf("\033[1;34m*******************************************************************************\033[0m\n");

        GlogInit("MQTT", "./build", FLOG_INFO);
        //init_mqtt_config.InitServerConfigFile();
        InitServerConnect();
        format_parse.InitQueue();
        //GlogStop();

        return NULL;
    }
}

/**
 * The function is using for test in local
 */
int main(int argc, const char *argv[])
{
    int ret = 1;
    pthread_t femqtt_start;
    pthread_attr_t attr;

    GET_PARAM param;
    param.mqttconfig_path = "/home/fbox/config/mqtt/mqttconfig-v1.1.json";
    param.fbox_sn = "123456789010";

    if( start_mqtt_flag == 0 ) {
        start_mqtt_flag = 1;

        ret = pthread_attr_init(&attr);
        if( ret != 0 ) {
            printf("pthread_attr_init failure\n");
            return ERROR;
        }

        ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if( ret != 0 ) {
            printf("pthread_attr_setdetachstate failure\n");
            return ERROR;
        }

        if( pthread_create(&femqtt_start, &attr, MqttStart, (void *)&param) ) {
            printf("pthread of mqtt thread create error\n");
            return ERROR;
        }
    }

    ret =  pthread_attr_destroy(&attr);
    if( ret != 0 ) {
        printf("pthread_attr_destroy failure\n");
        return ERROR;
    }

    while( 1 ) {
        sleep(5);
    }

    return 0;
}
