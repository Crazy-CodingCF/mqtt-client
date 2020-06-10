/************************************************************************
  > File Name: MqttPackFormat.cpp
  > Author: caofei
  > Mail: caofei_lexue@163.com
  > Created Time: 2020年01月19日 星期日 13时42分48秒
 ***********************************************************************/
#include <glog/logging.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <cstdio>
#include <sstream>
#include <memory>
#include "GlogInit.h"
#include "MqttPackFormat.h"
#include "MqttConnectServer.h"
#include "MqttMessageMap.h"
#include "config.h"

/** get Json::Value from mqttconfig.json */
extern Json::Value g_function_config;
extern MqttConfigInfoType g_server_info;
extern CMqttService connect_service;
extern CMqttFormatParse format_parse;

CMqttPackFormat::CMqttPackFormat()
{
    _online = false;
}

CMqttPackFormat::~CMqttPackFormat()
{

}

/**
 * Gets the message from the FCS from the message queue
 *
 * @param payload_str  the message of fcs sent
 */
int CMqttPackFormat::GetFcsMessage(const char *payload_str)
{
    char function[100] = {0};
    JSONCPP_STRING errs;
    Json::Value payload;
    Json::CharReaderBuilder readerBuilder;

    unique_ptr<Json::CharReader> const jsonReader(readerBuilder.newCharReader());
    bool res = jsonReader->parse(payload_str, payload_str+strlen(payload_str), &payload, &errs);
    if(!res || !errs.empty()) {
        LOG(ERROR) << "parse Json err. : " << errs << endl;
        return SUCCESS;
    }

    Json::Value data = payload["data"];
    if(data["protocol"].asString() != "MQTT") {
        LOG(ERROR) << "FCS protocol is not part of MQTT";
        return SUCCESS; 
    } else if((data["protocol"].asString() == "MQTT") && (data["flexem_mqtt"].asString() == "abort")) {
        return ERROR;  //Abort MQTT operate
    }

    if( !g_function_config.isNull() ) {
        int num = g_function_config.size();
        int i = 0;

        for(i = 0; i < num; i++) {
            memset(function, 0, sizeof(function));
            sprintf(function, "function_%d", i+1);
            Json::Value root = g_function_config[function];

            Json::Value pub = root["pub"];
            if( !pub.isNull() ) {
                Json::Value func = pub["function"];
                if(func.asString() == payload["function"].asString()) {
                    CMqttPackFormat::ReorgnizeFcsMessage(root, payload);
                    continue;                                //'continue' : same function and different topic
                }
            }

            Json::Value pub_sub = root["pub_sub"];
            if( !pub_sub.isNull() ) {
                Json::Value func = pub_sub["function"];
                string fun = payload["function"].asString();

                fun = fun.substr(0, fun.find("Reply", 0));   //Solve the problem that the function name returned by FCS does not match
                if( func.asString() == fun ) {
                    CMqttPackFormat::ReorgnizeFcsMessage(root, payload);
                    continue;                                //'continue' : same function and different topic
                }
            }
        }
    } else {
        LOG(ERROR) << "g_function_config is null";
        return SUCCESS;
    }

    return SUCCESS;
}

/**
 * Process the data sent by FCS and process it logically in combination with the
 * format
 *
 * @param root the format of data
 * @param payload the content of data
 */
void CMqttPackFormat::ReorgnizeFcsMessage(const Json::Value &root, const Json::Value &payload)
{
    vector<FORMAT> storeFormat;
    vector<FORMAT> storePayload;
    vector<FORMAT> storePayloadErrors;
    vector<ITEM_FORMAT> storeObj;
    vector<ITEM_FORMAT> storeData;

    Json::Value pub = root["pub"];
    Json::Value pub_sub = root["pub_sub"];
    Json::Value topic;
    Json::Value format;

    if( !pub.isNull() ) {
        topic = pub["topic"];
        format = pub["format"];
    } else if( !pub_sub.isNull() ) {
        format_parse.pub_sub_status = "pub_sub";
        topic = pub_sub["topic"];
        Json::Value pub_sub_format = pub_sub["format"];
        format = pub_sub_format["pub_format"];
    } else {
        LOG(ERROR) << "pub or pub_sub is null";
        return;
    }

    format_parse.ParseJsonFormat(storeFormat, format);
#ifdef   LOG_PRINT
    cout << "------------------------------------------------" << endl;
#endif
    Json::Value items = payload["data"];
    format_parse.ParseJsonFormat(storePayload, items);
#ifdef   LOG_PRINT
    cout << "------------------------------------------------" << endl;
#endif
    format_parse.ParseJsonFormat(storePayloadErrors, items["items_errors"]);
#ifdef   LOG_PRINT
    cout << "------------------------------------------------" << endl;
#endif
    if(CMqttPackFormat::PackJsonFormat(storeObj, storeFormat, storePayloadErrors) == ERROR) return;
    if(CMqttPackFormat::MergeFcsFormatData(storeData, storeObj, storePayload, topic) == ERROR) return;
    if(CMqttPackFormat::PackServerMessage(storeData, topic, items) == ERROR) return;

    storeFormat.clear();
    storeFormat.shrink_to_fit();

    storePayload.clear();
    storePayload.shrink_to_fit();

    storeObj.clear();
    storeObj.shrink_to_fit();

    storeData.clear();
    storeData.shrink_to_fit();

    return;
}

/**
 * @brief  The main function of this function is Adjust the entry categories in the formatted data and restore to vector
 *
 * @param[in] storeObj  Where data is stored after recombination
 * @param[in] storeFormat vector of data to combine
 *
 * @return void
 */
