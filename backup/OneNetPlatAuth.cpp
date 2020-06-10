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

#include <string>
#include <glog/logging.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include "OneNetPlatAuth.h"
#include "GlogInit.h"


COneNetPlatAuth::COneNetPlatAuth()
{

}

COneNetPlatAuth::~COneNetPlatAuth()
{

}

int COneNetPlatAuth::GenerateAuthToken(string device_key, string method, string version, string res, string et, string &token)
{
	string sign;
	string onenet_password;
	string StringForSignature = et+"\n"+method+"\n"+res+"\n"+version;

	sign = COneNetPlatAuth::GenerateTokenSign(StringForSignature, device_key, method);
	if(sign.empty()) {
		LOG(ERROR) << "Onenet get sign error !";
	}

	COneNetPlatAuth::OnetnetRulEncode(version);
	COneNetPlatAuth::OnetnetRulEncode(res);
	COneNetPlatAuth::OnetnetRulEncode(et);
	COneNetPlatAuth::OnetnetRulEncode(method);
	COneNetPlatAuth::OnetnetRulEncode(sign);
	onenet_password = "version="+version+"&res="+res+"&et="+et+"&method="+method+"&sign="+sign;
	token = onenet_password;

	return SUCCESS;
}

string COneNetPlatAuth::GenerateTokenSign(string StringForSignature, string device_key, string method) 
{
	string decrypted_device_access_key;
	string sign;
	unsigned char sha1[EVP_MAX_MD_SIZE] = {'\0'};
	int rc = 0;

	COneNetPlatAuth::OnenetBase64Decode((char *)device_key.c_str(), device_key.size(), decrypted_device_access_key);
	rc = COneNetPlatAuth::OnenetHmacEncode((const unsigned char *)StringForSignature.c_str(),StringForSignature.size(), (const unsigned char*)decrypted_device_access_key.c_str(), decrypted_device_access_key.size(),sha1);
	COneNetPlatAuth::OnenetBase64Encode(sha1, rc, sign);

	return sign;
}

int COneNetPlatAuth::OnenetBase64Decode(char *input, int input_len, string &decrypted_device_access_key)
{
	char output[EVP_MAX_MD_SIZE] = {0};
	BIO *bio = BIO_new_mem_buf(input, -1);
	BIO *b64 = BIO_new(BIO_f_base64());
	bio = BIO_push(b64, bio);

	//Do not use newlines to flush buffer
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

	int dec_len = COneNetPlatAuth::CalcBase64DecodeLength(input, input_len);

	if (BIO_read(bio, output,dec_len) != dec_len)
		return -1;

	string temp = output;
	decrypted_device_access_key = temp;
	BIO_free_all(bio);

	return 0;
}

int COneNetPlatAuth::CalcBase64DecodeLength(const char *b64txt, int len) 
{
	int padding = 0;

	if ('=' == b64txt[len - 1] && '=' == b64txt[len - 2])
		padding = 2;
	else if ('=' == b64txt[len-1]) //last char is =
		padding = 1;

	return (len * 3) / 4 - padding;
}


int COneNetPlatAuth::OnenetHmacEncode(const unsigned char *input, int input_len, const unsigned char *key, int key_len, unsigned char *output)
{
	if(NULL == input || 0 == input_len)
		return -1;

	unsigned int output_len = 0;

	HMAC(EVP_sha1(), key, key_len, input, input_len, output, &output_len);

	return output_len;
}


int COneNetPlatAuth::OnenetBase64Encode(const unsigned char *input, int input_len, string &out)
{
	BIO *bio, *b64;
	BUF_MEM *buf;
	char temp[(int)(EVP_MAX_MD_SIZE * 4 / 3) + 2] = {0};

	b64 = BIO_new(BIO_f_base64());
	if (!b64) {
		return -1;
	}

	bio = BIO_new(BIO_s_mem());
	if (!bio) {
		BIO_free_all(b64);
		return -1;
	}

	bio = BIO_push(b64, bio);

	//Ignore newlines - write everything in one line
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
	BIO_write(bio,input, input_len);
	BIO_flush(bio);
	BIO_get_mem_ptr(bio, &buf);

	strncpy(temp,buf->data,buf->length);

	BIO_free_all(bio);

	out = temp;
	return 0;
}

void COneNetPlatAuth::OnetnetRulEncode(string &input)
{
	for(uint32_t i = 0; i < input.size(); i++) {
		if(input[i] == '+') {
			input.erase(i,1);
			input.insert(i,"%2B",3);
		} else if(input[i] == ' ') {
			input.erase(i,1);
			input.insert(i,"%20",3);
		} else if(input[i] == '/') {
			input.erase(i,1);
			input.insert(i,"%2F",3);
		} else if(input[i] == '?') {
			input.erase(i,1);
			input.insert(i,"%3F",3);
		} else if(input[i] == '%') {
			input.erase(i,1);
			input.insert(i,"%25",3);
		} else if(input[i] == '#') {
			input.erase(i,1);
			input.insert(i,"%23",3);
		} else if(input[i] == '&') {
			input.erase(i,1);
			input.insert(i,"%26",3);
		} else if(input[i] == '=') {
			input.erase(i,1);
			input.insert(i,"%3D",3);
		}
	}
}

void OneNetPlat::AuthPlatform(MqttConfigInfoType &server_info, Json::Value &server_config)
{
	Json::Value onenet_plat = server_config["onenet_plat"];
	if(!onenet_plat.isNull()) {
		string product_id = onenet_plat["product_id"].asString();
		string device_name = onenet_plat["device_name"].asString();
		string method = onenet_plat["method"].asString();
		string device_key = onenet_plat["device_key"].asString();

		if((method[0] == 'h') && (method[1] == 'm') && (method[2] == 'a') && (method[3] == 'c')) {
			method.erase(0,4);
		}

		string token;
		string et = "4102419661"; //2100-01-01 01:01:01
		string res = "products/"+product_id+"/devices/"+device_name;
		string version = "2018-10-31";

		COneNetPlatAuth onenet_plat_auth;
		onenet_plat_auth.GenerateAuthToken(device_key, method, version, res, et, token);

		server_info.clientId = device_name;
		server_info.userName = product_id;
		server_info.passWord = token;
	}
}
