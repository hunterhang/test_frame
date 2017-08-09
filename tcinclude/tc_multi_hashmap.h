#ifndef	__TC_MULTI_HASHMAP_H__
#define __TC_MULTI_HASHMAP_H__

#include <vector>
#include <memory>
#include <cassert>
#include <iostream>
#include "tc_ex.h"
#include "tc_mem_vector.h"
#include "tc_pack.h"
#include "tc_mem_chunk.h"
#include "tc_functor.h"
#include "tc_hash_fun.h"

namespace taf
{
/////////////////////////////////////////////////
/** 
 * @file tc_multi_hashmap.h  
 * @brief ֧�ֶ�key��hashmap��. 
 *  
 * @author hendysu@tencent.com
 */
/////////////////////////////////////////////////

/**
*  @brief Multi Hash map�쳣��
*/
struct TC_Multi_HashMap_Exception : public TC_Exception
{
	TC_Multi_HashMap_Exception(const string &buffer) : TC_Exception(buffer){};
    TC_Multi_HashMap_Exception(const string &buffer, int err) : TC_Exception(buffer, err){};
    ~TC_Multi_HashMap_Exception() throw(){};
};

////////////////////////////////////////////////////////////////////////////////////
/**
 *  @brief �����ڴ��֧�ֶ�key��hashmap.
 *  
 * ���в�����Ҫ�Լ����� 
 *  
 * ���д洢�ĵ�ַ������32λ���棬Ϊ�ڴ���������Ҫ�����ڴ�������ܳ���32λ��Χ
 */
class TC_Multi_HashMap
{
public:
    struct HashMapIterator;
    struct HashMapLockIterator;
	/**
	* @brief �����洢�����ݽṹ
	*/
    struct BlockData
    {
        string			_key;       /**����Key������key(��������ȥ����key��)*/
        string			_value;     /**����value*/
        bool			_dirty;     /**�Ƿ���������*/
		uint8_t			_iVersion;	/**���ݰ汾��1Ϊ��ʼ�汾��0Ϊ����*/
        time_t			_synct;     /**sync time, ��һ���������Ļ�дʱ��*/
        BlockData()
        : _dirty(false)
		, _iVersion(1)
        , _synct(0)
        {
        }
    };

	/**
	* @brief ���������ݽṹ����Ϊget�ķ���ֵ
	*/
	struct Value
	{
		string			_mkey;		/**��key*/
		BlockData		_data;		/**��������*/
	};

	/**
	* @brief �ж�ĳ����־λ�Ƿ��Ѿ�����
	* @param bitset, Ҫ�жϵ��ֽ�
	* @param bw, Ҫ�жϵ�λ
	*/
	static bool ISSET(uint8_t bitset, uint8_t bw) { return bool((bitset & (0x01 << bw)) >> bw); }
	
	/**
	* @brief ����ĳ����־λ
	*/
	static void SET(uint8_t &iBitset, uint8_t bw)
	{
		iBitset |= 0x01 << bw;
	}
	
	/**
	* @brief ���ĳ����־λ
	*/
	static void UNSET(uint8_t &iBitset, uint8_t bw)
	{
		iBitset &= 0xFF ^ (0x01 << bw);
	}

	//////////////////////////////////////////////////////////////////////////
	/**
	 * @brief ��key���ݿ�
	 */
	class MainKey
	{
		/**
		* @brief ͷ����bitwiseλ������
		*/
		enum BITWISE
		{
			NEXTCHUNK_BIT = 0,		/**�Ƿ�����һ��chunk*/
			INTEGRITY_BIT,			/**��key�µ������Ƿ�����*/
		};

	public:
		/** 
		  * @brief ��keyͷ
		  */
		struct tagMainKeyHead
		{
            uint32_t	_iSize;         /**������С*/
			uint32_t	_iIndex;		/**��key hash����*/
			uint32_t	_iAddr;			/**��key���������׵�ַ*/
			uint32_t	_iNext;			/**��key������һ����key*/
			uint32_t	_iPrev;			/**��key������һ����key*/
			uint32_t	_iGetNext;		/**��key Get������һ����key*/
			uint32_t	_iGetPrev;		/**��key Get������һ����key*/
			uint32_t	_iBlockCount;	/**��key�����ݸ���*/
            uint8_t		_iBitset;		/** 8��bit�����ڱ�ʶ��ͬ��boolֵ����bit�ĺ����BITWISEö�ٶ���*/
            union
            {
                uint32_t	_iNextChunk;    /** ��һ��Chunk���ַ, _bNextChunk=trueʱ��Ч*/
                uint32_t	_iDataLen;      /** ��ǰ���ݿ���ʹ���˵ĳ���, _bNextChunk=falseʱ��Ч*/
            };
            char			_cData[0];      /** ���ݿ�ʼ��ַ*/
			tagMainKeyHead()
				: _iSize(0)
				, _iIndex(0)
				, _iAddr(0)
				, _iNext(0)
				, _iPrev(0)
				, _iGetNext(0)
				, _iGetPrev(0)
				, _iBlockCount(0)
				, _iBitset(0)
				, _iDataLen(0)
			{
				_cData[0] = 0;
			}
		}__attribute__((packed));

	     /**
		 * @brief 
		 *  	  һ��chunk�Ų������ݣ�����ҽ�����chunk,�ǵ�һ��chunk��chunkͷ��
         */
        struct tagChunkHead
        {
            uint32_t    _iSize;        /** ��ǰchunk��������С*/
            bool        _bNextChunk;    /** �Ƿ�����һ��chunk*/
            union
            {
                uint32_t  _iNextChunk;    /** ��һ�����ݿ��ַ, _bNextChunk=trueʱ��Ч*/
                uint32_t  _iDataLen;      /** ��ǰ���ݿ���ʹ���˵ĳ���, _bNextChunk=falseʱ��Ч*/
            };
            char        _cData[0];      /** ���ݿ�ʼ��ַ*/

			tagChunkHead()
			:_iSize(0)
			,_bNextChunk(false)
			,_iDataLen(0)
			{
				_cData[0] = 0;
			}

        }__attribute__((packed));

        /**
         * @brief ���캯��
         * @param pMap
         * @param iAddr, ��key�ĵ�ַ
         */
        MainKey(TC_Multi_HashMap *pMap, uint32_t iAddr)
        : _pMap(pMap)
        , _iHead(iAddr)
        {
			// ����ʱ������Ե�ַ������ÿ�ζ�����
			_pHead = _pMap->getAbsolute(iAddr);
        }

        /**
         * @brief �������캯��
         * @param mk
         */
        MainKey(const MainKey &mk)
        : _pMap(mk._pMap)
        , _iHead(mk._iHead)
		, _pHead(mk._pHead)
        {
        }

        /**
         * @brief ��ֵ������
         * @param mk
         *
         * @return Block&
         */
        MainKey& operator=(const MainKey &mk)
        {
            _iHead = mk._iHead;
            _pMap  = mk._pMap;
			_pHead = mk._pHead;
            return (*this);
        }

        /**
         *
         * @param mb
         *
         * @return bool
         */
        bool operator==(const MainKey &mk) const { return _iHead == mk._iHead && _pMap == mk._pMap; }

        /**
         *
         * @param mb
         *
         * @return bool
         */
        bool operator!=(const MainKey &mk) const { return _iHead != mk._iHead || _pMap != mk._pMap; }

        /**
         * @brief ��ȡ��keyͷָ��
         *
         * @return tagMainKeyHead*
         */
        tagMainKeyHead* getHeadPtr() { return (tagMainKeyHead *)_pHead; }


        /**
         * @brief ������keyͷ��ַ��ȡ��keyͷָ��
         * @param iAddr, ��keyͷ��ַ
         * @return tagMainKeyHead*
         */
		tagMainKeyHead* getHeadPtr(uint32_t iAddr) { return ((tagMainKeyHead*)_pMap->getAbsolute(iAddr)); }

        /**
         * @brief ��ȡ��keyͷ�ĵ�ַ
         *
         * @return uint32_t
         */
        uint32_t getHead() { return _iHead; }
        
        /**
         * @brief ��ȡ��key
         * @param mk, ��key
         * @return int
         *          TC_Multi_HashMap::RT_OK, ����
         *          �����쳣
         */
        int get(string &mk);

        /**
         * @brief ������key
         * @param pData
         * @param iDatalen
         * @param vtData, ��̭������
         */
        int set(const void *pData, uint32_t iDataLen, vector<TC_Multi_HashMap::Value> &vtData);

		/**
         * @brief ����ǰ��key�ƶ�����key���ϵ���һ����key
         * @return true, �Ƶ���һ����key��, false, û����һ����key
         *
         */
        bool next();

        /**
         * @brief ����ǰ��key�ƶ�����key���ϵ���һ����key
         * @return true, �Ƶ���һ����key��, false, û����һ����key
         *
         */
        bool prev();

		/**
         * @brief �ͷ����пռ�
         */
        void deallocate();

        /**
         * @brief ����keyʱ���øú�������ʼ����key�����Ϣ
		 * @param iIndex, ��key hash����
         * @param iAllocSize, �ڴ��С
         */
        void makeNew(uint32_t iIndex, uint32_t iAllocSize);

        /**
         * @brief ����key������ɾ����ǰ��key
         * ���ر�ɾ������key�µ���������
         * @return int, TC_Multi_HashMap::RT_OK�ɹ�������ʧ��
         */
        int erase(vector<Value> &vtData);

        /**
         * @brief ˢ��get����, ����ǰ��key����get����ͷ��
         */
        void refreshGetList();

    protected:

        /**
         * @brief ��ȡchunkͷָ��
         * @param iAddr, chunkͷ��ַ����
         *
         * @return tagChunkHead*
         */
        tagChunkHead *getChunkHead(uint32_t iAddr) { return ((tagChunkHead*)_pMap->getAbsolute(iAddr)); }

		/**
         * @brief ���������������, ��������chunk, ��Ӱ��ԭ������
         * ʹ�����ӵ�����������iDataLen
         * �ͷŶ����chunk
         * @param iDataLen
		 * @param vtData, ���ر���̭������
         *
         * @return int
         */
        int allocate(uint32_t iDataLen, vector<TC_Multi_HashMap::Value> &vtData);