int CMqttPackFormat::PackJsonFormat(vector<ITEM_FORMAT>& storeObj, const vector<FORMAT>& storeFormat, const vector<FORMAT> &storePayloadErrors)
{
    if( storeFormat.empty() ){
        LOG(ERROR) << "storeFormat is null";
        return ERROR;
    }

    for(vector<FORMAT>::const_iterator itr = storeFormat.cbegin(); itr != storeFormat.cend(); itr++) {
        ITEM_FORMAT temp;
        if (((itr+13) != storeFormat.cend()) && ((*itr).json_type == Json::objectValue) &&\
                ((*itr).json_size == 13) && ((*(itr+1)).json_name == "anonymous_name") &&\
                ((*(itr+2)).json_name == "bool_label_false") && ((*(itr+3)).json_name == "bool_label_true") &&\
                ((*(itr+4)).json_name == "combe_type") && ((*(itr+5)).json_name == "decimal_point") &&\
                ((*(itr+6)).json_name == "default_value") && ((*(itr+7)).json_name == "item_id") &&\
                ((*(itr+8)).json_name == "obj_type") && ((*(itr+9)).json_name == "relate_name") &&\
                ((*(itr+10)).json_name == "string_high_bit") && ((*(itr+11)).json_name == "string_number") &&\
                ((*(itr+12)).json_name == "string_type") && ((*(itr+13)).json_name == "value_type")) {
            temp.item_attri[USE_NAME].json_value.s_string = (*(itr+1)).json_value.s_string;
            temp.item_attri[BOOL_LABEL_FALSE].json_value.s_string = (*(itr+2)).json_value.s_string;
            temp.item_attri[BOOL_LABEL_TRUE].json_value.s_string = (*(itr+3)).json_value.s_string;
            temp.item_attri[COMBE_TYPE].json_value.s_string = (*(itr+4)).json_value.s_string;
            temp.item_attri[DECIMAL_POINT].json_value.i_int = (*(itr+5)).json_value.i_int;
            CMqttPackFormat::DefalutValueAssign(temp, (*(itr+13)).json_value.s_string, (*(itr+6)));
            temp.item_attri[ITEM_ID].json_value.s_string = (*(itr+7)).json_value.s_string;
            temp.item_attri[OBJECT_TYPE].json_value.s_string = (*(itr+8)).json_value.s_string;
            temp.item_attri[RELATE_NAME].json_value.s_string = (*(itr+9)).json_value.s_string;
            temp.item_attri[STRING_HIGH_BIT].json_value.b_bool = (*(itr+10)).json_value.b_bool;
            temp.item_attri[STRING_NUMBER].json_value.i_int = (*(itr+11)).json_value.i_int;
            temp.item_attri[STRING_TYPE].json_value.s_string = (*(itr+12)).json_value.s_string;
            temp.item_attri[VALUE].json_value.s_string = (*(itr+13)).json_value.s_string;
            itr = itr + 13;
        } else {
            temp.object.json_name = (*itr).json_name;
            temp.object.json_type = (*itr).json_type;
            temp.object.json_size = (*itr).json_size;
        }

        vector<FORMAT>::const_iterator itrPayEr;
        for(itrPayEr = storePayloadErrors.cbegin(); itrPayEr != storePayloadErrors.cend(); itrPayEr++) {
            if((*itrPayEr).json_type >= Json::arrayValue) {
                continue;
            }

            if((temp.item_attri[ITEM_ID].json_value.s_string == (*itrPayEr).json_name)) {
                temp.item_attri[VALUE].json_value.s_string = "string";  //Change the item of the exception to string
                break;
            }
        }

        storeObj.emplace_back(temp);
    }
#ifdef   LOG_PRINT
    cout << "------------------------------------------------" << endl;
    int num = 1;
    for(vector<ITEM_FORMAT>::iterator itr = storeObj.begin(); itr != storeObj.end(); itr++){
        if( (*itr).object.json_name.empty() ){
            printf("%d-use_name[%s],bool_label_false[%s],bool_label_true[%s],combe_type[%s],item_id[%s],object_type[%s],"
                    "relate_name[%s],string_high_bit[%d],string_number[%lld],string_type[%s],value_type[%s],"
                    "decimal_point[%lld]\n",\
                    num,\
                    (*itr).item_attri[USE_NAME].json_value.s_string.c_str(),\
                    (*itr).item_attri[BOOL_LABEL_FALSE].json_value.s_string.c_str(),\
                    (*itr).item_attri[BOOL_LABEL_TRUE].json_value.s_string.c_str(),\
                    (*itr).item_attri[COMBE_TYPE].json_value.s_string.c_str(),\
                    (*itr).item_attri[ITEM_ID].json_value.s_string.c_str(),\
                    (*itr).item_attri[OBJECT_TYPE].json_value.s_string.c_str(),\
                    (*itr).item_attri[RELATE_NAME].json_value.s_string.c_str(),\
                    (*itr).item_attri[STRING_HIGH_BIT].json_value.b_bool,\
                    (*itr).item_attri[STRING_NUMBER].json_value.i_int,\
                    (*itr).item_attri[STRING_TYPE].json_value.s_string.c_str(),\
                    (*itr).item_attri[VALUE].json_value.s_string.c_str(),\
                    (*itr).item_attri[DECIMAL_POINT].json_value.i_int);
        }else{
            printf("%d-name[%s],type[%d],size[%d]\n",num,\
                    (*itr).object.json_name.c_str(), (*itr).object.json_type, (*itr).object.json_size);
        }
        num++;
    }

    cout << "storeFormat size : " << storeFormat.size() << endl;
    cout << "storeFormat capacity : " << storeFormat.capacity() << endl;
    cout << "------------------------------------------------" << endl;
#endif

    return SUCCESS;
}

int CMqttPackFormat::DefalutValueAssign(ITEM_FORMAT &temp, string type, FORMAT value) 
{
    if(type == "integer") {
        temp.item_attri[DEFAULT_VALUE].json_value.i_int = value.json_value.i_int;
#ifdef   LOG_PRINT
        printf("name:%s, value: %lld\n", temp.item_attri[USE_NAME].json_value.s_string.c_str(),temp.item_attri[DEFAULT_VALUE].json_value.i_int);
#endif
    } else if(type == "long") {
        temp.item_attri[DEFAULT_VALUE].json_value.i_int = value.json_value.i_int;
#ifdef   LOG_PRINT
        printf("name:%s, value: %lld\n", temp.item_attri[USE_NAME].json_value.s_string.c_str(),temp.item_attri[DEFAULT_VALUE].json_value.i_int);
#endif
    } else if(type == "unsigned_integer") {
        temp.item_attri[DEFAULT_VALUE].json_value.u_uint = value.json_value.i_int; //jsoncpp not find unsigned_integer
#ifdef   LOG_PRINT
        printf("name:%s, value: %lu\n", temp.item_attri[USE_NAME].json_value.s_string.c_str(),temp.item_attri[DEFAULT_VALUE].json_value.u_uint);
#endif
    } else if((type == "float") || (type == "double")) {
        temp.item_attri[DEFAULT_VALUE].json_value.d_double = value.json_value.d_double;
#ifdef   LOG_PRINT
        printf("name:%s, value: %lf\n", temp.item_attri[USE_NAME].json_value.s_string.c_str(),temp.item_attri[DEFAULT_VALUE].json_value.d_double);
#endif
    } else if(type == "string") {
        temp.item_attri[DEFAULT_VALUE].json_value.s_string = value.json_value.s_string;
#ifdef   LOG_PRINT
        printf("name:%s, value: %s\n", temp.item_attri[USE_NAME].json_value.s_string.c_str(),temp.item_attri[DEFAULT_VALUE].json_value.s_string.c_str());
#endif
    } else if(type == "boolean") {
        temp.item_attri[DEFAULT_VALUE].json_value.b_bool = value.json_value.b_bool;
#ifdef   LOG_PRINT
        printf("name:%s, value: %d\n", temp.item_attri[USE_NAME].json_value.s_string.c_str(),temp.item_attri[DEFAULT_VALUE].json_value.b_bool);
#endif
    }

    return SUCCESS;
}


