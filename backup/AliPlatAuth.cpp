/*
 * Copyright (c) 2010-2020, Flexem Technologies Inc
 * All rights reserved.
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
 * 'AS IS' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include <iostream>
#include "AliPlatAuth.h"
#include "MqttEncryptData.h"

using namespace std;

void AliPlat::AuthPlatform(MqttConfigInfoType &server_info, Json::Value &server_config)
{
	Json::Value ali_plat = server_config["ali_plat"];
	if(!ali_plat.isNull()) {
		string region = ali_plat["region"].asString();
		string product_key = ali_plat["product_key"].asString();
		string device_name = ali_plat["device_name"].asString();
		string device_secret = ali_plat["device_secret"].asString();
		string timestamp = ali_plat["timestamp"].asString();
		string method = ali_plat["method"].asString();

		string ali_server_url = product_key+".iot-as-mqtt."+region+".aliyuncs.com";
		string ali_client_id;
		if(server_info.connectType == "raw_tcp") {
			ali_client_id = server_info.clientId+"|securemode=3,signmethod="+method+",timestamp="+timestamp+"|";
		} else if(server_info.connectType == "secure_tcp") {
			ali_client_id = server_info.clientId+"|securemode=2,signmethod="+method+",timestamp="+timestamp+"|";
		}
		string ali_username = device_name+"&"+product_key;
		string ali_password;
		if(!timestamp.empty()) {
			ali_password = "clientId"+server_info.clientId+"deviceName"+device_name+"productKey"+product_key+"timestamp"+timestamp;
		} else {
			ali_password = "clientId"+server_info.clientId+"deviceName"+device_name+"productKey"+product_key;
		}
		CEncryptMethod encryptdata;
		string ali_encryt_password;
		encryptdata.HmacEncode(method, device_secret, ali_password, ali_encryt_password);

		server_info.serverUrl = ali_server_url;
		server_info.serverPort = 1883;
		server_info.clientId = ali_client_id;
		server_info.userName = ali_username;
		server_info.passWord = ali_encryt_password;
	}
}