        /**
         * @brief �ҽ�chunk, ���core��ҽ�ʧ��, ��֤�ڴ�黹������
         * @param pChunk, ��һ��chunkָ��
         * @param chunks, ���е�chunk��ַ
         *
         * @return int
         */
        int joinChunk(tagChunkHead *pChunk, const vector<uint32_t>& chunks);

        /**
         * @brief ����ָ����С���ڴ�ռ䣬���ܻ��ж��chunk
         * @param fn, ����Ŀռ��С
         * @param chunks, ����ɹ����ص�chunks��ַ�б�
         * @param vtData, ��̭������
         * @return int
         */
        int allocateChunk(uint32_t fn, vector<uint32_t> &chunks, vector<TC_Multi_HashMap::Value> &vtData);

		/**
         * @brief �ͷ�ָ��chunk֮�������chunk
         * @param iChunk �ͷŵ�ַ
         */
        void deallocate(uint32_t iChunk);

        /**
         * @brief ��ȡ��key�洢�ռ��С
         *
         * @return uint32_t
         */
        uint32_t getDataLen();

		/**
         * @brief ��ȡ��key
         * @param pData
         * @param iDatalen
         * @return int,
         *          TC_Multi_HashMap::RT_OK, ����
         *          �����쳣
         */
        int get(void *pData, uint32_t &iDataLen);

	public:
		bool ISFULLDATA()						{ return TC_Multi_HashMap::ISSET(getHeadPtr()->_iBitset, INTEGRITY_BIT); }
	protected:
		bool HASNEXTCHUNK()						{ return TC_Multi_HashMap::ISSET(getHeadPtr()->_iBitset, NEXTCHUNK_BIT); }
		
	public:
		void SETFULLDATA(bool b)				{ if(b) TC_Multi_HashMap::SET(getHeadPtr()->_iBitset, INTEGRITY_BIT); else TC_Multi_HashMap::UNSET(getHeadPtr()->_iBitset, INTEGRITY_BIT); }
	protected:
		void SETNEXTCHUNK(bool b)				{ if(b) TC_Multi_HashMap::SET(getHeadPtr()->_iBitset, NEXTCHUNK_BIT); else TC_Multi_HashMap::UNSET(getHeadPtr()->_iBitset, NEXTCHUNK_BIT); }

    private:

        /**
         * Map
         */
        TC_Multi_HashMap         *_pMap;

        /**
         * ��keyͷ�׵�ַ, ��Ե�ַ���ڴ������
         */
        uint32_t				_iHead;

		/**
		* ��keyͷ�׵�ַ�����Ե�ַ
		*/
		void					*_pHead;

	};

    ///////////////////////////////////////////////////////////////////////////////////
    /**
    * @brief ������key�����ݿ�
    */
    class Block
    {
    public:

		/**
		* @brief blockͷ����bitwiseλ������
		*/
		enum BITWISE
		{
			NEXTCHUNK_BIT = 0,		/**�Ƿ�����һ��chunk*/
			DIRTY_BIT,				/**�Ƿ�Ϊ������λ*/
			ONLYKEY_BIT,			/**�Ƿ�ΪOnlyKey*/
		};

        /**
         * @brief block����ͷ
         */
        struct tagBlockHead
        {
            uint32_t		_iSize;         /**block��������С*/
            uint32_t		_iIndex;        /**hash������*/
            uint32_t		_iUKBlockNext;  /**��������block������һ��Block, û����Ϊ0*/
            uint32_t		_iUKBlockPrev;  /**��������block������һ��Block, û����Ϊ0*/
            uint32_t		_iMKBlockNext;  /**��key block������һ��Block, û����Ϊ0*/
            uint32_t		_iMKBlockPrev;  /**��key block������һ��Block, û����Ϊ0*/
            uint32_t		_iSetNext;      /**Set���ϵ���һ��Block, û����Ϊ0*/
            uint32_t		_iSetPrev;      /**Set���ϵ���һ��Block, û����Ϊ0*/
			uint32_t		_iMainKey;		/**ָ��������keyͷ*/
            time_t			_iSyncTime;     /**�ϴλ�дʱ��*/
			uint8_t			_iVersion;		/**���ݰ汾��1Ϊ��ʼ�汾��0Ϊ����*/
			uint8_t			_iBitset;		/**8��bit�����ڱ�ʶ��ͬ��boolֵ����bit�ĺ����BITWISEö�ٶ���*/
            union
            {
                uint32_t	_iNextChunk;    /**��һ��Chunk��, _iBitwise�е�NEXTCHUNK_BITΪ1ʱ��Ч*/
                uint32_t	_iDataLen;      /**��ǰ���ݿ���ʹ���˵ĳ���, _iBitwise�е�NEXTCHUNK_BITΪ0ʱ��Ч*/
            };
            char			_cData[0];      /**���ݿ�ʼ����*/

			tagBlockHead()
			:_iSize(0)
			,_iIndex(0)
			,_iUKBlockNext(0)
			,_iUKBlockPrev(0)
			,_iMKBlockNext(0)
			,_iMKBlockPrev(0)
			,_iSetNext(0)
			,_iSetPrev(0)
			,_iMainKey(0)
			,_iSyncTime(0)
			,_iVersion(1)
			,_iBitset(0)
			,_iDataLen(0)
			{
				_cData[0] = 0;
			}

        }__attribute__((packed));

        /**
		 * @brief 
		 *  	  һ��chunk�Ų����������ݣ�����ҽ�����chunk,����chunkͷ��
         */
        struct tagChunkHead
        {
            uint32_t    _iSize;          /**��ǰchunk��������С*/
            bool        _bNextChunk;     /**�Ƿ�����һ��chunk*/
            union
            {
                uint32_t  _iNextChunk;     /**��һ��chunk, _bNextChunk=trueʱ��Ч*/
                uint32_t  _iDataLen;       /**��ǰchunk��ʹ���˵ĳ���, _bNextChunk=falseʱ��Ч*/
            };
            char        _cData[0];      /**���ݿ�ʼ����*/

			tagChunkHead()
			:_iSize(0)
			,_bNextChunk(false)
			,_iDataLen(0)
			{
				_cData[0] = 0;
			}

        }__attribute__((packed));

        /**
         * @brief ���캯��
         * @param Map
         * @param Block�ĵ�ַ
         * @param pAdd
         */
        Block(TC_Multi_HashMap *pMap, uint32_t iAddr)
        : _pMap(pMap)
        , _iHead(iAddr)
        {
			// ����ʱ������Ե�ַ������ÿ�ζ�����
			_pHead = _pMap->getAbsolute(iAddr);
        }

        /**
         * @brief �������캯��
         * @param mb
         */
        Block(const Block &mb)
        : _pMap(mb._pMap)
        , _iHead(mb._iHead)
		, _pHead(mb._pHead)
        {
        }

        /**
         * @brief  ��ֵ�����
         * @param mb
         *
         * @return Block&
         */
        Block& operator=(const Block &mb)
        {
            _iHead = mb._iHead;
            _pMap  = mb._pMap;
			_pHead = mb._pHead;
            return (*this);
        }

        /**
         *
         * @param mb
         *
         * @return bool
         */
        bool operator==(const Block &mb) const { return _iHead == mb._iHead && _pMap == mb._pMap; }

        /**
         *
         * @param mb
         *
         * @return bool
         */
        bool operator!=(const Block &mb) const { return _iHead != mb._iHead || _pMap != mb._pMap; }

        /**
         * @brief ��ȡBlockͷָ��
         *
         * @return tagBlockHead*
         */
        tagBlockHead *getBlockHead() {return (tagBlockHead*)_pHead; }

        /**
         * @brief ����ͷ��ַ��ȡBlockͷָ��
         * @param iAddr, blockͷ��ַ
         * @return tagBlockHead*
         */
        tagBlockHead *getBlockHead(uint32_t iAddr) { return ((tagBlockHead*)_pMap->getAbsolute(iAddr)); }

        /**
         * @brief ��ȡͷ����ַ
         *
         * @return size_t
         */
        uint32_t getHead() { return _iHead;}

        /**
         * @brief ��ȡ��ǰͰ�������һ��block��ͷ����ַ
         * @param bUKList, ������������������key��
         * @return uint32_t
         */
        uint32_t getLastBlockHead(bool bUKList);

        /**
         * @brief ��ȡ��дʱ��
         *
         * @return time_t
         */
        time_t getSyncTime() { return getBlockHead()->_iSyncTime; }

        /**
         * @brief ���û�дʱ��
         * @param iSyncTime
         */
        void setSyncTime(time_t iSyncTime) { getBlockHead()->_iSyncTime = iSyncTime; }

		/**
		* @brief ��ȡ���ݰ汾
		*/
		uint8_t getVersion() { return getBlockHead()->_iVersion; }

		/**
		* @brief �������ݰ汾
		*/
		void setVersion(uint8_t iVersion) { getBlockHead()->_iVersion = iVersion; }

        /**
         * @brief ��ȡBlock�е�����
         * @param data Ҫ��ȡ���ݵ�Block
         * @return int
         *          TC_Multi_HashMap::RT_OK, ����, �����쳣
         *          TC_Multi_HashMap::RT_ONLY_KEY, ֻ��Key
         *          �����쳣
         */
        int getBlockData(TC_Multi_HashMap::BlockData &data);

        /**
         * @brief ��ȡԭʼ����
         * @param pData
         * @param iDatalen
         * @return int,
         *          TC_Multi_HashMap::RT_OK, ����
         *          �����쳣
         */
        int get(void *pData, uint32_t &iDataLen);

        /**
         * @brief ��ȡԭʼ����
         * @param s
         * @return int
         *          TC_Multi_HashMap::RT_OK, ����
         *          �����쳣
         */
        int get(string &s);

        /**
         * @brief ��������
         * @param pData
         * @param iDatalen
		 * @param bOnlyKey
		 * @param iVersion, ���ݰ汾��Ӧ�ø���get�������ݰ汾д�أ�Ϊ0��ʾ���������ݰ汾
         * @param vtData, ��̭������
		 * @return int
		 *				RT_OK, ���óɹ�
		 *				RT_DATA_VER_MISMATCH, Ҫ���õ����ݰ汾�뵱ǰ�汾������Ӧ������get����set
		 *				����Ϊʧ��
         */
        int set(const void *pData, uint32_t iDataLen, bool bOnlyKey, uint8_t iVersion, vector<TC_Multi_HashMap::Value> &vtData);

