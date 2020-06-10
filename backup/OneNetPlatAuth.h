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
#ifndef __ONENET_PLAT_AUTH__
#define __ONENET_PLAT_AUTH__

#include <iostream>
#include "MqttInitConfig.h"

using namespace std;

class COneNetPlatAuth
{
	public:
		COneNetPlatAuth();
		~COneNetPlatAuth();

		int GenerateAuthToken(string device_key,string method, string version, string res, string et,string &token);
		string GenerateTokenSign(string StringForSignature, string device_key, string method);
		int OnenetBase64Decode(char *input, int intput_len, string &decrypted_device_access_key);
		int CalcBase64DecodeLength(const char *b64txt, int len);
		int OnenetHmacEncode(const unsigned char *input, int input_len, const unsigned char *key, int key_len, unsigned char *output);
		int OnenetBase64Encode(const unsigned char *input, int input_len, string &out);
		void OnetnetRulEncode(string &input);
		
	private:
};

class OneNetPlat:public MultiPlatform
{
	public:
		void AuthPlatform(MqttConfigInfoType &server_info, Json::Value &server_config);
	private:
};

#endif
