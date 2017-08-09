#ifndef __TC_PACK_H_
#define __TC_PACK_H_

#include <netinet/in.h>
#include <vector>
#include <map>
#include <string>
#include "tc_ex.h"

namespace taf
{

/////////////////////////////////////////////////
/** 
* @file tc_pack.h 
* @brief  �����ƴ�������. 
*  
* �����Ƶ��������࣬ͨ��<<��>>������ʵ��. 
*  
* ��string��������ѹ����ʽ������<255: 1���ֽڳ���, ���ݣ�����>=255, 4���ֽ�(����), ����. 
*  
* ��������/����ֵ�������ֽ���, ���̰߳�ȫ�� 
*  
* @author  jarodruan@tencent.com
*/             
/////////////////////////////////////////////////

/**
* @brief  ����쳣��
*/
struct TC_PackIn_Exception : public TC_Exception
{
	TC_PackIn_Exception(const string &buffer) : TC_Exception(buffer){};
    TC_PackIn_Exception(const string &buffer, int err) : TC_Exception(buffer, err){};
    ~TC_PackIn_Exception() throw(){};
};

/**
 * @brief  ����쳣
 */
struct TC_PackOut_Exception : public TC_Exception
{
	TC_PackOut_Exception(const string &buffer) : TC_Exception(buffer){};
    TC_PackOut_Exception(const string &buffer, int err) : TC_Exception(buffer, err){};
    ~TC_PackOut_Exception() throw(){};
};

/**
* @brief  �����, �û����һ�������ư�
*/
class TC_PackIn
{
public:

    /**
     * @brief  ���캯��
     */
    TC_PackIn() : _pii(this, true, string::npos)
    {

    }

protected:
    /**
     *
     */
    class TC_PackInInner
    {
    public:
        /**
         * @brief  
         * @param pi
         */
        TC_PackInInner(TC_PackIn *ppi, bool bInsert, string::size_type nPos = string::npos)
        : _ppi(ppi)
        , _bInsert(bInsert)
        , _nPos(nPos)
        {

        }

        /**
         * @brief  
         */
        ~TC_PackInInner()
        {
            if(_nPos == string::npos)
            {
                 return;
            }

            if(_nPos > _buffer.length())
            {
                throw TC_PackIn_Exception("TC_PackIn cur has beyond error.");
            }

            if(_bInsert)
            {
                _ppi->getBuffer().insert(_nPos, _buffer.c_str(), _buffer.length());
            }
            else
            {
                _ppi->getBuffer().replace(_nPos, _buffer.length(), _buffer);
            }
        }

        /**
         *
         */
        void clear() { _buffer = ""; }

        /**
         * @brief  
         *
         * @return size_t
         */
        size_t length() const   { return _buffer.length(); }

        /**
         * @brief  
         *
         * @return const string&
         */
        const string &topacket() const { return _buffer; }

        /**
         *
         *
         * @return string&
         */
        string& getBuffer() { return _buffer; }

        /**
         *
         * @param b
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (bool t);

        /**
         * @brief  
         * @param t
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (char t);

        /**
         * @brief  
         * @param t
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (unsigned char t);

        /**
         * @brief  
         * @param t
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (short t);

        /**
         * @brief  
         * @param t
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (unsigned short t);

        /**
         * @brief  
         * @param t
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (int t);

        /**
         * @brief  
         * @param t
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (unsigned int t);

        /**
         * @brief  
         * @param t
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (long t);

        /**
         * @brief  
         * @param t
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (unsigned long t);

        /**
         * @brief  
         * @param t
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (long long t);

        /**
         * @brief  
         * @param t
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (unsigned long long t);

        /**
         * @brief  
         * @param t
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (float t);

        /**
         * @brief  
         * @param t
         *
         * @return TC_PackIn&
         */
        TC_PackInInner& operator << (double t);

        /**
        * @brief  ���0�����ַ���, ������'\0'Ҳ��copy��ȥ
        * @param sBuffer
        * return void
        */
        TC_PackInInner& operator << (const char *sBuffer);

        /**
        * @brief  ��Ӷ������ַ���
        * ����>=255: 1���ֽ�(255) ����, ����
        * ����<255, 1���ֽ�(����), ����
        * @param sBuffer, bufferָ��
        * @param iLen, �ֽ���
        * return void
        */
        TC_PackInInner& operator << (const string& sBuffer);

        /**
         * @brief  
         * @param pi
         *
         * @return TC_PackInInner&
         */
        TC_PackInInner& operator << (const TC_PackIn& pi);

