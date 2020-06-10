/************************************************************************
	> File Name: ../include/MqttPackFormat.h
	> Author: caofei
	> Mail: caofei_lexue@163.com
	> Created Time: 2020年01月19日 星期日 13时43分03秒
***********************************************************************/
#ifndef __MQTT_PACK_FORMAT__
#define __MQTT_PACK_FORMAT__

#include <iostream>
#include <vector>
#include "MqttParseFormat.h"
#include "json/json.h"

using namespace std;

/**
 * @brief json format pack\n
 */
class CMqttPackFormat
{
	public:
		bool _online;
		CMqttPackFormat();
		~CMqttPackFormat();

		/*------------------------------received------------------------------------------*/
		int GetFcsMessage(const char *payload_str);
		void ReorgnizeFcsMessage(const Json::Value &root, const Json::Value &payload);

		int PackJsonFormat(vector<ITEM_FORMAT>& storeObj, const vector<FORMAT>& storeFormat, const vector<FORMAT> &storePayloadErrors);
		int DefalutValueAssign(ITEM_FORMAT &temp, string type, FORMAT value);
		int MergeFcsFormatData(vector<ITEM_FORMAT> &storeData, const vector<ITEM_FORMAT> &storeObj, const vector<FORMAT> &storePayload, const Json::Value &topic);
		void AssignFcsValueToVector(ITEM_FORMAT &getData, vector<FORMAT>::const_iterator &postData);
		int IsNewAttri(vector<ITEM_FORMAT> &storeData, vector<ITEM_FORMAT>::const_iterator &itrObj, const vector<FORMAT> &storePayload, const Json::Value &topic);
		int PackServerMessage(const vector<ITEM_FORMAT> &storeData, const Json::Value &topic, const Json::Value &items);
		int PackServerMessageRecursion(Json::Value &root, vector<ITEM_FORMAT>::const_iterator &itr_begin, vector<ITEM_FORMAT>::const_iterator &itr_end, bool flag);
		void AssignVectorValueToMessage(Json::Value &root, vector<ITEM_FORMAT>::const_iterator &itr);
		void ModifyTheFloatPrecision(const vector<ITEM_FORMAT>::const_iterator& itrObj, FORMAT &itrPay);

		/*------------------------------special function----------------------------------*/
		int GetPictureBase64(string &base64_str, string base64_picture_path);
		long GetTimeStamp(string &timestamp);

	private:
		string CheckStringType(vector<ITEM_FORMAT>::const_iterator itr);
		string GetDeviceId(string topic);
		string GetAliMethod(string topic);
        string AdjustiTopicInfo(string topic, const Json::Value &items);
};

#endif