/**
 * Compare the same fields in the format and assign the corresponding values and
 * types
 *
 * Filter out the corresponding monitor name and value based on the
 * corresponding property item and assign to the new vector
 *
 * @param storeData the vector of store data
 * @param storeObj the vector of format data
 * @param storePayload the vector of content data
 */
int CMqttPackFormat::MergeFcsFormatData(vector<ITEM_FORMAT> &storeData, const vector<ITEM_FORMAT> &storeObj, const vector<FORMAT> &storePayload,const Json::Value &topic)
{
    if( storeObj.empty() || storePayload.empty() ) {
        LOG(ERROR) << "vector storeObj is null or storePayload is null";
        return ERROR;
    }

    for(vector<ITEM_FORMAT>::const_iterator itrObj = storeObj.cbegin(); itrObj != storeObj.cend();itrObj++) {
        if((*itrObj).object.json_name.empty()) {
            if((*itrObj).item_attri[OBJECT_TYPE].json_value.s_string == "item") {         //deal with items
                vector<FORMAT>::const_iterator itrPay;
                vector<FORMAT>::const_iterator itrPayEr;
                for(itrPay = storePayload.cbegin(); itrPay != storePayload.cend(); itrPay++) {
                    CMqttPackFormat::ModifyTheFloatPrecision(itrObj, (FORMAT&)*itrPay);
                    if(((*itrObj).item_attri[ITEM_ID].json_value.s_string == (*itrPay).json_name) &&\
                            (format_parse.JudgeDataType(itrObj, (FORMAT&)*itrPay) != ERROR)) {
                        ITEM_FORMAT temp;

                        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
                        temp.item_attri[VALUE].json_type = (*itrPay).json_type;
                        temp.item_attri[COMBE_TYPE].json_value.s_string = (*itrObj).item_attri[COMBE_TYPE].json_value.s_string;
                        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
                        temp.item_attri[STRING_HIGH_BIT].json_value.b_bool = (*itrObj).item_attri[STRING_HIGH_BIT].json_value.b_bool;
                        temp.item_attri[STRING_NUMBER].json_value.i_int = (*itrObj).item_attri[STRING_NUMBER].json_value.i_int;
                        temp.item_attri[STRING_TYPE].json_value.s_string = (*itrObj).item_attri[STRING_TYPE].json_value.s_string;
                        CMqttPackFormat::AssignFcsValueToVector(temp, itrPay);
                        storeData.emplace_back(temp);
                        break;
                    }
                }

                //Do not use non-existent monitoring points
                if(itrPay == storePayload.cend()) {
                    ITEM_FORMAT temp;
                    temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
                    temp.item_attri[VALUE].json_type = Json::nullValue;               //Represents a none-xistent point is a null type
                    temp.item_attri[COMBE_TYPE].json_value.s_string = (*itrObj).item_attri[COMBE_TYPE].json_value.s_string;
                    temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
                    storeData.emplace_back(temp);
                }

            } else if((*itrObj).item_attri[OBJECT_TYPE].json_value.s_string == "attri") {  //deal with attri
                vector<FORMAT>::const_iterator itrPay;
                for(itrPay = storePayload.cbegin(); itrPay != storePayload.cend(); itrPay++) {
                    if(((*itrObj).item_attri[RELATE_NAME].json_value.s_string == "flexem_picture_base64") &&\
                            ((*itrPay).json_name == "flexem_picture_path")) {
                        ITEM_FORMAT temp;
                        string base64_str;

                        CMqttPackFormat::GetPictureBase64(base64_str, (*itrPay).json_value.s_string);
                        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
                        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
                        temp.item_attri[VALUE].json_type = Json::stringValue;
                        temp.item_attri[VALUE].json_value.s_string = base64_str;
                        storeData.emplace_back(temp);
                        break;
                    } else if(((*itrObj).item_attri[RELATE_NAME].json_value.s_string == (*itrPay).json_name) &&\
                            (format_parse.JudgeDataType(itrObj, (FORMAT&)*itrPay) != ERROR)) {
                        ITEM_FORMAT temp;

                        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
                        temp.item_attri[VALUE].json_type = (*itrPay).json_type;
                        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
                        CMqttPackFormat::AssignFcsValueToVector(temp, itrPay);
                        storeData.emplace_back(temp);
                        break;
                    }
                }

                if(itrPay == storePayload.cend()) {
                    if(IsNewAttri(storeData, itrObj, storePayload, topic) == ERROR) {        //special attributes and need to add its value
                        LOG(ERROR) << "not find " << (*itrObj).item_attri[USE_NAME].json_value.s_string;
                        ITEM_FORMAT temp;

                        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
                        temp.item_attri[VALUE].json_type = Json::nullValue;               //Represents a none-xistent point is a null type
                        temp.item_attri[COMBE_TYPE].json_value.s_string = (*itrObj).item_attri[COMBE_TYPE].json_value.s_string;
                        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
                        storeData.emplace_back(temp);
                    }
                }
            } else if((*itrObj).item_attri[OBJECT_TYPE].json_value.s_string == "define_attri") {  //made self attri
                ITEM_FORMAT temp;

                temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
                temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
                if((*itrObj).item_attri[VALUE].json_value.s_string == "integer") {
                    temp.item_attri[VALUE].json_type = Json::intValue;
                    temp.item_attri[VALUE].json_value.i_int = (*itrObj).item_attri[DEFAULT_VALUE].json_value.i_int;
                } else if((*itrObj).item_attri[VALUE].json_value.s_string == "long") {
                    Json::Int64 Value = (*itrObj).item_attri[DEFAULT_VALUE].json_value.i_int;
                    temp.item_attri[VALUE].json_type = Json::intValue;
                    temp.item_attri[VALUE].json_value.i_int = Value;
                } else if((*itrObj).item_attri[VALUE].json_value.s_string == "unsigned_integer") {
                    temp.item_attri[VALUE].json_type = Json::uintValue;
                    temp.item_attri[VALUE].json_value.u_uint = (*itrObj).item_attri[DEFAULT_VALUE].json_value.u_uint;
                } else if(((*itrObj).item_attri[VALUE].json_value.s_string == "float")) {
                    temp.item_attri[VALUE].json_type = Json::realValue;
                    temp.item_attri[VALUE].json_value.d_double = (*itrObj).item_attri[DEFAULT_VALUE].json_value.d_double;
                } else if(((*itrObj).item_attri[VALUE].json_value.s_string == "double")) {
                    temp.item_attri[VALUE].json_type = Json::realValue;
                    temp.item_attri[VALUE].json_value.d_double = (*itrObj).item_attri[DEFAULT_VALUE].json_value.d_double;
                } else if((*itrObj).item_attri[VALUE].json_value.s_string== "string") {
                    temp.item_attri[VALUE].json_type = Json::stringValue;
                    temp.item_attri[VALUE].json_value.s_string = (*itrObj).item_attri[DEFAULT_VALUE].json_value.s_string;
                } else if((*itrObj).item_attri[VALUE].json_value.s_string == "boolean") {
                    temp.item_attri[VALUE].json_type = Json::booleanValue;
                    temp.item_attri[VALUE].json_value.b_bool = (*itrObj).item_attri[DEFAULT_VALUE].json_value.b_bool;
                }

                storeData.emplace_back(temp);
            }
        } else {
            //object
            ITEM_FORMAT temp;

            temp.object.json_name = (*itrObj).object.json_name;
            temp.object.json_type = (*itrObj).object.json_type;
            temp.object.json_size = (*itrObj).object.json_size;
            storeData.emplace_back(temp);
        }
    }
#ifdef LOG_PRINT
    int num = 1;
    for(vector<ITEM_FORMAT>::iterator itr = storeData.begin(); itr != storeData.end(); itr++) {
        uint32_t type;
        if((*itr).object.json_name.empty()) {
            //item_attri
            type = (*itr).item_attri[VALUE].json_type;
        } else {
            //object
            type = (*itr).object.json_type;
        }

        switch(type) 
        {
        case Json::intValue:
            cout << num <<  " name:" << (*itr).item_attri[USE_NAME].json_value.s_string << " type:" << (*itr).item_attri[VALUE].json_type << " value:" << (*itr).item_attri[VALUE].json_value.i_int << " item_attri:" << (*itr).item_attri[OBJECT_TYPE].json_value.s_string << " combe_type:" << (*itr).item_attri[COMBE_TYPE].json_value.s_string << endl;
            break;
        case Json::uintValue:
            cout << num <<  " name:" << (*itr).item_attri[USE_NAME].json_value.s_string << " type:" << (*itr).item_attri[VALUE].json_type << " value:" << (*itr).item_attri[VALUE].json_value.u_uint << " item_attri:" << (*itr).item_attri[OBJECT_TYPE].json_value.s_string << " combe_type:" << (*itr).item_attri[COMBE_TYPE].json_value.s_string << endl;
            break;
        case Json::realValue:
            cout << num <<  " name:" << (*itr).item_attri[USE_NAME].json_value.s_string << " type:" << (*itr).item_attri[VALUE].json_type << " value:" << (*itr).item_attri[VALUE].json_value.d_double << " item_attri:" << (*itr).item_attri[OBJECT_TYPE].json_value.s_string << " combe_type:" << (*itr).item_attri[COMBE_TYPE].json_value.s_string << endl;
            break;
        case Json::stringValue:
            cout << num <<  " name:" << (*itr).item_attri[USE_NAME].json_value.s_string << " type:" << (*itr).item_attri[VALUE].json_type << " value:" << (*itr).item_attri[VALUE].json_value.s_string << " item_attri:" << (*itr).item_attri[OBJECT_TYPE].json_value.s_string << " combe_type:" << (*itr).item_attri[COMBE_TYPE].json_value.s_string << endl;
            break;
        case Json::booleanValue:
            cout << num <<  " name:" << (*itr).item_attri[USE_NAME].json_value.s_string << " type:" << (*itr).item_attri[VALUE].json_type << " value:" << (*itr).item_attri[VALUE].json_value.b_bool << " item_attri:" << (*itr).item_attri[OBJECT_TYPE].json_value.s_string << " combe_type:" << (*itr).item_attri[COMBE_TYPE].json_value.s_string << endl;
            break;
        case Json::objectValue:
            cout << num <<  " name:" << (*itr).object.json_name << " type:" << (*itr).object.json_type << " size:" << (*itr).object.json_size << endl;
            break;
        case Json::arrayValue:
            cout << num <<  " name:" << (*itr).object.json_name << " type:" << (*itr).object.json_type << " size:" << (*itr).object.json_size << endl;
            break;
        case Json::nullValue:
            LOG(WARNING) << (*itr).item_attri[USE_NAME].json_value.s_string << " not exsit";
            break;
        default:
            LOG(ERROR) << "name :" << (*itr).item_attri[USE_NAME].json_value.s_string << " not find type : " << type;
            break;
        }
        num++;
    }
    cout << "------------------------------------------------" << endl;
#endif
} 

