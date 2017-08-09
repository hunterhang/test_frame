#ifndef __TC_HTTP_ASYNC_H_
#define __TC_HTTP_ASYNC_H_

#include "tc_thread_pool.h"
#include "tc_http.h"
#include "tc_autoptr.h"
#include "tc_socket.h"
//#include "util/tc_timeoutQueue.h"

namespace taf
{

/////////////////////////////////////////////////
/** 
* @file tc_http_async.h 
* @brief http�첽������. 
*  
* httpͬ������ʹ��TC_HttpRequest::doRequest�Ϳ�����  
* @author jarodruan@tencent.com 
*/            
/////////////////////////////////////////////////


/**
* @brief �߳��쳣
*/
struct TC_HttpAsync_Exception : public TC_Exception
{
    TC_HttpAsync_Exception(const string &buffer) : TC_Exception(buffer){};
    TC_HttpAsync_Exception(const string &buffer, int err) : TC_Exception(buffer, err){};
    ~TC_HttpAsync_Exception() throw(){};
};

/**
 *  @brief �첽�̴߳�����.
 *  
 * �첽HTTP���� ��ʹ�÷�ʽʾ������: 
 *  
 *    ʵ���첽�ص�����
 *  
  *  �첽����ص�ִ�е�ʱ������TC_HttpAsync���߳�ִ�е�
 *  
 *   ������ָ��new����, ���ù���������
 *  
 *   class AsyncHttpCallback : public
 *  
 *    TC_HttpAsync::RequestCallback
 *  
 *  {
 *  
 *   public:
 *  
 *  	 AsyncHttpCallback(const string &sUrl) : _sUrl(sUrl)
 *  
  *     {
 *  
  *     }
 *  
 *  	virtual void onException(const string &ex)
 *  
 *     {
 *  
 *  	 cout << "onException:" << _sUrl << ":" << ex << endl;
 *  
  *	   }
 *  
 *     //���������ʱ��onResponse������
 *  
  *   //bClose��ʾ����˹ر������� ,�Ӷ���Ϊ�յ���һ��������http��Ӧ
  * 	
 *   virtual void onResponse(bool bClose, TC_HttpResponse
 *  
  * 	 &stHttpResponse)
 *  
  *  	 {
 *  
  *  		cout << "onResponse:" << _sUrl << ":" <<
 *  
  *  		TC_Common::tostr(stHttpResponse.getHeaders()) <<
 *  
  *  		endl;
  *  	}
  *  
  *  	virtual void onTimeout()
 *  
 *  	{
 *  
 *  		 cout << "onTimeout:" << _sUrl << endl;
 *  
  *      }
 *  
 *  
  *      //���ӱ��ر�ʱ����
 *  
  *      virtual void onClose()
 *  
 *  	 {
 *  
 *  		 cout << "onClose:" << _sUrl << endl;
 *  
 *  	 }
 *  
 *   protected:
 *  
 *  	 string _sUrl;
 *  
  *  };
  *
 *   //��װһ������, ����ʵ���������
 *  
 *  int addAsyncRequest(TC_HttpAsync &ast, const string &sUrl) {
 *  
 *  	TC_HttpRequest stHttpReq; stHttpReq.setGetRequest(sUrl);
 *  
  *
  *      //new����һ���첽�ص�����
 *  
  * 	 TC_HttpAsync::RequestCallbackPtr p = new
 *  
  * 	 AsyncHttpCallback(sUrl);
  *
  * 	 return ast.doAsyncRequest(stHttpReq, p);
 *  
  * 	 }
  *
  *  //����ʹ�õ�ʾ����������:
 *  
 *   TC_HttpAsync ast;
 *  
 *   ast.setTimeout(1000);  //�����첽����ʱʱ��
 *  
 *   ast.start();
 *
 *   //�����Ĵ�����Ҫ�жϷ���ֵ,����ֵ=0�ű�ʾ�����Ѿ����ͳ�ȥ��
 *  
 *   int ret = addAsyncRequest(ast, "www.baidu.com");    
 *
 *   addAsyncRequest(ast, "www.qq.com");
 *  
 *   addAsyncRequest(ast, "www.google.com");
 *  
 *   addAsyncRequest(ast, "http://news.qq.com/a/20100108/002269.htm");
 *  
 *   addAsyncRequest(ast, "http://news.qq.com/zt/2010/mtjunshou/");
 *  
 *   addAsyncRequest(ast,"http://news.qq.com/a/20100108/000884.htm");
 *  
 *   addAsyncRequest(ast,"http://news.qq.com/a/20100108/000884.htm");
 *  
 *   addAsyncRequest(ast,"http://tech.qq.com/zt/2009/renovate/index.htm");
 * 
 *   ast.waitForAllDone();
 *  
 *   ast.terminate(); 
 */
class TC_HttpAsync : public TC_Thread, public TC_ThreadLock
{
public:
    /**
     * @brief �첽����ص�����
     */
    class RequestCallback : public TC_HandleBase
    {
    public:
        /**
		 * @brief ��������Ӧ������. 
		 *  
         * @param bClose          ��ΪԶ�̷������ر�������Ϊhttp������
         * @param stHttpResponse  http��Ӧ��
         */
        virtual void onResponse(bool bClose, TC_HttpResponse &stHttpResponse) = 0;

