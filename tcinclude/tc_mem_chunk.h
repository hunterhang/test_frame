#ifndef	__TC_MEM_CHUNK_H__
#define __TC_MEM_CHUNK_H__

#include <string>
#include <vector>

using namespace std;

namespace taf
{
/////////////////////////////////////////////////
/**  
* @file tc_mem_chunk.h
* @brief �ڴ������ 
*  
* @author jarodruan@tencent.com,hendysutencent.com          
*/ 
/////////////////////////////////////////////////
/**
* @brief ԭʼ�ڴ��, ��TC_MemChunkAllocator����������ά��
*
* ���������ڴ�ֳɴ�С��ͬ�Ŀ�,�γ�����,���ܹ�������ͷ���Щ��С��ͬ�Ŀ�
*/
class TC_MemChunk
{
public:

    /**
    * @brief ���캯��
    */
    TC_MemChunk();

    /**
     * @brief ����Chunk��Ҫ���ڴ���С
     * @param iBlockSize
     * @param iBlockCount
     *
     * @return size_t
     */
    static size_t calcMemSize(size_t iBlockSize, size_t iBlockCount);

    /**
     * @brief ����block����
     * @param iMemSize
     * @param iBlockSize
     *
     * @return size_t
     */
    static size_t calcBlockCount(size_t iMemSize, size_t iBlockSize);

     /**
     * @brief tagChunkHead�Ĵ�С
     *
     * @return size_t
     */
    static size_t getHeadSize() { return sizeof(tagChunkHead); }

    /**
     * @brief ��ʼ��, Ҫ��֤pָ����ڴ�ָ��=getMemSize��С
     * @param pAddr        ��ַ, ����Ӧ�ó���ľ��Ե�ַ
     * @param iBlockSize   block��С
     * @param iBlockCount  block����
     */
    void create(void *pAddr, size_t iBlockSize, size_t iBlockCount);

    /**
     * @brief ������
     * @param pAddr ��ַ, ����Ӧ�ó���ľ��Ե�ַ
     */
    void connect(void *pAddr);

    /**
	 * @brief ��ȡblock�Ĵ�С 
     * @return block�Ĵ�С
     */
    size_t getBlockSize() const { return _pHead->_iBlockSize; }

    /**
     * @brief ��ȡ���е��ڴ��С
     *
     * @return ���е��ڴ��С
     */
    size_t getMemSize() const { return _pHead->_iBlockSize * _pHead->_iBlockCount + sizeof(tagChunkHead); }

    /**
     * @brief ��ȡ���Դ�����ݵ�������
     *
     * @return ������
     */
    size_t getCapacity() const { return _pHead->_iBlockSize * _pHead->_iBlockCount; }

    /**
     * @brief ��ȡblock�ĸ���
     *
     * @return block�ĸ���
     */
    size_t getBlockCount() const { return _pHead->_iBlockCount; }

    /**
	 * @brief �Ƿ��п���block 
     * @return ���÷���true�����򷵻�false
     */
    bool isBlockAvailable() const { return _pHead->_blockAvailable > 0; }

    /**
	 * @brief  ��ȡ�������õ�block�ĸ��� 
     * @return ���õ�block�ĸ��� 
     */
    size_t getBlockAvailableCount() const { return _pHead->_blockAvailable; }

    /**
     * @brief ����һ������
     *
     * @return ָ�����������ָ��
     */
    void* allocate();

	/**
	* @brief ����һ������. 
	* ������1Ϊ����������������û�пɷ���ռ�ʱ���� 0 , 
	* ͨ���������ǱȽ�С(��ʹ��64λ����ϵͳ��), 4���ֽ����� 
	* ���ڽ�ʡ�ڴ� 
	*/
	void* allocate2(size_t &iIndex);

    /**
     * @brief �ͷ�����
     * @param ָ��Ҫ�ͷ������ָ��
     */
    void deallocate(void *pAddr);

    /**
     * @brief ���������ͷ�����
     * @param ��������
     */
	void deallocate2(size_t iIndex);

    /**
     * @brief �ؽ�
     */
    void rebuild();

