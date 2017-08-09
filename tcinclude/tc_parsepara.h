#ifndef __TC_PARSEPARA_H
#define __TC_PARSEPARA_H

#include <map>
#include <string>

using namespace std;

namespace taf
{
/////////////////////////////////////////////////
/** 
* @file tc_parsepara.h 
* @brief  name=value��ʽ������(�Ǳ�׼��httpЭ��) 
*  
* @author  jarodruan@tencent.com   
*/            
/////////////////////////////////////////////////


	/** 
	* @brief  �ṩname=value&name1=value1��ʽ�Ľ�������.
	* 
	* ���Ժ�map����ת��,���Ǳ�׼��cgi�����Ľ���
	* 
	* ����׼cgi����������ѿո��+��ת��,���̰߳�ȫ��
    * @param ����name=value&name=value�ַ���
    */
class TC_Parsepara
{
public:

	TC_Parsepara(){};

    /**
    * @brief  ���캯��
    * @param ����name=value&name=value�ַ���
    */
	TC_Parsepara(const string &sParam);

    /**
    * @brief  copy contructor
    * @param : name=value&name=value�ַ���
    */
	TC_Parsepara(const map<string, string> &mpParam);

    /**
    * @brief  copy contructor
    */
	TC_Parsepara(const TC_Parsepara &para);

    /**
    * =
    */
	TC_Parsepara &operator=(const TC_Parsepara &para);

    /**
    * ==
    */
	bool operator==(const TC_Parsepara &para);

    /**
    *+
    */
	const TC_Parsepara operator+(const TC_Parsepara &para);

    /**
    * +=
    */
	TC_Parsepara& operator+=(const TC_Parsepara &para);

    /**
    * @brief  decontructor
    */
	~TC_Parsepara();

    /**
    * @brief  ��������
    * @param ��������
    * @param ����ֵ
    */
	typedef int (*TC_ParseparaTraverseFunc)(string, string, void *);

    /**
    *@brief  ����[], ��ȡ����ֵ
    *@return string &����ֵ
    */
	string &operator[](const string &sName);

    /**
    * @brief  �����ַ���,������
    * @param sParam:�ַ�������
    * @return ��
    */
	void load(const string &sParam);

    /**
    * @brief  ����map,������
    * @param mpParam:�ַ�������
    * @return void
    */
	void load(const map<string, string> &mpParam);

    /**
    * @brief  ת���ַ���
    * @return string
    */
	string tostr() const;

    /**
    * @brief  ��ȡ����ֵ
    * @param sName ��������
    * @return string
    */
	string getValue(const string &sName) const;

    /**
    * @brief  ���ò���ֵ
    * @param sName ��������
    * @param sValue ����ֵ
    * @return void
    */
	void setValue(const string &sName, const string &sValue);

    /**
    * @brief  �����ǰ����ֵ��
    * return void
    */
	void clear();

    /**
    * @brief  ���÷�ʽ��ȡ����map
    * @return map<string,string>& ���ز���map
    */
    map<string,string> &toMap();

    /**
    * @brief  ���÷�ʽ��ȡ����map
    * @return map<string,string>& ���ز���map
    */
    const map<string,string> &toMap() const;

    /**
    * @brief  ����ÿ������ֵ��
    * @param func: ����
    * @param pParam: ����,����func��
    * @return void
    */
	void traverse(TC_ParseparaTraverseFunc func, void *pParam);

    /**
    * @brief  ���ַ�������,%XXת���ַ�,������httpЭ��ı���
    * @param sParam ����
    * @return string,�������ַ���
    */
	static string decodestr(const string &sParam);

    /**
	* @brief  ���ַ�������,�����ַ�ת��%XX, 
	*   	  ������httpЭ��ı���(���˶Կո�=>+�����⴦��)
    * @param sParam ����
    * @return string, �������ַ���
    */
	static string encodestr(const string &sParam);

protected:

    /**
    * @brief  �ַ���ת����map
    * @param sParam ����
    * @param mpParam map
    * @return void
    */
	void decodeMap(const string &sParam, map<string, string> &mpParam) const;

    /**
    * @brief  mapת�����ַ���
    * @param mpParam map
    * @return string, ת������ַ���
    */
	string encodeMap(const map<string, string> &mpParam) const;

    /**
    * @brief  "%xx" ת��Ϊ�ַ�
    * @param sWhat: %xx�ַ�������������ַ�
    * @return char �����ַ�
    */
	static char x2c(const string &sWhat);

protected:

	map<string, string> _param;
};

}
#endif /*_TC_PARSEPARA_H*/