        /**
         * @brief �Ƿ���������
         *
         * @return bool
         */
        bool isDirty()      { return ISDIRTY(); }

        /**
         * @brief ��������
         * @param b
         */
        void setDirty(bool b);

        /**
         * @brief �Ƿ�ֻ��key
         *
         * @return bool
         */
        bool isOnlyKey()    { return ISONLYKEY(); }

        /**
         * @brief ��ǰԪ���ƶ�����������block������һ��block
         * @return true, �Ƶ���һ��block��, false, û����һ��block
         *
         */
        bool nextBlock();

        /**
         * @brief ��ǰԪ���ƶ�����������block������һ��block
         * @return true, �Ƶ���һ��block��, false, û����һ��block
         *
         */
        bool prevBlock();

		/**
         * @brief �ͷ�block�����пռ�
         */
        void deallocate();

        /**
         * @brief ��blockʱ���øú���
         * ��ʼ����block��һЩ��Ϣ
		 * @param iMainKeyAddr, ������key��ַ
         * @param uIndex, ��������hash����
         * @param iAllocSize, �ڴ��С
		 * @param bHead, ���뵽��key���ϵ�˳��ǰ������
         */
        void makeNew(uint32_t iMainKeyAddr, uint32_t uIndex, uint32_t iAllocSize, bool bHead);

        /**
         * @brief ��Block������ɾ����ǰBlock
         * @return
         */
        void erase();

        /**
         * @brief ˢ��set����, ����ǰblock����Set����ͷ��
         */
        void refreshSetList();

    protected:

        /**
         * @brief ����chunkͷ��ַ��ȡchunkͷָ��
         * @param iAddr
         *
         * @return tagChunkHead*
         */
        tagChunkHead *getChunkHead(uint32_t iAddr) { return ((tagChunkHead*)_pMap->getAbsolute(iAddr)); }

		/**
         * @brief ���������������, ��������chunk, ��Ӱ��ԭ������
		 * ʹ�����ӵ�����������iDataLen���ͷŶ����chunk 
         * @param iDataLen
		 * @param vtData, ��̭������
         *
         * @return int,
         */
        int allocate(uint32_t iDataLen, vector<TC_Multi_HashMap::Value> &vtData);

        /**
         * @brief �ҽ�chunk, ���core��ҽ�ʧ��, ��֤�ڴ�黹������
         * @param pChunk, ��һ��chunkָ��
         * @param chunks, ���е�chunk��ַ
         *
         * @return int
         */
        int joinChunk(tagChunkHead *pChunk, const vector<uint32_t>& chunks);

        /**
         * @brief ����ָ����С���ڴ�ռ�, ���ܻ��ж��chunk
         * @param fn, ����Ŀռ��С
         * @param chunks, ����ɹ����ص�chunks��ַ�б�
         * @param vtData, ��̭������
         * @return int
         */
        int allocateChunk(uint32_t fn, vector<uint32_t> &chunks, vector<TC_Multi_HashMap::Value> &vtData);

		/**
         * @brief �ͷ�ָ��chunk֮�������chunk
         * @param iChunk �ͷŵ�ַ
         */
        void deallocate(uint32_t iChunk);

        /**
         * @brief ��ȡ���ݳ���
         *
         * @return size_t
         */
        uint32_t getDataLen();

		bool HASNEXTCHUNK()						{ return TC_Multi_HashMap::ISSET(getBlockHead()->_iBitset, NEXTCHUNK_BIT); }
		bool ISDIRTY()							{ return TC_Multi_HashMap::ISSET(getBlockHead()->_iBitset, DIRTY_BIT); }
		bool ISONLYKEY()						{ return TC_Multi_HashMap::ISSET(getBlockHead()->_iBitset, ONLYKEY_BIT); }

		void SETNEXTCHUNK(bool b)				{ if(b) TC_Multi_HashMap::SET(getBlockHead()->_iBitset, NEXTCHUNK_BIT); else TC_Multi_HashMap::UNSET(getBlockHead()->_iBitset, NEXTCHUNK_BIT); }
		void SETDIRTY(bool b)					{ if(b) TC_Multi_HashMap::SET(getBlockHead()->_iBitset, DIRTY_BIT); else TC_Multi_HashMap::UNSET(getBlockHead()->_iBitset, DIRTY_BIT); }
		void SETONLYKEY(bool b)					{ if(b) TC_Multi_HashMap::SET(getBlockHead()->_iBitset, ONLYKEY_BIT); else TC_Multi_HashMap::UNSET(getBlockHead()->_iBitset, ONLYKEY_BIT); }

    private:

        /**
         * Map
         */
        TC_Multi_HashMap         *_pMap;

        /**
         * block�����׵�ַ, ��Ե�ַ���ڴ������
         */
        uint32_t				_iHead;

		/**
		* Block�׵�ַ�����Ե�ַ
		*/
		void					*_pHead;

    };

    ////////////////////////////////////////////////////////////////////////
    /**
    * @brief �ڴ����ݿ����������ͬʱΪ����������key�������ڴ�
    *
    */
    class BlockAllocator
    {
    public:

        /**
         * @brief ���캯��
         */
        BlockAllocator(TC_Multi_HashMap *pMap)
        : _pMap(pMap)
        , _pChunkAllocator(new TC_MemMultiChunkAllocator())
        {
        }

        /**
         * @brief ��������
         */
        ~BlockAllocator()
        {
            if(_pChunkAllocator != NULL)
            {
                delete _pChunkAllocator;
            }
            _pChunkAllocator = NULL;
        }


        /**
         * @brief ��ʼ��
         * @param pHeadAddr, ��ַ, ����Ӧ�ó���ľ��Ե�ַ
         * @param iSize, �ڴ��С
         * @param iMinBlockSize, ��С���ݿ��С
         * @param iMaxBlockSize, ������ݿ��С
         * @param fFactor, ���ݿ���������
         */
        void create(void *pHeadAddr, size_t iSize, size_t iMinBlockSize, size_t iMaxBlockSize, float fFactor)
        {
            _pChunkAllocator->create(pHeadAddr, iSize, iMinBlockSize, iMaxBlockSize, fFactor);
        }

        /**
         * @brief ���ӵ��Ѿ��ṹ�����ڴ�(�繲���ڴ�)
         * @param pAddr, ��ַ, ����Ӧ�ó���ľ��Ե�ַ
         */
        void connect(void *pHeadAddr)
        {
            _pChunkAllocator->connect(pHeadAddr);
        }

        /**
         * @brief ��չ�ռ�
         * @param pAddr
         * @param iSize
         */
        void append(void *pAddr, size_t iSize)
        {
            _pChunkAllocator->append(pAddr, iSize);
        }

        /**
         * @brief �ؽ��ڴ�ṹ
         */
        void rebuild()
        {
            _pChunkAllocator->rebuild();
        }

        /**
         * @brief ��ȡÿ�����ݿ�ͷ����Ϣ
         *
         * @return vector<TC_MemChunk::tagChunkHead>
         */
        vector<TC_MemChunk::tagChunkHead> getBlockDetail() const  { return _pChunkAllocator->getBlockDetail(); }

        /**
         * @brief ���ڴ��С
         *
         * @return size_t
         */
        size_t getMemSize() const       { return _pChunkAllocator->getMemSize(); }

        /**
         * @brief ʵ�ʵ���������
         *
         * @return size_t
         */
        size_t getCapacity() const      { return _pChunkAllocator->getCapacity(); }

        /**
         * @brief ÿ��block�е�chunk����(ÿ��block�е�chunk������ͬ)
         *
         * @return vector<size_t>
         */
        vector<size_t> singleBlockChunkCount() const { return _pChunkAllocator->singleBlockChunkCount(); }

        /**
         * @brief ����block��chunk����
         *
         * @return size_t
         */
        size_t allBlockChunkCount() const    { return _pChunkAllocator->allBlockChunkCount(); }

        /**
		 * @brief ���ڴ��з���һ���µ�Block��ʵ����ֻ����һ��chunk�� 
		 *  	  ����ʼ��Blockͷ
         * @param iMainKeyAddr, ��block������key��ַ
         * @param index, block hash����
		 * @param bHead, �¿���뵽��key���ϵ�˳��ǰ������
         * @param iAllocSize: in/��Ҫ����Ĵ�С, out/����Ŀ��С
         * @param vtData, ������̭������
         * @return size_t, �ڴ���ַ����, 0��ʾû�пռ���Է���
         */
        uint32_t allocateMemBlock(uint32_t iMainKeyAddr, uint32_t index, bool bHead, uint32_t &iAllocSize, vector<TC_Multi_HashMap::Value> &vtData);

		/**
		* @brief ���ڴ��з���һ����keyͷ��ֻ��Ҫһ��chunk����
		* @param index, ��key hash����
		* @param iAllocSize: in/��Ҫ����Ĵ�С, out/����Ŀ��С
		* @param vtData, �����ͷŵ��ڴ������
		* @return size_t, ��keyͷ�׵�ַ,0��ʾû�пռ���Է���
		*/
		uint32_t allocateMainKeyHead(uint32_t index, vector<TC_Multi_HashMap::Value> &vtData);

        /**
         * @brief Ϊ��ַΪiAddr��Block����һ��chunk         *
         * @param iAddr,�����Block�ĵ�ַ
         * @param iAllocSize, in/��Ҫ����Ĵ�С, out/����Ŀ��С
         * @param vtData �����ͷŵ��ڴ������
         * @return size_t, ��Ե�ַ,0��ʾû�пռ���Է���
         */
        uint32_t allocateChunk(uint32_t iAddr, uint32_t &iAllocSize, vector<TC_Multi_HashMap::Value> &vtData);

        /**
         * @brief �ͷ�Block
         * @param v, ��Ҫ�ͷŵ�chunk�ĵ�ַ�б�
         */
        void deallocateMemChunk(const vector<uint32_t> &v);

        /**
         * @brief �ͷ�Block
         * @param iChunk, ��Ҫ�ͷŵ�chunk��ַ
         */
        void deallocateMemChunk(uint32_t iChunk);

    protected:
		/**
		 * @brief ������copy����
		 */
        BlockAllocator(const BlockAllocator &);
		/**
		 * @brief ������ֵ
		 */
        BlockAllocator& operator=(const BlockAllocator &);
        bool operator==(const BlockAllocator &mba) const;
        bool operator!=(const BlockAllocator &mba) const;