    protected:
        TC_PackIn   *_ppi;
        bool        _bInsert;
        string::size_type   _nPos;
        string      _buffer;
    };

public:
    /**
    * @brief  ������buffer������
    */
    void clear() { _pii.clear(); }

    /**
    * @brief  �������
    * @return size_t
    */
    size_t length() const   { return _pii.length(); }

    /**
    * @brief  ���ص�ǰ��������
    * @return string
    */
    const string& topacket() const { return _pii.topacket(); }

    /**
     * @brief  
     *
     * @return string&
     */
    string& getBuffer() {return _pii.getBuffer(); }

    /**
     * @brief  
     * @param t
     *
     * @return TC_PackIn&
     */
    template<typename T>
    TC_PackIn& operator << (T t)
    {
        _pii << t;
        return *this;
    }

    /**
     * @brief  
     *
     * @return TC_PackIn&
     */
    TC_PackInInner insert(string::size_type nPos)
    {
        return TC_PackInInner(this, true, nPos);
    }

    /**
     * @brief  �滻
     * @param iCur
     *
     * @return TC_PackIn&
     */
    TC_PackInInner replace(string::size_type nPos)
    {
        return TC_PackInInner(this, false, nPos);
    }

protected:

    /**
     * @brief  
     */
    TC_PackInInner  _pii;
};

/**
* @brief  �����, �û���һ�������ư�
*/
class TC_PackOut
{
public:

    /**
    * @brief  ���캯��, ���ڽ��
    * @param pBuffer : ��Ҫ�����buffer, ��buffer��Ҫ�������������Ч
    * @param iLength : pBuffer����
    */
    TC_PackOut(const char *pBuffer, size_t iLength)
    {
        init(pBuffer, iLength);
    }

    /**
    * @brief  ���캯��
    */
    TC_PackOut(){};

    /**
    * @brief  ��ʼ��, ���ڽ��
    * @param pBuffer : ��Ҫ�����buffer, ��buffer��Ҫ�������������Ч
    * @param iLength : pBuffer����
    */
    void init(const char *pBuffer, size_t iLength)
    {
        _pbuffer    = pBuffer;
        _length     = iLength;
        _pos        = 0;
    }

    /**
     * @brief  �ж��Ƿ��Ѿ������ĩβ��
     * 
     * @return bool
     */
    bool isEnd();

    /**
     * @brief  
     * @param t
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (bool &t);

    /**
     * @brief  
     * @param t
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (char &t);

    /**
     * @brief  
     * @param t
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (unsigned char &t);

    /**
     * @brief  
     * @param t
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (short &t);

    /**
     * @brief  
     * @param t
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (unsigned short &t);

    /**
     * @brief  
     * @param t
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (int &t);

    /**
     * @brief  
     * @param t
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (unsigned int &t);

    /**
     * @brief  
     * @param t
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (long &t);

    /**
     * @brief  
     * @param t
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (unsigned long &t);

    /**
     * @brief  
     * @param t
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (long long &t);

    /**
     * @brief  
     * @param t
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (unsigned long long &t);

    /**
     * @brief  
     * @param f
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (float &f);

    /**
     * @brief  
     * @param f
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (double &f);

    /**
     * @brief  
     * @param sBuffer
     *
     * @return TC_PackOut&
     */
    TC_PackOut& operator >> (char *sBuffer);

    /**
    * @brief  ��Ӷ������ַ���(�ȼ�¼����
    * ����>=255: 1���ֽ�(255) ����, ����
    * ����<255, 1���ֽ�(����), ����
    * @param sBuffer, bufferָ��
    * @param iLen, �ֽ���
    * return void
    */
    TC_PackOut& operator >> (string& sBuffer);

protected:

    /**
    * ���ʱ��buffer
    */
    const char      *_pbuffer;

    /**
    * ���ʱ��buffer����
    */
    size_t 			_length;

