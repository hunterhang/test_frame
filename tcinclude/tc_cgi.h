#ifndef __TC_CGI_H
#define __TC_CGI_H

#include <sstream>
#include <istream>
#include <map>
#include <vector>
#include "tc_ex.h"

namespace taf
{
/////////////////////////////////////////////////
/** 
* @file tc_cgi.h
* @brief CGI������
*  
* @author jarodruan@tencent.com 
*/            
/////////////////////////////////////////////////
class TC_Cgi;
class TC_Cgi_Upload;
class TC_HttpRequest;

/**
* @brief �����ļ��쳣��
*/
struct TC_Cgi_Exception : public TC_Exception
{
	TC_Cgi_Exception(const string &buffer) : TC_Exception(buffer){};
    TC_Cgi_Exception(const string &buffer, int err) : TC_Exception(buffer, err){};
	~TC_Cgi_Exception() throw(){};
};

/**
* @brief ȫ�ֵ���Ԫ����������ú���, 
*   	 �������TC_Common::tostr�� vector<TC_Cgi_Upload>�����������
*/
ostream &operator<<(ostream &os, const TC_Cgi_Upload &tcCgiUpload);

/**
* @brief cgi�ϴ��ļ�������ͨ�������ȡcgi�ϴ����ļ���Ϣ 
*/
class TC_Cgi_Upload
{
public:
    friend ostream &operator<<(ostream &os, const TC_Cgi_Upload &tcCgiUpload);

    /**
    * @brief ���캯��
    */
    TC_Cgi_Upload()
    :_sFileName("")
    , _sRealFileName("")
    , _sServerFileName("")
    , _iSize(0)
    , _bOverSize(false)
    {
    }

    /**
	* @brief �������캯��. 
    */

    TC_Cgi_Upload(const TC_Cgi_Upload &tcCgiUpload);

    /**
    * @brief ��ֵ���캯��
    */
    TC_Cgi_Upload & operator=(const TC_Cgi_Upload &tcCgiUpload);

    /**
	* @brief ��ȡ�ϴ�����Ϣ. 
	*  
    * return �ϴ��ļ�����Ϣ
    */
    string tostr() const;

    /**
	* @brief ��ȡ�ͻ���IE INPUT�ϴ��ؼ�������. 
	*  
    * return INPUT�ϴ��ؼ�����
    */
    string getFormFileName() const
    {
        return _sFileName;
    }

    /**
	* @brief ����INPUT�ؼ��û����������,���ͻ�����ʵ���ļ�����. 
	*  
    * return  �ͻ�����ʵ���ļ�����
    */
    string retRealFileName() const
    {
        return _sRealFileName;
    }

    /**
	* @brief �ϴ�����������,���������ļ�����. 
	*  
    * return ���������ļ�����
    */
    string getSeverFileName() const
    {
        return _sServerFileName;
    }

    /**
	* @brief ��ȡ�ϴ����ļ���С. 
	*  
    * return size_t���ͣ��ϴ����ļ���С
    */
    size_t getFileSize() const
    {
        return _iSize;
    }

    /**
	* @brief �ϴ����ļ��Ƿ񳬹���С. 
	*  
    * return ������С����true�����򷵻�false
    */
    bool isOverSize() const
    {
        return _bOverSize;
    }

protected:

    /**
    * �ϴ��ļ�,�����file�ؼ�����
    */
    string  _sFileName;

    /**
    * �ϴ��ļ���ʵ����,�ͻ��˵��ļ�����
    */
    string  _sRealFileName;

    /**
    * �ϴ��ļ�������������
    */
    string  _sServerFileName;

    /**
    * �ϴ��ļ���С,�ֽ���
     */
    size_t  _iSize;

    /**
    * �ϴ��ļ��Ƿ񳬹���С
    */
    bool    _bOverSize;