    public:
        /**
         * map
         */
    	TC_Multi_HashMap                *_pMap;

        /**
         * chunk������
         */
        TC_MemMultiChunkAllocator		*_pChunkAllocator;
    };

    ////////////////////////////////////////////////////////////////
	/** 
	  * @brief ����map��������
	  */
    class HashMapLockItem
    {
    public:

    	/**
    	 *
         * @param pMap, 
         * @param iAddr, ��LockItem��Ӧ��Block�׵�ַ
    	 */
    	HashMapLockItem(TC_Multi_HashMap *pMap, uint32_t iAddr);

        /**
         *
         * @param mcmdi
         */
        HashMapLockItem(const HashMapLockItem &mcmdi);

        /**
         *
         * @param mcmdi
         *
         * @return HashMapLockItem&
         */
        HashMapLockItem &operator=(const HashMapLockItem &mcmdi);

    	/**
    	 *
    	 * @param mcmdi
    	 *
    	 * @return bool
    	 */
    	bool operator==(const HashMapLockItem &mcmdi);

    	/**
    	 *
    	 * @param mcmdi
    	 *
    	 * @return bool
    	 */
    	bool operator!=(const HashMapLockItem &mcmdi);

        /**
         * @brief �Ƿ���������
         *
         * @return bool
         */
        bool isDirty();

        /**
         * @brief �Ƿ�ֻ��Key
         *
         * @return bool
         */
        bool isOnlyKey();

        /**
         * @brief ���Syncʱ��
         *
         * @return time_t
         */
        time_t getSyncTime();

    	/**
         * @brief ��ȡ����ֵ
		 * @param v
         * @return int
         *          RT_OK:���ݻ�ȡOK
         *          RT_ONLY_KEY: key��Ч, v��ЧΪ��
         *          ����ֵ, �쳣
         *
    	 */
		int get(TC_Multi_HashMap::Value &v);

    	/**
    	 * @brief ����ȡkey
		 * @param mk, ��key
		 * @param uk, ������key(����key�����������)
         * @return int
         *          RT_OK:���ݻ�ȡOK
         *          ����ֵ, �쳣
    	 */
    	int get(string &mk, string &uk);

        /**
         * @brief ��ȡ��Ӧblock����Ե�ַ
         *
         * @return size_t
         */
        uint32_t getAddr() const { return _iAddr; }

    protected:

        /**
         * @brief ��������
         * @param mk, ��key
		 * @param uk, ����key�����������
         * @param v, ����ֵ
		 * @param iVersion, ���ݰ汾(1-255), 0��ʾ����ע�汾
         * @param vtData, ��̭������
         * @return int
         */
        int set(const string &mk, const string &uk, const string& v, uint8_t iVersion, vector<TC_Multi_HashMap::Value> &vtData);

        /**
         * @brief ����Key, ������(Only Key)
         * @param mk, ��key
		 * @param uk, ����key�����������
         * @param vtData, ��̭������
         *
         * @return int
         */
        int set(const string &mk, const string &uk, vector<TC_Multi_HashMap::Value> &vtData);

        /**
         * @brief �жϵ�ǰitem�Ƿ���ָ��key��item, ����ǻ�����value
         * @param pKey
         * @param iKeyLen
         *
         * @return bool
         */
        bool equal(const string &mk, const string &uk, TC_Multi_HashMap::Value &v, int &ret);

        /**
         * @brief �жϵ�ǰitem�Ƿ���ָ��key��item
         * @param pKey
         * @param iKeyLen
         *
         * @return bool
         */
        bool equal(const string &mk, const string &uk, int &ret);

    	/**
         * @brief ����ǰitem�ƶ�����һ��item
    	 *
    	 * @return HashMapLockItem
    	 */
    	void nextItem(int iType);

        /**
         * @brief ����ǰitem�ƶ�����һ��item
         * @param iType
         */
    	void prevItem(int iType);

        friend class TC_Multi_HashMap;
        friend struct TC_Multi_HashMap::HashMapLockIterator;

    private:
        /**
         * map
         */
    	TC_Multi_HashMap *_pMap;

        /**
         * ��Ӧ��block�ĵ�ַ
         */
        uint32_t      _iAddr;
    };

    /////////////////////////////////////////////////////////////////////////
	/** 
	  * @brief  ��������� 
	   */ 
    struct HashMapLockIterator
    {
    public:

		/** 
		  * @brief ���������ʽ
		  */
        enum
        {
            IT_BLOCK    = 0,        /**��ͨ��˳��*/
            IT_SET      = 1,        /**Setʱ��˳��*/
            IT_GET      = 2,		/**Getʱ��˳��*/
			IT_MKEY		= 3,		/**ͬһ��key�µ�block����*/
			IT_UKEY		= 4,		/**ͬһ���������µ�block����*/
        };

        /**
         * @brief ��������˳��
         */
        enum
        {
            IT_NEXT     = 0,        /**˳��*/
            IT_PREV     = 1,        /**����*/
        };

        /**
         *
         */
        HashMapLockIterator();

    	/**
         * @brief ���캯��
		 * @param pMap,
         * @param iAddr, ��Ӧ��block��ַ
    	 * @param iType, ��������
		 * @param iOrder, ������˳��
    	 */
    	HashMapLockIterator(TC_Multi_HashMap *pMap, uint32_t iAddr, int iType, int iOrder);

        /**
         * @brief copy
         * @param it
         */
        HashMapLockIterator(const HashMapLockIterator &it);

        /**
         * @brief ����
         * @param it
         *
         * @return HashMapLockIterator&
         */
        HashMapLockIterator& operator=(const HashMapLockIterator &it);

        /**
         *
         * @param mcmi
         *
         * @return bool
         */
        bool operator==(const HashMapLockIterator& mcmi);

        /**
         *
         * @param mv
         *
         * @return bool
         */
        bool operator!=(const HashMapLockIterator& mcmi);

    	/**
    	 * @brief ǰ��++
    	 *
    	 * @return HashMapLockIterator&
    	 */
    	HashMapLockIterator& operator++();

    	/**
         * @brief ����++
         *
         * @return HashMapLockIterator&
    	 */
    	HashMapLockIterator operator++(int);

    	/**
    	 *
    	 *
         * @return HashMapLockItem&i
    	 */
    	HashMapLockItem& operator*() { return _iItem; }

    	/**
    	 *
    	 *
    	 * @return HashMapLockItem*
    	 */
    	HashMapLockItem* operator->() { return &_iItem; }

    public:
        /**
         *
         */
        TC_Multi_HashMap  *_pMap;

        /**
         *
         */
        HashMapLockItem _iItem;

    	/**
    	 * �������ķ�ʽ
    	 */
    	int        _iType;

        /**
         * ��������˳��
         */
        int        _iOrder;

    };

    ////////////////////////////////////////////////////////////////
	/** 
	 *  @brief map��HashItem��, һ��HashItem��Ӧ���������
	 */
    class HashMapItem
    {
    public:

        /**
         *
         * @param pMap
         * @param iIndex, Hash����
         */
        HashMapItem(TC_Multi_HashMap *pMap, uint32_t iIndex);

        /**
         *
         * @param mcmdi
         */
        HashMapItem(const HashMapItem &mcmdi);

        /**
         *
         * @param mcmdi
         *
         * @return HashMapItem&
         */
        HashMapItem &operator=(const HashMapItem &mcmdi);

        /**
         *
         * @param mcmdi
         *
         * @return bool
         */
        bool operator==(const HashMapItem &mcmdi);

        /**
         *
         * @param mcmdi
         *
         * @return bool
         */
        bool operator!=(const HashMapItem &mcmdi);

        /**
         * @brief ��ȡ��ǰhashͰ����������, ע��ֻ��ȡ��key/value������
         * ����ֻ��key������, ����ȡ
         * @param vtData
         * @return
         */
        void get(vector<TC_Multi_HashMap::Value> &vtData);

        /**
         * @brief ��ȡ��ǰitem��hash����
         * 
         * @return int
         */
        uint32_t getIndex() const { return _iIndex; }

        /**
         * @brief ����ǰitem�ƶ�Ϊ��һ��item
         *
         */
        void nextItem();

        friend class TC_Multi_HashMap;
        friend struct TC_Multi_HashMap::HashMapIterator;

    private:
        /**
         * map
         */
        TC_Multi_HashMap *_pMap;

        /**
         * ��Ӧ�����ݿ�����
         */
        uint32_t      _iIndex;
    };

    /////////////////////////////////////////////////////////////////////////
	/**
	* @brief ���������
	*/
    struct HashMapIterator
    {
    public:

        /**
         * @brief ���캯��
         */
        HashMapIterator();

    	/**
         * @brief ���캯��
         * @param iIndex, hash����
    	 * @param type
    	 */
    	HashMapIterator(TC_Multi_HashMap *pMap, uint32_t iIndex);

        /**
         * @brief copy
         * @param it
         */
        HashMapIterator(const HashMapIterator &it);

        /**
         * @brief ����
         * @param it
         *
         * @return HashMapLockIterator&
         */
        HashMapIterator& operator=(const HashMapIterator &it);

        /**
         *
         * @param mcmi
         *
         * @return bool
         */
        bool operator==(const HashMapIterator& mcmi);

        /**
         *
         * @param mv
         *
         * @return bool
         */
        bool operator!=(const HashMapIterator& mcmi);

    	/**
    	 * @brief ǰ��++
    	 *
    	 * @return HashMapIterator&
    	 */
    	HashMapIterator& operator++();

    	/**
         * @brief ����++
         *
         * @return HashMapIterator&
    	 */
    	HashMapIterator operator++(int);

    	/**
    	 *
    	 *
         * @return HashMapItem&i
    	 */
    	HashMapItem& operator*() { return _iItem; }

    	/**
    	 *
    	 *
    	 * @return HashMapItem*
    	 */
    	HashMapItem* operator->() { return &_iItem; }

    public:
        /**
         *
         */
        TC_Multi_HashMap  *_pMap;

        /**
         *
         */
        HashMapItem _iItem;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////