void CMqttPackFormat::ModifyTheFloatPrecision(const vector<ITEM_FORMAT>::const_iterator& itrObj, FORMAT &itrPay)
{
    double value;
    char str[10] = {0};
    char c_str[10] = "%0.";

    if(((*itrObj).item_attri[ITEM_ID].json_value.s_string == itrPay.json_name)) {
        if(((*itrObj).item_attri[VALUE].json_value.s_string == "float") || ((*itrObj).item_attri[VALUE].json_value.s_string == "double")) {
            int c = (int)(*itrObj).item_attri[DECIMAL_POINT].json_value.i_int;

            sprintf(c_str+3, "%d", c);
            if(c > 9) {
                sprintf(c_str+5, "f");
            } else {
                sprintf(c_str+4, "f");
            }

            sprintf(str, c_str, itrPay.json_value.d_double);
            sscanf(str, "%lf", &value);

            itrPay.json_value.d_double = value;
        }
    }
}

void CMqttPackFormat::AssignFcsValueToVector(ITEM_FORMAT &getData, vector<FORMAT>::const_iterator& postData)
{
    switch( (*postData).json_type )
    {
    case Json::intValue:
        getData.item_attri[VALUE].json_value.i_int = (*postData).json_value.i_int;
        break;
    case Json::uintValue:
        getData.item_attri[VALUE].json_value.u_uint = (*postData).json_value.u_uint;
        break;
    case Json::realValue:
        getData.item_attri[VALUE].json_value.d_double = (*postData).json_value.d_double;
        break;
    case Json::stringValue:
        getData.item_attri[VALUE].json_value.s_string = (*postData).json_value.s_string;
        break;
    case Json::booleanValue:
        getData.item_attri[VALUE].json_value.b_bool = (*postData).json_value.b_bool;
        break;
    default:
        LOG(ERROR) << "not find match type";
        break;
    }
}


/**
 * Add properties that FCS does not provide,including timestamp, picture
 * base64_str and so on;
 *
 * @param storeData the vector of store data
 * @param itrObj FORMAT information
 */

