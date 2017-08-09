#ifndef	__TC_BIT_MAP_H__
#define __TC_BIT_MAP_H__

#include <iostream>
#include <string>
#include <vector>
#include "tc_ex.h"

using namespace std;

namespace taf
{
/////////////////////////////////////////////////
/** 
 * @file  tc_bitmap.h 
 * @brief  ��λbitmap��. 
 *  
 * @author  jarodruan@tencent.com,kevintian
 */             
/////////////////////////////////////////////////
/**
 * @brief �쳣
 */
struct TC_BitMap_Exception : public TC_Exception
{
	TC_BitMap_Exception(const string &buffer) : TC_Exception(buffer){};
	~TC_BitMap_Exception() throw(){};
};


/**
 * @brief �ڴ�bitmap��ÿ������1λ������֧�ֶ�λ��������������λ.
 * 
 *  �������̲��������������Ҫ��������õ�ʱ��ӣ�ͨ������Ⱥ������.
 * 
 *  ע��Ⱥ������Ӧ��/8��Ȼ����β�ŷ�Ⱥ��
 */
class TC_BitMap
{
public:
    /**
	 * @brief �ڴ��bitmap��ÿ����������1λ 
     *
     */
    class BitMap
    {
    public:
    	
        static const int _magic_bits[8];

        #define _set_bit(n,m)   (n|_magic_bits[m])
        #define _clear_bit(n,m) (n&(~_magic_bits[m]))
        #define _get_bit(n,m)   (n&_magic_bits[m])

        /**�����ڴ�汾*/
        #define BM_VERSION      1

        /**
         * @brief ����Ԫ�ظ���������Ҫ�ڴ�Ĵ�С
         * @param iElementCount, ��Ҫ�����Ԫ�ظ���(Ԫ�ش�0��ʼ��)
         * 
         * @return size_t
         */
        static size_t calcMemSize(size_t iElementCount);

        /**
         * @brief ��ʼ��
         * @param pAddr ���Ե�ַ
         * @param iSize ��С, ����(calcMemSize)�������
         * @return      0: �ɹ�, -1:�ڴ治��
         */
        void create(void *pAddr, size_t iSize);

        /**
         * @brief ���ӵ��ڴ��
         * @param pAddr  ��ַ, ����(calcMemSize)�������
         * @return       0, �ɹ�, -1,�汾����, -2:��С����
         */
        int connect(void *pAddr, size_t iSize);

        /**
         * @brief �Ƿ��б�ʶ
		 * @param i 
         * @return int, >0:�б�ʶ, =0:�ޱ�ʶ, <0:������Χ
         */
        int get(size_t i);

        /**
         * @brief ���ñ�ʶ
		 * @param i 
         * @return int, >0:�б�ʶ, =0:�ޱ�ʶ, <0:������Χ
         */
        int set(size_t i);

        /**
         * @brief �����ʶ
         * @param i
         * 
         * @return int, >0:�б�ʶ, =0:�ޱ�ʶ, <0:������Χ
         */
        int clear(size_t i);

		/**
		 * @brief ������е�����
		 * 
		 * @return int 
		 */
		int clear4all();

        /**
         * @brief dump���ļ�
         * @param sFile
         * 
         * @return int
         */
        int dump2file(const string &sFile);

        /**
         * @brief ���ļ�load
         * @param sFile
         * 
         * @return int
         */
        int load5file(const string &sFile);

		/**�����ڴ�ͷ��*/
        struct tagBitMapHead
        {
           char     _cVersion;          /**�汾, ��ǰ�汾Ϊ1*/
           size_t   _iMemSize;          /**�����ڴ��С*/
        }__attribute__((packed));

        /**
		 * @brief ��ȡͷ����ַ 
         * @return tagBitMapHead* �����ڴ�ͷ��
         */
        BitMap::tagBitMapHead *getAddr() const   { return _pHead; }

        /**
		 * @brief ��ȡ�ڴ��С 
         * @return �ڴ��С
         */
        size_t getMemSize() const                   { return _pHead->_iMemSize; }

    protected:

        /**
         * �����ڴ�ͷ��
         */
        tagBitMapHead               *_pHead;

        /**
         * ���ݿ�ָ��
         */
        unsigned char *             _pData;
    };

    /**
     * @brief ����Ԫ�ظ���������Ҫ�ڴ�Ĵ�С
     * @param iElementCount  ��Ҫ�����Ԫ�ظ���(Ԫ�ش�0��ʼ��)
	 * @param iBitCount     ÿ��Ԫ��֧�ּ�λ(Ĭ��1λ) (λ��>=1) 
     * @return              �����ڴ�Ĵ�С
     */
    static size_t calcMemSize(size_t iElementCount, unsigned iBitCount = 1);

    /**
     * @brief ��ʼ��
     * @param pAddr ���Ե�ַ
     * @param iSize ��С, ����(calcMemSize)�������
     * @return      0: �ɹ�, -1:�ڴ治��
     */
    void create(void *pAddr, size_t iSize, unsigned iBitCount = 1);

    /**
     * @brief ���ӵ��ڴ��
     * @param pAddr ��ַ������(calcMemSize)�������
     * @return      0���ɹ�, -1���汾����, -2:��С����
     */
    int connect(void *pAddr, size_t iSize, unsigned iBitCount = 1);

    /**
     * @brief �Ƿ��б�ʶ
     * @param i      Ԫ��ֵ
	 * @param iBit   �ڼ�λ 
     * @return       int, >0:�б�ʶ, =0:�ޱ�ʶ, <0:������Χ
     */
    int get(size_t i, unsigned iBit = 1);

    /**
     * @brief ���ñ�ʶ
     * @param i     Ԫ��ֵ
	 * @param iBit  �ڼ�λ 
     * @return      int, >0:�б�ʶ, =0:�ޱ�ʶ, <0:������Χ
     */
    int set(size_t i, unsigned iBit = 1);

    /**
     * @brief �����ʶ
     * @param i     Ԫ��ֵ
	 * @param iBit  �ڼ�λ 
     * @return      int, >0:�б�ʶ, =0:�ޱ�ʶ, <0:������Χ
     */
    int clear(size_t i, unsigned iBit = 1);

	/**
	 * @brief ������еı�ʶ
	 * 
	 * @param iBit  �ڼ�λ
	 * @return int 
	 */
	int clear4all(unsigned iBit = (unsigned)(-1));

    /**
     * @brief dump���ļ�
     * @param sFile 
     * 
     * @return int
     */
    int dump2file(const string &sFile);

    /**
     * @brief ���ļ�load
     * @param sFile
     * 
     * @return int
     */
    int load5file(const string &sFile);

protected:
    vector<BitMap>   _bitmaps;
};

}

#endif