    /**
     * @brief mapͷ
     */
    struct tagMapHead
    {
        char		_cMaxVersion;        /**��汾*/
        char		_cMinVersion;        /**С�汾*/
        bool		_bReadOnly;          /**�Ƿ�ֻ��*/
        bool		_bAutoErase;         /**�Ƿ�����Զ���̭*/
        char		_cEraseMode;         /**��̭��ʽ:0x00:����Get����̭, 0x01:����Set����̭*/
        size_t		_iMemSize;           /**�ڴ��С*/
        size_t		_iMinDataSize;       /**��С���ݿ��С*/
        size_t		_iMaxDataSize;       /**������ݿ��С*/
        float		_fFactor;            /**����*/
        float		_fHashRatio;         /**chunks����/hash����*/
		float		_fMainKeyRatio;		 /**chunks����/��key hash����*/
        size_t		_iElementCount;      /**��Ԫ�ظ���*/
        size_t		_iEraseCount;        /**ÿ����̭����*/
        size_t		_iDirtyCount;        /**�����ݸ���*/
        uint32_t	_iSetHead;           /**Setʱ������ͷ��*/
        uint32_t	_iSetTail;           /**Setʱ������β��*/
        uint32_t	_iGetHead;           /**Getʱ������ͷ��*/
        uint32_t	_iGetTail;           /**Getʱ������β��*/
        uint32_t	_iDirtyTail;         /**��������β��*/
        uint32_t	_iBackupTail;        /**�ȱ�ָ��*/
        uint32_t	_iSyncTail;          /**��д����*/
        time_t		_iSyncTime;          /**��дʱ��*/
        size_t		_iUsedChunk;         /**�Ѿ�ʹ�õ��ڴ��*/
        size_t		_iGetCount;          /**get����*/
        size_t		_iHitCount;          /**���д���*/
		size_t		_iMKOnlyKeyCount;	 /**��key��onlykey����*/
		size_t		_iOnlyKeyCount;		 /**������OnlyKey����, �����ͨ��Ϊ0*/
		size_t		_iMaxBlockCount;	 /**��key�������ļ�¼���������ֵҪ��أ�����̫�󣬷���ᵼ�²�ѯ����*/
        size_t		_iReserve[4];        /**����*/
    }__attribute__((packed));

    /**
     * @brief ��Ҫ�޸ĵĵ�ַ
     */
    struct tagModifyData
    {
        size_t		_iModifyAddr;       /**�޸ĵĵ�ַ*/
        char		_cBytes;            /**�ֽ���*/
        size_t		_iModifyValue;      /**ֵ*/
    }__attribute__((packed));

    /**
     * @brief �޸����ݿ�ͷ��
     */
    struct tagModifyHead
    {
        char            _cModifyStatus;         /**�޸�״̬: 0:Ŀǰû�����޸�, 1: ��ʼ׼���޸�, 2:�޸����, û��copy���ڴ���*/
        size_t          _iNowIndex;             /**���µ�Ŀǰ������, ���ܲ���1000��*/
        tagModifyData   _stModifyData[1000];     /**һ�����1000���޸�*/
    }__attribute__((packed));

    /**
     * @brief HashItem
     */
    struct tagHashItem
    {
        uint32_t	_iBlockAddr;		/**ָ����������ڴ��ַ����*/
        uint32_t	_iListCount;		/**�������*/
    }__attribute__((packed));

	/**
	* @brief ��key HashItem
	*/
	struct tagMainKeyHashItem
	{
		uint32_t	_iMainKeyAddr;		/**��key�������ƫ�Ƶ�ַ*/
		uint32_t	_iListCount;		/**��ͬ��key hash��������key����*/
	}__attribute__((packed));

    /**64λ����ϵͳ�û����汾��, 32λ����ϵͳ��ż���汾��*/
#if __WORDSIZE == 64

	/**
	* @brief ����汾��
	*/
    enum
    {
        MAX_VERSION         = 1,		/**��ǰmap�Ĵ�汾��*/
        MIN_VERSION         = 1,		/**��ǰmap��С�汾��*/
    };

#else
	/**
	* @brief ����汾��
	*/
    enum
    {
        MAX_VERSION         = 1,		/**��ǰmap�Ĵ�汾��*/
        MIN_VERSION         = 0,		/**��ǰmap��С�汾��*/
    };

#endif

	/**
	* @brief ������̭��ʽ
	*/
    enum
    {
        ERASEBYGET          = 0x00,		/**����Get������̭*/
        ERASEBYSET          = 0x01,		/**����Set������̭*/
    };

	/**
	* @brief ������������ʱ��ѡ��
	*/
	enum DATATYPE
	{
		PART_DATA			= 0,		/**����������*/
		FULL_DATA			= 1,		/**��������*/
		AUTO_DATA			= 2,		/**�����ڲ�������������״̬������*/
	};

    /**
     * @brief  get, set��int����ֵ
     */
    enum
    {
        RT_OK                   = 0,    /**�ɹ�*/
        RT_DIRTY_DATA           = 1,    /**������*/
        RT_NO_DATA              = 2,    /**û������*/
        RT_NEED_SYNC            = 3,    /**��Ҫ��д*/
        RT_NONEED_SYNC          = 4,    /**����Ҫ��д*/
        RT_ERASE_OK             = 5,    /**��̭���ݳɹ�*/
        RT_READONLY             = 6,    /**mapֻ��*/
        RT_NO_MEMORY            = 7,    /**�ڴ治��*/
        RT_ONLY_KEY             = 8,    /**ֻ��Key, û��Value*/
        RT_NEED_BACKUP          = 9,    /**��Ҫ����*/
        RT_NO_GET               = 10,   /**û��GET��*/
		RT_DATA_VER_MISMATCH	= 11,	/**д�����ݰ汾��ƥ��*/
		RT_PART_DATA			= 12,	/**��key���ݲ�����*/
        RT_DECODE_ERR           = -1,   /**��������*/
        RT_EXCEPTION_ERR        = -2,   /**�쳣*/
        RT_LOAD_DATA_ERR        = -3,   /**���������쳣*/
        RT_VERSION_MISMATCH_ERR = -4,   /**�汾��һ��*/
        RT_DUMP_FILE_ERR        = -5,   /**dump���ļ�ʧ��*/
        RT_LOAD_FILE_ERR        = -6,   /**load�ļ����ڴ�ʧ��*/
		RT_NOTALL_ERR           = -7,   /**û�и�����ȫ*/
    };

    /**���������*/
    typedef HashMapIterator     hash_iterator;
    typedef HashMapLockIterator lock_iterator;

    /**����hash������*/
    typedef TC_Functor<uint32_t, TL::TLMaker<const string &>::Result> hash_functor;

    //////////////////////////////////////////////////////////////////////////////////////////////
    //map�Ľӿڶ���

    /**
     * @brief ���캯��
     */
    TC_Multi_HashMap()
    : _iMinDataSize(0)
    , _iMaxDataSize(0)
    , _fFactor(1.0)
    , _fHashRatio(2.0)
	, _fMainKeyRatio(1.0)
    , _pDataAllocator(new BlockAllocator(this))
    , _lock_end(this, 0, 0, 0)
    , _end(this, (uint32_t)(-1))
    , _hashf(magic_string_hash())
    {
    }

    /**
     * @brief ��ʼ�����ݿ�ƽ����С
     * ��ʾ�ڴ�����ʱ�򣬻����n����С�飬 n������С��*�������ӣ�, n������С��*��������*�������ӣ�..., ֱ��n������
     * n��hashmap�Լ����������
     * ���ַ������ͨ�������ݿ��¼�䳤�Ƚ϶��ʹ�ã� ���ڽ�Լ�ڴ棬������ݼ�¼�������Ǳ䳤�ģ� ����С��=���죬��������=1�Ϳ�����
     * @param iMinDataSize: ��С���ݿ��С
     * @param iMaxDataSize: ������ݿ��С
     * @param fFactor: ��������
     */
    void initDataBlockSize(size_t iMinDataSize, size_t iMaxDataSize, float fFactor);

    /**
     * @brief ʼ��chunk���ݿ�/hash���ֵ, Ĭ����2,
     * ����Ҫ���ı�����create֮ǰ����
     *
     * @param fRatio
     */
    void initHashRatio(float fRatio)                { _fHashRatio = fRatio;}

    /**
	 * @brief ��ʼ��chunk����/��key hash����, Ĭ����1, 
	 * ������һ����key�������ж�������� ����Ҫ���ı�����create֮ǰ���� 
     *
     * @param fRatio
     */
    void initMainKeyHashRatio(float fRatio)         { _fMainKeyRatio = fRatio;}

    /**
     * @brief ��ʼ��, ֮ǰ��Ҫ����:initDataAvgSize��initHashRatio
     * @param pAddr �ⲿ����õĴ洢�ľ��Ե�ַ
     * @param iSize �洢�ռ��С
     * @return ʧ�����׳��쳣
     */
    void create(void *pAddr, size_t iSize);

    /**
     * @brief ���ӵ��Ѿ���ʽ�����ڴ��
     * @param pAddr, �ڴ��ַ
     * @param iSize, �ڴ��С
     * @return ʧ�����׳��쳣
     */
    void connect(void *pAddr, size_t iSize);

    /**
     * @brief ԭ�������ݿ��������չ�ڴ�, ע��ͨ��ֻ�ܶ�mmap�ļ���Ч
     * (���iSize�ȱ������ڴ��С,�򷵻�-1)
     * @param pAddr, ��չ��Ŀռ�
     * @param iSize
     * @return 0:�ɹ�, -1:ʧ��
     */
    int append(void *pAddr, size_t iSize);

    /**
     * @brief ��ȡÿ�ִ�С�ڴ���ͷ����Ϣ
     *
     * @return vector<TC_MemChunk::tagChunkHead>: ��ͬ��С�ڴ��ͷ����Ϣ
     */
    vector<TC_MemChunk::tagChunkHead> getBlockDetail() { return _pDataAllocator->getBlockDetail(); }

    /**
     * @brief ����block��chunk�ĸ���
     *
     * @return size_t
     */
    size_t allBlockChunkCount()                     { return _pDataAllocator->allBlockChunkCount(); }

    /**
     * @brief ÿ��block��chunk�ĸ���(��ͬ��С�ڴ��ĸ�����ͬ)
     *
     * @return vector<size_t>
     */
    vector<size_t> singleBlockChunkCount()          { return _pDataAllocator->singleBlockChunkCount(); }

    /**
     * @brief ��ȡ������hashͰ�ĸ���
     *
     * @return size_t
     */
    size_t getHashCount()                           { return _hash.size(); }