int CMqttPackFormat::IsNewAttri(vector<ITEM_FORMAT> &storeData, vector<ITEM_FORMAT>::const_iterator &itrObj, const vector<FORMAT> &storePayload, const Json::Value &topic)
{
    vector<FORMAT>::const_iterator itrPay;
    if((*itrObj).item_attri[RELATE_NAME].json_value.s_string == "flexem_timestamp") {
        ITEM_FORMAT temp;
        string time;

        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
        if(((*itrObj).item_attri[VALUE].json_value.s_string == "string")) {
            CMqttPackFormat::GetTimeStamp(time);
            temp.item_attri[VALUE].json_type = Json::stringValue;
            temp.item_attri[VALUE].json_value.s_string = time;
        } else {
            temp.item_attri[VALUE].json_type = Json::intValue;
            temp.item_attri[VALUE].json_value.i_int = CMqttPackFormat::GetTimeStamp(time);
        }

        storeData.emplace_back(temp);
        return SUCCESS;
    } else if((*itrObj).item_attri[RELATE_NAME].json_value.s_string == "flexem_message") {
        ITEM_FORMAT temp;
        CMqttMessageMap MessageMap;

        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
        if(((*itrObj).item_attri[VALUE].json_value.s_string == "string")) {
            temp.item_attri[VALUE].json_type = Json::stringValue;
            for(itrPay = storePayload.cbegin(); itrPay != storePayload.cend(); itrPay++) {
                if(((*itrPay).json_name == "flexem_error_code")) {
                    temp.item_attri[VALUE].json_value.s_string = MessageMap.GetMessageMap((*itrPay).json_value.i_int);
                    break;
                }
            }

            if(itrPay == storePayload.cend()) {
                LOG(ERROR) << "Not find flexem_error_code !!!!";
                temp.item_attri[VALUE].json_value.s_string = MessageMap.GetMessageMap(100);
            }
        }

        storeData.emplace_back(temp);
        return SUCCESS;
    } else if((*itrObj).item_attri[RELATE_NAME].json_value.s_string == "gy_deviceId") {
        ITEM_FORMAT temp;

        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
        if(((*itrObj).item_attri[VALUE].json_value.s_string == "string")) {
            temp.item_attri[VALUE].json_type = Json::stringValue;
            temp.item_attri[VALUE].json_value.s_string = GetDeviceId(topic.asString());
        }

        storeData.emplace_back(temp);
        return SUCCESS;
    } else if((*itrObj).item_attri[RELATE_NAME].json_value.s_string == "gy_error_code") {
        ITEM_FORMAT temp;

        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
        if(((*itrObj).item_attri[VALUE].json_value.s_string == "string")) {
            temp.item_attri[VALUE].json_type = Json::stringValue;
            for(itrPay = storePayload.cbegin(); itrPay != storePayload.cend(); itrPay++) {
                if(((*itrPay).json_name == "flexem_error_code") && ((*itrPay).json_value.i_int == 200)) {
                    temp.item_attri[VALUE].json_value.s_string = "0";
                    break;
                } else if(((*itrPay).json_name == "flexem_error_code") && ((*itrPay).json_value.i_int != 200)) {
                    LOG(ERROR) << "flexem_error_code value not 200";
                    temp.item_attri[VALUE].json_value.s_string = "-1";
                    break;
                }
            }

            if(itrPay == storePayload.cend()) {
                LOG(ERROR) << "Not find flexem_error_code !!!!";
                temp.item_attri[VALUE].json_value.s_string = "-1";
            }
        }

        storeData.emplace_back(temp);
        return SUCCESS;
    } else if((*itrObj).item_attri[RELATE_NAME].json_value.s_string == "gy_uuid") {
        ITEM_FORMAT temp;

        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
        if(((*itrObj).item_attri[VALUE].json_value.s_string == "long")) {
            temp.item_attri[VALUE].json_type = Json::intValue;
            temp.item_attri[VALUE].json_value.i_int = format_parse.gy_mqtt_uuid;
        }

        storeData.emplace_back(temp);
        return SUCCESS;
    } else if((*itrObj).item_attri[RELATE_NAME].json_value.s_string == "ali_method") {
        ITEM_FORMAT temp;

        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
        if(((*itrObj).item_attri[VALUE].json_value.s_string == "string")) {
            temp.item_attri[VALUE].json_type = Json::stringValue;
            temp.item_attri[VALUE].json_value.s_string = GetAliMethod(topic.asString());
        }

        storeData.emplace_back(temp);
        return SUCCESS;
    } else if((*itrObj).item_attri[RELATE_NAME].json_value.s_string == "ali_protocol_version") {
        ITEM_FORMAT temp;

        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
        if(((*itrObj).item_attri[VALUE].json_value.s_string == "string")) {
            temp.item_attri[VALUE].json_type = Json::stringValue;
            temp.item_attri[VALUE].json_value.s_string = "1.0";
        }

        storeData.emplace_back(temp);
        return SUCCESS;
    } else if((*itrObj).item_attri[RELATE_NAME].json_value.s_string == "flexem_mqtt_ver") {
        ITEM_FORMAT temp;

        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
        if(((*itrObj).item_attri[VALUE].json_value.s_string == "string")) {
            temp.item_attri[VALUE].json_type = Json::stringValue;
            string MAJOR = to_string(VERSION_MAJOR);
            string MINOR = to_string(VERSION_MINOR);
            string RELEASE = to_string(VERSION_RELEASE);
            string mqtt_ver = MAJOR+"."+MINOR+"."+RELEASE;
            temp.item_attri[VALUE].json_value.s_string = mqtt_ver;
        } else if(((*itrObj).item_attri[VALUE].json_value.s_string == "unsigned_integer")) {
            temp.item_attri[VALUE].json_type = Json::uintValue;
            temp.item_attri[VALUE].json_value.u_uint = VERSION_MAJOR*1000+VERSION_MINOR*100+VERSION_RELEASE;
        }

        storeData.emplace_back(temp);
        return SUCCESS;
    } else if((*itrObj).item_attri[RELATE_NAME].json_value.s_string == "flexem_message_id") {
        ITEM_FORMAT temp;
        string time;

        temp.item_attri[USE_NAME].json_value.s_string = (*itrObj).item_attri[USE_NAME].json_value.s_string;
        temp.item_attri[OBJECT_TYPE].json_value.s_string = (*itrObj).item_attri[OBJECT_TYPE].json_value.s_string;
        if(((*itrObj).item_attri[VALUE].json_value.s_string == "long")) {
            temp.item_attri[VALUE].json_type = Json::intValue;
            temp.item_attri[VALUE].json_value.i_int = CMqttPackFormat::GetTimeStamp(time);
        } else if(((*itrObj).item_attri[VALUE].json_value.s_string == "string")) {
            temp.item_attri[VALUE].json_type = Json::stringValue;
            temp.item_attri[VALUE].json_value.s_string = to_string(CMqttPackFormat::GetTimeStamp(time));
        }

        storeData.emplace_back(temp);
        return SUCCESS;
    }

    return ERROR;
}