        /**
		 * @brief ÿ���յ�������httpͷ��ȫ�˶�����ã� 
		 * stHttpResponse�����ݿ��ܲ�����ȫ��http��Ӧ���� ,ֻ�в���body���� 
         * @param stHttpResponse  �յ���http����
         * @return                true:������ȡ����, false:����ȡ������
         */
        virtual bool onReceive(TC_HttpResponse &stHttpResponse) { return true;};

        /**
		 * @brief �쳣. 
		 *  
         * @param ex �쳣ԭ��
         */
        virtual void onException(const string &ex) = 0;

        /**
         * @brief ��ʱû����Ӧ
         */
        virtual void onTimeout() = 0;

        /**
         * @brief ���ӱ��ر�
         */
        virtual void onClose() = 0;
    };

    typedef TC_AutoPtr<RequestCallback> RequestCallbackPtr;

protected:
    /**
     * @brief �첽http����(������)
     */
    class AsyncRequest : public TC_HandleBase
    {
    public:
        /**
		 * @brief ����. 
		 *  
         * @param stHttpRequest
         * @param callbackPtr
         */
        AsyncRequest(TC_HttpRequest &stHttpRequest, RequestCallbackPtr &callbackPtr);

        /**
         * @brief ����
         */
        ~AsyncRequest();

        /**
         * @brief ��ȡ���
         * 
         * @return int
         */
        int getfd() { return _fd.getfd(); }

        /**
         * @brief ����������.
         * 
         * @return int
         */
        int doConnect();

        /**
		 * @brief ��������addr������,����DNS����. 
		 *  
         * @param addr ������ֱ������͸�������������ͨ��DNS������ĵ�ַ
         * @return int
         */
        int doConnect(struct sockaddr* addr);

        /**
         * @brief �����쳣
         */
        void doException();

        /**
         * @brief ��������
         */
        void doRequest();

        /**
         * @brief ������Ӧ
         */
        void doReceive();

        /**
         * @brief �ر�����
         */
        void doClose();

        /**
         * @brief ��ʱ
         */
        void timeout();

        /**
		 * @brief ����ΨһID. 
		 *  
         * @param uniqId
         */
        void setUniqId(uint32_t uniqId)    { _iUniqId = uniqId;}

        /**
         * @brief ��ȡΨһID.
         * 
         * @return uint32_t
         */
        uint32_t getUniqId() const         { return _iUniqId; }
           
        /**
         * @brief ���ô��������http�첽�߳�.
         * 
         * @param pHttpAsync ���첽�̴߳������
         */
        void setHttpAsync(TC_HttpAsync *pHttpAsync) { _pHttpAsync = pHttpAsync; }

        /**
		 * @brief ���÷���������ʱ�󶨵�ip��ַ. 
		 *  
         * @param addr
         */
		void setBindAddr(const struct sockaddr* addr);

    protected:
        /**
		 * @brief ��������. 
		 *  
         * @param buf
         * @param len
		 * @param flag 
         * @return int
         */
        int recv(void* buf, uint32_t len, uint32_t flag);