	/**
	* @brief ��ȡ��key hashͰ����
	*/
	size_t getMainKeyHashCount()					{ return _hashMainKey.size(); }

    /**
     * @brief ��ȡԪ�صĸ���
     *
     * @return size_t
     */
    size_t size()                                   { return _pHead->_iElementCount; }

    /**
     * @brief ������Ԫ�ظ���
     *
     * @return size_t
     */
    size_t dirtyCount()                             { return _pHead->_iDirtyCount;}

	/**
     * @brief ����OnlyKey����Ԫ�ظ���
     *
     * @return size_t
     */
    size_t onlyKeyCount()                           { return _pHead->_iOnlyKeyCount;}

	/**
     * @brief ��key OnlyKey����Ԫ�ظ���
     *
     * @return size_t
     */
    size_t onlyKeyCountM()                          { return _pHead->_iMKOnlyKeyCount;}

    /**
     * @brief ����ÿ����̭����
     * @param n
     */
    void setEraseCount(size_t n)                    { _pHead->_iEraseCount = n; }

    /**
     * @brief ��ȡÿ����̭����
     *
     * @return size_t
     */
    size_t getEraseCount()                          { return _pHead->_iEraseCount; }

    /**
     * @brief ����ֻ��
     * @param bReadOnly
     */
    void setReadOnly(bool bReadOnly)                { _pHead->_bReadOnly = bReadOnly; }

    /**
     * @brief �Ƿ�ֻ��
     *
     * @return bool
     */
    bool isReadOnly()                               { return _pHead->_bReadOnly; }

    /**
     * @brief �����Ƿ�����Զ���̭
     * @param bAutoErase
     */
    void setAutoErase(bool bAutoErase)              { _pHead->_bAutoErase = bAutoErase; }

    /**
     * @brief �Ƿ�����Զ���̭
     *
     * @return bool
     */
    bool isAutoErase()                              { return _pHead->_bAutoErase; }

    /**
     * @brief ������̭��ʽ
     * TC_Multi_HashMap::ERASEBYGET
     * TC_Multi_HashMap::ERASEBYSET
     * @param cEraseMode
     */
    void setEraseMode(char cEraseMode)              { _pHead->_cEraseMode = cEraseMode; }

    /**
     * @brief ��ȡ��̭��ʽ
     *
     * @return bool
     */
    char getEraseMode()                             { return _pHead->_cEraseMode; }

    /**
     * @brief ���û�дʱ����(��)
     * @param iSyncTime
     */
    void setSyncTime(time_t iSyncTime)              { _pHead->_iSyncTime = iSyncTime; }

    /**
     * @brief ��ȡ��дʱ��
     *
     * @return time_t
     */
    time_t getSyncTime()                            { return _pHead->_iSyncTime; }

    /**
     * @brief ��ȡͷ��������Ϣ
     * 
     * @return tagMapHead&
     */
    tagMapHead& getMapHead()                        { return *_pHead; }

    /**
     * @brief ������������hash��ʽ
     * @param hashf
     */
    void setHashFunctor(hash_functor hashf)         { _hashf = hashf; }

	/**
	* @brief ������key��hash��ʽ
	* @param hashf
	*/
	void setHashFunctorM(hash_functor hashf)			{ _mhashf = hashf; } 

    /**
     * @brief ����hash������
     * 
     * @return hash_functor&
     */
    hash_functor &getHashFunctor()                  { return _hashf; }
	hash_functor &getHashFunctorM()					{ return _mhashf; }

    /**
     * @brief ��ȡָ��������hash item
     * @param index, hash����
     *
     * @return tagHashItem&
     */
    tagHashItem *item(size_t iIndex)                { return &_hash[iIndex]; }

	/**
	* @brief ������key hash����ȡ��key item
	* @param iIndex, ��key��hash����
	*/
	tagMainKeyHashItem* itemMainKey(size_t iIndex)	{ return &_hashMainKey[iIndex]; }

    /**
     * @brief dump���ļ�
     * @param sFile
     *
     * @return int
     *          RT_DUMP_FILE_ERR: dump���ļ�����
     *          RT_OK: dump���ļ��ɹ�
     */
    int dump2file(const string &sFile);

    /**
     * @brief ���ļ�load
     * @param sFile
     *
     * @return int
     *          RT_LOAL_FILE_ERR: load����
     *          RT_VERSION_MISMATCH_ERR: �汾��һ��
     *          RT_OK: load�ɹ�
     */
    int load5file(const string &sFile);

    /**
     * @brief ���hashmap
     * ����map�����ݻָ�����ʼ״̬
     */
    void clear();

	/**
	* @brief �����key�Ƿ����
	* @param mk, ��key
	*
	* @return int
	*		TC_Multi_HashMap::RT_OK, ��key���ڣ���������
	*		TC_Multi_HashMap::RT_ONLY_KEY, ��key���ڣ�û������
	*		TC_Multi_HashMap::RT_PART_DATA, ��key���ڣ���������ݿ��ܲ�����
	*		TC_Multi_HashMap::RT_NO_DATA, ��key������
	*/
	int checkMainKey(const string &mk);

	/**
	* @brief ������key�����ݵ�������
	* @param mk, ��key
	* @param bFull, trueΪ�������ݣ�falseΪ����������
	*
	* @return
	*          RT_READONLY: ֻ��
	*          RT_NO_DATA: û�е�ǰ����
	*          RT_OK: ���óɹ�
	*          ��������ֵ: ����
	*/
	int setFullData(const string &mk, bool bFull);

    /**
     * @brief ������ݸɾ�״̬
     * @param mk, ��key
	 * @param uk, ����key�����������
     *
     * @return int
     *          RT_NO_DATA: û�е�ǰ����
     *          RT_ONLY_KEY:ֻ��Key
     *          RT_DIRTY_DATA: ��������
     *          RT_OK: �Ǹɾ�����
     *          ��������ֵ: ����
     */
    int checkDirty(const string &mk, const string &uk);

	/**
	* @brief 
	*   	 �����key�����ݵĸɾ�״̬��ֻҪ��key������һ�������ݣ��򷵻���
	* @param mk, ��key
	* @return int
	*          RT_NO_DATA: û�е�ǰ����
	*          RT_ONLY_KEY:ֻ��Key
	*          RT_DIRTY_DATA: ��������
	*          RT_OK: �Ǹɾ�����
	*          ��������ֵ: ����
	*/
	int checkDirty(const string &mk);

    /**
     * @brief ����Ϊ������, �޸�SETʱ����, �ᵼ�����ݻ�д
     * @param mk, ��key
	 * @param uk, ����key�����������
     *
     * @return int
     *          RT_READONLY: ֻ��
     *          RT_NO_DATA: û�е�ǰ����
     *          RT_ONLY_KEY:ֻ��Key
     *          RT_OK: ���������ݳɹ�
     *          ��������ֵ: ����
     */
    int setDirty(const string &mk, const string &uk);

    /**
     * @brief ����Ϊ�ɾ�����, �޸�SET��, �������ݲ���д
     * @param mk, ��key
	 * @param uk, ����key�����������
     *
     * @return int
     *          RT_READONLY: ֻ��
     *          RT_NO_DATA: û�е�ǰ����
     *          RT_ONLY_KEY:ֻ��Key
     *          RT_OK: ���óɹ�
     *          ��������ֵ: ����
     */
    int setClean(const string &mk, const string &uk);

	/**
	* @brief �������ݵĻ�дʱ��
	* @param mk,
	* @param uk,
	* @param iSynctime
	*
	* @return int
	*          RT_READONLY: ֻ��
	*          RT_NO_DATA: û�е�ǰ����
	*          RT_ONLY_KEY:ֻ��Key
	*          RT_OK: ���������ݳɹ�
	*          ��������ֵ: ����
	*/
	int setSyncTime(const string &mk, const string &uk, time_t iSyncTime);

    /**
     * @brief ��ȡ����, �޸�GETʱ����
     * @param mk, ��key
	 * @param uk, ����key�����������
	 @ @param v, ���ص�����
     *
     * @return int:
     *          RT_NO_DATA: û������
     *          RT_ONLY_KEY:ֻ��Key
     *          RT_OK:��ȡ���ݳɹ�
     *          ��������ֵ: ����
     */
    int get(const string &mk, const string &uk, Value &v);

    /**
     * @brief ��ȡ��key�µ���������, �޸�GETʱ����
     * @param mk, ��key
     * @param vs, ���ص����ݼ�
     *
     * @return int:
     *          RT_NO_DATA: û������
     *          RT_ONLY_KEY: ֻ��Key
	 *          RT_PART_DATA: ���ݲ�ȫ��ֻ�в�������
     *          RT_OK: ��ȡ���ݳɹ�
     *          ��������ֵ: ����
     */
    int get(const string &mk, vector<Value> &vs);

    /**
	 * @brief ��ȡ��key hash�µ��������� 
	 *  	  , ���޸�GETʱ��������Ҫ����Ǩ��
     * @param mh, ��key hashֵ
     * @param vs, ���ص����ݼ�������key���з����map
     *
     * @return int:
     *          RT_OK: ��ȡ���ݳɹ�
     *          ��������ֵ: ����
     */
    int get(uint32_t &mh, map<string, vector<Value> > &vs);

    /**
     * @brief ��������, �޸�ʱ����, �ڴ治��ʱ���Զ���̭�ϵ�����
     * @param mk: ��key
	 * @param uk: ����key�����������
     * @param v: ����ֵ
	 * @param iVersion: ���ݰ汾, Ӧ�ø���get�������ݰ汾д�أ�Ϊ0��ʾ���������ݰ汾
     * @param bDirty: �Ƿ���������
	 * @param eType: set���������ͣ�PART_DATA-�����µ����ݣ�FULL_DATA-���������ݣ�AUTO_DATA-���������������;���
	 * @param bHead: ���뵽��key����˳��ǰ������
     * @param vtData: ����̭�ļ�¼
     * @return int:
     *          RT_READONLY: mapֻ��
     *          RT_NO_MEMORY: û�пռ�(����̭��������»����)
	 *			RT_DATA_VER_MISMATCH, Ҫ���õ����ݰ汾�뵱ǰ�汾������Ӧ������get����set
     *          RT_OK: ���óɹ�
     *          ��������ֵ: ����
     */
    int set(const string &mk, const string &uk, const string &v, uint8_t iVersion, 
		bool bDirty, DATATYPE eType, bool bHead,vector<Value> &vtData);

