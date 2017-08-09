#ifndef __TC_JSON_H
#define __TC_JSON_H

#include <string>
#include <map>
#include <vector>
#include <assert.h>
#include <stdio.h>

#include "tc_autoptr.h"

using namespace std;

namespace taf
{
/////////////////////////////////////////////////
// ˵��: json�����Ĺ�����
// Author : zhangcunli@tencent.com
/////////////////////////////////////////////////

/**
* ������׳����쳣
*/
struct TC_Json_Exception : public TC_Exception
{
    TC_Json_Exception(const string &buffer) : TC_Exception(buffer){};
    TC_Json_Exception(const string &buffer, int err) : TC_Exception(buffer, err){};
    ~TC_Json_Exception() throw(){};
};

enum eJsonType
{
    eJsonTypeString,
    eJsonTypeNum,
    eJsonTypeObj,
    eJsonTypeArray,
    eJsonTypeBoolean
};

/*
 * json���͵Ļ��ࡣû���κ�����
 */
class JsonValue : public taf::TC_HandleBase
{
public:
    virtual eJsonType getType()=0;
    virtual ~JsonValue(){}
};
typedef taf::TC_AutoPtr<JsonValue> JsonValuePtr;

/*
 * json���� string���� ����"dd\ndfd"
 */
class JsonValueString : public JsonValue
{
public:
    JsonValueString(const string & s):value(s)
    {
    }
    JsonValueString(){}

    eJsonType getType()
    {
        return eJsonTypeString;
    }
    virtual ~JsonValueString(){}
    string value;
};
typedef taf::TC_AutoPtr<JsonValueString> JsonValueStringPtr;

/*
 * json���� number���� ���� 1.5e8
 */
class JsonValueNum : public JsonValue
{
public:
    JsonValueNum(double d,bool b=false):value(d),isInt(b)
    {
    }
    JsonValueNum()
    {
        isInt=false;
		value=0.0f;
    }
    eJsonType getType()
    {
        return eJsonTypeNum;
    }
    virtual ~JsonValueNum(){}
public:
    double value;
    bool isInt;
};
typedef taf::TC_AutoPtr<JsonValueNum> JsonValueNumPtr;

/*
 * json���� object���� ���� {"aa","bb"}
 */
class JsonValueObj: public JsonValue
{
public:
    eJsonType getType()
    {
        return eJsonTypeObj;
    }
    virtual ~JsonValueObj(){}
public:
    map<string,JsonValuePtr> value;
};
typedef taf::TC_AutoPtr<JsonValueObj> JsonValueObjPtr;

/*
 * json���� array���� ���� ["aa","bb"]
 */
class JsonValueArray: public JsonValue
{
public:
    eJsonType getType()
    {
        return eJsonTypeArray;
    }
    void push_back(JsonValuePtr & p)
    {
        value.push_back(p);
    }
    virtual ~JsonValueArray(){}
public:
    vector<JsonValuePtr> value;
};
typedef taf::TC_AutoPtr<JsonValueArray> JsonValueArrayPtr;

/*
 * json���� boolean���� ���� true
 */
class JsonValueBoolean : public JsonValue
{
public:
    eJsonType getType()
    {
        return eJsonTypeBoolean;
    }
    bool getVaule()
    {
        return value;
    }
    virtual ~JsonValueBoolean(){}
public:
    bool value;
};
typedef taf::TC_AutoPtr<JsonValueBoolean> JsonValueBooleanPtr;

/*
 * ����json�ַ����õ��� ���ַ�����
 */
class BufferJsonReader
{
    const char *        _buf;		///< ������
    size_t              _buf_len;	///< ����������
    size_t              _cur;		///< ��ǰλ��

public:

    BufferJsonReader () :_buf(NULL),_buf_len(0), _cur(0) {}

    void reset() { _cur = 0;}

    void setBuffer(const char * buf, size_t len)
    {
        _buf = buf;
        _buf_len = len;
        _cur = 0;
    }

    void setBuffer(const std::vector<char> &buf)
    {
        _buf = &buf[0];
        _buf_len = buf.size();
        _cur = 0;
    }

    size_t getCur()
    {
        return _cur;
    }

    const char * getPoint()
    {
        return _buf+_cur;
    }

    char read()
    {
        check();
        _cur ++;
        return *(_buf+_cur-1);
    }

    char get()
    {
        check();
        return *(_buf+_cur);
    }

    char getBack()
    {
        assert(_cur>0);
        return *(_buf+_cur-1);
    }

    void back()
    {
        assert(_cur>0);
        _cur--;
    }

    void check()
    {
        if (_cur + 1 > _buf_len)
        {
            char s[64];
            snprintf(s, sizeof(s), "buffer overflow when peekBuf, over %u.", (uint32_t)_buf_len);
            throw TC_Json_Exception(s);
        }
    }

    bool hasEnd()
    {
        return _cur >= _buf_len;
    }
};

/*
 * ����json���ࡣ����static
 */
class TC_Json
{
public:
    //json���͵��ַ�����ת��
    static string writeValue(const JsonValuePtr & p);
    //json�ַ�����json�ṹ��ת��
    static JsonValuePtr getValue(const string & str);
private:
    //string ���͵�josn�ַ���
    static string writeString(const JsonValueStringPtr & p);
    static string writeString(const string & s);
    //num ���͵�josn�ַ���
    static string writeNum(const JsonValueNumPtr & p);
    //obj ���͵�josn�ַ���
    static string writeObj(const JsonValueObjPtr & p);
    //array ���͵�josn�ַ���
    static string writeArray(const JsonValueArrayPtr & p);
    //boolean ���͵�josn�ַ���
    static string writeBoolean(const JsonValueBooleanPtr & p);

    //��ȡjson��value���� Ҳ�������е�json���� ��������Ϲ淶�����쳣
    static JsonValuePtr getValue(BufferJsonReader & reader);
    //��ȡjson��object ��������Ϲ淶�����쳣
    static JsonValueObjPtr getObj(BufferJsonReader & reader);
    //��ȡjson��array(����) ��������Ϲ淶�����쳣
    static JsonValueArrayPtr getArray(BufferJsonReader & reader);
    //��ȡjson��string �� "dfdf" ��������Ϲ淶�����쳣
    static JsonValueStringPtr getString(BufferJsonReader & reader,char head='\"');
    //��ȡjson������ �� -213.56 ��������Ϲ淶�����쳣
    static JsonValueNumPtr getNum(BufferJsonReader & reader,char head);
    //��ȡjson��booleanֵ  �� true false ��������Ϲ淶�����쳣
    static JsonValueBooleanPtr getBoolean(BufferJsonReader & reader,char c);
    //��ȡjson�� null ��������Ϲ淶�����쳣
    static JsonValuePtr getNull(BufferJsonReader & reader,char c);
    //��ȡ16������ʽ��ֵ ��\u003f ��������Ϲ淶�����쳣
    static uint16_t getHex(BufferJsonReader & reader);
    //�ж�һ���ַ��Ƿ����json����Ŀհ��ַ�
    static bool isspace(char c);
};

}

#endif

