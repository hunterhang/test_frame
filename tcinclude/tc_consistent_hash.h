#ifndef __CONSISTENT_HASH__
#define __CONSISTENT_HASH__

#include "tc_md5.h"

using namespace std;

namespace taf
{

/////////////////////////////////////////////////
/**
 * @file tc_consistent_hash.h
 * @brief һ����hash�㷨��.
 *
 * @author  devinchen@tencent.com
 */
/////////////////////////////////////////////////

struct node_T
{
	/**
	 *�ڵ�hashֵ
	 */
	unsigned int iHashCode;

	/**
	 *�ڵ��±�
	 */
	unsigned int iIndex;
};



/**
 *  @brief һ����hash�㷨��
 */
class  TC_ConsistentHash
{
    public:

		/**
		 *  @brief ���캯��
		 */
        TC_ConsistentHash()
        {
        }

        /**
         * @brief �ڵ�Ƚ�.
         *
         * @param m1 node_T���͵Ķ��󣬱ȽϽڵ�֮һ
		 * @param m2 node_T���͵Ķ��󣬱ȽϽڵ�֮һ
         * @return less or not �ȽϽ����less����ture�����򷵻�false
         */
        static bool less_hash(const node_T & m1, const node_T & m2)
        {
            return m1.iHashCode < m2.iHashCode;
        }

        /**
         * @brief ���ӽڵ�.
         *
         * @param node  �ڵ�����
		 * @param index �ڵ���±�ֵ
         * @return      �ڵ��hashֵ
         */
        unsigned addNode(const string & node, unsigned int index)
        {
            node_T stItem;
            stItem.iHashCode = hash_md5(TC_MD5::md5bin(node));
            stItem.iIndex = index;
            vHashList.push_back(stItem);

            sort(vHashList.begin(), vHashList.end(), less_hash);

            return stItem.iHashCode;
        }

        /**
         * @brief ɾ���ڵ�.
         *
		 * @param node  �ڵ�����
         * @return       0 : ɾ���ɹ�  -1 : û�ж�Ӧ�ڵ�
         */
        int removeNode(const string & node)
        {
            unsigned iHashCode = hash_md5(TC_MD5::md5bin(node));
            vector<node_T>::iterator it;
            for(it=vHashList.begin() ; it!=vHashList.end(); it++)
            {
                if(it->iHashCode == iHashCode)
                {
                    vHashList.erase(it);
                    return 0;
                }
            }
            return -1;
        }

        /**
         * @brief ��ȡĳkey��Ӧ���Ľڵ�node���±�.
         *
         * @param key      key����
		 * @param iIndex  ��Ӧ���Ľڵ��±�
         * @return        0:��ȡ�ɹ�   -1:û�б���ӵĽڵ�
         */
        int getIndex(const string & key, unsigned int & iIndex)
        {
            unsigned iCode = hash_md5(TC_MD5::md5bin(key));
            if(vHashList.size() == 0)
            {
                iIndex = 0;
                return -1;
            }

            int low = 0;
            int high = vHashList.size();

            if(iCode <= vHashList[0].iHashCode || iCode > vHashList[high-1].iHashCode)
            {
                iIndex = vHashList[0].iIndex;
                return 0;
            }

            while (low < high - 1)
            {
                int mid = (low + high) / 2;
                if (vHashList[mid].iHashCode > iCode)
                {
                    high = mid;
                }
                else
                {
                    low = mid;
                }
            }
            iIndex = vHashList[low+1].iIndex;
            return 0;
        }

   protected:
        /**
         * @brief ����md5ֵ��hash���ֲ���Χ�� 0 -- 2^32-1.
         *
		 * @param  sMd5 md5ֵ
         * @return      hashֵ
         */
        unsigned int hash_md5(const string & sMd5)
        {
            char *p = (char *) sMd5.c_str();
            return (*(int*)(p)) ^ (*(int*)(p+4)) ^ (*(int*)(p+8)) ^ (*(int*)(p+12));
        }

        vector<node_T> vHashList;

};

}
#endif