    /**
     * @brief chunkͷ��
     */
    struct tagChunkHead
    {
        size_t  _iBlockSize;            /**�����С*/
        size_t  _iBlockCount;           /**block����*/
        size_t  _firstAvailableBlock;   /**��һ�����õ�block����*/
        size_t  _blockAvailable;        /**����block����*/
    }__attribute__((packed));

    /**
     * @brief ��ȡͷ����Ϣ
     *
     * @return ͷ����Ϣ
     */
    tagChunkHead getChunkHead() const;

	/**
	* @brief ����������ȡ���Ե�ַ
	*/
	void* getAbsolute(size_t iIndex);

    /**
     * @brief ���Ե�ַ��������
     * 
	 * @param pAddr    ���Ե�ַ 
     * @return size_t  ����ֵ
     */
	size_t getRelative(void *pAddr);

protected:
    /**
     * @brief ��ʼ��
     */
    void init(void *pAddr);

private:

    /**
     * @brief ����ͷָ��
     */
    tagChunkHead    *_pHead;

    /**
     * @brief ������ָ��
     */
    unsigned char   *_pData;
};

/**
* @brief �ڴ����������ṩ������ͷŵĹ���  
*  
* ֻ�ܷ�����ͬ��С���ڴ��,���²��ԭʼ�ڴ�����, 
*  
* �ڴ�ṹ: �ڴ�鳤��, 4���ֽ� ;
*  
* Block�� С, 4���ֽ�; 
*  
* Chunk����, 4���ֽ� ;
*  
* TC_MemChunk ��ʱֻ֧��ͬһ��Block��С��MemChunk 
*/
class TC_MemChunkAllocator
{
public:

    /**
    * @brief ���캯��
    */
    TC_MemChunkAllocator();

    /**
     * @brief ��ʼ��
     * @param pAddr, ��ַ, ����Ӧ�ó���ľ��Ե�ַ
     * @param iSize, �ڴ��С
     * @param iBlockSize, block�Ĵ�С
     */
    void create(void *pAddr, size_t iSize, size_t iBlockSize);

    /**
     * @brief ����
     * @param pAddr ��ַ, ����Ӧ�ó���ľ��Ե�ַ
     */
    void connect(void *pAddr);

	/**
	 * @brief ��ȡͷ��ַָ��
	 */
	void *getHead()	const       { return _pHead; }

    /**
     * @brief ÿ��block�Ĵ�С
     *
     * @return block�Ĵ�С
     */
    size_t getBlockSize()  const { return _pHead->_iBlockSize; }

    /**
	 * @brief �ܼ��ڴ��С 
     * @return �ڴ��С
     */
    size_t getMemSize()  const { return _pHead->_iSize; }

    /**
	 * @brief ���Դ�����ݵ������� 
     * @return ������
     */
    size_t getCapacity() const { return _chunk.getCapacity(); }

    /**
	 * @brief ����һ������,���Ե�ַ 
     */
    void* allocate();

	/**
	* @brief ����һ�����飬������1Ϊ���������������� 
	*   	 û�пɷ���ռ�ʱ����0
	* @param ��������
	*/
	void* allocate2(size_t &iIndex);

    /**
     * @brief �ͷ�����, ���Ե�ַ
     * @param pAddr ����ľ��Ե�ַ
     */
    void deallocate(void *pAddr);

    /**
     * @brief �ͷ����� 
     * @param iIndex ��������
     */
    void deallocate2(size_t iIndex);

    /**
	 * @brief ��ȡ����chunk������ϼƵ�block�ĸ��� 
     * @return �ϼƵ�block�ĸ���
     */
    size_t blockCount() const           { return _chunk.getBlockCount(); }

	/**
	* @brief ����������ȡ���Ե�ַ 
	* @param ����
	*/
	void* getAbsolute(size_t iIndex)    { return _chunk.getAbsolute(iIndex); };

    /**
     * @brief ���Ե�ַ��������
	 * @param pAddr   ���Ե�ַ 
     * @return size_t ����
     */
	size_t getRelative(void *pAddr)     { return _chunk.getRelative(pAddr); };

    /**
	 * @brief ��ȡͷ����Ϣ 
     * @return ͷ����Ϣ
     */
    TC_MemChunk::tagChunkHead getBlockDetail() const;

