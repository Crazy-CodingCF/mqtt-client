/************************************************************************
	> File Name: MqttPictureToBase64.h
	> Author: caofei
	> Mail: caofei_lexue@163.com
	> Created Time: 2020年01月22日 星期三 16时48分44秒
***********************************************************************/
#ifndef __MQTT_PICTURE_TO_BASE64__
#define __MQTT_PICTURE_TO_BASE64__

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

/**
 * The class is to operate picture to base64 and post mqtt server
 */
class PictureToBase64
{
	public:
		string GetPictureBase64(const string &filename);

	private:
		bool Base64Encode(const string& input, string* output);
		bool Base64Decode(const string& input, string* output);
		string ReadFileIntoString(const char *filename);
};

#endif
