/************************************************************************
	> File Name: MqttPictureToBase64.cpp
	> Author: caofei
	> Mail: caofei_lexue@163.com
	> Created Time: 2020年01月22日 星期三 16时47分42秒
***********************************************************************/
#include "MqttPictureToBase64.h"
#include <glog/logging.h>

bool PictureToBase64::Base64Encode(const string& input, string* output) {
	typedef boost::archive::iterators::base64_from_binary<boost::archive::iterators::transform_width<string::const_iterator, 6, 8> > Base64EncodeIterator;
	stringstream result;

	copy(Base64EncodeIterator(input.begin()) , Base64EncodeIterator(input.end()), ostream_iterator<char>(result));

	size_t equal_count = (3 - input.length() % 3) % 3;
	for (size_t i = 0; i < equal_count; i++) {
		result.put('=');
	}

	*output = result.str();

	return output->empty() == false;
}



bool PictureToBase64::Base64Decode(const string& input, string* output) {

	typedef boost::archive::iterators::transform_width<boost::archive::iterators::binary_from_base64<string::const_iterator>, 8, 6> Base64DecodeIterator;
	stringstream result;

	try {
		copy(Base64DecodeIterator(input.begin()) , Base64DecodeIterator(input.end()), ostream_iterator<char>(result));
	} catch(...) {
		return false;
	}

	*output = result.str();

	return output->empty() == false;
}

string PictureToBase64::ReadFileIntoString(const char *filename)
{
	ifstream ifile(filename);
	if(!ifile) {
		LOG(ERROR) << "file can't open of file not exist...";
		return "";
	}

	ostringstream buf;
	char ch;

	while(buf && ifile.get(ch)) {
		buf.put(ch);
	}
	ifile.close();

	return buf.str();
}

string PictureToBase64::GetPictureBase64(const string &filename)
{
	string input_str;
	string base64_str;
	string output_str;

	input_str = PictureToBase64::ReadFileIntoString(filename.c_str());
	if(input_str == "") {
		LOG(ERROR) << "can't change base64...";
		return "";
	}

	PictureToBase64::Base64Encode(input_str, &base64_str);
	base64_str.insert(0, "data:image/jpeg;base64,");

	return base64_str;
}
