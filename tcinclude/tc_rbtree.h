#ifndef __TC_RBTREE_H
#define __TC_RDTREE_H

#include <iostream>
#include <string>
#include <cassert>
#include "tc_ex.h"
#include "tc_pack.h"
#include "tc_mem_chunk.h"
#include "tc_functor.h"

using namespace std;

namespace taf
{
/////////////////////////////////////////////////
/** 
* @file tc_rbtree.h 
* @brief rbtree map��
*  
* @author  jarodruan@tencent.com
*/           
/////////////////////////////////////////////////
/**
* @brief RBTree map�쳣��
*/
struct TC_RBTree_Exception : public TC_Exception
{
    TC_RBTree_Exception(const string &buffer) : TC_Exception(buffer){};
    TC_RBTree_Exception(const string &buffer, int err) : TC_Exception(buffer, err){};
    ~TC_RBTree_Exception() throw(){};
};
 /**
 * @brief �ڴ�rbtree����Ҫֱ��ʹ�ø��࣬ͨ��jmem�����ʹ�� 
 *  
 *  �ú����ͨ��TC_MemMutilChunkAllocator������ռ䣬֧�ֲ�ͬ��С���ڴ��ķ���,
 *  
 *  ��������������ڴ������������������ĵĿռ䣨������64λOS���棩��
 *  
 * ֧���ڴ�͹����ڴ�,�Խӿڵ����в�������Ҫ������ 
 *  
 *  �ڲ�������������֧�����ݻ�д��
 *  
 *  �����ݳ���һ�����ݿ�ʱ�����ƴ�Ӷ�����ݿ飻
 *  
 *  Setʱ�����ݿ����꣬�Զ���̭�ʱ��û�з��ʵ����ݣ�Ҳ���Բ���̭��ֱ�ӷ��ش���
 *  
 *  ֧��dump���ļ�������ļ�load��
 */
class TC_RBTree
{
public:
    struct RBTreeLockIterator;
    struct RBTreeIterator;

