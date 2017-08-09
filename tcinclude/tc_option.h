#ifndef __TC_OPTION_H
#define __TC_OPTION_H

#include <map>
#include <vector>
#include <string>
#include <sstream>
using namespace std;

namespace taf
{
/////////////////////////////////////////////////
/** 
 * @file tc_option.h 
 * @brief �����в���������.
 *  
 * @author jarodruan@tencent.com 
 */
            
/////////////////////////////////////////////////
/**
 * @brief ��������࣬ͨ�����ڽ��������в���
 *
 * ֧��������ʽ�Ĳ���:  ./main.exe --name=value --with abc def 
 */
class TC_Option
{
public:
    /**
     * @brief ���캯��
     */
    TC_Option(){};

    /**
	 * @brief ����. 
	 *  
     * @param argc ��������
     * @param argv ��������
     *
     */
    void decode(int argc, char *argv[]);

    /**
	 * @brief �Ƿ����ĳ��--��ʶ�Ĳ���. 
	 *  
	 * @param sName  Ҫ�жϵı�ʶ
     * @return bool ���ڷ���true�����򷵻�false
     */
    bool hasParam(const string &sName);

    /**
	 * @brief ��ȡĳ��--��ʾ�Ĳ�����������������ڻ��߲���ֵΪ�� , 
	 *  	  ������""
	 * @param sName   ��ʶ
     * @return string ��ʶ�Ĳ���ֵ
     */
    string getValue(const string &sName);

    /**
     * @brief ��ȡ����--��ʶ�Ĳ���.
     *
     * @return map<string,string> map���͵ı�ʶ�Ͳ���ֵ�Ķ�Ӧ��ϵ
     */
    map<string, string>& getMulti();

    /**
	 * @brief ��ȡ������ͨ�Ĳ���, �����е�abc, 
	 *  	  def����������˳����vector��
     * @return vector<string> ˳���Ų�����vector
     */
    vector<string>& getSingle();

protected:

    /**
	 * @brief �����ַ�����ȡ����ʶ�����Ӧ�Ĳ���ֵ�� 
	 *  	  ������--name=value ���ַ������н�����ȡ��name��vaue
     * @param s Ҫ�������ַ���
     */
    void parse(const string &s);

protected:
    /**
     *��ű�ʶ�����Ӧ�����Ķ�Ӧ��ϵ�����磺����--name=value�����name��value
     */
    map<string, string> _mParam;

    /**
     *�����ͨ������vetor
     */
    vector<string>      _vSingle;
};

}

#endif