long CMqttPackFormat::GetTimeStamp(string &timestamp)
{
    char ctime[30] = {0};     //year month day, hour min sec
    time_t nowTime;           //timestamp
    struct tm *timeNow;
    struct timeval tv;

    time(&nowTime);
    timeNow = localtime(&nowTime);
    gettimeofday(&tv, NULL);
    memset(ctime, 0, sizeof(ctime));
    sprintf(ctime, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", timeNow->tm_year + 1900,\
            timeNow->tm_mon + 1, timeNow->tm_mday, timeNow->tm_hour, timeNow->tm_min,\
            timeNow->tm_sec, (int)(tv.tv_usec / 1000));

    timestamp = ctime;

    return nowTime;
}

/**
 * Package server data
 *
 * @param storeData the vector of store data
 * @param topic the publish topic
 */
int CMqttPackFormat::PackServerMessage(const vector<ITEM_FORMAT> &storeData, const Json::Value &topic, const Json::Value &items) 
{
    string pub_topic;

    if(topic.isNull()) {
        LOG(ERROR) << "topic is NULL";
        return ERROR;
    } else {
        pub_topic = AdjustiTopicInfo(topic.asString(), items);
    }

    if(storeData.empty()) {
        LOG(ERROR) << "storeData is NULL";
        return ERROR;
    }

    Json::Value root;
    Json::StreamWriterBuilder writerBuilder;
    ostringstream os;

    vector<ITEM_FORMAT>::const_iterator itr_begin = storeData.cbegin();
    if(((*itr_begin).object.json_name != "root") || ((*itr_begin).object.json_type != Json::objectValue)) {
        LOG(ERROR) << "The vector of data is not right, beacause of first member is not 'root'";
        return ERROR;
    }

    vector<ITEM_FORMAT>::const_iterator itr_end = storeData.cend();
    uint32_t num = (*itr_begin).object.json_size;
    itr_begin++;
    for(uint32_t i = 0; i < num; i++) {
        if(CMqttPackFormat::PackServerMessageRecursion(root, itr_begin, itr_end, false) == ERROR) {
            LOG(ERROR) << "vector message error";
            return ERROR;
        }
    }

    if(root.empty()) {
        LOG(WARNING) << "Payload is null !!!";
        return ERROR;
    }

    writerBuilder.settings_["indentation"] = "";
    writerBuilder.settings_["precision"] = 16;
    writerBuilder.settings_["emitUTF8"] = true;

    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());
    jsonWriter->write(root, &os);
    string out = os.str();
    connect_service.PublishMessage(out, (char *)pub_topic.c_str(), _online);
}

int CMqttPackFormat::PackServerMessageRecursion(Json::Value &root, vector<ITEM_FORMAT>::const_iterator &itr_begin, vector<ITEM_FORMAT>::const_iterator &itr_end, bool flag)
{
    if(itr_begin == itr_end) {
        LOG(ERROR) << "iterator is end";
        return ERROR;
    }

    if(!(*itr_begin).object.json_name.empty())
    {
        if((*itr_begin).object.json_type == Json::objectValue) {
            //object
            uint32_t num = (*itr_begin).object.json_size;
            string name = (*itr_begin).object.json_name;
            Json::Value temp;
            itr_begin++;

            for(uint32_t i = 0; i < num; i++) {
                CMqttPackFormat::PackServerMessageRecursion(temp, itr_begin, itr_end, false);
            }

            if(flag == true) {
                root = temp;
            } else {
                if(!temp.empty()) root[name] = temp;
            }
        } else if((*itr_begin).object.json_type == Json::arrayValue) {
            //array
            uint32_t num = (*itr_begin).object.json_size;
            string name = (*itr_begin).object.json_name;
            Json::Value temp;
            Json::Value temp1;
            int temp_flag = 0;
            itr_begin++;

            if(itr_begin != itr_end) {
                if((*itr_begin).object.json_name.empty()) {
                    if((*itr_begin).item_attri[COMBE_TYPE].json_value.s_string == "name_value") {
                        temp_flag = 3; //item will get into object to combine
                    } else if((*itr_begin).item_attri[COMBE_TYPE].json_value.s_string == "key_value") {
                        temp_flag = 2;  //next is items
                    } else if((*itr_begin).item_attri[COMBE_TYPE].json_value.s_string == "bao_xing") {
                        temp_flag = 4;  //special format
                    }
                } else {
                    temp_flag = 1; //next is object
                }
            }

            for(uint32_t i = 0; i < num; i++) {
                if((temp_flag == 1) || (temp_flag == 3)) {
                    //combine items form same array
                    CMqttPackFormat::PackServerMessageRecursion(temp1, itr_begin, itr_end, true);
                    if(!temp1.empty()) temp.append(temp1); //must into 'if' otherwise the data will be lost
                } else if(temp_flag == 4) {
                    Json::Value temp1;  //single item format complete;
                    CMqttPackFormat::PackServerMessageRecursion(temp1, itr_begin, itr_end, true);
                    if(!temp1.empty()) temp.append(temp1);
                } else {
                    CMqttPackFormat::PackServerMessageRecursion(temp1, itr_begin, itr_end, true);
                }
            }

            if(!temp1.empty() && (temp_flag == 2)) {
                temp.append(temp1);  //temp1 need include any items
            }

            if(flag == true ) {
                root = temp;
            } else {
                if(!temp.empty()) root[name] = temp;
            }
        }
    } else {
        //item_attri
        CMqttPackFormat::AssignVectorValueToMessage(root, itr_begin);
        itr_begin++;
    }

    return SUCCESS;
}