    friend class TC_Cgi;
};

/**
* @brief cgi���������. 
*  
* ��Ҫ���������� 
*  
* 1 ֧�ֲ������� 
*  
* 2 ֧��cookies���� 
*  
* 3 ֧���ļ��ϴ�,�����ϴ��ļ���������,�ļ�������С 
*  
* 4 �ϴ��ļ�ʱ, ��Ҫ����ļ��Ƿ񳬹�����С 
*  
* 5 �ϴ��ļ�ʱ, ��Ҫ����ϴ��ļ������Ƿ�����
*  
* ˵��:����ļ�ͬʱ�ϴ�ʱ,�������file�ؼ�����ȡ��ͬname,�����޷���ȷ���ϴ��ļ� 
*  
* ע��:����parseCgi������׼����,
*  
* ������ļ��ϴ���Ҫ����setUpload, ������Ҫ��parseCgi֮ǰ����
*
*/
class TC_Cgi
{
public:

    /**
    * @brief TC_Cgi���캯��
    */
    TC_Cgi();

    /**
    * @brief ��������
    */
    virtual ~TC_Cgi();

    /**
    * @brief ���廷������
    */
    enum
    {
        ENM_SERVER_SOFTWARE,
        ENM_SERVER_NAME,
        ENM_GATEWAY_INTERFACE,
        ENM_SERVER_PROTOCOL,
        ENM_SERVER_PORT,
        ENM_REQUEST_METHOD,
        ENM_PATH_INFO,
        ENM_PATH_TRANSLATED,
        ENM_SCRIPT_NAME,
        ENM_HTTP_COOKIE,
        ENM_QUERY_STRING,
        ENM_REMOTE_HOST,
        ENM_REMOTE_ADDR,
        ENM_AUTH_TYPE,
        ENM_REMOTE_USER,
        ENM_REMOTE_IDENT,
        ENM_CONTENT_TYPE,
        ENM_CONTENT_LENGTH,
        ENM_HTTP_USER_AGENT
    };

    /**
	 * @brief �����ϴ��ļ�. 
	 *  
     * @param sUploadFilePrefix, �ļ�ǰ׺(����·��), ������ļ��ϴ�,���ļ��������Ը�ǰ׺Ϊ���Ƶ�·����
     *                           ����ж���ļ��ϴ�,���ļ������Դ��ں����"_���"
     * @param iMaxUploadFiles    ����ϴ��ļ�����,<0:û������
     * @param iUploadMaxSize     ÿ���ļ��ϴ�������С(�ֽ�)
     */
    void setUpload(const string &sUploadFilePrefix, int iMaxUploadFiles = 5, size_t iUploadMaxSize = 1024*1024*10, size_t iMaxContentLength = 1024*1024*10);

    /**
	 * @brief �ӱ�׼�������cgi. 
     */
    void parseCgi();

    /**
	 * @brief ֱ�Ӵ�http�������. 
	 *  
     * @param request http����
     */
    void parseCgi(const TC_HttpRequest &request);

    /**
	* @brief ��ȡcgi��url����multimap. 
	*  
    * @return multimap<string, string>cgi��url����
    */
    const multimap<string, string> &getParamMap() const;

    /**
     * @brief ��ȡcgi��������map.
     *
     * @return map<string,string>cgi�Ļ�������
     */
    map<string, string> getEnvMap() const { return _env; }

    /**
	* @brief ��ȡcgi�Ĳ���map, ��multimapת����map���� 
	*   	 , ����һ���������ƶ�Ӧ�������ֵ�����, ֻȡ����һ��ֵ.
	*  
    * @return map<string, string>
    */
    map<string, string> getParamMapEx() const;

    /**
	* @brief ��ȡcookies�Ĳ���map. 
	*  
    * @return map<string, string>
    */
    const map<string, string> &getCookiesMap() const;

    /**
	* @brief ��ȡcgi��ĳ������. 
	*  
    * @param sName  ��������
    * @return       
    */
    string &operator[](const string &sName);

    /**
	* @brief ��ȡcgi��ĳ������. 
	*  
    * @param sName ��������
    * @return      ������ֵ
    */
    string getValue(const string& sName) const;