    /**
	 * @brief ����key, ��������(only key), 
	 *  	  �ڴ治��ʱ���Զ���̭�ϵ�����
     * @param mk: ��key
	 * @param uk: ����key�����������
	 * @param eType: set���������ͣ�PART_DATA-�����µ����ݣ�FULL_DATA-���������ݣ�AUTO_DATA-���������������;���
	 * @param bHead: ���뵽��key����˳��ǰ������
     * @param vtData: ����̭������
     *
     * @return int
     *          RT_READONLY: mapֻ��
     *          RT_NO_MEMORY: û�пռ�(����̭��������»����)
     *          RT_OK: ���óɹ�
     *          ��������ֵ: ����
     */
    int set(const string &mk, const string &uk, DATATYPE eType, bool bHead, vector<Value> &vtData);

    /**
	 * @brief ��������key, ������key������ 
	 *  	  , �ڴ治��ʱ���Զ���̭�ϵ�����
     * @param mk: ��key
     * @param vtData: ����̭������
     *
     * @return int
     *          RT_READONLY: mapֻ��
     *          RT_NO_MEMORY: û�пռ�(����̭��������»����)
     *          RT_OK: ���óɹ�
     *          ��������ֵ: ����
     */
    int set(const string &mk, vector<Value> &vtData);

	/**
	* @brief ������������, �ڴ治��ʱ���Զ���̭�ϵ�����
	* @param vtSet: ��Ҫ���õ�����
	* @param eType: set���������ͣ�PART_DATA-�����µ����ݣ�FULL_DATA-���������ݣ�AUTO_DATA-���������������;���
	* @param bHead: ���뵽��key����˳��ǰ������
	* @param bForce, �Ƿ�ǿ�Ʋ������ݣ�Ϊfalse���ʾ��������Ѿ������򲻸���
	* @param vtErased: �ڴ治��ʱ����̭������
	*
	* @return 
	*          RT_READONLY: mapֻ��
	*          RT_NO_MEMORY: û�пռ�(����̭��������»����)
	*          RT_OK: ���óɹ�
	*          ��������ֵ: ����
	*/
	int set(const vector<Value> &vtSet, DATATYPE eType, bool bHead, bool bForce, vector<Value> &vtErased);

    /**
	 * @brief 
	 *  	  ɾ�����ݣ�����ǿ��ɾ��ĳ�����ݣ�����Ӧ�õ��������ɾ����key���������ݵĺ���
     * @param mk: ��key
	 * @param uk: ����key�����������
     * @param data: ��ɾ���ļ�¼
     * @return int:
     *          RT_READONLY: mapֻ��
     *          RT_NO_DATA: û�е�ǰ����
     *          RT_ONLY_KEY:ֻ��Key, ɾ���ɹ�
     *          RT_OK: ɾ�����ݳɹ�
     *         ��������ֵ: ����
     */
    int del(const string &mk, const string &uk, Value &data);

    /**
     * @brief ɾ����key����������
     * @param mk: ��key
     * @param data: ��ɾ���ļ�¼
     * @return int:
     *          RT_READONLY: mapֻ��
     *          RT_NO_DATA: û�е�ǰ����
     *          RT_ONLY_KEY:ֻ��Key, ɾ���ɹ�
     *          RT_OK: ɾ�����ݳɹ�
     *         ��������ֵ: ����
     */
    int del(const string &mk, vector<Value> &data);

    /**
     * @brief ��̭����, ÿ��ɾ��һ��, ����Getʱ����̭
     * �ⲿѭ�����øýӿ���̭����
     * ֱ��: Ԫ�ظ���/chunks * 100 < Ratio, bCheckDirty Ϊtrueʱ����������������̭����
     * @param ratio: �����ڴ�chunksʹ�ñ��� 0< Ratio < 100
     * @param data: ��ɾ�������ݼ�
	 * @param bCheckDirty: �Ƿ���������
     * @return int:
     *          RT_READONLY: mapֻ��
     *          RT_OK: �����ټ�����̭��
     *          RT_ONLY_KEY:ֻ��Key, ɾ���ɹ�
     *          RT_DIRTY_DATA:�����������ݣ���bCheckDirty=trueʱ���п��ܲ������ַ���ֵ
     *          RT_ERASE_OK:��̭��ǰ���ݳɹ�, ������̭
     *          ��������ֵ: ����, ͨ������, ��������erase��̭
     */
    int erase(int ratio, vector<Value> &vtData, bool bCheckDirty = false);

    /**
     * @brief ��д, ÿ�η�����Ҫ��д��һ��
     * ���ݻ�дʱ���뵱ǰʱ�䳬��_pHead->_iSyncTime����Ҫ��д
     * _pHead->_iSyncTime��setSyncTime�����趨, Ĭ��10����

     * �ⲿѭ�����øú������л�д
     * mapֻ��ʱ��Ȼ���Ի�д
     * @param iNowTime: ��ǰʱ��
     *                  ��дʱ���뵱ǰʱ�����_pHead->_iSyncTime����Ҫ��д
     * @param data : ������Ҫ��д������
     * @return int:
     *          RT_OK: ������������ͷ����, ����sleepһ���ٳ���
     *          RT_ONLY_KEY:ֻ��Key, ɾ���ɹ�, ��ǰ���ݲ�Ҫ��д,��������sync��д
     *          RT_NEED_SYNC:��ǰ���ص�data������Ҫ��д
     *          RT_NONEED_SYNC:��ǰ���ص�data���ݲ���Ҫ��д
     *          ��������ֵ: ����, ͨ������, ��������sync��д
     */
    int sync(time_t iNowTime, Value &data);

    /**
     * @brief ��ʼ��д, ������дָ��
     */
    void sync();

    /**
     * @brief ��ʼ����֮ǰ���øú���
     *
     * @param bForceFromBegin: �Ƿ�ǿ�ƴ�ͷ��ʼ����
     * @return void
     */
    void backup(bool bForceFromBegin = false);

    /**
	 * @brief ��ʼ�������� 
	 *  	  , ÿ�η�����Ҫ���ݵ�����(һ����key�µ���������)
     * @param data
     *
     * @return int
     *          RT_OK: �������
     *          RT_NEED_BACKUP:��ǰ���ص�data������Ҫ����
     *          RT_ONLY_KEY:ֻ��Key, ��ǰ���ݲ�Ҫ����
     *          ��������ֵ: ����, ͨ������, ��������backup
     */
    int backup(vector<Value> &vtData);

    /////////////////////////////////////////////////////////////////////////////////////////
    // �����Ǳ���map����, ��Ҫ��map����

    /**
     * @brief ����
     *
     * @return
     */
    lock_iterator end() { return _lock_end; }

    /**
     * @brief block����
     *
     * @return lock_iterator
     */
    lock_iterator begin();

    /**
     *@brief  block����
     *
     * @return lock_iterator
     */
    lock_iterator rbegin();

    /**
     * @brief ��Setʱ������ĵ�����
     *
     * @return lock_iterator
     */
    lock_iterator beginSetTime();

    /**
     * @brief Set������ĵ�����
     *
     * @return lock_iterator
     */
    lock_iterator rbeginSetTime();

    /**
     * @brief ��Getʱ������ĵ�����
     *
     * @return lock_iterator
     */
    lock_iterator beginGetTime();

    /**
     * @brief Get������ĵ�����
     *
     * @return lock_iterator
     */
    lock_iterator rbeginGetTime();

    /**
     * @brief ��ȡ������β��������(�ʱ��û�в�����������)
     *
     * ���صĵ�����++��ʾ����ʱ��˳��==>(���ʱ��û�в�����������)
     *
     * @return lock_iterator
     */
    lock_iterator beginDirty();

    /////////////////////////////////////////////////////////////////////////////////////////
    // �����Ǳ���map����, ����Ҫ��map����

    /**
     * @brief ����hashͰ����
     * 
     * @return hash_iterator
     */
    hash_iterator hashBegin();

    /**
     *@brief  ����
     *
     * @return
     */
    hash_iterator hashEnd() { return _end; }

    /**
     * @brief ����Key��������
     * @param mk: ��key
	 * @param uk: ����key�����������
	 * @return lock_iterator
     */
    lock_iterator find(const string &mk, const string &uk);

	/**
	* @brief ��ȡ��key����block������
	* @param mk: ��key
	* @return size_t
	*/
	size_t count(const string &mk);

	/**
	* @brief ������key���ҵ�һ��blockλ��. 
	*  
	* ��count��Ͽ��Ա���ĳ����key�µ��������� Ҳ����ֱ��ʹ�õ�������ֱ��end
	* @param mk: ��key
	* @return lock_iterator
	*/
	lock_iterator find(const string &mk);

    /**
     * @brief ����
     *
     * @return string
     */
    string desc();

	/**
	* @brief �Կ��ܵĻ�block���м�飬���ɽ����޸�
	* @param iHash, hash����
	* @param bRepaire, �Ƿ�����޸�
	*
	* @return size_t, ���ػ����ݸ���
	*/
	size_t checkBadBlock(uint32_t iHash, bool bRepair);

protected:

    friend class Block;
    friend class BlockAllocator;
    friend class HashMapIterator;
    friend class HashMapItem;
    friend class HashMapLockIterator;
    friend class HashMapLockItem;

	/**
	*  @brief ��ֹcopy����
	*/
    TC_Multi_HashMap(const TC_Multi_HashMap &mcm);
   	/**
	*  @brief ��ֹ����
	*/
    TC_Multi_HashMap &operator=(const TC_Multi_HashMap &mcm);

	/** 					 
	*  @brief �������ݸ��¹���ʧ�ܵ��Զ��ָ���
	*  �����п��ܽ��йؼ����ݸ��µĺ������ʼ����
	*/
	struct FailureRecover
    {
        FailureRecover(TC_Multi_HashMap *pMap) : _pMap(pMap)
        {
			// ����ʱ�ָ������𻵵�����
            _pMap->doRecover();
			assert(_iRefCount ++ == 0);
        }
		
        ~FailureRecover()
        {
			// ����ʱ�����Ѿ��ɹ����µ�����
            _pMap->doUpdate();
			assert(_iRefCount-- == 1);
        }
		
    protected:
        TC_Multi_HashMap   *_pMap;
		// ����Ƕ�׵���
		static int			_iRefCount;
    };