string CMqttPackFormat::CheckStringType(vector<ITEM_FORMAT>::const_iterator itr)
{
    string value_str;
    if((*itr).item_attri[STRING_TYPE].json_value.s_string == "ASCII") {
        if((*itr).item_attri[STRING_HIGH_BIT].json_value.b_bool == true) {               //Need Requires high-low byte conversion
            int file_size = (*itr).item_attri[STRING_NUMBER].json_value.i_int;
            int data_size = strlen((*itr).item_attri[VALUE].json_value.s_string.c_str());
            if(data_size <= 0) {
                return "";
            } else if(data_size > file_size) {
                LOG(ERROR) << "ERROR : data size too long !!";
                data_size = file_size;
            }
#ifdef LOG_PRINT
            cout << "ascii-1 : " << (*itr).item_attri[VALUE].json_value.s_string << endl;
            cout << "file_size : " << file_size << endl;
            cout << "data_size : " << data_size << endl;
#endif

            char *my_str = new char[data_size+1];
            memcpy(my_str, (*itr).item_attri[VALUE].json_value.s_string.c_str(), data_size);
#ifdef LOG_PRINT
            for(int i = 0; i < data_size; i++) {
                printf("0x%x  ",my_str[i]);
            }
            cout << endl;
#endif

            int data_num;
            if(data_size%2 != 0) {
                data_num = data_size - 1;
            } else {
                data_num = data_size;
            }

            for(int i = 0; i < data_num; ) {
                char temp = my_str[i];
                my_str[i] = my_str[i+1]; 
                my_str[i+1] = temp;
                i = i + 2;
            }
            my_str[data_size] = '\0';

#ifdef LOG_PRINT
            for(uint32_t i = 0; i < strlen(my_str); i++) {
                printf("0x%x  ",my_str[i]);
            }
            cout << endl;
#endif
            value_str = my_str;
            delete []my_str;
        } else if((*itr).item_attri[STRING_HIGH_BIT].json_value.b_bool == false) {       //Dot not operate
            int file_size = (*itr).item_attri[STRING_NUMBER].json_value.i_int;
            int data_size = strlen((*itr).item_attri[VALUE].json_value.s_string.c_str());
            if(data_size > file_size) {
                LOG(ERROR) << "ERROR : data size too long !!";
                data_size = file_size;
            }

#ifdef LOG_PRINT
            cout << "ascii-0 : " << (*itr).item_attri[VALUE].json_value.s_string << endl;
            cout << "file_size : " << file_size << endl;
            cout << "data_size : " << data_size << endl;

            char *my_str = new char[data_size];
            memcpy(my_str, (*itr).item_attri[VALUE].json_value.s_string.c_str(), data_size);

            for(int i = 0; i < data_size; i++) {
                printf("0x%x  ",my_str[i]);
            }
            printf("\n");
            delete []my_str;
#endif
            value_str = (*itr).item_attri[VALUE].json_value.s_string.c_str();
        }
    } else if((*itr).item_attri[STRING_TYPE].json_value.s_string == "Unicode") {
        int file_size = (*itr).item_attri[STRING_NUMBER].json_value.i_int;
        int data_size = strlen((*itr).item_attri[VALUE].json_value.s_string.c_str());
        if(data_size > file_size*2) {
            LOG(ERROR) << "ERROR : data size too long !!";
            data_size = file_size;
        }

#ifdef LOG_PRINT
        cout << "Unicode : " << (*itr).item_attri[VALUE].json_value.s_string << endl;
        cout << "file_size : " << file_size*2 << endl;
        cout << "data_size : " << data_size << endl;
#endif

        char *my_str = new char[file_size*2];
        char *str = new char[file_size*2];
        memcpy(my_str, (*itr).item_attri[VALUE].json_value.s_string.c_str(), file_size*2);
        int j = 0;
        for(int i = 0; i < file_size*2 && j <= i; i++) {
            if(my_str[i] != '\0') {
                str[j] = my_str[i];
                j++;
            }
        }
        str[j] = '\0';
        delete []my_str;

#ifdef LOG_PRINT
        cout << str << endl;
        for(uint32_t i = 0; i < strlen(str); i++) {
            printf("0x%x  ", str[i]);
        }
        printf("\n");
#endif
        value_str = str;
        delete []str;
    } else {
        value_str = (*itr).item_attri[VALUE].json_value.s_string;
    }

    return value_str;
}

void CMqttPackFormat::AssignVectorValueToMessage(Json::Value &root, vector<ITEM_FORMAT>::const_iterator &itr)
{
    vector<ITEM_FORMAT>::const_iterator temp = itr;

    //Locate the current object's data type to define the object format
    while(1) {
        if(!(*temp).object.json_name.empty() && ((*temp).object.json_type >= 6)) {
            break;
        } else {
            temp--;
        }
    }

    switch((*itr).item_attri[VALUE].json_type)
    {
    case Json::intValue:
        if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "name_value") {
            Json::Int64 value = (*itr).item_attri[VALUE].json_value.i_int;
            if((*temp).object.json_type == Json::objectValue) {   //special format to orgnizise
                Json::Value temp;
                temp["value"] = value;
                root[(*itr).item_attri[USE_NAME].json_value.s_string] = temp;
            } else {
                root["name"] = (*itr).item_attri[USE_NAME].json_value.s_string;
                root["value"] = value;
            }
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "key_value") {
            Json::Int64 value = (*itr).item_attri[VALUE].json_value.i_int;
            root[(*itr).item_attri[USE_NAME].json_value.s_string] = value;
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "onenet") {
            Json::Int64 value = (*itr).item_attri[VALUE].json_value.i_int;
            Json::Value v;
            v["v"] = value;
            root[(*itr).item_attri[USE_NAME].json_value.s_string].append(v);
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "bao_xing") {
            Json::Int64 value = (*itr).item_attri[VALUE].json_value.i_int;
            root.append((*itr).item_attri[USE_NAME].json_value.s_string);
            root.append(value);
            root.append(123456677);
            root.append(0);
        } else {
            Json::Int64 value = (*itr).item_attri[VALUE].json_value.i_int;
            root[(*itr).item_attri[USE_NAME].json_value.s_string] = value;
        }
        break;
    case Json::uintValue:
        if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "name_value") {
            Json::UInt64 value = (*itr).item_attri[VALUE].json_value.u_uint;
            if((*temp).object.json_type == Json::objectValue) {
                Json::Value temp;
                temp["value"] = value;
                root[(*itr).item_attri[USE_NAME].json_value.s_string] = temp;
            } else {
                root["name"] = (*itr).item_attri[USE_NAME].json_value.s_string;
                root["value"] = value;
            }
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "key_value") {
            Json::UInt64 value = (*itr).item_attri[VALUE].json_value.u_uint;
            root[(*itr).item_attri[USE_NAME].json_value.s_string] = value;
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "onenet") {
            Json::UInt64 value = (*itr).item_attri[VALUE].json_value.u_uint;
            Json::Value v;
            v["v"] = value;
            root[(*itr).item_attri[USE_NAME].json_value.s_string].append(v);
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "bao_xing") {
            Json::UInt64 value = (*itr).item_attri[VALUE].json_value.u_uint;
            root.append((*itr).item_attri[USE_NAME].json_value.s_string);
            root.append(value);
            root.append(123456677);
            root.append(0);
        } else {
            Json::UInt64 value = (*itr).item_attri[VALUE].json_value.u_uint;
            root[(*itr).item_attri[USE_NAME].json_value.s_string] = value;
        }
        break;
    case Json::realValue:
        if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "name_value") {
            if((*temp).object.json_type == Json::objectValue) {
                Json::Value temp;
                temp["value"] = (*itr).item_attri[VALUE].json_value.d_double;
                root[(*itr).item_attri[USE_NAME].json_value.s_string] = temp;
            } else {
                root["name"] = (*itr).item_attri[USE_NAME].json_value.s_string;
                root["value"] = (*itr).item_attri[VALUE].json_value.d_double;
            }
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "key_value") {
            root[(*itr).item_attri[USE_NAME].json_value.s_string] = (*itr).item_attri[VALUE].json_value.d_double;
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "onenet") {
            Json::Value v;
            v["v"] = (*itr).item_attri[VALUE].json_value.d_double;
            root[(*itr).item_attri[USE_NAME].json_value.s_string].append(v);
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "bao_xing") {
            root.append((*itr).item_attri[USE_NAME].json_value.s_string);
            root.append((*itr).item_attri[VALUE].json_value.d_double);
            root.append(123456677);
            root.append(0);
        } else {
            root[(*itr).item_attri[USE_NAME].json_value.s_string] = (*itr).item_attri[VALUE].json_value.d_double;
        }
        break;
    case Json::stringValue:
        if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "name_value") {
            if((*temp).object.json_type == Json::objectValue) {
                Json::Value temp;
                temp["value"] = CMqttPackFormat::CheckStringType(itr);
                root[(*itr).item_attri[USE_NAME].json_value.s_string] = temp;
            } else {
                root["name"] = (*itr).item_attri[USE_NAME].json_value.s_string;
                root["value"] = CMqttPackFormat::CheckStringType(itr);
            }
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "key_value") {
            root[(*itr).item_attri[USE_NAME].json_value.s_string] = CMqttPackFormat::CheckStringType(itr);
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "onenet") {
            Json::Value v;
            v["v"] = CMqttPackFormat::CheckStringType(itr);
            root[(*itr).item_attri[USE_NAME].json_value.s_string].append(v);
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "bao_xing") {
            root.append((*itr).item_attri[USE_NAME].json_value.s_string);
            root.append(CMqttPackFormat::CheckStringType(itr));
            root.append(123456677);
            root.append(0);
        } else {
            root[(*itr).item_attri[USE_NAME].json_value.s_string] = CMqttPackFormat::CheckStringType(itr);
        }
        break;
    case Json::booleanValue:
        if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "name_value") {
            if((*temp).object.json_type == Json::objectValue) {
                Json::Value temp;
                temp["value"] = (*itr).item_attri[VALUE].json_value.b_bool;
                root[(*itr).item_attri[USE_NAME].json_value.s_string] = temp;
            } else {
                root["name"] = (*itr).item_attri[USE_NAME].json_value.s_string;
                root["value"] = (*itr).item_attri[VALUE].json_value.b_bool;
            }
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "key_value") {
            root[(*itr).item_attri[USE_NAME].json_value.s_string] = (*itr).item_attri[VALUE].json_value.b_bool;
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "onenet") {
            Json::Value v;
            v["v"] = (*itr).item_attri[VALUE].json_value.b_bool;
            root[(*itr).item_attri[USE_NAME].json_value.s_string].append(v);
        } else if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "bao_xing") {
            root.append((*itr).item_attri[USE_NAME].json_value.s_string);
            root.append((*itr).item_attri[VALUE].json_value.b_bool);
            root.append(123456677);
            root.append(0);
        } else {
            root[(*itr).item_attri[USE_NAME].json_value.s_string] = (*itr).item_attri[VALUE].json_value.b_bool;
        }
        break;
    case Json::nullValue:
#if 0
        if((*itr).item_attri[COMBE_TYPE].json_value.s_string == "name_value") {
            if((*temp).object.json_type == Json::objectValue) {
                Json::Value temp;
                temp["value"] = Json::nullValue;
                root[(*itr).item_attri[USE_NAME].json_value.s_string] = temp;
            } else {
                root["name"] = (*itr).item_attri[USE_NAME].json_value.s_string;
                root["value"] = Json::nullValue;
            }
        } else {
            root[(*itr).item_attri[USE_NAME].json_value.s_string] = (*itr).item_attri[VALUE].json_value.b_bool;
        }
#else
        LOG(WARNING) << (*itr).item_attri[USE_NAME].json_value.s_string << " is not existent";
#endif
        break;
    default:
        LOG(ERROR) << "not find match type";
        break;
    }
}

int CMqttPackFormat::GetPictureBase64(string &base64_str, string base64_picture_path)
{
    if(base64_picture_path.empty()) {
        LOG(ERROR) << "error! : Don't have picture path";
        return ERROR;
    }

    LOG(INFO) << "base64_picture_path : " << base64_picture_path;
    PictureToBase64 picture_to_base64;
    base64_str = picture_to_base64.GetPictureBase64(base64_picture_path);

    return SUCCESS;
}

string CMqttPackFormat::GetDeviceId(string topic)
{
    string gwId;
    string slaveId;
    string equipCode;
    string deviceId;

    try {
        equipCode = topic.substr(topic.rfind("/")+1);
        topic = topic.erase(topic.rfind("/"));
        slaveId = topic.substr(topic.rfind("/")+1);
        topic = topic.erase(topic.rfind("/"));
        gwId = topic.substr(topic.rfind("/")+1);
    } catch(exception &e) {
        LOG(ERROR) << "error : " << e.what();
    }
    deviceId = gwId+"_"+slaveId+"_"+equipCode;

    return deviceId;
}

string CMqttPackFormat::GetAliMethod(string topic)
{
    string site_one;
    string site_two;
    string site_three;
    string site_four;
    string method;

    try {
        site_four = topic.substr(topic.rfind("/")+1);
        topic = topic.erase(topic.rfind("/"));
        site_three = topic.substr(topic.rfind("/")+1);
        topic = topic.erase(topic.rfind("/"));
        site_two = topic.substr(topic.rfind("/")+1);
        topic = topic.erase(topic.rfind("/"));
        site_one = topic.substr(topic.rfind("/")+1);
    } catch(exception &e) {
        LOG(ERROR) << "error : " << e.what();
    }

    if(site_one == "thing") {
        method = site_one+"."+site_two+"."+site_three+"."+site_four;
    } else {
        method = site_two+"."+site_three+"."+site_four;
    }

    return method;
}

string CMqttPackFormat::AdjustiTopicInfo(string topic, const Json::Value &items)
{
    string request_id = "{request_id}";
    string cmdid = "{cmdid}";
    string::size_type idx = string::npos;
    string get_request_id;
    string pub_topic;

    idx = topic.find(request_id);
    if (idx != string::npos) {
        get_request_id = items["flexem_message_id"].asString();
        pub_topic = topic.erase(topic.rfind("/"));
        pub_topic = pub_topic + "/request_id=" + get_request_id;
    } else {
        idx = topic.find(cmdid);
        if(idx != string::npos) {
            get_request_id = items["flexem_message_id"].asString();
            pub_topic = topic.erase(topic.rfind("/"));
            pub_topic = pub_topic + "/" + get_request_id;
        } else {
            return topic;
        }
    }

    return pub_topic;
}