        /**
		 * @brief ��������. 
		 *  
         * @param buf ��������
         * @param len ���ͳ���
		 * @param flag  
         * @return int
         */
        int send(const void* buf, uint32_t len, uint32_t flag);

    protected:
        TC_HttpAsync               *_pHttpAsync;
        TC_HttpResponse             _stHttpResp;
        TC_Socket                   _fd;
        string                      _sHost;
        uint32_t                    _iPort;
        uint32_t                    _iUniqId;
        string                      _sReq;
        string                      _sRsp;
        RequestCallbackPtr          _callbackPtr;
		bool						_bindAddrSet;
		struct sockaddr 			_bindAddr;
    };

    typedef TC_AutoPtr<AsyncRequest> AsyncRequestPtr;

public:

    typedef TC_TimeoutQueue<AsyncRequestPtr> http_queue_type;

    /**
     * @brief ���캯��
     */
    TC_HttpAsync();

    /**
     * @brief ��������
     */
    ~TC_HttpAsync();

    /**
	 * @brief �첽��������. 
	 *  
     * @param stHttpRequest
     * @param httpCallbackPtr
     * @param bUseProxy,�Ƿ�ʹ�ô���ʽ����
     * @param addr, bUseProxyΪfalse ֱ������ָ���ĵ�ַ 
     * @return int, <0:��������ʧ��, ����ͨ��strerror(����ֵ)
     *             =0:�ɹ�
     */
    int doAsyncRequest(TC_HttpRequest &stHttpRequest, RequestCallbackPtr &callbackPtr, bool bUseProxy=false, struct sockaddr* addr=NULL);

    /**
     * @brief ����proxy��ַ
     * 
     */
    int setProxyAddr(const char* Host,uint16_t Port);

    /**
	 * @brief ���ô���ĵ�ַ. 
	 *  
	 * ��ͨ��������������,ֱ�ӷ��͵������������ip��ַ) 
     * @param sProxyAddr ��ʽ 192.168.1.2:2345 ���� sslproxy.qq.com:2345
	 */
    int setProxyAddr(const char* sProxyAddr);

    /**
	 * @brief ���ð󶨵ĵ�ַ. 
	 *  
	 * @param sProxyAddr ��ʽ 192.168.1.2
	 */
	int setBindAddr(const char* sBindAddr);

    /**
	 * @brief ���ð󶨵ĵ�ַ. 
	 *  
	 * @param addr ֱ���� addr ��ֵ
	 */
	void setProxyAddr(const struct sockaddr* addr);

    /**
	 * @brief �����첽����. 
	 *  
     * �����Ѿ���Ч(���������ֻ��һ���߳�)
     * @param num, �첽������߳���
     */
    void start(int iThreadNum = 1);

    /**
	 * @brief ���ó�ʱ(��������ֻ����һ�ֳ�ʱʱ��). 
	 *  
     * @param timeout: ����, ���Ǿ���ĳ�ʱ����ֻ����s����
     */
    void setTimeout(int millsecond) { _data->setTimeout(millsecond); }

    /**
	 * @brief �ȴ�����ȫ������(�ȴ����뾫����100ms����). 
	 *  
     * @param millsecond, ���� -1��ʾ��Զ�ȴ�
     */
    void waitForAllDone(int millsecond = -1);

    /**
     * @brief �����߳�
     */
    void terminate();

protected:

    typedef TC_Functor<void, TL::TLMaker<AsyncRequestPtr, int>::Result> async_process_type;

    /**
	 * @brief ��ʱ����. 
	 *  
     * @param ptr
     */
    static void timeout(AsyncRequestPtr& ptr);

    /**
     * @brief �������紦��
     */
    static void process(AsyncRequestPtr &p, int events);

    /**
     * @brief ��������紦���߼�
     */
    void run() ;

    /**
     * @brief ɾ���첽�������
     */
    void erase(uint32_t uniqId); 

    friend class AsyncRequest;

protected:
    TC_ThreadPool               _tpool;

    http_queue_type             *_data;
    
    TC_Epoller                  _epoller;

    bool                        _terminate;

    struct sockaddr             _proxyAddr;

    struct sockaddr             _bindAddr;

	bool			            _bindAddrSet;
};

}
#endif

