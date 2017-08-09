#ifndef __TC_ENCODER_H_
#define __TC_ENCODER_H_

#include <vector>

#include "tc_ex.h"

namespace taf
{
/////////////////////////////////////////////////
/** 
* @file tc_encoder.h 
* @brief  ת���� 
*  
* @author  jarodruan@tencent.com ,coonzhang@tencent.com
* 
* 
*/
/////////////////////////////////////////////////

/**
*  @brief  ת���쳣��
*/
struct TC_Encoder_Exception : public TC_Exception
{
	TC_Encoder_Exception(const string &buffer) : TC_Exception(buffer){};
    TC_Encoder_Exception(const string &buffer, int err) : TC_Exception(buffer, err){};
	~TC_Encoder_Exception() throw(){};
};

/**
*  @brief �����ṩ�������ñ����ת��.
*  
*   	  Gbk��utf8֮����໥ת�뺯����ͨ����̬�����ṩ.
*  
*   	  1��GBK ==> UTF8��ת��
*  
*   	  2��UTF8 ==> GBK��ת��
*/
class TC_Encoder
{
public:
    /**
	* @brief  gbk ת���� utf8.
	*  
    * @param sOut        ���buffer
    * @param iMaxOutLen  ���buffer���ĳ���/sOut�ĳ���
    * @param sIn         ����buffer
    * @param iInLen      ����buffer����
    * @throws            TC_Encoder_Exception
    * @return 
    */
    static void  gbk2utf8(char *sOut, int &iMaxOutLen, const char *sIn, int iInLen);

    /**
	* @brief  gbk ת���� utf8. 
	*  
    * @param sIn   ����buffer*
    * @throws      TC_Encoder_Exception
    * @return      ת�����utf8����
    */
    static string gbk2utf8(const string &sIn);

    /**
	* @brief  gbk ת���� utf8. 
	*  
    * @param sIn    ����buffer
    * @param vtStr  ���gbk��vector
    * @throws       TC_Encoder_Exception
    * @return
    */
    static void gbk2utf8(const string &sIn, vector<string> &vtStr);

    /**
	* @brief  utf8 ת���� gbk. 
	*  
    * @param sOut       ���buffer
    * @param iMaxOutLen ���buffer���ĳ���/sOut�ĳ���
    * @param sIn        ����buffer
    * @param iInLen     ����buffer����
    * @throws           TC_Encoder_Exception
    * @return
    */
    static void utf82gbk(char *sOut, int &iMaxOutLen, const char *sIn, int iInLen);

    /**
	* @brief  utf8 ת���� gbk. 
	*  
    * @param sIn  ����buffer
    * @throws     TC_Encoder_Exception
    * @return    ת�����gbk����
    */
    static string utf82gbk(const string &sIn);

	/**	
	* @brief  ��string��\n�滻��,ת���ַ����е�ĳ���ַ� 
	*  
	* ȱʡ:\n ת��Ϊ \r\0; \rת��Ϊ\,
	*  
	* ��Ҫ���ڽ�string��¼��һ�У�ͨ������дbin-log�ĵط�;
	* @param str   ��ת���ַ���
	* @param f     ��Ҫת����ַ�
	* @param t     ת�����ַ�
	* @param u     ���õ�ת���
	* @return str  ת������ַ���
	*/
	static string transTo(const string& str, char f = '\n', char t = '\r', char u = '\0');

	/**
	* @brief  ���滻�����ݻָ�Դ����,�� transTo ���ַ�����ԭ�� 
	*  
	*  ȱʡ:\r\0 ��ԭΪ\n��\r\r��ԭΪ,
	*  
	*  ��Ҫ���ڽ�string��¼��һ�У�ͨ������дbin-log�ĵط�
	* @param str  ����ԭ���ַ���(������transTo��õ����ַ���)
	* @param f    ��ת����ַ�
	* @param t    ת�����ַ�
	* @param u    ���õ�ת���
	* @return str ��ԭ����ַ���
	*/
	static string transFrom(const string& str, char f = '\n', char t = '\r', char u = '\0');
};

}


#endif