    /**
	* @brief ��ȡĳһ���ƵĲ����Ķ��ֵ. 
	*  
	* ���ڽ���checkbox����ؼ���ֵ( һ��������,�������ֵ)
    * @param sName   ��������
    * @param vtValue �����ƵĲ���ֵ��ɵ�vector
    * @return        vector<string>, �����ƵĲ���ֵ��ɵ�vector
    */
    const vector<string> &getMultiValue(const string& sName, vector<string> &vtValue) const;

    /**
	* @brief ��ȡcookieֵ. 
	*  
    * @param sName cookie����
    * @return      string���͵�cookieֵ
    */
    string getCookie(const string& sName) const;

    /**
	* @brief ����cookieֵ. 
	*  
    * @param sName    cookie����
    * @param sValue   cookieֵ
    * @param sExpires ��������
    * @param sPath    cookie��Ч·��
    * @param sDomain  cookie��Ч��
    * @param bSecure  �Ƿ�ȫ(sslʱ��Ч)
    * @return         �����ַ���������cookieֵ
    */
    string setCookie(const string &sName, const string &sValue, const string &sExpires="", const string &sPath="/", const string &sDomain = "", bool bSecure = false);

    /**
	* @brief ���������Ƿ�Ϊ��. 
	*  
    * @return ��������Ϊ�շ���true�����򷵻�false
    */
    bool isParamEmpty() const;

    /**
	* @brief �����Ƿ����. 
	*  
    * @param sName ��������
    * @return      ���ڷ���true�����򷵻�false
    */
    bool isParamExist(const string& sName) const;

    /**
	* @brief �ϴ��ļ��Ƿ񳬹���С������ļ��ϴ�ʱ, 
	*   	 ֻҪ��һ���ļ�������С,�򳬹�
    * @return �������ϱ�׼������С�ķ���true�����򷵻�false
    */
    bool  isUploadOverSize() const;

    /**
	* @brief �ϴ��ļ��Ƿ񳬹���С,����ļ��ϴ�ʱ, 
	*   	 ֻҪ��һ���ļ�������С,�򳬹�
    * @param vtUploads ���س�����С���ļ�����(�����file�ؼ�������)
	* @return          �������ϱ�׼������С�ķ���true�����򷵻�false
    */
    bool  isUploadOverSize(vector<TC_Cgi_Upload> &vtUploads) const;

    /**
     * @brief �Ƿ񳬹��ϴ��ļ�����.
     *
     * @return �����ϴ���������true�����򷵻�false
     */
    bool isOverUploadFiles() const { return _bOverUploadFiles; }

    /**
	* @brief ��ȡ�ϴ��ļ�����. 
	*  
    * @return size_t�ϴ��ļ��ĸ���
    */
    size_t getUploadFilesCount() const;

    /**
	* @brief ��ȡ�ϴ��ļ��������Ϣ
	*  
	* @return map<string,TC_Cgi_Upload>�ṹ�У� 
	*   	  �����ļ������ļ������Ϣ��map
    */
    const map<string, TC_Cgi_Upload> &getUploadFilesMap() const;

    /**
	* @brief ��ȡ��������. 
	*  
    * @param iEnv  ö�ٱ���
    * @return      ��������
    */
    string getCgiEnv(int iEnv);

    /**
	* @brief ��ȡ��������. 
	*  
    * @param sEnv ������������
    * @return     ����������ֵ
    */
    string getCgiEnv(const string& sEnv);

    /**
	 * @brief ���û�������. 
	 *  
     * @param sName ������������
     * @param sValue ����������ֵ
     */
    void setCgiEnv(const string &sName, const string &sValue);

    /**
	* @brief ����htmlͷ��content-type .
	*  
    * @param sHeader ȱʡֵΪ"text/html"
    * @return 
    */
    static string htmlHeader(const string &sHeader = "text/html");

    /**
	* @brief http�����url����, %����Ļ����ַ�. 
	*  
    * @param sUrl http����url
    * @return    �������ַ���
    */
    static string decodeURL(const string &sUrl);

