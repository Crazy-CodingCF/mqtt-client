#include <boost/algorithm/hex.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/uuid/detail/sha1.hpp>
#include <openssl/hmac.h>
#include <iomanip> 
#include <sstream>
#include "MqttEncryptData.h"

int CEncryptMethod::GetMd5(std::string &str_md5, const char * const buffer, size_t buffer_size) 
{
	if(buffer == nullptr) {
		LOG(ERROR) << "ERROR : ali_password is null";
		return ERROR;
	} else {
		LOG(INFO) << buffer;
	}

	boost::uuids::detail::md5 boost_md5;
	boost_md5.process_bytes(buffer, buffer_size);
	boost::uuids::detail::md5::digest_type digest;
	boost_md5.get_digest(digest);
	const auto char_digest = reinterpret_cast<const char*>(&digest);
	str_md5.clear();
	boost::algorithm::hex(char_digest,char_digest+sizeof(boost::uuids::detail::md5::digest_type), std::back_inserter(str_md5));

	return SUCCESS;
}

int CEncryptMethod::GetSHA1(std::string &str_sha1, const char * const buffer, size_t buffer_size)
{
	if(buffer == nullptr) {
		LOG(ERROR) << "ERROR : ali_password is null";
		return ERROR;
	} else {
		LOG(INFO) << buffer;
	}

	char hash[20] = {0};
	boost::uuids::detail::sha1 boost_sha1;
	boost_sha1.process_bytes(buffer, buffer_size);
	boost::uuids::detail::sha1::digest_type digest;
	boost_sha1.get_digest(digest);
	for(int i = 0; i < 5; ++i) {
		const char *tmp = reinterpret_cast<char*>(digest);
		hash[i*4] = tmp[i*4+3];
		hash[i*4+1] = tmp[i*4+2];
		hash[i*4+2] = tmp[i*4+1];
		hash[i*4+3] = tmp[i*4]; 
	}
	
	str_sha1.clear();
	std::ostringstream buf;
	for(int i = 0; i < 20; ++i) {
		buf << setiosflags(ios::uppercase) << std::hex << ((hash[i] & 0x0000000F0) >> 4);
		buf << setiosflags(ios::uppercase) << std::hex << (hash[i] & 0x00000000F);
	}

	str_sha1 = buf.str();
	return SUCCESS;
}

int CEncryptMethod::HmacEncode(string method, string key, string data, string &encrypt_data) 
{
	unsigned int output_length = 0;
	unsigned char output[100] = {0};
	const EVP_MD * engine = NULL;

	if(method == "hmacsha512") {
		engine = EVP_sha512();
	} else if(method == "hmacsha256") {
		engine = EVP_sha256();
	} else if(method == "hmacsha1") {
		engine = EVP_sha1();
	} else if(method == "hmacmd5") {
		engine = EVP_md5();
	} else if(method == "hmacsha224") {
		engine = EVP_sha224();
	} else if(method == "hmacsha384") {
		engine = EVP_sha384();
	} else if(method == "hmacsha") {
		engine = EVP_sha();
	} else {
		cout << "Algorithm " << method << " is not supported by this program!" << endl;
		return ERROR;
	}

	HMAC_CTX ctx;
	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, key.c_str(), key.length(), engine, NULL);
	HMAC_Update(&ctx, (const unsigned char *)data.c_str(), data.length());
	HMAC_Final(&ctx, output, &output_length);
	HMAC_CTX_cleanup(&ctx);

	encrypt_data.clear();
	std::ostringstream buf;
	for(unsigned int i = 0; i < output_length; ++i) {
		buf << setiosflags(ios::uppercase) << std::hex << ((output[i] & 0x0000000F0) >> 4); 
		buf << setiosflags(ios::uppercase) << std::hex << (output[i] & 0x00000000F); 
	}

	encrypt_data = buf.str();
	return ERROR;
}
