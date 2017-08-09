#ifndef __TC_DES_H
#define __TC_DES_H

#include <string>
#include <stdint.h>
#include "tc_ex.h"
using namespace std;

namespace taf
{
/////////////////////////////////////////////////
/** 
 * @file tc_des.h
 * @brief des�ӽ�����.
 *  
 * @author  jarodruan@tencent.com
 */             
/////////////////////////////////////////////////



/**
 * @brief des�쳣. 
 */
struct TC_DES_Exception : public TC_Exception
{
    TC_DES_Exception(const string &buffer) : TC_Exception(buffer){};
    ~TC_DES_Exception() throw(){};
};


/**
 * @brief des/3des���ܽ���Դ��, �������κο�.
 *
 * ���������е�d3des.h d3des.c�޸����.
 *  
 * ����des����,8λ��Կ,����8λ���Ҳ�0x00,����8λ,ֻȡ��8λ��Ч. 
 *  
 * ��������8λ����,���뷽ʽΪ:��1λ��һ��0x01,��2λ������0x02,...
 *  
 * ������8λ�����,���油�˸�0x08.
 *
 * ����3des�ӽ���,����:ֻ֧��3des-ecb���ܷ�ʽ�� 
 *  
 * 24λ��Կ,����24λ���Ҳ�0x00,����24λ,ֻȡ��24λ��Ч; 
 *  
 * ��������8λ���룬���뷽ʽΪ����1λ��һ��0x01,��2λ������0x02,...
 *  
 * ������8λ����ģ����油�˸�0x08.
 *
 * Key������null�������ַ���.
 *
 */
class TC_Des
{
public:
    /**
	 * @brief des����.
	 *  
	 * @param sIn     ����buffer
	 * @param iInLen  ����buffer����
	 * @return        string ���ܺ������
     */
    static string encrypt(const char *key, const char *sIn, size_t iInlen);

    /**
	 * @brief des����. 
	 *  
	 * @param sIn     ����buffer
	 * @param iInlen  ����buffer����
	 * @return        string ����������, �������ʧ��, ��Ϊ��
     */
    static string decrypt(const char *key, const char *sIn, size_t iInlen);

    /**
	 * @brief  3des����. 
	 *  
	 * @param sIn     ����buffer
	 * @param iInLen  ����buffer����
	 * @return        string ���ܺ������
     */
    static string encrypt3(const char *key, const char *sIn, size_t iInlen);

    /**
	 * @brief  3des����. 
	 *  
	 * @param sIn      ����buffer
	 * @param iInlen   ����buffer����
	 * @return         string����������, �������ʧ��, ��Ϊ��
     */
    static string decrypt3(const char *key, const char *sIn, size_t iInlen);

    /**
	 * @brief  �������/���� . 
     */
    enum
    {
        EN0 = 0, /**����*/
        DE1 = 1  /**����*/
    };

protected:
    /**
	 * @brief  ��ȡkey. 
	 *  
     * @param key  keyֵ
     * @param mode ģʽ ��0�������, 1�������
     */
	static void deskey(const char *key, short mode, uint32_t *k);

    /**
	 * @brief  des����/����. 
	 *  
     * @param from  8���ֽ�
     * @param to    ���ܽ���ֻ�е�8���ֽ�
     */
	static void des(const char *from, char *to, uint32_t *KnL);

    /**
	 * @brief  ��ȡkey. 
	 *  
     * @param key   keyֵ
     * @param mode  ģʽ��0�������, 1�������
     */
    static void des3key(const char *key, short mode, uint32_t *KnL, uint32_t *KnR, uint32_t *Kn3);

    /**
	 * @brief  3des. 
	 *  
     * @param from   8���ֽ�
     * @param into  ���ܽ���ֻ�е�8���ֽ�
     */
    static void des3(const char *from, char *into, uint32_t *KnL, uint32_t *KnR, uint32_t *Kn3);

private:
    static void cookey(register uint32_t *raw1, uint32_t *k);
    static void scrunch(register const char *outof, register uint32_t *into);
    static void unscrun(register uint32_t *outof, register char *into);
    static void desfunc(register uint32_t *block, register uint32_t *keys);
};

}
#endif