    /**
	 * @brief ��ʼ�� 
     * @param pAddr, �ⲿ����õĴ洢��ַ
     */
    void init(void *pAddr);

    /**
     * @brief ���������ݸ���
     */
    void incDirtyCount()    { saveValue(&_pHead->_iDirtyCount, _pHead->_iDirtyCount+1); }

    /**
     * @brief ���������ݸ���
     */
    void delDirtyCount()    { saveValue(&_pHead->_iDirtyCount, _pHead->_iDirtyCount-1); }

    /**
     * @brief �������ݸ���
     */
    void incElementCount()  { saveValue(&_pHead->_iElementCount, _pHead->_iElementCount+1); }

    /**
     * @brief �������ݸ���
     */
    void delElementCount()  { saveValue(&_pHead->_iElementCount, _pHead->_iElementCount-1); }

	/**
     * @brief ����������OnlyKey���ݸ���
     */
	void incOnlyKeyCount()    { saveValue(&_pHead->_iOnlyKeyCount, _pHead->_iOnlyKeyCount+1); }

	/**
     * @brief ����������OnlyKey���ݸ���
     */
	void delOnlyKeyCount()    { saveValue(&_pHead->_iOnlyKeyCount, _pHead->_iOnlyKeyCount-1); }

	/**
     * @brief ������key��OnlyKey���ݸ���
     */
	void incOnlyKeyCountM()    { saveValue(&_pHead->_iMKOnlyKeyCount, _pHead->_iMKOnlyKeyCount+1); }

	/**
     * @brief ������key��OnlyKey���ݸ���
     */
	void delOnlyKeyCountM()    { saveValue(&_pHead->_iMKOnlyKeyCount, _pHead->_iMKOnlyKeyCount-1); }

    /**
     * @brief ����Chunk��
     */
    void incChunkCount()    { saveValue(&_pHead->_iUsedChunk, _pHead->_iUsedChunk + 1); }

    /**
     * @brief ����Chunk��
     */
    void delChunkCount()    { saveValue(&_pHead->_iUsedChunk, _pHead->_iUsedChunk - 1); }

    /**
     * @brief ����hit����
     */
    void incGetCount()      { saveValue(&_pHead->_iGetCount, _pHead->_iGetCount+1); }

    /**
     * @brief �������д���
     */
    void incHitCount()      { saveValue(&_pHead->_iHitCount, _pHead->_iHitCount+1); }

    /**
     * @brief ĳhash�������ݸ���+1
     * @param index
     */
    void incListCount(uint32_t index) { saveValue(&item(index)->_iListCount, item(index)->_iListCount+1); }

	/**
	* @brief ĳhashֵ��key������key����+1
	*/
	void incMainKeyListCount(uint32_t index) { saveValue(&itemMainKey(index)->_iListCount, itemMainKey(index)->_iListCount+1); }

    /**
     * @brief ĳhash�������ݸ���-1
     * @param index
     */
    void delListCount(uint32_t index) { saveValue(&item(index)->_iListCount, item(index)->_iListCount-1); }

	/**
	* @brief ĳhashֵ��key������key����-1
	*/
	void delMainKeyListCount(uint32_t index) { saveValue(&itemMainKey(index)->_iListCount, itemMainKey(index)->_iListCount-1); }

	/**
	* @brief ĳhashֵ��key����blockdata����+/-1
	* @param mk, ��key
	* @param bInc, �����ӻ��Ǽ���
	*/
	void incMainKeyBlockCount(const string &mk, bool bInc = true);

	/**
	* @brief ������key��������¼����Ϣ
	*/
	void updateMaxMainKeyBlockCount(size_t iCount);

    /**
     * @brief ��Ե�ַ���ɾ��Ե�ַ
     * @param iAddr
     *
     * @return void*
     */
    void *getAbsolute(uint32_t iAddr) { return _pDataAllocator->_pChunkAllocator->getAbsolute(iAddr); }

    /**
     * @brief ���Ե�ַ������Ե�ַ
     *
     * @return size_t
     */
    uint32_t getRelative(void *pAddr) { return (uint32_t)_pDataAllocator->_pChunkAllocator->getRelative(pAddr); }

    /**
     * @brief ��̭iNowAddr֮�������(������̭������̭)
     * @param iNowAddr, ��ǰ��key���ڷ����ڴ棬�ǲ��ܱ���̭��
     *                  0��ʾ��ֱ�Ӹ�����̭������̭
     * @param vector<Value>, ����̭������
     * @return size_t, ��̭�����ݸ���
     */
    size_t eraseExcept(uint32_t iNowAddr, vector<Value> &vtData);

    /**
     * @brief ����Key����hashֵ
     * @param mk: ��key
	 * @param uk: ����key�����������
     *
     * @return uint32_t
     */
    uint32_t hashIndex(const string &mk, const string &uk);

    /**
     * @brief ����Key����hashֵ
     * @param k: key
     *
     * @return uint32_t
     */
	uint32_t hashIndex(const string &k);

	/**
	* @brief ������key������key��hash
	* @param mk: ��key
	* @return uint32_t
	*/
	uint32_t mhashIndex(const string &mk); 

    /**
     * @brief ����hash��������ָ��key(mk+uk)�����ݵ�λ��, ����������
	 * @param mk: ��key
	 * @param uk: ����key�����������
	 * @param index: ����������hash����
	 * @param v: ����������ݣ��򷵻�����ֵ
	 * @param ret: ����ķ���ֵ
	 * @return lock_iterator: �����ҵ������ݵ�λ�ã��������򷵻�end()
     *
     */
    lock_iterator find(const string &mk, const string &uk, uint32_t index, Value &v, int &ret);

    /**
     * @brief ����hash��������ָ��key(mk+uk)�����ݵ�λ��
	 * @param mk: ��key
	 * @param uk: ����key�����������
	 * @param index: ����������hash����
	 * @param ret: ����ķ���ֵ
	 * @return lock_iterator: �����ҵ������ݵ�λ�ã��������򷵻�end()
     *
     */
    lock_iterator find(const string &mk, const string &uk, uint32_t index, int &ret);

	/**
	* @brief ������key hash����������key�ĵ�ַ���Ҳ�������0
	* @param mk: ��key
	* @param index: ��key hash����
	* @param ret: ���巵��ֵ
	* @return uint32_t: �����ҵ�����key���׵�ַ���Ҳ�������0
	*/
	uint32_t find(const string &mk, uint32_t index, int &ret);

    /**
     * @brief ��������hash������
     * @param iMaxHash: ����block hashͰ��Ԫ�ظ���
     * @param iMinHash: ��С��block hashͰ��Ԫ�ظ���
     * @param fAvgHash: ƽ��Ԫ�ظ���
     */
    void analyseHash(uint32_t &iMaxHash, uint32_t &iMinHash, float &fAvgHash);

    /**
     * @brief ������key hash������
     * @param iMaxHash: ������key hashͰ��Ԫ�ظ���
     * @param iMinHash: ��С����key hashͰ��Ԫ�ظ���
     * @param fAvgHash: ƽ��Ԫ�ظ���
     */
    void analyseHashM(uint32_t &iMaxHash, uint32_t &iMinHash, float &fAvgHash);

    /**
	* @brief �޸ľ����ֵ
	* @param pModifyAddr
	* @param iModifyValue
	*/
    template<typename T>
	void saveValue(void* pModifyAddr, T iModifyValue)
    {
        //��ȡԭʼֵ
        T tmp = *(T*)pModifyAddr;
        
        //����ԭʼֵ
        _pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._iModifyAddr  = (char*)pModifyAddr - (char*)_pHead;
        _pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._iModifyValue = tmp;
        _pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._cBytes       = sizeof(iModifyValue);
        _pstModifyHead->_iNowIndex++;
		
        _pstModifyHead->_cModifyStatus = 1;

        //�޸ľ���ֵ
        *(T*)pModifyAddr = iModifyValue;
		
        assert(_pstModifyHead->_iNowIndex < sizeof(_pstModifyHead->_stModifyData) / sizeof(tagModifyData));
    }

	/**
	* @brief ��ĳ��ֵ��ĳλ���и���
	* @param pModifyAddr, ���޸ĵ�(������)�ڴ��ַ
	* @param bit, ��Ҫ�޸ĵ���������λ
	* @param b, ��Ҫ�޸ĵ�������λ��ֵ
	*/
	template<typename T>
	void saveValue(T *pModifyAddr, uint8_t bit, bool b)
	{
		T tmp = *pModifyAddr;	// ȡ��ԭֵ
		if(b)
		{
			tmp |= 0x01 << bit;
		}
		else
		{
			tmp &= T(-1) ^ (0x01 << bit);
		}
		saveValue(pModifyAddr, tmp);
	}
	
    /**
	* @brief �ָ�����
	*/
    void doRecover();
	
    /**
	* @brief ȷ�ϴ������
	*/
    void doUpdate();

    /**
     * @brief ��ȡ����n����n���������
     * @param n
     *
     * @return size_t
     */
    size_t getMinPrimeNumber(size_t n);

protected:

    /**
     * ͷ��ָ��
     */
    tagMapHead                  *_pHead;

    /**
     * ��С�����ݿ��С
     */
    size_t                      _iMinDataSize;

    /**
     * �������ݿ��С
     */
    size_t                      _iMaxDataSize;

    /**
     * ���ݿ���������
     */
    float                       _fFactor;

    /**
     * ����chunk���ݿ�/hash���ֵ
     */
    float                       _fHashRatio;

	/**
	* ��key hash����/����hash����
	*/
	float						_fMainKeyRatio;

    /**
     * ��������hash������
     */
    TC_MemVector<tagHashItem>   _hash;

	/**
	* ��key hash������
	*/
	TC_MemVector<tagMainKeyHashItem>	_hashMainKey;

    /**
     * �޸����ݿ�
     */
    tagModifyHead               *_pstModifyHead;

    /**
     * block���������󣬰���Ϊ����������key�������ڴ�
     */
    BlockAllocator              *_pDataAllocator;

    /**
     * β��
     */
    lock_iterator               _lock_end;

    /**
     * β��
     */
    hash_iterator               _end;

    /**
     * ��������hashֵ���㹫ʽ
     */
    hash_functor                _hashf;

	/**
	* ��key��hash���㺯��, ������ṩ����ʹ�������_hashf
	*/
	hash_functor				_mhashf;
};

}

#endif