    /**
     * @brief �ؽ�
     */
    void rebuild();

    /**
     * @brief ͷ���ڴ��
     */
    struct tagChunkAllocatorHead
    {
        size_t  _iSize;
        size_t  _iBlockSize;
    }__attribute__((packed));

    /**
	 * @brief ȡ��ͷ����С 
     * @return ͷ����С
     */
    static size_t getHeadSize() { return sizeof(tagChunkAllocatorHead); }

protected:

    /**
     * @brief ��ʼ��
     */
    void init(void *pAddr);

    /**
     * @brief ��ʼ��
     */
    void initChunk();

    /**
     * @brief ����
     */
    void connectChunk();

	/**
	*@brief ������copy���� 
	*/ 
    TC_MemChunkAllocator(const TC_MemChunkAllocator &);
	/**
	 *@brief ������ֵ  
	 */ 
    TC_MemChunkAllocator& operator=(const TC_MemChunkAllocator &);
    bool operator==(const TC_MemChunkAllocator &mca) const;
    bool operator!=(const TC_MemChunkAllocator &mca) const;

private:

    /**
     * ͷָ��
     */
    tagChunkAllocatorHead   *_pHead;

    /**
     *  chunk��ʼ��ָ��
     */
    void                    *_pChunk;

    /**
     * chunk����
     */
    TC_MemChunk             _chunk;
};

/**
 * @brief ��������,���Է�������ͬ��С�Ŀ�
 *  
 * �ڲ�ÿ�ֿ���TC_MemChunkAllocator������, 
 *  
 * ÿ�ִ�С��ͬ��ĸ�������ͬ��, �ڴ�����Ĳ�������: 
 *  
 *  ȷ����Ҫ�������ڴ棬������Ҫ����A�ֽڵ��ڴ棻
 *  
 * �����С����>=A���ڴ�飬���ȷ����С��ӽ��ģ�
 *  
 * �����û�к����ڴ�飬������С<A���ڴ�飬���ȷ����С��ӽ��ģ�
 *  
 * �����Ȼû�к����ڴ�飬�򷵻�NULL��
 *  
 * ��ʼ��ʱָ��:��С��� С, �����С, ����С��ֵ
 *  
 * �Զ��������ĸ���(ÿ�ִ�С��ĸ�����ͬ) 
 */
class TC_MemMultiChunkAllocator
{
public:

    /**
    * @brief ���캯��
    */
    TC_MemMultiChunkAllocator();

    /**
     * @brief ��������
     */
    ~TC_MemMultiChunkAllocator();


    /**
     * @brief ��ʼ��
     * @param pAddr          ��ַ, ����Ӧ�ó���ľ��Ե�ַ
     * @param iSize          �ڴ��С
     * @param iMinBlockSize  block�Ĵ�С����
     * @param iMaxBlockSize  block�Ĵ�С����
     * @param fFactor        ����
     */
    void create(void *pAddr, size_t iSize, size_t iMinBlockSize, size_t iMaxBlockSize, float fFactor = 1.1);

    /**
     * @brief ������
     * @param pAddr ��ַ, ����Ӧ�ó���ľ��Ե�ַ
     */
    void connect(void *pAddr);

    /**
     * @brief ��չ�ռ�
     * 
     * @param pAddr �Ѿ��ǿռ䱻��չ֮��ĵ�ַ
     * @param iSize
     */
    void append(void *pAddr, size_t iSize);

    /**
     * @brief ��ȡÿ��block�Ĵ�С, �����������ӵ��ڴ��Ĵ�С
     *
     * @return vector<size_t>block��С��vector
     */
    vector<size_t> getBlockSize()  const;

    /**
	 * @brief ÿ��block��chunk����(������ȵ�) 
     * @return chunk���� 
     */
    size_t getBlockCount() const { return _iBlockCount; }

    /**
     * @brief ��ȡÿ����ͷ����Ϣ, �����������ӵ��ڴ��Ĵ�С
     * @param i
     *
     * @return vector<TC_MemChunk::tagChunkHead>
     */
    vector<TC_MemChunk::tagChunkHead> getBlockDetail() const;