    /**
	* @brief ��������
	*/
    struct BlockData
    {
        string  _key;       /**����Key*/
        string  _value;     /**����value*/
        bool    _dirty;     /**�Ƿ���������*/
        time_t  _synct;     /**sync time, ��һ���������Ļ�дʱ��*/
        BlockData()
        : _dirty(false)
        , _synct(0)
        {
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////
    /**
	* @brief �ڴ����ݿ�,��ȡ�ʹ������ 
    */
    class Block
    {
    public:

        /**
         * @brief block����ͷ
         */
        struct tagBlockHead
        {
            uint16_t    _iSize;         /**block��������С*/
            char        _iColor;        /**��ɫ*/
            uint32_t    _iParent;       /**���ڵ�*/
            uint32_t    _iLeftChild;    /**������*/
            uint32_t    _iRightChild;   /**������*/
            uint32_t    _iSetNext;      /**Set���ϵ���һ��Block*/
            uint32_t    _iSetPrev;      /**Set���ϵ���һ��Block*/
            uint32_t    _iGetNext;      /**Get���ϵ���һ��Block*/
            uint32_t    _iGetPrev;      /**Get���ϵ���һ��Block*/
            time_t      _iSyncTime;     /**�ϴλ�дʱ��*/
            bool        _bDirty;        /**�Ƿ���������*/
            bool        _bOnlyKey;      /**�Ƿ�ֻ��key, û������*/
            bool        _bNextChunk;    /**�Ƿ�����һ��chunk*/
            union
            {
                uint32_t  _iNextChunk;    /**��һ��Chunk��, _bNextChunk=trueʱ��Ч, tagChunkHead*/
                uint32_t  _iDataLen;      /**��ǰ���ݿ���ʹ���˵ĳ���, _bNextChunk=falseʱ��Ч*/
            };
            char        _cData[0];      /**���ݿ�ʼ����*/
        }__attribute__((packed));

        /**
         * @brief ��ͷ����block, ��Ϊchunk
         */
        struct tagChunkHead
        {
            uint16_t    _iSize;         /**block��������С*/
            bool        _bNextChunk;    /**�Ƿ�����һ��chunk*/
            union
            {
                uint32_t  _iNextChunk;    /**��һ�����ݿ�, _bNextChunk=trueʱ��Ч, tagChunkHead*/
                uint32_t  _iDataLen;      /**��ǰ���ݿ���ʹ���˵ĳ���, _bNextChunk=falseʱ��Ч*/
            };
            char        _cData[0];      /**���ݿ�ʼ����*/
        }__attribute__((packed));

        /**
         * @brief ���캯��
         * @param Map
         * @param ��ǰMemBlock�ĵ�ַ
         * @param pAdd
         */
        Block(TC_RBTree *pMap, uint32_t iAddr)
        : _pMap(pMap)
        , _iHead(iAddr)
        {
            _pHead = getBlockHead(_iHead);
        }

        /**
         * @brief copy
         * @param mb
         */
        Block(const Block &mb)
        : _pMap(mb._pMap)
        , _iHead(mb._iHead)
        {
            _pHead = getBlockHead(_iHead);
        }

        /**
         * @brief ��ȡblockͷ���Ե�ַ
         * @param iAddr
         *
         * @return tagChunkHead*
         */
        tagBlockHead *getBlockHead(uint32_t iAddr) { return ((tagBlockHead*)_pMap->getAbsolute(iAddr)); }

        /**
         * @brief ��ȡMemBlockͷ��ַ
         *
         * @return void*
         */
        tagBlockHead *getBlockHead()    { return _pHead; }

        /**
         * @brief �Ƿ������ӽڵ�
         * 
         * @return bool
         */
        bool hasLeft();

        /**
         * ��ǰԪ���ƶ������ӽڵ�
         * @return true, �Ƶ���һ��block��, false, û����һ��block
         *
         */
        bool moveToLeft();

        /**
         * @brief �Ƿ������ӽڵ�
         * 
         * @return bool
         */
        bool hasRight();

        /**
         * @brief ��ǰԪ���ƶ����ӽڵ�
         * @return true, �Ƶ���һ��block��, false, û����һ��block
         *
         */
        bool moveToRight();

        /**
         * @brief �Ƿ��и��ڵ�
         * 
         * @return bool
         */
        bool hasParent();

        /**
         * @brief ��ǰԪ���ƶ���
         * @return true, �Ƶ���һ��block��, false, û����һ��block
         *
         */
        bool moveToParent();

        /**
         * @brief ͷ��
         *
         * @return uint32_t
         */
        uint32_t getHead()  { return _iHead;}

        /**
         * ����Getʱ��
         *
         * @return time_t
         */
        time_t getSyncTime() { return getBlockHead()->_iSyncTime; }

        /**
         * ���û�дʱ��
         * @param iSyncTime
         */
        void setSyncTime(time_t iSyncTime) { getBlockHead()->_iSyncTime = iSyncTime; }

        /**
         * ��ȡBlock�е�����
         *
         * @return int
         *          TC_RBTree::RT_OK, ����, �����쳣
         *          TC_RBTree::RT_ONLY_KEY, ֻ��Key
         *          �����쳣
         */
        int getBlockData(TC_RBTree::BlockData &data);

        /**
         * ��ȡ����
         * @param pData
         * @param iDatalen
         * @return int,
         *          TC_RBTree::RT_OK, ����
         *          �����쳣
         */
        int get(void *pData, uint32_t &iDataLen);

        /**
         * ��ȡ����
         * @param s
         * @return int
         *          TC_RBTree::RT_OK, ����
         *          �����쳣
         */
        int get(string &s);

        /**
         * ��������
         * @param pData
         * @param iDatalen
         * @param vtData, ��̭������
         */
        int set(const string& k, const string& v, bool bNewBlock, bool bOnlyKey, vector<TC_RBTree::BlockData> &vtData);

        /**
         * �Ƿ���������
         *
         * @return bool
         */
        bool isDirty()      { return getBlockHead()->_bDirty; }

        /**
         * ��������
         * @param b
         */
        void setDirty(bool b);

        /**
         * �Ƿ�ֻ��key
         *
         * @return bool
         */
        bool isOnlyKey()    { return getBlockHead()->_bOnlyKey; }

        /**
         * �ͷ�block�����пռ�
         */
        void deallocate();

        /**
         * ��blockʱ���øú���
         * ����һ���µ�block
         * @param iAllocSize, �ڴ��С
         */
        void makeNew(uint32_t iAllocSize);

        /**
         * �����½ڵ�
         * 
         * @param i 
         * @param k 
         */
        void insertRBTree(tagBlockHead *i, const string &k);

        /**
         * ��Block������ɾ����ǰBlock
         * ֻ��Block��Ч, ��Chunk����Ч��
         * @return
         */
        void erase();

        /**
         * ˢ��set����, ����Set����ͷ��
         */
        void refreshSetList();

        /**
         * ˢ��get����, ����Get����ͷ��
         */
        void refreshGetList();

    protected:

        Block& operator=(const Block &mb);
        bool operator==(const Block &mb) const;
        bool operator!=(const Block &mb) const;

        /**
         * ��ȡChunkͷ���Ե�ַ
         *
         * @return tagChunkHead*
         */
        tagChunkHead *getChunkHead()                {return getChunkHead(_iHead);}

        /**
         * ��ȡchunkͷ���Ե�ַ
         * @param iAddr
         *
         * @return tagChunkHead*
         */
        tagChunkHead *getChunkHead(uint32_t iAddr)  { return ((tagChunkHead*)_pMap->getAbsolute(iAddr)); }

        /**
         * �ӵ�ǰ��chunk��ʼ�ͷ�
         * @param iChunk �ͷŵ�ַ
         */
        void deallocate(uint32_t iChunk);

        /**
         * ���������������, ��������chunk, ��Ӱ��ԭ������
         * ʹ�����ӵ�����������iDataLen
         * �ͷŶ����chunk
         * @param iDataLen
         *
         * @return int,
         */
        int allocate(uint32_t iDataLen, vector<TC_RBTree::BlockData> &vtData);

        /**
         * �ҽ�chunk, ���core��ҽ�ʧ��, ��֤�ڴ�黹������
         * @param pChunk
         * @param chunks
         *
         * @return int
         */
        int joinChunk(tagChunkHead *pChunk, const vector<uint32_t> chunks);

        /**
         * ����n��chunk��ַ, ע���ͷ��ڴ��ʱ�����ͷ����ڷ���Ķ���
         * @param fn, ����ռ��С
         * @param chunks, ����ɹ����ص�chunks��ַ�б�
         * @param vtData, ��̭������
         * @return int
         */
        int allocateChunk(uint32_t fn, vector<uint32_t> &chunks, vector<TC_RBTree::BlockData> &vtData);

        /**
         * ��ȡ���ݳ���
         *
         * @return uint32_t
         */
        uint32_t getDataLen();

        /**
         * ����
         * 
         * @param i
         */
        void rotateLeft(tagBlockHead *i, uint32_t iAddr);

        /**
         * ����
         * 
         * @param i
         */
        void rotateRight(tagBlockHead *i, uint32_t iAddr);

        /**
         * ɾ�������
         * 
         * @param i 
         */
        void eraseFixUp(tagBlockHead *i, uint32_t iAddr, tagBlockHead *p, uint32_t iPAddr);

        /**
         * ɾ��
         * 
         * @param i 
         */
        void erase(tagBlockHead *i, uint32_t iAddr);

        /**
         * ��������
         * 
         * @param i 
         */
        void insertFixUp(tagBlockHead *i, uint32_t iAddr);

        /**
         * ���뵽get/set������
         */
        void insertGetSetList(TC_RBTree::Block::tagBlockHead *i);

    private:

        /**
         * Map
         */
        TC_RBTree         *_pMap;

        /**
         * block�����׵�ַ, ��Ե�ַ
         */
        uint32_t          _iHead;

        /**
         * ͷ����ָ��
         */
        tagBlockHead *    _pHead;
    };

    ////////////////////////////////////////////////////////////////////////
    /*
    * �ڴ����ݿ������
    *
    */
    class BlockAllocator
    {
    public:

        /**
         * ���캯��
         */
        BlockAllocator(TC_RBTree *pMap)
        : _pMap(pMap)
        , _pChunkAllocator(new TC_MemMultiChunkAllocator())
        {
        }

        /**
         * ��������
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
         * ��ʼ��
         * @param pHeadAddr, ��ַ, ����Ӧ�ó���ľ��Ե�ַ
         * @param iSize, �ڴ��С
         * @param iMinBlockSize, ��С���ݿ��С
         * @param iMaxBlockSize, ������ݿ��С
         * @param fFactor, ����
         */
        void create(void *pHeadAddr, size_t iSize, size_t iMinBlockSize, size_t iMaxBlockSize, float fFactor)
        {
            _pChunkAllocator->create(pHeadAddr, iSize, iMinBlockSize, iMaxBlockSize, fFactor);
        }

        /**
         * ������
         * @param pAddr, ��ַ, ����Ӧ�ó���ľ��Ե�ַ
         */
        void connect(void *pHeadAddr)
        {
            _pChunkAllocator->connect(pHeadAddr);
        }

        /**
         * ��չ�ռ�
         * @param pAddr
         * @param iSize
         */
        void append(void *pAddr, size_t iSize)
        {
            _pChunkAllocator->append(pAddr, iSize);
        }

        /**
         * �ؽ�
         */
        void rebuild()
        {
            _pChunkAllocator->rebuild();
        }

        /**
         * ��ȡÿ�����ݿ�ͷ����Ϣ
         *
         * @return TC_MemChunk::tagChunkHead
         */
        vector<TC_MemChunk::tagChunkHead> getBlockDetail() const  { return _pChunkAllocator->getBlockDetail(); }

        /**
         * �ڴ��С
         *
         * @return size_t
         */
        size_t getMemSize() const       { return _pChunkAllocator->getMemSize(); }

        /**
         * ��������������
         *
         * @return size_t
         */
        size_t getCapacity() const      { return _pChunkAllocator->getCapacity(); }

        /**
         * ÿ��block�е�chunk����(ÿ��block�е�chunk������ͬ)
         *
         * @return vector<size_t>
         */
        vector<size_t> singleBlockChunkCount() const { return _pChunkAllocator->singleBlockChunkCount(); }

        /**
         * ����block��chunk����
         *
         * @return size_t
         */
        size_t allBlockChunkCount() const    { return _pChunkAllocator->allBlockChunkCount(); }

        /**
         * ���ڴ��з���һ���µ�Block
         *
         * @param iAllocSize: in/��Ҫ����Ĵ�С, out/����Ŀ��С
         * @param vtData, �����ͷŵ��ڴ������
         * @return uint32_t, ��Ե�ַ,0��ʾû�пռ���Է���
         */
        uint32_t allocateMemBlock(uint32_t &iAllocSize, vector<TC_RBTree::BlockData> &vtData);

        /**
         * Ϊ��ַΪiAddr��Block����һ��chunk
         *
         * @param iAddr,�����Block�ĵ�ַ
         * @param iAllocSize, in/��Ҫ����Ĵ�С, out/����Ŀ��С
         * @param vtData �����ͷŵ��ڴ������
         * @return uint32_t, ��Ե�ַ,0��ʾû�пռ���Է���
         */
        uint32_t allocateChunk(uint32_t iAddr, uint32_t &iAllocSize, vector<TC_RBTree::BlockData> &vtData);

        /**
         * �ͷ�Block
         * @param v
         */
        void deallocateMemBlock(const vector<uint32_t> &v);

        /**
         * �ͷ�Block
         * @param v
         */
        void deallocateMemBlock(uint32_t v);

    protected:
        //������copy����
        BlockAllocator(const BlockAllocator &);
        //������ֵ
        BlockAllocator& operator=(const BlockAllocator &);
        bool operator==(const BlockAllocator &mba) const;
        bool operator!=(const BlockAllocator &mba) const;

    public:
        /**
         * map
         */
        TC_RBTree                  *_pMap;

        /**
         * chunk������
         */
        TC_MemMultiChunkAllocator   *_pChunkAllocator;
    };

    ////////////////////////////////////////////////////////////////
    // map��������
    class RBTreeLockItem
    {
    public:

        /**
         *
         * @param pMap
         * @param iAddr
         */
        RBTreeLockItem(TC_RBTree *pMap, uint32_t iAddr);

        /**
         *
         * @param mcmdi
         */
        RBTreeLockItem(const RBTreeLockItem &mcmdi);

        /**
         * 
         */
        RBTreeLockItem(){};

        /**
         *
         * @param mcmdi
         *
         * @return RBTreeLockItem&
         */
        RBTreeLockItem &operator=(const RBTreeLockItem &mcmdi);

        /**
         *
         * @param mcmdi
         *
         * @return bool
         */
        bool operator==(const RBTreeLockItem &mcmdi);

        /**
         *
         * @param mcmdi
         *
         * @return bool
         */
        bool operator!=(const RBTreeLockItem &mcmdi);

        /**
         * �Ƿ���������
         *
         * @return bool
         */
        bool isDirty();

        /**
         * �Ƿ�ֻ��Key
         *
         * @return bool
         */
        bool isOnlyKey();

        /**
         * ���Syncʱ��
         *
         * @return time_t
         */
        time_t getSyncTime();

        /**
         * ��ȡֵ, ���ֻ��Key(isOnlyKey)�������, vΪ��
         * @return int
         *          RT_OK:���ݻ�ȡOK
         *          RT_ONLY_KEY: key��Ч, v��ЧΪ��
         *          ����ֵ, �쳣
         *
         */
        int get(string& k, string& v);

        /**
         * ��ȡֵ
         * @return int
         *          RT_OK:���ݻ�ȡOK
         *          ����ֵ, �쳣
         */
        int get(string& k);

        /**
         * ���ݿ���Ե�ַ
         *
         * @return uint32_t
         */
        uint32_t getAddr() const { return _iAddr; }

    protected:

        /**
         * ��������
         * @param k
         * @param v
         * @param vtData, ��̭������
         * @return int
         */
        int set(const string& k, const string& v, bool bNewBlock, vector<TC_RBTree::BlockData> &vtData);

        /**
         * ����Key, ������
         * @param k
         * @param vtData
         *
         * @return int
         */
        int set(const string& k, bool bNewBlock, vector<TC_RBTree::BlockData> &vtData);

        /**
         *
         * @param pKey
         * @param iKeyLen
         *
         * @return bool
         */
        bool equal(const string &k, string &k1, string &v, int &ret);

        /**
         *
         * @param pKey
         * @param iKeyLen
         *
         * @return bool
         */
        bool equal(const string& k, string &k1, int &ret);

        /**
         * ��һ��item
         *
         * @return RBTreeLockItem
         */
        void nextItem(int iType);

        /**
         * ��һ��item
         * @param iType
         */
        void prevItem(int iType);

        friend class TC_RBTree;
        friend struct TC_RBTree::RBTreeLockIterator;

    private:
        /**
         * map
         */
        TC_RBTree *_pMap;

        /**
         * block�ĵ�ַ
         */
        uint32_t  _iAddr;
    };

    /////////////////////////////////////////////////////////////////////////
    // ���������
    struct RBTreeLockIterator
    {
    public:

        //���������ʽ
        enum
        {
            IT_RBTREE       = 1,        //rbtree��С����
            IT_SET          = 2,        //Setʱ��˳��
            IT_GET          = 3,        //Getʱ��˳��
        };

        //����˳��
        enum
        {
            IT_NEXT = 0,
            IT_PREV = 1,
        };

        /**
         *
         */
        RBTreeLockIterator();

        /**
         * ���캯��
         * @param iAddr, ��ַ
         * @param type
         */
        RBTreeLockIterator(TC_RBTree *pMap, uint32_t iAddr, int iType, int iOrder);

        /**
         * copy
         * @param it
         */
        RBTreeLockIterator(const RBTreeLockIterator &it);

        /**
         * ����
         * @param it
         *
         * @return RBTreeLockIterator&
         */
        RBTreeLockIterator& operator=(const RBTreeLockIterator &it);

        /**
         *
         * @param mcmi
         *
         * @return bool
         */
        bool operator==(const RBTreeLockIterator& mcmi);

        /**
         *
         * @param mv
         *
         * @return bool
         */
        bool operator!=(const RBTreeLockIterator& mcmi);

        /**
         * ǰ��++
         *
         * @return RBTreeLockIterator&
         */
        RBTreeLockIterator& operator++();

        /**
         * ����++
         *
         * @return RBTreeLockIterator&
         */
        RBTreeLockIterator operator++(int);

        /**
         *
         *
         * @return RBTreeLockItem&i
         */
        RBTreeLockItem& operator*() { return _iItem; }

        /**
         *
         *
         * @return RBTreeLockItem*
         */
        RBTreeLockItem* operator->() { return &_iItem; }

        /**
         * ����˳��
         * @param iOrder
         */
        void setOrder(int iOrder) { _iOrder = iOrder; }

    public:
        /**
         *
         */
        TC_RBTree  *_pMap;

        /**
         *
         */
        RBTreeLockItem _iItem;

        /**
         * �������ķ�ʽ
         */
        int         _iType;

        /**
         * ˳��
         */
        int         _iOrder;
    };

    ////////////////////////////////////////////////////////////////
    // map��������
    class RBTreeItem
    {
    public:

        /**
         *
         * @param pMap
         * @param key
         */
        RBTreeItem(TC_RBTree *pMap, const string &key, bool bEnd);

        /**
         *
         * @param mcmdi
         */
        RBTreeItem(const RBTreeItem &mcmdi);

        /**
         * 
         */
        RBTreeItem() : _bEnd(true)
        {
        }

        /**
         *
         * @param mcmdi
         *
         * @return RBTreeLockItem&
         */
        RBTreeItem &operator=(const RBTreeItem &mcmdi);

        /**
         *
         * @param mcmdi
         *
         * @return bool
         */
        bool operator==(const RBTreeItem &mcmdi);

        /**
         *
         * @param mcmdi
         *
         * @return bool
         */
        bool operator!=(const RBTreeItem &mcmdi);

        /**
         * ��ȡ��ǰ����
         * 
         * @return RT_OK, ��ȡ�ɹ�
         *         RT_ONLY_KEY, ��onlykey
         *         RT_NO_DATA, ������
         *         RT_EXCEPTION_ERR, �쳣
         */
        int get(TC_RBTree::BlockData &stData);

    protected:

        /**
         * ��ȡkey
         * 
         * @return string
         */
        string getKey() const { return _key; }

        /**
         * ������
         * 
         * @return bool
         */
        bool isEnd() const { return _bEnd; }

        /**
         * ��һ��item
         *
         * @return 
         */
        void nextItem();

        /**
         * ��һ��item
         */
        void prevItem();

        friend class TC_RBTree;
        friend struct TC_RBTree::RBTreeIterator;

    private:
        /**
         * map
         */
        TC_RBTree *_pMap;

        /**
         * block�ĵ�ַ
         */
        string    _key;

        /**
         * �Ƿ��ǽ�β
         */
        bool      _bEnd;
    };

    /////////////////////////////////////////////////////////////////////////
    // ���������
    struct RBTreeIterator
    {
    public:

        //����˳��
        enum
        {
            IT_NEXT = 0,
            IT_PREV = 1,
        };

        /**
         *
         */
        RBTreeIterator();

        /**
         * ���캯��
         * @param iAddr, ��ַ
         * @param type
         */
        RBTreeIterator(TC_RBTree *pMap, const string &key, bool bEnd, int iOrder);

        /**
         * copy
         * @param it
         */
        RBTreeIterator(const RBTreeIterator &it);

        /**
         * ����
         * @param it
         *
         * @return RBTreeIterator&
         */
        RBTreeIterator& operator=(const RBTreeIterator &it);

        /**
         *
         * @param mcmi
         *
         * @return bool
         */
        bool operator==(const RBTreeIterator& mcmi);

        /**
         *
         * @param mv
         *
         * @return bool
         */
        bool operator!=(const RBTreeIterator& mcmi);

        /**
         * ǰ��++
         *
         * @return RBTreeIterator&
         */
        RBTreeIterator& operator++();

        /**
         * ����++
         *
         * @return RBTreeIterator&
         */
        RBTreeIterator operator++(int);

        /**
         *
         *
         * @return RBTreeItem&i
         */
        RBTreeItem& operator*() { return _iItem; }

        /**
         *
         *
         * @return RBTreeItem*
         */
        RBTreeItem* operator->() { return &_iItem; }

    public:
        /**
         *
         */
        TC_RBTree   *_pMap;

        /**
         *
         */
        RBTreeItem  _iItem;

        /**
         * ˳��
         */
        int         _iOrder;
    };

public:
    //////////////////////////////////////////////////////////////////////////////////////////////////
    // map����
    //
    /**
     * mapͷ
     */
    struct tagMapHead
    {
        char        _cMaxVersion;        //��汾
        char        _cMinVersion;        //С�汾
        bool        _bReadOnly;          //�Ƿ�ֻ��
        bool        _bAutoErase;         //�Ƿ�����Զ���̭
        char        _cEraseMode;         //��̭��ʽ:0x00:����Get����̭, 0x01:����Set����̭
        size_t      _iMemSize;           //�ڴ��С
        uint32_t    _iMinDataSize;       //��С���ݿ��С
        uint32_t    _iMaxDataSize;       //������ݿ��С
        float       _fFactor;            //����
        uint32_t    _iElementCount;      //��Ԫ�ظ���
        uint32_t    _iEraseCount;        //ÿ��ɾ������
        uint32_t    _iDirtyCount;        //�����ݸ���
        uint32_t    _iSetHead;           //Setʱ������ͷ��
        uint32_t    _iSetTail;           //Setʱ������β��
        uint32_t    _iGetHead;           //Getʱ������ͷ��
        uint32_t    _iGetTail;           //Getʱ������β��
        uint32_t    _iDirtyTail;         //��������β��
        time_t      _iSyncTime;          //��дʱ��
        uint32_t    _iUsedChunk;         //�Ѿ�ʹ�õ��ڴ��
        uint32_t    _iGetCount;          //get����
        uint32_t    _iHitCount;          //���д���
        uint32_t    _iBackupTail;        //�ȱ�ָ��
        uint32_t    _iSyncTail;          //��д����
        uint32_t    _iOnlyKeyCount;        // OnlyKey����
        uint32_t    _iRootAddr;          //��Ԫ�ص�ַ
        uint32_t    _iReserve[4];       //����
    }__attribute__((packed));

    /**
     * ��Ҫ�޸ĵĵ�ַ
     */
    struct tagModifyData
    {
        size_t  _iModifyAddr;       //�޸ĵĵ�ַ
        char    _cBytes;            //�ֽ���
        size_t  _iModifyValue;      //ֵ
    }__attribute__((packed));

    /**
     * �޸����ݿ�ͷ��
     */
    struct tagModifyHead
    {
        char            _cModifyStatus;         //�޸�״̬: 0:Ŀǰû�����޸�, 1: ��ʼ׼���޸�, 2:�޸����, û��copy���ڴ���
        uint32_t        _iNowIndex;             //���µ�Ŀǰ������, ���ܲ���1000��
        tagModifyData   _stModifyData[1000];    //һ�����1000���޸�
    }__attribute__((packed));

    //64λ����ϵͳ�û���С�汾��, 32λ����ϵͳ��ż��С�汾
    //ע����hashmap�İ汾���
#if __WORDSIZE == 64

    //����汾��
    enum
    {
        MAX_VERSION         = 2,    //��ǰmap�Ĵ�汾��
        MIN_VERSION         = 1,    //��ǰmap��С�汾��
    };

#else
    //����汾��
    enum
    {
        MAX_VERSION         = 2,    //��ǰmap�Ĵ�汾��
        MIN_VERSION         = 0,    //��ǰmap��С�汾��
    };

#endif

    //������̭��ʽ
    enum
    {
        ERASEBYGET          = 0x00, //����Get������̭
        ERASEBYSET          = 0x01, //����Set������̭
    };

    const static char RED       = 0;    //��ɫ�ڵ�
    const static char BLACK     = 1;    //��ɫ�ڵ�

    /**
     * get, set��int����ֵ
     */
    enum
    {
        RT_OK                   = 0,    //�ɹ�
        RT_DIRTY_DATA           = 1,    //������
        RT_NO_DATA              = 2,    //û������
        RT_NEED_SYNC            = 3,    //��Ҫ��д
        RT_NONEED_SYNC          = 4,    //����Ҫ��д
        RT_ERASE_OK             = 5,    //��̭���ݳɹ�
        RT_READONLY             = 6,    //mapֻ��
        RT_NO_MEMORY            = 7,    //�ڴ治��
        RT_ONLY_KEY             = 8,    //ֻ��Key, û��Value
        RT_NEED_BACKUP          = 9,    //��Ҫ����
        RT_NO_GET               = 10,   //û��GET��
        RT_DECODE_ERR           = -1,   //��������
        RT_EXCEPTION_ERR        = -2,   //�쳣
        RT_LOAD_DATA_ERR        = -3,   //���������쳣
        RT_VERSION_MISMATCH_ERR = -4,   //�汾��һ��
        RT_DUMP_FILE_ERR        = -5,   //dump���ļ�ʧ��
        RT_LOAL_FILE_ERR        = -6,   //load�ļ����ڴ�ʧ��
        RT_NOTALL_ERR           = -7,   //û�и�����ȫ
    };

    //���������
    typedef RBTreeLockIterator lock_iterator;
    typedef RBTreeIterator nolock_iterator;

    //����key�Ƚϴ�����
    typedef TC_Functor<bool, TL::TLMaker<const string &, const string &>::Result> less_functor;

    /**
     * ȱʡ��Сд�ȽϷ���
     */
    struct default_less
    {
        bool operator()(const string &k1, const string &k2)
        {
            return k1 < k2;
        }
    };

    //////////////////////////////////////////////////////////////////////////////////////////////
    //map�Ľӿڶ���
    /**
     * ���캯��
     */
    TC_RBTree()
    : _iMinDataSize(0)
    , _iMaxDataSize(0)
    , _fFactor(1.0)
    , _lock_end(this, 0, 0, 0)
    , _nolock_end(this, "", true, 0)
    , _pDataAllocator(new BlockAllocator(this))
    , _lessf(default_less())
    {
    }

    /**
     * ��ʼ�����ݿ�ƽ����С
     * ��ʾ�ڴ�����ʱ�򣬻����n����С�飬 n������С��*�������ӣ�, n������С��*��������*�������ӣ�..., ֱ��n������
     * n��hashmap�Լ����������
     * ���ַ������ͨ�������ݿ��¼�䳤�Ƚ϶��ʹ�ã� ���ڽ�Լ�ڴ棬������ݼ�¼�������Ǳ䳤�ģ� ����С��=���죬��������=1�Ϳ�����
     * @param iMinDataSize: ��С���ݿ��С
     * @param iMaxDataSize: ������ݿ��С
     * @param fFactor: ��������
     */
    void initDataBlockSize(uint32_t iMinDataSize, uint32_t iMaxDataSize, float fFactor);

    /**
     * ��ʼ��, ֮ǰ��Ҫ����:initDataAvgSize��initHashRadio
     * @param pAddr ���Ե�ַ
     * @param iSize ��С
     * @return ʧ�����׳��쳣
     */
    void create(void *pAddr, size_t iSize);

    /**
     * ���ӵ��ڴ��
     * @param pAddr, ��ַ
     * @param iSize, �ڴ��С
     * @return ʧ�����׳��쳣
     */
    void connect(void *pAddr, size_t iSize);

    /**
     * ԭ�������ݿ��������չ�ڴ�, ע��ͨ��ֻ�ܶ�mmap�ļ���Ч
     * (���iSize�ȱ������ڴ��С,�򷵻�-1)
     * @param pAddr, ��չ��Ŀռ�
     * @param iSize
     * @return 0:�ɹ�, -1:ʧ��
     */
    int append(void *pAddr, size_t iSize);

    /**
     * ��ȡÿ�ִ�С�ڴ���ͷ����Ϣ
     *
     * @return vector<TC_MemChunk::tagChunkHead>: ��ͬ��С�ڴ��ͷ����Ϣ
     */
    vector<TC_MemChunk::tagChunkHead> getBlockDetail() { return _pDataAllocator->getBlockDetail(); }

    /**
     * ����block��chunk�ĸ���
     *
     * @return size_t
     */
    size_t allBlockChunkCount()                     { return _pDataAllocator->allBlockChunkCount(); }

    /**
     * ÿ��block��chunk�ĸ���(��ͬ��С�ڴ��ĸ�����ͬ)
     *
     * @return vector<size_t>
     */
    vector<size_t> singleBlockChunkCount()          { return _pDataAllocator->singleBlockChunkCount(); }

    /**
     * Ԫ�صĸ���
     *
     * @return size_t
     */
    uint32_t size()                                   { return _pHead->_iElementCount; }

    /**
     * ������Ԫ�ظ���
     *
     * @return size_t
     */
    uint32_t dirtyCount()                             { return _pHead->_iDirtyCount;}

    /**
     * OnlyKey����Ԫ�ظ���
     *
     * @return uint32_t
     */
    uint32_t onlyKeyCount()                             { return _pHead->_iOnlyKeyCount;}

    /**
     * ����ÿ����̭����
     * @param n
     */
    void setEraseCount(uint32_t n)                    { _pHead->_iEraseCount = n; }

    /**
     * ��ȡÿ����̭����
     *
     * @return uint32_t
     */
    uint32_t getEraseCount()                          { return _pHead->_iEraseCount; }

    /**
     * ����ֻ��
     * @param bReadOnly
     */
    void setReadOnly(bool bReadOnly)                { _pHead->_bReadOnly = bReadOnly; }

    /**
     * �Ƿ�ֻ��
     *
     * @return bool
     */
    bool isReadOnly()                               { return _pHead->_bReadOnly; }

    /**
     * �����Ƿ�����Զ���̭
     * @param bAutoErase
     */
    void setAutoErase(bool bAutoErase)              { _pHead->_bAutoErase = bAutoErase; }

    /**
     * �Ƿ�����Զ���̭
     *
     * @return bool
     */
    bool isAutoErase()                              { return _pHead->_bAutoErase; }

    /**
     * ������̭��ʽ
     * TC_RBTree::ERASEBYGET
     * TC_RBTree::ERASEBYSET
     * @param cEraseMode
     */
    void setEraseMode(char cEraseMode)              { _pHead->_cEraseMode = cEraseMode; }

    /**
     * ��ȡ��̭��ʽ
     *
     * @return bool
     */
    char getEraseMode()                             { return _pHead->_cEraseMode; }

    /**
     * ���û�дʱ��(��)
     * @param iSyncTime
     */
    void setSyncTime(time_t iSyncTime)              { _pHead->_iSyncTime = iSyncTime; }

    /**
     * ��ȡ��дʱ��
     *
     * @return time_t
     */
    time_t getSyncTime()                            { return _pHead->_iSyncTime; }

    /**
     * ��ȡͷ��������Ϣ
     * 
     * @return tagMapHead&
     */
    tagMapHead& getMapHead()                        { return *_pHead; }

    /**
     * dump���ļ�
     * @param sFile
     *
     * @return int
     *          RT_DUMP_FILE_ERR: dump���ļ�����
     *          RT_OK: dump���ļ��ɹ�
     */
    int dump2file(const string &sFile);

    /**
     * ���ļ�load
     * @param sFile
     *
     * @return int
     *          RT_LOAL_FILE_ERR: load����
     *          RT_VERSION_MISMATCH_ERR: �汾��һ��
     *          RT_OK: load�ɹ�
     */
    int load5file(const string &sFile);

    /**
     * ���hashmap
     * ����map�����ݻָ�����ʼ״̬
     */
    void clear();

    /**
     * ������ݸɾ�״̬
     * @param k
     *
     * @return int
     *          RT_NO_DATA: û�е�ǰ����
     *          RT_ONLY_KEY:ֻ��Key
     *          RT_DIRTY_DATA: ��������
     *          RT_OK: �Ǹɾ�����
     *          ��������ֵ: ����
     */
    int checkDirty(const string &k);

    /**
     * ����Ϊ������, �޸�SETʱ����, �ᵼ�����ݻ�д
     * @param k
     *
     * @return int
     *          RT_READONLY: ֻ��
     *          RT_NO_DATA: û�е�ǰ����
     *          RT_ONLY_KEY:ֻ��Key
     *          RT_OK: ���������ݳɹ�
     *          ��������ֵ: ����
     */
    int setDirty(const string& k);

    /**
     * ����Ϊ�ɾ�����, �޸�SET��, �������ݲ���д
     * @param k
     *
     * @return int
     *          RT_READONLY: ֻ��
     *          RT_NO_DATA: û�е�ǰ����
     *          RT_ONLY_KEY:ֻ��Key
     *          RT_OK: ���óɹ�
     *          ��������ֵ: ����
     */
    int setClean(const string& k);

    /**
     * ��ȡ����, �޸�GETʱ����
     * @param k
     * @param v
     * @param iSyncTime:�����ϴλ�д��ʱ��
     *
     * @return int:
     *          RT_NO_DATA: û������
     *          RT_ONLY_KEY:ֻ��Key
     *          RT_OK:��ȡ���ݳɹ�
     *          ��������ֵ: ����
     */
    int get(const string& k, string &v, time_t &iSyncTime);

    /**
     * ��ȡ����, �޸�GETʱ����
     * @param k
     * @param v
     *
     * @return int:
     *          RT_NO_DATA: û������
     *          RT_ONLY_KEY:ֻ��Key
     *          RT_OK:��ȡ���ݳɹ�
     *          ��������ֵ: ����
     */
    int get(const string& k, string &v);

    /**
     * ��������, �޸�ʱ����, �ڴ治��ʱ���Զ���̭�ϵ�����
     * @param k: �ؼ���
     * @param v: ֵ
     * @param bDirty: �Ƿ���������
     * @param vtData: ����̭�ļ�¼
     * @return int:
     *          RT_READONLY: mapֻ��
     *          RT_NO_MEMORY: û�пռ�(����̭��������»����)
     *          RT_OK: ���óɹ�
     *          ��������ֵ: ����
     */
    int set(const string& k, const string& v, bool bDirty, vector<BlockData> &vtData);

    /**
     * ����key, ��������
     * @param k
     * @param vtData
     *
     * @return int
     *          RT_READONLY: mapֻ��
     *          RT_NO_MEMORY: û�пռ�(����̭��������»����)
     *          RT_OK: ���óɹ�
     *          ��������ֵ: ����
     */
    int set(const string& k, vector<BlockData> &vtData);

    /**
     * ɾ������
     * @param k, �ؼ���
     * @param data, ��ɾ���ļ�¼
     * @return int:
     *          RT_READONLY: mapֻ��
     *          RT_NO_DATA: û�е�ǰ����
     *          RT_ONLY_KEY:ֻ��Key, ɾ���ɹ�
     *          RT_OK: ɾ�����ݳɹ�
     *         ��������ֵ: ����
     */
    int del(const string& k, BlockData &data);

    /**
     * ��̭����, ÿ��ɾ��һ��, ����Getʱ����̭
     * �ⲿѭ�����øýӿ���̭����
     * ֱ��: Ԫ�ظ���/chunks * 100 < radio, bCheckDirty Ϊtrueʱ����������������̭����
     * @param radio: �����ڴ�chunksʹ�ñ��� 0< radio < 100
     * @param data: ��ǰ��ɾ����һ����¼
     * @return int:
     *          RT_READONLY: mapֻ��
     *          RT_OK: �����ټ�����̭��
     *          RT_ONLY_KEY:ֻ��Key, ɾ���ɹ�
     *          RT_DIRTY_DATA:�����������ݣ���bCheckDirty=trueʱ���п��ܲ������ַ���ֵ
     *          RT_ERASE_OK:��̭��ǰ���ݳɹ�, ������̭
     *          ��������ֵ: ����, ͨ������, ��������erase��̭
     */
    int erase(int radio, BlockData &data, bool bCheckDirty = false);

    /**
     * ��д, ÿ�η�����Ҫ��д��һ��
     * ���ݻ�дʱ���뵱ǰʱ�䳬��_pHead->_iSyncTime����Ҫ��д
     * _pHead->_iSyncTime��setSyncTime�����趨, Ĭ��10����

     * �ⲿѭ�����øú������л�д
     * mapֻ��ʱ��Ȼ���Ի�д
     * @param iNowTime: ��ǰʱ��
     *                  ��дʱ���뵱ǰʱ�����_pHead->_iSyncTime����Ҫ��д
     * @param data : ��д������
     * @return int:
     *          RT_OK: ������������ͷ����, ����sleepһ���ٳ���
     *          RT_ONLY_KEY:ֻ��Key, ɾ���ɹ�, ��ǰ���ݲ�Ҫ��д,��������sync��д
     *          RT_NEED_SYNC:��ǰ���ص�data������Ҫ��д
     *          RT_NONEED_SYNC:��ǰ���ص�data���ݲ���Ҫ��д
     *          ��������ֵ: ����, ͨ������, ��������sync��д
     */
    int sync(time_t iNowTime, BlockData &data);

    /**
     * ��ʼ��д, ������дָ��
     */
    void sync();

    /**
     * ��ʼ����֮ǰ���øú���
     *
     * @param bForceFromBegin: �Ƿ�ǿ����ͷ��ʼ����
     * @return void
     */
    void backup(bool bForceFromBegin = false);

    /**
     * ��ʼ��������, ÿ�η�����Ҫ���ݵ�һ������
     * @param data
     *
     * @return int
     *          RT_OK: �������
     *          RT_NEED_BACKUP:��ǰ���ص�data������Ҫ����
     *          RT_ONLY_KEY:ֻ��Key, ��ǰ���ݲ�Ҫ����
     *          ��������ֵ: ����, ͨ������, ��������backup
     */
    int backup(BlockData &data);

    /**
     * ���ñȽϷ�ʽ
     * @param lessf
     */
    void setLessFunctor(less_functor lessf)         { _lessf = lessf; }

    /**
     * ��ȡ�ȽϷ�ʽ
     * 
     * @return less_functor& 
     */
    less_functor &getLessFunctor()                  { return _lessf; }

    /////////////////////////////////////////////////////////////////////////////////////////
    // �����Ǳ���map����, ����Ҫ��map�Ӵ������, ���Ǳ���Ч����һ��Ӱ��
    // (ֻ��get�Լ�������++��ʱ�����)
    // ��ȡ�ĵ����������ݲ���֤ʵʱ��Ч,�����Ѿ���ɾ����,��ȡ����ʱ��Ҫ�ж����ݵĺϷ���
    
    /**
     * ����
     *
     * @return
     */
    nolock_iterator nolock_end() { return _nolock_end; }

    /**
     * ����С��������
     * 
     * @return nolock_iterator
     */
    nolock_iterator nolock_begin();

    /**
     * ���Ӵ�С����
     * 
     * @return nolock_iterator
     */
    nolock_iterator nolock_rbegin();

    /////////////////////////////////////////////////////////////////////////////////////////
    // �����Ǳ���map����, ��Ҫ��map�Ӵ������(��������������Ч��Χ��ȫ��������)
    // ��ȡ�������Լ�����������ʵʱ��Ч

    /**
     * ����
     *
     * @return
     */
    lock_iterator end() { return _lock_end; }

    /**
     * ����С��������(sort˳��)
     * 
     * @return lock_iterator 
     */
    lock_iterator begin();

    /**
     * ���Ӵ�С��(sort����)
     * 
     * @return lock_iterator 
     */
    lock_iterator rbegin();

    /**
     * ����(++��ʾ˳��)
     * 
     * @param k 
     * 
     * @return lock_iterator 
     */
    lock_iterator find(const string& k);

    /**
     * ����(++��ʾ����)
     * 
     * @param k 
     * 
     * @return lock_iterator 
     */
    lock_iterator rfind(const string& k);

    /**
     * ���ز��ҹؼ��ֵ��½�(���ؼ�ֵ>=����Ԫ�صĵ�һ��λ��)
     * map���Ѿ�������1,2,3,4�Ļ������lower_bound(2)�Ļ������ص�2����upper-bound(2)�Ļ������صľ���3
     * @param k
     * 
     * @return lock_iterator
     */
    lock_iterator lower_bound(const string &k);

    /**
     * ���ز��ҹؼ��ֵ��Ͻ�(���ؼ�ֵ>����Ԫ�صĵ�һ��λ��)
     * map���Ѿ�������1,2,3,4�Ļ������lower_bound(2)�Ļ������ص�2����upper-bound(2)�Ļ������صľ���3
     * @param k
     * 
     * @return lock_iterator
     */
    lock_iterator upper_bound(const string &k);

    /**
     * ����
     * @param k1
     * @param k2
     * 
     * @return pair<lock_iterator,lock_iterator>
     */
    pair<lock_iterator, lock_iterator> equal_range(const string& k1, const string &k2);

    //////////////////////////////////////////////////////////////////////////////////////
    // 
    /**
     * ��Setʱ������ĵ�����
     *
     * @return lock_iterator
     */
    lock_iterator beginSetTime();

    /**
     * Set������ĵ�����
     *
     * @return lock_iterator
     */
    lock_iterator rbeginSetTime();

    /**
     * ��Getʱ������ĵ�����
     *
     * @return lock_iterator
     */
    lock_iterator beginGetTime();

    /**
     * Get������ĵ�����
     *
     * @return lock_iterator
     */
    lock_iterator rbeginGetTime();

    /**
     * ��ȡ������β��������(�ʱ��û�в�����������)
     *
     * ���صĵ�����++��ʾ����ʱ��˳��==>(���ʱ��û�в�����������)
     *
     * @return lock_iterator
     */
    lock_iterator beginDirty();

    ///////////////////////////////////////////////////////////////////////////
    /**
     * ����
     *
     * @return string
     */
    string desc();

protected:

    friend class Block;
    friend class BlockAllocator;
    friend class RBTreeLockIterator;
    friend class RBTreeLockItem;

    //��ֹcopy����
    TC_RBTree(const TC_RBTree &mcm);
    //��ֹ����
    TC_RBTree &operator=(const TC_RBTree &mcm);

    struct FailureRecover
    {
        FailureRecover(TC_RBTree *pMap) : _pMap(pMap)
        {
            _pMap->doRecover();
        }

        ~FailureRecover()
        {
            _pMap->doUpdate();
        }
       
    protected:
        TC_RBTree   *_pMap;
    };

    /**
     * ��ʼ��
     * @param pAddr
     */
    void init(void *pAddr);

    /**
     * ���������ݸ���
     */
    void incDirtyCount()    { saveValue(&_pHead->_iDirtyCount, _pHead->_iDirtyCount+1); }

    /**
     * ���������ݸ���
     */
    void delDirtyCount()    { saveValue(&_pHead->_iDirtyCount, _pHead->_iDirtyCount-1); }

    /**
     * �������ݸ���
     */
    void incElementCount()  { saveValue(&_pHead->_iElementCount, _pHead->_iElementCount+1); }

    /**
     * �������ݸ���
     */
    void delElementCount()  { saveValue(&_pHead->_iElementCount, _pHead->_iElementCount-1); }

    /**
     * ����OnlyKey���ݸ���
     */
    void incOnlyKeyCount()    { saveValue(&_pHead->_iOnlyKeyCount, _pHead->_iOnlyKeyCount+1); }

    /**
     * ����OnlyKey���ݸ���
     */
    void delOnlyKeyCount()    { saveValue(&_pHead->_iOnlyKeyCount, _pHead->_iOnlyKeyCount-1); }

    /**
     * ����Chunk��
     * ֱ�Ӹ���, ��Ϊ�п���һ�η����chunk����
     * �������������ڴ�ռ�, ����Խ�����
     */
    void incChunkCount()    { saveValue(&_pHead->_iUsedChunk, _pHead->_iUsedChunk+1); }

    /**
     * ����Chunk��
     * ֱ�Ӹ���, ��Ϊ�п���һ���ͷŵ�chunk����
     * �������������ڴ�ռ�, ����Խ�����
     */
    void delChunkCount()    { saveValue(&_pHead->_iUsedChunk, _pHead->_iUsedChunk-1); }

    /**
     * ����hit����
     */
    void incGetCount()      { saveValue(&_pHead->_iGetCount, _pHead->_iGetCount+1); }

    /**
     * �������д���
     */
    void incHitCount()      { saveValue(&_pHead->_iHitCount, _pHead->_iHitCount+1); }

    /**
     * ��Ե�ַ���ɾ��Ե�ַ
     * @param iAddr
     *
     * @return void*
     */
    void *getAbsolute(uint32_t iAddr) { if(iAddr == 0) return NULL; return _pDataAllocator->_pChunkAllocator->getAbsolute(iAddr); }

    /**
     * ��̭iNowAddr֮�������(������̭������̭)
     * @param iNowAddr, ��ǰBlock�������ڷ���ռ�, ���ܱ���̭
     *                  0��ʾ��ֱ�Ӹ�����̭������̭
     * @param vector<BlockData>, ����̭������
     * @return uint32_t,��̭�����ݸ���
     */
    uint32_t eraseExcept(uint32_t iNowAddr, vector<BlockData> &vtData);

    /**
     * ����key������һ�����ڵ�ǰkey��block
     * @param k
     * 
     * @return size_t
     */
    Block getLastBlock(const string &k);

    /**
     * ��ĳ����ַ��ʼ����
     * 
     * @param iAddr 
     * @param k 
     * @param ret 
     * 
     * @return lock_iterator 
     */
    lock_iterator find(uint32_t iAddr, const string& k, int &ret, bool bOrder = true);

    /**
     * ��ĳ����ַ��ʼ����
     * 
     * @param iAddr 
     * @param k 
     * @param v 
     * @param ret 
     * 
     * @return lock_iterator 
     */
    lock_iterator find(uint32_t iAddr, const string& k, string &v, int &ret);

    /**
     * �޸ľ����ֵ
     * @param iModifyAddr
     * @param iModifyValue
     */
    template<typename T>
    void saveValue(void* iModifyAddr, T iModifyValue, bool bModify = true)
    {
        //��ȡԭʼֵ
        T tmp = *(T*)iModifyAddr;
        
        //����ԭʼֵ
        _pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._iModifyAddr  = (char*)iModifyAddr - (char*)_pHead;
        _pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._iModifyValue = tmp;
        _pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._cBytes       = sizeof(iModifyValue);
        _pstModifyHead->_iNowIndex++;

        _pstModifyHead->_cModifyStatus = 1;

        if(bModify)
        {
            //�޸ľ���ֵ
            *(T*)iModifyAddr = iModifyValue;
        }

        assert(_pstModifyHead->_iNowIndex < sizeof(_pstModifyHead->_stModifyData) / sizeof(tagModifyData));
    }

    /**
     * �ָ�����
     */
    void doRecover();

    /**
     * ȷ�ϴ������
     */
    void doUpdate();

protected:

    /**
     * ����ָ��
     */
    tagMapHead                  *_pHead;

    /**
     * ��С�����ݿ��С
     */
    uint32_t                    _iMinDataSize;

    /**
     * �������ݿ��С
     */
    uint32_t                    _iMaxDataSize;

    /**
     * �仯����
     */
    float                       _fFactor;

    /**
     * β��
     */
    lock_iterator               _lock_end;

    /**
     * β��
     */
    nolock_iterator             _nolock_end;

    /**
     * �޸����ݿ�
     */
    tagModifyHead               *_pstModifyHead;

    /**
     * block����������
     */
    BlockAllocator              *_pDataAllocator;

    /**
     * �ȽϹ�ʽ
     */
    less_functor                _lessf;

};

}

#endif