    /**
    * ���ʱ�ĵ�ǰλ��
    */
    size_t 			_pos;
};

//////////////////////////////////////////////////////////

/**
 * @brief  bool����
 * @param i
 *
 * @return TC_PackIn&
 */
inline TC_PackIn& encode(TC_PackIn& pi, bool i)
{
    pi << i;
	return pi;
}

/**
 * @brief  int����
 * @param i
 *
 * @return TC_PackIn&
 */
inline TC_PackIn& encode(TC_PackIn& pi,int i)
{
    pi << i;
	return pi;
}

/**
 * @brief  byte����
 * @param i
 *
 * @return TC_PackIn&
 */
inline TC_PackIn& encode(TC_PackIn& pi, char i)
{
    pi << i;
	return pi;
}

/**
 * @brief  short����
 * @param i
 *
 * @return TC_PackIn&
 */
inline TC_PackIn& encode(TC_PackIn& pi, short i)
{
    pi << i;
	return pi;
}

/**
 * @brief  string����
 * @param s
 *
 * @return TC_PackIn&
 */
inline TC_PackIn& encode(TC_PackIn& pi, const string &i)
{
    pi << i;
	return pi;
}

/**
 * @brief  float����
 * @param f
 *
 * @return TC_PackIn&
 */
inline TC_PackIn& encode(TC_PackIn& pi, float i)
{
    pi << i;
	return pi;
}

/**
 * @brief  double����
 * @param f
 *
 * @return TC_PackIn&
 */
inline TC_PackIn& encode(TC_PackIn& pi, double i)
{
    pi << i;
	return pi;
}

/**
 * @brief  �����α���
 * @param f
 *
 * @return TC_PackIn&
 */
inline TC_PackIn& encode(TC_PackIn& pi, long long i)
{
    pi << i;
	return pi;
}

/**
 * @brief  �ṹ����
 * @param T
 * @param t
 *
 * @return TC_PackIn&
 */
template<typename T>
inline TC_PackIn& encode(TC_PackIn& pi, const T &i)
{
    return i.encode(pi);
}

/**
 * @brief  vector����
 * @param T
 * @param t
 *
 * @return TC_PackIn&
 */
template<typename T>
inline TC_PackIn& encode(TC_PackIn& pi, const vector<T> &t)
{
	encode(pi, (int)t.size());
    for(size_t i = 0; i < t.size(); i++)
    {
        encode(pi, t[i]);
    }
    return pi;
}

/**
 * @brief  Map����
 * @param K
 * @param V
 * @param t
 *
 * @return TC_PackIn&
 */
template<typename K, typename V>
inline TC_PackIn& encode(TC_PackIn& pi, const map<K, V> &t)
{
	encode(pi, (int)t.size());
    typename map<K, V>::const_iterator it = t.begin();
    while(it != t.end())
    {
		encode(pi, it->first);
		encode(pi, it->second);
        ++it;
    }

    return pi;
}

//////////////////////////////////////////////////////////

/**
 * @brief  bool����
 * @param oPtr
 * @param t
 */
inline void decode(TC_PackOut &po, bool &t)
{
	po >> t;
}

/**
 * @brief  short����
 * @param oPtr
 * @param t
 */
inline void decode(TC_PackOut &po, short &t)
{
	po >> t;
}

/**
 * @brief  int����
 * @param oPtr
 * @param t
 */
inline void decode(TC_PackOut &po, int &t)
{
	po >> t;
}

/**
 * @brief  byte����
 * @param oPtr
 * @param t
 */
inline void decode(TC_PackOut &po, char &t)
{
	po >> t;
}

/**
 * @brief  string����
 * @param oPtr
 * @param t
 */
inline void decode(TC_PackOut &po, string &t)
{
	po >> t;
}

/**
 * @brief  float����
 * @param oPtr
 * @param t
 */
inline void decode(TC_PackOut &po, float &t)
{
	po >> t;
}

/**
 * @brief  double����
 * @param oPtr
 * @param t
 */
inline void decode(TC_PackOut &po, double &t)
{
	po >> t;
}

/**
 * @brief  long long����
 * @param oPtr
 * @param t
 */
inline void decode(TC_PackOut &po, long long &t)
{
	po >> t;
}

/**
 * @brief  �ṹ����
 * @param T
 * @param oPtr
 * @param t
 */
template<typename T>
inline void decode(TC_PackOut &po, T &t)
{
	t.decode(po);
}

/**
 * @brief  vector����
 * @param T
 * @param oPtr
 * @param t
 */
template<typename T>
inline void decode(TC_PackOut &po, vector<T> &t)
{
	int n;
	po >> n;
	for(int i = 0; i < n; i++)
	{
        T tt;
        decode(po, tt);

        t.push_back(tt);
    }
}

/**
 * @brief  map����
 * @param K
 * @param V
 * @param oPtr
 * @param t
 */
template<typename K, typename V>
inline void decode(TC_PackOut &po, map<K, V> &t)
{
	int n;
	po >> n;
	for(int i = 0; i < n; i++)
	{
        K k;
		V v;
        decode(po, k);
        decode(po, v);
		t[k] = v;
    }
}

}


#endif