    /**
     * @brief �ܼ��ڴ��С, �����������ӵ��ڴ��Ĵ�С
     *
     * @return size_t
     */
    size_t getMemSize()  const              { return _pHead->_iTotalSize; }

    /**
	 * @brief �������Է����ݵ�����, �����������ӵ��ڴ����������� 
     * @return ���Է����ݵ�����
     */
    size_t getCapacity() const;

    /**
	 * @brief һ��chunk��block����, �����������ӵ��ڴ��� 
     * @return vector<size_t>block����
     */
    vector<size_t> singleBlockChunkCount() const;

    /**
	 * @brief ����chunk������ϼƵ�block�ĸ��� 
     * @return �ϼƵ�block�ĸ���
     */
    size_t allBlockChunkCount() const;

    /**
	 * @brief ����һ������,���Ե�ַ 
     * @param iNeedSize   ��Ҫ����Ĵ�С
     * @param iAllocSize  ��������ݿ��С
     */
    void* allocate(size_t iNeedSize, size_t &iAllocSize);

    /**
     * @brief ����һ������, ������������
     * @param iNeedSize    ��Ҫ����Ĵ�С
     * @param iAllocSize   ��������ݿ��С
     * @param               size_t����1Ϊ������������0��ʾ��Ч
     */
    void* allocate2(size_t iNeedSize, size_t &iAllocSize, size_t &iIndex);

    /**
     * @brief �ͷ�����
     * @param p ���Ե�ַ
     */
    void deallocate(void *pAddr);

    /**
     * @brief �ͷ�����
     * @param iIndex ��������
     */
    void deallocate2(size_t iIndex);

    /**
     * @brief �ؽ�
     */
    void rebuild();

    /**
     * @brief �����������ɾ��Ե�ַ
	 * @param iIndex ������� 
     * @return       ���Ե�ַָ��
     */
    void *getAbsolute(size_t iIndex);

    /**
     * @brief  ���Ե�ַ����������ַ
     * @param  ���Ե�ַ
     * @return ������ַ
     */
    size_t getRelative(void *pAddr);

    /**
     * @brief ͷ���ڴ��
     */
    struct tagChunkAllocatorHead
    {
        size_t  _iSize;             /**��ǰ���С*/
        size_t  _iTotalSize;        /**������������һ��Ĵ�С*/
        size_t  _iMinBlockSize;
        size_t  _iMaxBlockSize;
        float   _fFactor;
        size_t  _iNext;             /**��һ����������ַ, ���û����Ϊ0*/
    }__attribute__((packed));

    /**
     * @brief ͷ����С
     *
     * @return size_t
     */
    static size_t getHeadSize() { return sizeof(tagChunkAllocatorHead); }

protected:

    /**
     * @brief ��ʼ��
     */
    void init(void *pAddr);

    /**
     * @brief ����
     */
	void calc();

    /**
     * @brief ���
     */
    void clear();

    /**
     * @brief ���һ��������
     * 
     * @return TC_MemMultiChunkAllocator*
     */
    TC_MemMultiChunkAllocator *lastAlloc();

	/**
	 *@brief ������copy����
	 */
    TC_MemMultiChunkAllocator(const TC_MemMultiChunkAllocator &);
	/** 
	  * @brief ������ֵ
	  */
    TC_MemMultiChunkAllocator& operator=(const TC_MemMultiChunkAllocator &);
    bool operator==(const TC_MemMultiChunkAllocator &mca) const;
    bool operator!=(const TC_MemMultiChunkAllocator &mca) const;

private:

    /**
     * ͷָ��
     */
    tagChunkAllocatorHead   *_pHead;

    /**
     *  chunk��ʼ��ָ��
     */
    void                    *_pChunk;

    /**
     * �����С
     */
    vector<size_t>          _vBlockSize;

    /**
     * ÿ��chunk��block�ĸ���
     */
    size_t                  _iBlockCount;

    /**
     * chunk����
     */
    vector<TC_MemChunkAllocator*>       _allocator;

    /**
     * ���е���������
     */
    size_t                              _iAllIndex;

    /**
     * �����Ķ�������
     */
    TC_MemMultiChunkAllocator           *_nallocator;
};

}

#endif