    /**
	 * @brief ��url���б���, �����ֺ���ĸ��%XX����. 
	 *  
	 * @param sUrl http����url
     * @return     ������url
     */
    static string encodeURL(const string &sUrl);

    /**
	 * @brief ��Դ�ַ�������HTML����(<>"&) 
	 *  
     * @param src         Դ�ַ���
	 * @param blankEncode �Ƿ�Կո�Ҳ����(�ո�, \t, \r\n, \n) 
     * @return            HTML�������ַ���
     */
    static string encodeHTML(const string &src, bool blankEncode = false);

    /**
	 * @brief ��Դ�ַ�������XML����(<>"&'). 
	 *  
	 * @param src  Դ�ַ���
     * @return     XML�������ַ���
     */
    static string encodeXML(const string &src);

protected:

    /**
    * @brief ����,���ǲ�����,��֤����������ᱻʹ��
    */
    TC_Cgi &operator=(const TC_Cgi &tcCgi);

    /**
	* @brief GET method. 
	*  
    * @param sBuffer GET��QueryString
    * return 
    */
    void getGET(string &sBuffer);

    /**
	* @brief POST method. 
	*  
    * @param sBuffer   POST��QueryString
    * return 
    */
    void getPOST(string &sBuffer);

    /**
	* @brief �����ļ��ϴ�. 
	*  
    * @param mmpParams  [out]�������multimap
    * return 
    */
    void parseUpload(multimap<string, string> &mmpParams);

    /**
     * @brief ����form����
     */
    void parseFormData(multimap<string, string> &mmpParams, const string &sBoundary);

    /**
     * @brief  ���Կ���
     */
    void ignoreLine();

    /**
	 * @brief д�ļ�. 
	 *  
     * @param sFileName �ļ�����
     * @param sBuffer   Ҫд�������
     */
    bool writeFile(FILE*fp, const string &sFileName, const string &sBuffer, size_t &iTotalWrite);

    /**
	* @brief ���ϴ�ģʽ�½���. 
	*  
    * @param mmpParams  [out]�������multimap
    * @param sBuffer    [in]����QueryString
    * return 
    */
    void parseNormal(multimap<string, string> &mmpParams, const string& sBuffer);

    /**
	* @brief ����cookies. 
	*  
    * @param mpCooies [out]���cookiesmap
    * @param sBuffer  [in]����Cookies�ַ���
    * return
    */
    void parseCookies(map<string, string> &mpCooies, const string& sBuffer);

    /**
	* @brief ���ƽ���cgi input�Ļ�������. 
	*  
    * @param mmpParams [out]������� multimap
    * @param mpCooies [out]���cookies
    * return 
    */
    void readCgiInput(multimap<string, string> &mmpParams, map<string, string> &mpCooies);

protected:

    /**
     * buffer
     */
    string                      _buffer;

    /**
     * ��
     */
    istringstream               _iss;

    /**
     * ����
     */
    istream                     *_is;

    /**
     * ��������
     */
    map<string, string>         _env;

    /**
    * cgi����
    */
    multimap<string, string>    _mmpParams;

    /**
    * cookies
    */
    map<string, string>         _mpCookies;

    /**
    * �ϴ��ļ�����ǰ׺
    */
    string                      _sUploadFilePrefix;

    /**
    * �ϴ��ļ���������,<0:������
    */
    int                         _iMaxUploadFiles;

    /**
    * �ϴ��ļ�������С
    */
    size_t                      _iUploadMaxSize;

    /**
     * �Ƿ񳬹��ϴ��ļ�����
     */
    bool                        _bOverUploadFiles;

    /**
     * ����content-length
     */
    size_t                      _iMaxContentLength;

    /**
    * �Ƿ񳬹���С,��һ���ļ������򳬹�
    */
    bool                        _bUploadFileOverSize;

    /**
    * �ϴ��ļ������Ϣ�����ڸ�map��
    */
    map<string, TC_Cgi_Upload>  _mpUpload;
};

}

#endif
