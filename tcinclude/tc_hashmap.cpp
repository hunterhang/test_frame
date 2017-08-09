#include "tc_hashmap.h"
#include "tc_pack.h"
#include "tc_common.h"

namespace taf
{

int TC_HashMap::Block::getBlockData(TC_HashMap::BlockData &data)
{
	data._dirty = isDirty();
    data._synct = getSyncTime();

    string s;
	int ret   = get(s);

    if(ret != TC_HashMap::RT_OK)
    {
        return ret;
    }

    try
    {
        TC_PackOut po(s.c_str(), s.length());
        po >> data._key;

        //�������ֻ��Key
        if(!isOnlyKey())
        {
            po >> data._value;
        }
        else
        {
            return TC_HashMap::RT_ONLY_KEY;
        }
    }
    catch(exception &ex)
    {
        ret = TC_HashMap::RT_DECODE_ERR;
    }

    return ret;
}

size_t TC_HashMap::Block::getLastBlockHead()
{
	size_t iHead = _iHead;

	while(getBlockHead(iHead)->_iBlockNext != 0)
	{
		iHead = getBlockHead(iHead)->_iBlockNext;
	}
	return iHead;
}

int TC_HashMap::Block::get(void *pData, size_t &iDataLen)
{
	//û����һ��chunk, һ��chunk�Ϳ���װ��������
	if(!getBlockHead()->_bNextChunk)
	{
		memcpy(pData, getBlockHead()->_cData, min(getBlockHead()->_iDataLen, iDataLen));
		iDataLen = getBlockHead()->_iDataLen;
		return TC_HashMap::RT_OK;
	}
	else
	{
		size_t iUseSize = getBlockHead()->_iSize - sizeof(tagBlockHead);
		size_t iCopyLen = min(iUseSize, iDataLen);

		//copy����ǰ��block��
		memcpy(pData, getBlockHead()->_cData, iCopyLen);
		if (iDataLen < iUseSize)
		{
			return TC_HashMap::RT_NOTALL_ERR;   //copy���ݲ���ȫ
		}

		//�Ѿ�copy����
		size_t iHasLen  = iCopyLen;
		//���ʣ�೤��
		size_t iLeftLen = iDataLen - iCopyLen;

		tagChunkHead *pChunk    = getChunkHead(getBlockHead()->_iNextChunk);
		while(iHasLen < iDataLen)
		{
            iUseSize        = pChunk->_iSize - sizeof(tagChunkHead);
			if(!pChunk->_bNextChunk)
			{
				//copy����ǰ��chunk��
				size_t iCopyLen = min(pChunk->_iDataLen, iLeftLen);
				memcpy((char*)pData + iHasLen, pChunk->_cData, iCopyLen);
				iDataLen = iHasLen + iCopyLen;

				if(iLeftLen < pChunk->_iDataLen)
				{
					return TC_HashMap::RT_NOTALL_ERR;       //copy����ȫ
				}

				return TC_HashMap::RT_OK;
			}
			else
			{
				size_t iCopyLen = min(iUseSize, iLeftLen);
				//copy��ǰ��chunk
				memcpy((char*)pData + iHasLen, pChunk->_cData, iCopyLen);
				if (iLeftLen <= iUseSize)
				{
					iDataLen = iHasLen + iCopyLen;
					return TC_HashMap::RT_NOTALL_ERR;   //copy����ȫ
				}

				//copy��ǰchunk��ȫ
				iHasLen  += iCopyLen;
				iLeftLen -= iCopyLen;

				pChunk   =  getChunkHead(pChunk->_iNextChunk);
			}
		}
	}

	return TC_HashMap::RT_OK;
}

int TC_HashMap::Block::get(string &s)
{
	size_t iLen = getDataLen();

	char *cData = new char[iLen];
	size_t iGetLen = iLen;
	int ret = get(cData, iGetLen);
	if(ret == TC_HashMap::RT_OK)
	{
		s.assign(cData, iGetLen);
	}

	delete[] cData;

	return ret;
}

int TC_HashMap::Block::set(const void *pData, size_t iDataLen, bool bOnlyKey, vector<TC_HashMap::BlockData> &vtData)
{
	//���ȷ���ոչ��ĳ���, ���ܶ�һ��chunk, Ҳ������һ��chunk
	int ret = allocate(iDataLen, vtData);
	if(ret != TC_HashMap::RT_OK)
	{
		return ret;
	}

	if(bOnlyKey)
	{
		//ԭʼ������������
    	if(getBlockHead()->_bDirty)
    	{
    		_pMap->delDirtyCount();
    	}

    	//���ݱ��޸�, ����Ϊ������
    	getBlockHead()->_bDirty     = false;

		//ԭʼ���ݲ���OnlyKey����
		if(!getBlockHead()->_bOnlyKey)
		{
			_pMap->incOnlyKeyCount();
		}
	}
	else
	{
    	//ԭʼ���ݲ���������
    	if(!getBlockHead()->_bDirty)
    	{
    		_pMap->incDirtyCount();
    	}

    	//���ݱ��޸�, ����Ϊ������
    	getBlockHead()->_bDirty     = true;

		//ԭʼ������OnlyKey����
		if(getBlockHead()->_bOnlyKey)
		{
			_pMap->delOnlyKeyCount();
		}
	}

    //�����Ƿ�ֻ��Key
    getBlockHead()->_bOnlyKey   = bOnlyKey;

	size_t iUseSize = getBlockHead()->_iSize - sizeof(tagBlockHead);
	//û����һ��chunk, һ��chunk�Ϳ���װ��������
	if(!getBlockHead()->_bNextChunk)
	{
		memcpy(getBlockHead()->_cData, (char*)pData, iDataLen);
		//��copy����, �ٸ������ݳ���
		getBlockHead()->_iDataLen   = iDataLen;
	}
	else
	{
		//copy����ǰ��block��
		memcpy(getBlockHead()->_cData, (char*)pData, iUseSize);
		//ʣ��̶�
		size_t iLeftLen = iDataLen - iUseSize;
		size_t iCopyLen = iUseSize;

		tagChunkHead *pChunk    = getChunkHead(getBlockHead()->_iNextChunk);
		while(true)
		{
            //����chunk�Ŀ��ô�С
            iUseSize                = pChunk->_iSize - sizeof(tagChunkHead);

			if(!pChunk->_bNextChunk)
			{
                assert(iUseSize >= iLeftLen);
				//copy����ǰ��chunk��
				memcpy(pChunk->_cData, (char*)pData + iCopyLen, iLeftLen);
				//���һ��chunk, �������ݳ���, ��copy�����ٸ�ֵ����
				pChunk->_iDataLen = iLeftLen;
				iCopyLen += iLeftLen;
				iLeftLen -= iLeftLen;
				break;
			}
			else
			{
				//copy����ǰ��chunk��
				memcpy(pChunk->_cData, (char*)pData + iCopyLen, iUseSize);
				iCopyLen += iUseSize;
				iLeftLen -= iUseSize;

				pChunk   =  getChunkHead(pChunk->_iNextChunk);
			}
		}
        assert(iLeftLen == 0);
	}

	_pMap->doUpdate(true);
	return TC_HashMap::RT_OK;
}

void TC_HashMap::Block::setDirty(bool b)
{
	if(getBlockHead()->_bDirty != b)
	{
		if (b)
		{
			_pMap->incDirtyCount();
		}
		else
		{
			_pMap->delDirtyCount();
		}
		_pMap->update(&getBlockHead()->_bDirty, b);
        _pMap->doUpdate(true);
	}
}

bool TC_HashMap::Block::nextBlock()
{
	_iHead = getBlockHead()->_iBlockNext;

	return _iHead != 0;
}

bool TC_HashMap::Block::prevBlock()
{
	_iHead = getBlockHead()->_iBlockPrev;

	return _iHead != 0;
}

void TC_HashMap::Block::deallocate()
{
	vector<size_t> v;
	v.push_back(_iHead);

	if(getBlockHead()->_bNextChunk)
	{
		deallocate(getBlockHead()->_iNextChunk);
	}

	_pMap->_pDataAllocator->deallocateMemBlock(v);
}

void TC_HashMap::Block::makeNew(size_t index, size_t iAllocSize)
{
    getBlockHead()->_iSize          = iAllocSize;
	getBlockHead()->_iIndex         = index;
	getBlockHead()->_iSetNext       = 0;
	getBlockHead()->_iSetPrev       = 0;
	getBlockHead()->_iGetNext       = 0;
	getBlockHead()->_iGetPrev       = 0;
    getBlockHead()->_iSyncTime      = 0;
	getBlockHead()->_iBlockNext     = 0;
	getBlockHead()->_iBlockPrev     = 0;
	getBlockHead()->_bNextChunk     = false;
	getBlockHead()->_iDataLen       = 0;
	getBlockHead()->_bDirty         = true;
	getBlockHead()->_bOnlyKey       = false;

	_pMap->incDirtyCount();
	_pMap->incElementCount();
	_pMap->incListCount(index);

	//����block������
	if(_pMap->item(index)->_iBlockAddr == 0)
	{
		//��ǰhashͰû��Ԫ��
		_pMap->update(&_pMap->item(index)->_iBlockAddr, _iHead);
		_pMap->update(&getBlockHead()->_iBlockNext, (size_t)0);
		_pMap->update(&getBlockHead()->_iBlockPrev, (size_t)0);
	}
	else
	{
		//��ȻhashͰ��Ԫ��, ����Ͱ��ͷ
		_pMap->update(&getBlockHead(_pMap->item(index)->_iBlockAddr)->_iBlockPrev, _iHead);
		_pMap->update(&getBlockHead()->_iBlockNext, _pMap->item(index)->_iBlockAddr);
		_pMap->update(&_pMap->item(index)->_iBlockAddr, _iHead);
		_pMap->update(&getBlockHead()->_iBlockPrev, (size_t)0);
	}

	//����Set�����ͷ��
	if(_pMap->_pHead->_iSetHead == 0)
	{
		assert(_pMap->_pHead->_iSetTail == 0);
		_pMap->update(&_pMap->_pHead->_iSetHead, _iHead);
		_pMap->update(&_pMap->_pHead->_iSetTail, _iHead);
	}
	else
	{
        assert(_pMap->_pHead->_iSetTail != 0);
		_pMap->update(&getBlockHead()->_iSetNext, _pMap->_pHead->_iSetHead);
		_pMap->update(&getBlockHead(_pMap->_pHead->_iSetHead)->_iSetPrev, _iHead);
		_pMap->update(&_pMap->_pHead->_iSetHead, _iHead);
	}

    //����Get����ͷ��
	if(_pMap->_pHead->_iGetHead == 0)
	{
		assert(_pMap->_pHead->_iGetTail == 0);
		_pMap->update(&_pMap->_pHead->_iGetHead, _iHead);
		_pMap->update(&_pMap->_pHead->_iGetTail, _iHead);
	}
	else
	{
        assert(_pMap->_pHead->_iGetTail != 0);
		_pMap->update(&getBlockHead()->_iGetNext, _pMap->_pHead->_iGetHead);
		_pMap->update(&getBlockHead(_pMap->_pHead->_iGetHead)->_iGetPrev, _iHead);
		_pMap->update(&_pMap->_pHead->_iGetHead, _iHead);
	}

	//һ��д���²���
	_pMap->doUpdate(true);
}

void TC_HashMap::Block::erase()
{
	//////////////////�޸�����������/////////////
	if(_pMap->_pHead->_iDirtyTail == _iHead)
	{
		_pMap->update(&_pMap->_pHead->_iDirtyTail, getBlockHead()->_iSetPrev);
	}

    //////////////////�޸Ļ�д��������/////////////
    if(_pMap->_pHead->_iSyncTail == _iHead)
    {
        _pMap->update(&_pMap->_pHead->_iSyncTail, getBlockHead()->_iSetPrev);
    }

    //////////////////�޸ı�����������/////////////
    if(_pMap->_pHead->_iBackupTail == _iHead)
    {
        _pMap->update(&_pMap->_pHead->_iBackupTail, getBlockHead()->_iGetPrev);
    }

	////////////////////�޸�Set���������//////////
    {
    	bool bHead = (_pMap->_pHead->_iSetHead == _iHead);
    	bool bTail = (_pMap->_pHead->_iSetTail == _iHead);

    	if(!bHead)
    	{
    		if(bTail)
    		{
    			assert(getBlockHead()->_iSetNext == 0);
    			//��β��, β��ָ��ָ����һ��Ԫ��
    			_pMap->update(&_pMap->_pHead->_iSetTail, getBlockHead()->_iSetPrev);
    			_pMap->update(&getBlockHead(getBlockHead()->_iSetPrev)->_iSetNext, (size_t)0);
    		}
    		else
    		{
    			//����ͷ��Ҳ����β��
    			assert(getBlockHead()->_iSetNext != 0);
    			_pMap->update(&getBlockHead(getBlockHead()->_iSetPrev)->_iSetNext, getBlockHead()->_iSetNext);
    			_pMap->update(&getBlockHead(getBlockHead()->_iSetNext)->_iSetPrev, getBlockHead()->_iSetPrev);
    		}
    	}
    	else
    	{
    		if(bTail)
    		{
    			assert(getBlockHead()->_iSetNext == 0);
    			assert(getBlockHead()->_iSetPrev == 0);
    			//ͷ��Ҳ��β��, ָ�붼����Ϊ0
    			_pMap->update(&_pMap->_pHead->_iSetHead, (size_t)0);
    			_pMap->update(&_pMap->_pHead->_iSetTail, (size_t)0);
    		}
    		else
    		{
    			//ͷ������β��, ͷ��ָ��ָ����һ��Ԫ��
    			assert(getBlockHead()->_iSetNext != 0);
    			_pMap->update(&_pMap->_pHead->_iSetHead, getBlockHead()->_iSetNext);
                //��һ��Ԫ����ָ��Ϊ0
                _pMap->update(&getBlockHead(getBlockHead()->_iSetNext)->_iSetPrev, (size_t)0);
    		}
    	}
    }

	////////////////////�޸�Get���������//////////
	//
    {
    	bool bHead = (_pMap->_pHead->_iGetHead == _iHead);
    	bool bTail = (_pMap->_pHead->_iGetTail == _iHead);

    	if(!bHead)
    	{
    		if(bTail)
    		{
    			assert(getBlockHead()->_iGetNext == 0);
    			//��β��, β��ָ��ָ����һ��Ԫ��
    			_pMap->update(&_pMap->_pHead->_iGetTail, getBlockHead()->_iGetPrev);
    			_pMap->update(&getBlockHead(getBlockHead()->_iGetPrev)->_iGetNext, (size_t)0);
    		}
    		else
    		{
    			//����ͷ��Ҳ����β��
    			assert(getBlockHead()->_iGetNext != 0);
    			_pMap->update(&getBlockHead(getBlockHead()->_iGetPrev)->_iGetNext, getBlockHead()->_iGetNext);
    			_pMap->update(&getBlockHead(getBlockHead()->_iGetNext)->_iGetPrev, getBlockHead()->_iGetPrev);
    		}
    	}
    	else
    	{
    		if(bTail)
    		{
    			assert(getBlockHead()->_iGetNext == 0);
    			assert(getBlockHead()->_iGetPrev == 0);
    			//ͷ��Ҳ��β��, ָ�붼����Ϊ0
    			_pMap->update(&_pMap->_pHead->_iGetHead, (size_t)0);
    			_pMap->update(&_pMap->_pHead->_iGetTail, (size_t)0);
    		}
    		else
    		{
    			//ͷ������β��, ͷ��ָ��ָ����һ��Ԫ��
    			assert(getBlockHead()->_iGetNext != 0);
    			_pMap->update(&_pMap->_pHead->_iGetHead, getBlockHead()->_iGetNext);
                //��һ��Ԫ����ָ��Ϊ0
                _pMap->update(&getBlockHead(getBlockHead()->_iGetNext)->_iGetPrev, (size_t)0);
    		}
    	}
    }

	///////////////////��block������ȥ��///////////
	//
	//��һ��blockָ����һ��block
	if(getBlockHead()->_iBlockPrev != 0)
	{
		_pMap->update(&getBlockHead(getBlockHead()->_iBlockPrev)->_iBlockNext, getBlockHead()->_iBlockNext);
	}

	//��һ��blockָ����һ��
	if(getBlockHead()->_iBlockNext != 0)
	{
		_pMap->update(&getBlockHead(getBlockHead()->_iBlockNext)->_iBlockPrev, getBlockHead()->_iBlockPrev);
	}

	//////////////////�����hashͷ��, ��Ҫ�޸�hash��������ָ��//////
	//
	_pMap->delListCount(getBlockHead()->_iIndex);
	if(getBlockHead()->_iBlockPrev == 0)
	{
		//�����hashͰ��ͷ��, ����Ҫ����
		TC_HashMap::tagHashItem *pItem  = _pMap->item(getBlockHead()->_iIndex);
		assert(pItem->_iBlockAddr == _iHead);
		if(pItem->_iBlockAddr == _iHead)
		{
			_pMap->update(&pItem->_iBlockAddr, getBlockHead()->_iBlockNext);
		}
	}

	//////////////////������///////////////////
	//
	if (isDirty())
	{
		_pMap->delDirtyCount();
	}

	if(isOnlyKey())
	{
		_pMap->delOnlyKeyCount();
	}

	//Ԫ�ظ�������
	_pMap->delElementCount();

	//һ��д���²���
	_pMap->doUpdate(true);

	//�黹���ڴ���
	deallocate();
}

void TC_HashMap::Block::refreshSetList()
{
	assert(_pMap->_pHead->_iSetHead != 0);
	assert(_pMap->_pHead->_iSetTail != 0);

	//�޸�ͬ������
	if(_pMap->_pHead->_iSyncTail == _iHead && _pMap->_pHead->_iSetHead != _iHead)
	{
		_pMap->update(&_pMap->_pHead->_iSyncTail, getBlockHead()->_iSetPrev);
	}

	//�޸�������β������ָ��, ����һ��Ԫ��
	if(_pMap->_pHead->_iDirtyTail == _iHead && _pMap->_pHead->_iSetHead != _iHead)
	{
		//������β��λ��ǰ��
		_pMap->update(&_pMap->_pHead->_iDirtyTail, getBlockHead()->_iSetPrev);
	}
	else if (_pMap->_pHead->_iDirtyTail == 0)
	{
		//ԭ��û��������
		_pMap->update(&_pMap->_pHead->_iDirtyTail, _iHead);
	}

	//��ͷ�����ݻ���set������ʱ�ߵ������֧
	if(_pMap->_pHead->_iSetHead == _iHead)
	{
		_pMap->doUpdate(true);
        //ˢ��Get��
        refreshGetList();
		return;
	}

	size_t iPrev = getBlockHead()->_iSetPrev;
	size_t iNext = getBlockHead()->_iSetNext;

	assert(iPrev != 0);

	//��������ͷ��
	_pMap->update(&getBlockHead()->_iSetNext, _pMap->_pHead->_iSetHead);
	_pMap->update(&getBlockHead(_pMap->_pHead->_iSetHead)->_iSetPrev, _iHead);
	_pMap->update(&_pMap->_pHead->_iSetHead, _iHead);
	_pMap->update(&getBlockHead()->_iSetPrev, (size_t)0);

	//��һ��Ԫ�ص�Nextָ��ָ����һ��Ԫ��
	_pMap->update(&getBlockHead(iPrev)->_iSetNext, iNext);

	//��һ��Ԫ�ص�Prevָ����һ��Ԫ��
	if (iNext != 0)
	{
		_pMap->update(&getBlockHead(iNext)->_iSetPrev, iPrev);
	}
	else
	{
		//�ı�β��ָ��
		assert(_pMap->_pHead->_iSetTail == _iHead);
		_pMap->update(&_pMap->_pHead->_iSetTail, iPrev);
	}

	_pMap->doUpdate(true);

    //ˢ��Get��
    refreshGetList();
}

void TC_HashMap::Block::refreshGetList()
{
	assert(_pMap->_pHead->_iGetHead != 0);
	assert(_pMap->_pHead->_iGetTail != 0);

	//��ͷ������
	if(_pMap->_pHead->_iGetHead == _iHead)
	{
		return;
	}

	size_t iPrev = getBlockHead()->_iGetPrev;
	size_t iNext = getBlockHead()->_iGetNext;

	assert(iPrev != 0);

    //�����ڱ��ݵ�����
	if(_pMap->_pHead->_iBackupTail == _iHead)
	{
        _pMap->update(&_pMap->_pHead->_iBackupTail, iPrev);
	}

	//��������ͷ��
	_pMap->update(&getBlockHead()->_iGetNext, _pMap->_pHead->_iGetHead);
	_pMap->update(&getBlockHead(_pMap->_pHead->_iGetHead)->_iGetPrev, _iHead);
	_pMap->update(&_pMap->_pHead->_iGetHead, _iHead);
	_pMap->update(&getBlockHead()->_iGetPrev, (size_t)0);

	//��һ��Ԫ�ص�Nextָ��ָ����һ��Ԫ��
	_pMap->update(&getBlockHead(iPrev)->_iGetNext, iNext);

	//��һ��Ԫ�ص�Prevָ����һ��Ԫ��
	if (iNext != 0)
	{
		_pMap->update(&getBlockHead(iNext)->_iGetPrev, iPrev);
	}
	else
	{
		//�ı�β��ָ��
		assert(_pMap->_pHead->_iGetTail == _iHead);
		_pMap->update(&_pMap->_pHead->_iGetTail, iPrev);
	}

	_pMap->doUpdate(true);
}

void TC_HashMap::Block::deallocate(size_t iChunk)
{
	tagChunkHead *pChunk        = getChunkHead(iChunk);
	vector<size_t> v;
	v.push_back(iChunk);

    //��ȡ���к�����chunk��ַ
	while(true)
	{
		if(pChunk->_bNextChunk)
		{
			v.push_back(pChunk->_iNextChunk);
			pChunk = getChunkHead(pChunk->_iNextChunk);
		}
		else
		{
			break;
		}
	}

    //�ռ�ȫ���ͷŵ�
	_pMap->_pDataAllocator->deallocateMemBlock(v);
}

int TC_HashMap::Block::allocate(size_t iDataLen, vector<TC_HashMap::BlockData> &vtData)
{
	size_t fn   = 0;

    //һ�������������������
	fn = getBlockHead()->_iSize - sizeof(tagBlockHead);
	if(fn >= iDataLen)
	{
		//һ��block�Ϳ�����, ������chunk��Ҫ�ͷŵ�
		if(getBlockHead()->_bNextChunk)
		{
			size_t iNextChunk = getBlockHead()->_iNextChunk;
			//���޸��Լ�������
			_pMap->update(&getBlockHead()->_bNextChunk, false);
			_pMap->update(&getBlockHead()->_iDataLen, (size_t)0);
			_pMap->doUpdate(true);
			//�޸ĳɹ������ͷ�����, �Ӷ���֤, ����core��ʱ���������ڴ�鲻����
			deallocate(iNextChunk);
		}
		return TC_HashMap::RT_OK;
	}

    //���㻹��Ҫ���ٳ���
    fn = iDataLen - fn;

	if(getBlockHead()->_bNextChunk)
	{
		tagChunkHead *pChunk    = getChunkHead(getBlockHead()->_iNextChunk);

		while(true)
		{
            if(fn <= (pChunk->_iSize - sizeof(tagChunkHead)))
            {
                //�Ѿ�����Ҫchunk��, �ͷŶ����chunk
				if(pChunk->_bNextChunk)
				{
					size_t iNextChunk = pChunk->_iNextChunk;
					_pMap->update(&pChunk->_bNextChunk, false);
					_pMap->doUpdate(true);
					//һ���쳣core, �������������˿��õ�����, ���������ڴ�ṹ���ǿ��õ�
					deallocate(iNextChunk);
				}
				return TC_HashMap::RT_OK ;
            }

            //����, ����Ҫ���ٳ���
            fn -= pChunk->_iSize - sizeof(tagChunkHead);

			if(pChunk->_bNextChunk)
			{
				pChunk  =  getChunkHead(pChunk->_iNextChunk);
			}
			else
			{
				//û�к���chunk��, ��Ҫ�·���fn�Ŀռ�
				vector<size_t> chunks;
				int ret = allocateChunk(fn, chunks, vtData);
				if(ret != TC_HashMap::RT_OK)
				{
					//û���ڴ���Է���
					return ret;
				}

				_pMap->update(&pChunk->_bNextChunk, true);
				_pMap->update(&pChunk->_iNextChunk, chunks[0]);
				//chunkָ�����ĵ�һ��chunk
				pChunk  =  getChunkHead(chunks[0]);
				//�޸ĵ�һ��chunk������, ��֤�쳣core��ʱ��, chunk������������
				_pMap->update(&pChunk->_bNextChunk, false);
				_pMap->update(&pChunk->_iDataLen, (size_t)0);
				_pMap->doUpdate(true);

				//����ÿ��chunk
				return joinChunk(pChunk, chunks);
			}
		}
	}
	else
	{
		//û�к���chunk��, ��Ҫ�·���fn�ռ�
		vector<size_t> chunks;
		int ret = allocateChunk(fn, chunks, vtData);
		if(ret != TC_HashMap::RT_OK)
		{
			//û���ڴ���Է���
			return ret;
		}

		_pMap->update(&getBlockHead()->_bNextChunk, true);
		_pMap->update(&getBlockHead()->_iNextChunk, chunks[0]);
		//chunkָ�����ĵ�һ��chunk
		tagChunkHead *pChunk = getChunkHead(chunks[0]);
		//�޸ĵ�һ��chunk������, ��֤�쳣core��ʱ��, chunk������������
		_pMap->update(&pChunk->_bNextChunk, false);
		_pMap->update(&pChunk->_iDataLen, (size_t)0);
		_pMap->doUpdate(true);

		//����ÿ��chunk
		return joinChunk(pChunk, chunks);
	}

	return TC_HashMap::RT_OK;
}

int TC_HashMap::Block::joinChunk(tagChunkHead *pChunk, const vector<size_t> chunks)
{
	for (size_t i = 0; i < chunks.size(); ++i)
	{
		if (i == chunks.size() - 1)
		{
			_pMap->update(&pChunk->_bNextChunk, false);
			_pMap->doUpdate(true);
			return TC_HashMap::RT_OK;
		}
		else
		{
			_pMap->update(&pChunk->_bNextChunk, true);
			_pMap->update(&pChunk->_iNextChunk, chunks[i+1]);
			pChunk  =  getChunkHead(chunks[i+1]);
			_pMap->update(&pChunk->_bNextChunk, false);
			_pMap->update(&pChunk->_iDataLen, (size_t)0);
			_pMap->doUpdate(true);
		}
	}

	return TC_HashMap::RT_OK;
}

int TC_HashMap::Block::allocateChunk(size_t fn, vector<size_t> &chunks, vector<TC_HashMap::BlockData> &vtData)
{
    assert(fn > 0);

	while(true)
	{
        size_t iAllocSize = fn;
		//����ռ�
		size_t t = _pMap->_pDataAllocator->allocateChunk(_iHead, iAllocSize, vtData);
		if (t == 0)
		{
			//û���ڴ���Է�����, �Ȱѷ���Ĺ黹
			_pMap->_pDataAllocator->deallocateMemBlock(chunks);
			chunks.clear();
			return TC_HashMap::RT_NO_MEMORY;
		}

        //���÷�������ݿ�Ĵ�С
        getChunkHead(t)->_iSize = iAllocSize;

		chunks.push_back(t);

        //���乻��
        if(fn <= (iAllocSize - sizeof(tagChunkHead)))
        {
            break;
        }

        //����Ҫ���ٿռ�
        fn -= iAllocSize - sizeof(tagChunkHead);
	}

	return TC_HashMap::RT_OK;
}

size_t TC_HashMap::Block::getDataLen()
{
	size_t n = 0;
	if(!getBlockHead()->_bNextChunk)
	{
		n += getBlockHead()->_iDataLen;
		return n;
	}

    //��ǰ��Ĵ�С
	n += getBlockHead()->_iSize - sizeof(tagBlockHead);
	tagChunkHead *pChunk    = getChunkHead(getBlockHead()->_iNextChunk);

	while (true)
	{
		if(pChunk->_bNextChunk)
		{
            //��ǰ��Ĵ�С
			n += pChunk->_iSize - sizeof(tagChunkHead);
			pChunk   =  getChunkHead(pChunk->_iNextChunk);
		}
		else
		{
			n += pChunk->_iDataLen;
			break;
		}
	}

	return n;
}

////////////////////////////////////////////////////////

size_t TC_HashMap::BlockAllocator::allocateMemBlock(size_t index, size_t &iAllocSize, vector<TC_HashMap::BlockData> &vtData)
{
begin:
	void *pAddr = _pChunkAllocator->allocate(iAllocSize, iAllocSize);
	if(pAddr == NULL)
	{
		size_t ret = _pMap->eraseExcept(0, vtData);
		if(ret == 0)
			return 0;     //û�пռ�����ͷ���
		goto begin;
	}

	//������µ�MemBlock, ��ʼ��һ��
	size_t iAddr = _pMap->getRelative(pAddr);
	Block block(_pMap, iAddr);
	block.makeNew(index, iAllocSize);

	_pMap->incChunkCount();

	return iAddr;
}

size_t TC_HashMap::BlockAllocator::allocateChunk(size_t iAddr, size_t &iAllocSize, vector<TC_HashMap::BlockData> &vtData)
{
begin:
	void *pAddr = _pChunkAllocator->allocate(iAllocSize, iAllocSize);

	if(pAddr == NULL)
	{
		size_t ret = _pMap->eraseExcept(iAddr, vtData);
		if(ret == 0)
			return 0;     //û�пռ�����ͷ���
		goto begin;
	}

	_pMap->incChunkCount();

	return _pMap->getRelative(pAddr);
}

void TC_HashMap::BlockAllocator::deallocateMemBlock(const vector<size_t> &v)
{
	for(size_t i = 0; i < v.size(); i++)
	{
		_pChunkAllocator->deallocate(_pMap->getAbsolute(v[i]));
		_pMap->delChunkCount();
	}
}

void TC_HashMap::BlockAllocator::deallocateMemBlock(size_t v)
{
	_pChunkAllocator->deallocate(_pMap->getAbsolute(v));
	_pMap->delChunkCount();
}

///////////////////////////////////////////////////////////////////

TC_HashMap::HashMapLockItem::HashMapLockItem(TC_HashMap *pMap, size_t iAddr)
: _pMap(pMap)
, _iAddr(iAddr)
{
}

TC_HashMap::HashMapLockItem::HashMapLockItem(const HashMapLockItem &mcmdi)
: _pMap(mcmdi._pMap)
, _iAddr(mcmdi._iAddr)
{
}

TC_HashMap::HashMapLockItem &TC_HashMap::HashMapLockItem::operator=(const TC_HashMap::HashMapLockItem &mcmdi)
{
    if(this != &mcmdi)
    {
        _pMap   = mcmdi._pMap;
        _iAddr  = mcmdi._iAddr;
    }
    return (*this);
}

bool TC_HashMap::HashMapLockItem::operator==(const TC_HashMap::HashMapLockItem &mcmdi)
{
    return _pMap == mcmdi._pMap && _iAddr == mcmdi._iAddr;
}

bool TC_HashMap::HashMapLockItem::operator!=(const TC_HashMap::HashMapLockItem &mcmdi)
{
    return _pMap != mcmdi._pMap || _iAddr != mcmdi._iAddr;
}

bool TC_HashMap::HashMapLockItem::isDirty()
{
    Block block(_pMap, _iAddr);
    return block.isDirty();
}

bool TC_HashMap::HashMapLockItem::isOnlyKey()
{
    Block block(_pMap, _iAddr);
    return block.isOnlyKey();
}

time_t TC_HashMap::HashMapLockItem::getSyncTime()
{
    Block block(_pMap, _iAddr);
    return block.getSyncTime();
}

int TC_HashMap::HashMapLockItem::get(string& k, string& v)
{
	string s;

	Block block(_pMap, _iAddr);

	int ret = block.get(s);
	if(ret != TC_HashMap::RT_OK)
	{
		return ret;
	}

	try
	{
		TC_PackOut po(s.c_str(), s.length());
		po >> k;
        if(!block.isOnlyKey())
        {
            po >> v;
        }
        else
        {
            v = "";
            return TC_HashMap::RT_ONLY_KEY;
        }
	}
	catch ( exception &ex )
	{
		return TC_HashMap::RT_EXCEPTION_ERR;
	}

	return TC_HashMap::RT_OK;
}

int TC_HashMap::HashMapLockItem::get(string& k)
{
	string s;

	Block block(_pMap, _iAddr);

	int ret = block.get(s);
	if(ret != TC_HashMap::RT_OK)
	{
		return ret;
	}

	try
	{
		TC_PackOut po(s.c_str(), s.length());
		po >> k;
	}
	catch ( exception &ex )
	{
		return TC_HashMap::RT_EXCEPTION_ERR;
	}

	return TC_HashMap::RT_OK;
}

int TC_HashMap::HashMapLockItem::set(const string& k, const string& v, vector<TC_HashMap::BlockData> &vtData)
{
	Block block(_pMap, _iAddr);

	TC_PackIn pi;
	pi << k;
	pi << v;

	return block.set(pi.topacket().c_str(), pi.topacket().length(), false, vtData);
}

int TC_HashMap::HashMapLockItem::set(const string& k, vector<TC_HashMap::BlockData> &vtData)
{
	Block block(_pMap, _iAddr);

	TC_PackIn pi;
	pi << k;

	return block.set(pi.topacket().c_str(), pi.topacket().length(), true, vtData);
}

bool TC_HashMap::HashMapLockItem::equal(const string &k, string &v, int &ret)
{
	string k1;
	ret = get(k1, v);

	if ((ret == TC_HashMap::RT_OK || ret == TC_HashMap::RT_ONLY_KEY) && k == k1)
	{
		return true;
	}

	return false;
}

bool TC_HashMap::HashMapLockItem::equal(const string& k, int &ret)
{
	string k1;
	ret = get(k1);

	if (ret == TC_HashMap::RT_OK && k == k1)
	{
		return true;
	}

	return false;
}

void TC_HashMap::HashMapLockItem::nextItem(int iType)
{
	Block block(_pMap, _iAddr);

	if(iType == HashMapLockIterator::IT_BLOCK)
	{
		size_t index = block.getBlockHead()->_iIndex;

		//��ǰblock������Ԫ��
		if(block.nextBlock())
		{
			_iAddr = block.getHead();
			return;
		}

		index += 1;

		while(index < _pMap->_hash.size())
		{
			//��ǰ��hashͰҲû������
			if (_pMap->item(index)->_iBlockAddr == 0)
			{
				++index;
				continue;
			}

			_iAddr = _pMap->item(index)->_iBlockAddr;
			return;
		}

		_iAddr = 0;  //��β����
	}
	else if(iType == HashMapLockIterator::IT_SET)
	{
		_iAddr = block.getBlockHead()->_iSetNext;
	}
	else if(iType == HashMapLockIterator::IT_GET)
	{
		_iAddr = block.getBlockHead()->_iGetNext;
	}
}

void TC_HashMap::HashMapLockItem::prevItem(int iType)
{
	Block block(_pMap, _iAddr);

	if(iType == HashMapLockIterator::IT_BLOCK)
	{
		size_t index = block.getBlockHead()->_iIndex;
		if(block.prevBlock())
		{
			_iAddr = block.getHead();
			return;
		}

		while(index > 0)
		{
			//��ǰ��hashͰҲû������
			if (_pMap->item(index-1)->_iBlockAddr == 0)
			{
				--index;
				continue;
			}

			//��Ҫ�����Ͱ��ĩβ
			Block tmp(_pMap, _pMap->item(index-1)->_iBlockAddr);
			_iAddr = tmp.getLastBlockHead();

			return;
		}

		_iAddr = 0;  //��β����
	}
	else if(iType == HashMapLockIterator::IT_SET)
	{
		_iAddr = block.getBlockHead()->_iSetPrev;
	}
	else if(iType == HashMapLockIterator::IT_GET)
	{
		_iAddr = block.getBlockHead()->_iGetPrev;
	}
}

///////////////////////////////////////////////////////////////////

TC_HashMap::HashMapLockIterator::HashMapLockIterator()
: _pMap(NULL), _iItem(NULL, 0), _iType(IT_BLOCK), _iOrder(IT_NEXT)
{
}

TC_HashMap::HashMapLockIterator::HashMapLockIterator(TC_HashMap *pMap, size_t iAddr, int iType, int iOrder)
: _pMap(pMap), _iItem(_pMap, iAddr), _iType(iType), _iOrder(iOrder)
{
}

TC_HashMap::HashMapLockIterator::HashMapLockIterator(const HashMapLockIterator &it)
: _pMap(it._pMap),_iItem(it._iItem), _iType(it._iType), _iOrder(it._iOrder)
{
}

TC_HashMap::HashMapLockIterator& TC_HashMap::HashMapLockIterator::operator=(const HashMapLockIterator &it)
{
    if(this != &it)
    {
        _pMap       = it._pMap;
        _iItem      = it._iItem;
        _iType      = it._iType;
        _iOrder     = it._iOrder;
    }

    return (*this);
}

bool TC_HashMap::HashMapLockIterator::operator==(const HashMapLockIterator& mcmi)
{
    if (_iItem.getAddr() != 0 || mcmi._iItem.getAddr() != 0)
    {
        return _pMap == mcmi._pMap
            && _iItem == mcmi._iItem
            && _iType == mcmi._iType
            && _iOrder == mcmi._iOrder;
    }

    return _pMap == mcmi._pMap;
}

bool TC_HashMap::HashMapLockIterator::operator!=(const HashMapLockIterator& mcmi)
{
    if (_iItem.getAddr() != 0 || mcmi._iItem.getAddr() != 0 )
    {
        return _pMap != mcmi._pMap
            || _iItem != mcmi._iItem
            || _iType != mcmi._iType
            || _iOrder != mcmi._iOrder;
    }

    return _pMap != mcmi._pMap;
}

TC_HashMap::HashMapLockIterator& TC_HashMap::HashMapLockIterator::operator++()
{
	if(_iOrder == IT_NEXT)
	{
		_iItem.nextItem(_iType);
	}
	else
	{
		_iItem.prevItem(_iType);
	}
	return (*this);

}

TC_HashMap::HashMapLockIterator TC_HashMap::HashMapLockIterator::operator++(int)
{
	HashMapLockIterator it(*this);

	if(_iOrder == IT_NEXT)
	{
		_iItem.nextItem(_iType);
	}
	else
	{
		_iItem.prevItem(_iType);
	}

	return it;

}

///////////////////////////////////////////////////////////////////

TC_HashMap::HashMapItem::HashMapItem(TC_HashMap *pMap, size_t iIndex)
: _pMap(pMap)
, _iIndex(iIndex)
{
}

TC_HashMap::HashMapItem::HashMapItem(const HashMapItem &mcmdi)
: _pMap(mcmdi._pMap)
, _iIndex(mcmdi._iIndex)
{
}

TC_HashMap::HashMapItem &TC_HashMap::HashMapItem::operator=(const TC_HashMap::HashMapItem &mcmdi)
{
    if(this != &mcmdi)
    {
        _pMap   = mcmdi._pMap;
        _iIndex  = mcmdi._iIndex;
    }
    return (*this);
}

bool TC_HashMap::HashMapItem::operator==(const TC_HashMap::HashMapItem &mcmdi)
{
    return _pMap == mcmdi._pMap && _iIndex == mcmdi._iIndex;
}

bool TC_HashMap::HashMapItem::operator!=(const TC_HashMap::HashMapItem &mcmdi)
{
    return _pMap != mcmdi._pMap || _iIndex != mcmdi._iIndex;
}

void TC_HashMap::HashMapItem::get(vector<TC_HashMap::BlockData> &vtData)
{
    size_t iAddr = _pMap->item(_iIndex)->_iBlockAddr;

    while(iAddr != 0)
    {
        Block block(_pMap, iAddr);
        TC_HashMap::BlockData data;

        int ret = block.getBlockData(data);
        if(ret == TC_HashMap::RT_OK)
        {
            vtData.push_back(data);
        }

        iAddr = block.getBlockHead()->_iBlockNext;
    }
}

void TC_HashMap::HashMapItem::nextItem()
{
    if(_iIndex == (size_t)(-1))
    {
        return;
    }

    if(_iIndex >= _pMap->getHashCount() - 1)
    {
        _iIndex = (size_t)(-1);
        return;
    }
    _iIndex++;
}

int TC_HashMap::HashMapItem::setDirty()
{
	if(_pMap->getMapHead()._bReadOnly) return RT_READONLY;

	size_t iAddr = _pMap->item(_iIndex)->_iBlockAddr;

	while(iAddr != 0)
    {
        Block block(_pMap, iAddr);

		if(!block.isOnlyKey())
		{
			_pMap->doUpdate();

			block.setDirty(true);
			block.refreshSetList();
		}

        iAddr = block.getBlockHead()->_iBlockNext;
    }

	return RT_OK;
}

///////////////////////////////////////////////////////////////////

TC_HashMap::HashMapIterator::HashMapIterator()
: _pMap(NULL), _iItem(NULL, 0)
{
}

TC_HashMap::HashMapIterator::HashMapIterator(TC_HashMap *pMap, size_t iIndex)
: _pMap(pMap), _iItem(_pMap, iIndex)
{
}

TC_HashMap::HashMapIterator::HashMapIterator(const HashMapIterator &it)
: _pMap(it._pMap),_iItem(it._iItem)
{
}

TC_HashMap::HashMapIterator& TC_HashMap::HashMapIterator::operator=(const HashMapIterator &it)
{
    if(this != &it)
    {
        _pMap       = it._pMap;
        _iItem      = it._iItem;
    }

    return (*this);
}

bool TC_HashMap::HashMapIterator::operator==(const HashMapIterator& mcmi)
{
    if (_iItem.getIndex() != -1 || mcmi._iItem.getIndex() != -1)
    {
        return _pMap == mcmi._pMap && _iItem == mcmi._iItem;
    }

    return _pMap == mcmi._pMap;
}

bool TC_HashMap::HashMapIterator::operator!=(const HashMapIterator& mcmi)
{
    if (_iItem.getIndex() != -1 || mcmi._iItem.getIndex() != -1 )
    {
        return _pMap != mcmi._pMap || _iItem != mcmi._iItem;
    }

    return _pMap != mcmi._pMap;
}

TC_HashMap::HashMapIterator& TC_HashMap::HashMapIterator::operator++()
{
	_iItem.nextItem();
	return (*this);
}

TC_HashMap::HashMapIterator TC_HashMap::HashMapIterator::operator++(int)
{
	HashMapIterator it(*this);
	_iItem.nextItem();
	return it;
}

//////////////////////////////////////////////////////////////////////////////////

void TC_HashMap::init(void *pAddr)
{
	_pHead          = static_cast<tagMapHead*>(pAddr);
	_pstModifyHead  = static_cast<tagModifyHead*>((void*)((char*)pAddr + sizeof(tagMapHead)));
}

void TC_HashMap::initDataBlockSize(size_t iMinDataSize, size_t iMaxDataSize, float fFactor)
{
    _iMinDataSize   = iMinDataSize;
    _iMaxDataSize   = iMaxDataSize;
    _fFactor        = fFactor;
}

void TC_HashMap::create(void *pAddr, size_t iSize)
{
	if(sizeof(tagHashItem) * 1
	   + sizeof(tagMapHead)
	   + sizeof(tagModifyHead)
	   + sizeof(TC_MemMultiChunkAllocator::tagChunkAllocatorHead)
	   + 10 > iSize)
	{
        throw TC_HashMap_Exception("[TC_HashMap::create] mem size not enougth.");
    }

    if(_iMinDataSize == 0 || _iMaxDataSize == 0 || _fFactor < 1.0)
    {
        throw TC_HashMap_Exception("[TC_HashMap::create] init data size error:" + TC_Common::tostr(_iMinDataSize) + "|" + TC_Common::tostr(_iMaxDataSize) + "|" + TC_Common::tostr(_fFactor));
    }

	init(pAddr);

	_pHead->_cMaxVersion    = MAX_VERSION;
    _pHead->_cMinVersion    = MIN_VERSION;
	_pHead->_bReadOnly      = false;
    _pHead->_bAutoErase     = true;
    _pHead->_cEraseMode     = ERASEBYGET;
    _pHead->_iMemSize       = iSize;
    _pHead->_iMinDataSize   = _iMinDataSize;
    _pHead->_iMaxDataSize   = _iMaxDataSize;
    _pHead->_fFactor        = _fFactor;
    _pHead->_fRadio         = _fRadio;
	_pHead->_iElementCount  = 0;
	_pHead->_iEraseCount    = 10;
	_pHead->_iSetHead       = 0;
	_pHead->_iSetTail       = 0;
    _pHead->_iGetHead       = 0;
    _pHead->_iGetTail       = 0;
    _pHead->_iDirtyCount    = 0;
	_pHead->_iDirtyTail     = 0;
    _pHead->_iSyncTime      = 60 * 10;  //ȱʡʮ���ӻ�дһ��
	_pHead->_iUsedChunk     = 0;
	_pHead->_iGetCount      = 0;
	_pHead->_iHitCount      = 0;
	_pHead->_iBackupTail    = 0;
    _pHead->_iSyncTail      = 0;

    //����ƽ��block��С
	size_t iBlockSize   = (_pHead->_iMinDataSize + _pHead->_iMaxDataSize)/2 + sizeof(Block::tagBlockHead);

	//Hash����
	size_t iHashCount   = (iSize - sizeof(TC_MemChunkAllocator::tagChunkAllocatorHead)) / ((size_t)(iBlockSize*_fRadio) + sizeof(tagHashItem));
    //���������������Ϊhashֵ
    iHashCount          = getMinPrimeNumber(iHashCount);

	void *pHashAddr     = (char*)_pHead + sizeof(tagMapHead) + sizeof(tagModifyHead);

    size_t iHashMemSize = TC_MemVector<tagHashItem>::calcMemSize(iHashCount);
    _hash.create(pHashAddr, iHashMemSize);

	void *pDataAddr     = (char*)pHashAddr + _hash.getMemSize();

	_pDataAllocator->create(pDataAddr, iSize - ((char*)pDataAddr - (char*)_pHead), sizeof(Block::tagBlockHead) + _pHead->_iMinDataSize, sizeof(Block::tagBlockHead) + _pHead->_iMaxDataSize, _pHead->_fFactor);
}

void TC_HashMap::connect(void *pAddr, size_t iSize)
{
	init(pAddr);

	if(_pHead->_cMaxVersion != MAX_VERSION || _pHead->_cMinVersion != MIN_VERSION)
	{
        ostringstream os;
        os << (int)_pHead->_cMaxVersion << "." << (int)_pHead->_cMinVersion << " != " << ((int)MAX_VERSION) << "." << ((int)MIN_VERSION);
        throw TC_HashMap_Exception("[TC_HashMap::connect] hash map version not equal:" + os.str() + " (data != code)");
	}

    if(_pHead->_iMemSize != iSize)
    {
        throw TC_HashMap_Exception("[TC_HashMap::connect] hash map size not equal:" + TC_Common::tostr(_pHead->_iMemSize) + "!=" + TC_Common::tostr(iSize));
    }

	void *pHashAddr = (char*)_pHead + sizeof(tagMapHead) + sizeof(tagModifyHead);
	_hash.connect(pHashAddr);

	void *pDataAddr = (char*)pHashAddr + _hash.getMemSize();

	_pDataAllocator->connect(pDataAddr);

    _iMinDataSize   = _pHead->_iMinDataSize;
    _iMaxDataSize   = _pHead->_iMaxDataSize;
    _fFactor        = _pHead->_fFactor;
    _fRadio         = _pHead->_fRadio;
}

int TC_HashMap::append(void *pAddr, size_t iSize)
{
    if(iSize <= _pHead->_iMemSize)
    {
        return -1;
    }

    init(pAddr);

    if(_pHead->_cMaxVersion != MAX_VERSION || _pHead->_cMinVersion != MIN_VERSION)
    {
        ostringstream os;
        os << (int)_pHead->_cMaxVersion << "." << (int)_pHead->_cMinVersion << " != " << ((int)MAX_VERSION) << "." << ((int)MIN_VERSION);
        throw TC_HashMap_Exception("[TC_HashMap::append] hash map version not equal:" + os.str() + " (data != code)");
    }

    _pHead->_iMemSize = iSize;

	void *pHashAddr = (char*)_pHead + sizeof(tagMapHead) + sizeof(tagModifyHead);
	_hash.connect(pHashAddr);

	void *pDataAddr = (char*)pHashAddr + _hash.getMemSize();
	_pDataAllocator->append(pDataAddr, iSize - ((size_t)pDataAddr - (size_t)pAddr));

    _iMinDataSize   = _pHead->_iMinDataSize;
    _iMaxDataSize   = _pHead->_iMaxDataSize;
    _fFactor        = _pHead->_fFactor;
    _fRadio         = _pHead->_fRadio;

    return 0;
}

void TC_HashMap::clear()
{
    assert(_pHead);

	_pHead->_iElementCount  = 0;
	_pHead->_iSetHead       = 0;
	_pHead->_iSetTail       = 0;
    _pHead->_iGetHead       = 0;
    _pHead->_iGetTail       = 0;
    _pHead->_iDirtyCount    = 0;
	_pHead->_iDirtyTail     = 0;
	_pHead->_iUsedChunk     = 0;
	_pHead->_iGetCount      = 0;
	_pHead->_iHitCount      = 0;
	_pHead->_iBackupTail    = 0;
    _pHead->_iSyncTail      = 0;

    _hash.clear();

	_pDataAllocator->rebuild();
}

int TC_HashMap::dump2file(const string &sFile)
{
	FILE *fp = fopen(sFile.c_str(), "wb");
	if(fp == NULL)
	{
		return RT_DUMP_FILE_ERR;
	}

	size_t ret = fwrite((void*)_pHead, 1, _pHead->_iMemSize, fp);
	fclose(fp);

	if(ret == _pHead->_iMemSize)
	{
		return RT_OK;
	}
	return RT_DUMP_FILE_ERR;
}

int TC_HashMap::load5file(const string &sFile)
{
	FILE *fp = fopen(sFile.c_str(), "rb");
	if(fp == NULL)
	{
		return RT_LOAL_FILE_ERR;
	}
	fseek(fp, 0L, SEEK_END);
	size_t fs = ftell(fp);
	if(fs != _pHead->_iMemSize)
	{
		fclose(fp);
		return RT_LOAL_FILE_ERR;
	}

	fseek(fp, 0L, SEEK_SET);

	size_t iSize    = 1024*1024*10;
	size_t iLen     = 0;
	char *pBuffer = new char[iSize];
	bool bEof = false;
	while(true)
	{
		int ret = fread(pBuffer, 1, iSize, fp);
		if(ret != (int)iSize)
		{
			if(feof(fp))
			{
				bEof = true;
			}
			else
			{
				fclose(fp);
				delete[] pBuffer;
				return RT_LOAL_FILE_ERR;
			}

		}
		//���汾
		if(iLen == 0)
		{
			if(pBuffer[0] != MAX_VERSION || pBuffer[1] != MIN_VERSION)
			{
				fclose(fp);
				delete[] pBuffer;
				return RT_VERSION_MISMATCH_ERR;
			}
		}

		memcpy((char*)_pHead + iLen, pBuffer, ret);
		iLen += ret;
		if(bEof)
			break;
	}
	fclose(fp);
	delete[] pBuffer;
	if(iLen == _pHead->_iMemSize)
	{
		return RT_OK;
	}

	return RT_LOAL_FILE_ERR;
}

int TC_HashMap::recover(size_t i, bool bRepair)
{
    doUpdate();

    if( i >= _hash.size())
    {
        return 0;
    }

    size_t e     = 0;

check:
    size_t iAddr = item(i)->_iBlockAddr;

    Block block(this, iAddr);

    while(block.getHead() != 0)
    {
        BlockData data;
        int ret = block.getBlockData(data);
        if(ret != TC_HashMap::RT_OK && ret != TC_HashMap::RT_ONLY_KEY)
        {
            //����ɾ��block��
            ++e;

            if(bRepair)
            {
                block.erase();
                goto check;
            }
        }

        if(!block.nextBlock())
        {
            break;
        }
    }

    return e;
}

int TC_HashMap::checkDirty(const string &k)
{
	doUpdate();
	incGetCount();

	int ret         = TC_HashMap::RT_OK;
	size_t index    = hashIndex(k);
	lock_iterator it= find(k, index, ret);
	if(ret != TC_HashMap::RT_OK)
	{
		return ret;
	}

    //û������
	if(it == end())
	{
		return TC_HashMap::RT_NO_DATA;
	}

    //ֻ��Key
    if(it->isOnlyKey())
    {
        return TC_HashMap::RT_ONLY_KEY;
    }

	Block block(this, it->getAddr());
	if (block.isDirty())
	{
		return TC_HashMap::RT_DIRTY_DATA;
	}

	return TC_HashMap::RT_OK;
}

int TC_HashMap::setDirty(const string& k)
{
	doUpdate();

    if(_pHead->_bReadOnly) return RT_READONLY;

	incGetCount();

	int ret         = TC_HashMap::RT_OK;
	size_t index    = hashIndex(k);
	lock_iterator it= find(k, index, ret);
	if(ret != TC_HashMap::RT_OK)
	{
		return ret;
	}

    //û�����ݻ�ֻ��Key
	if(it == end())
	{
		return TC_HashMap::RT_NO_DATA;
	}

    //ֻ��Key
    if(it->isOnlyKey())
    {
        return TC_HashMap::RT_ONLY_KEY;
    }

	Block block(this, it->getAddr());
	block.setDirty(true);
	block.refreshSetList();

	return TC_HashMap::RT_OK;
}

int TC_HashMap::setDirtyAfterSync(const string& k)
{
	doUpdate();

    if(_pHead->_bReadOnly) return RT_READONLY;

	incGetCount();

	int ret         = TC_HashMap::RT_OK;
	size_t index    = hashIndex(k);
	lock_iterator it= find(k, index, ret);
	if(ret != TC_HashMap::RT_OK)
	{
		return ret;
	}

    //û�����ݻ�ֻ��Key
	if(it == end())
	{
		return TC_HashMap::RT_NO_DATA;
	}

    //ֻ��Key
    if(it->isOnlyKey())
    {
        return TC_HashMap::RT_ONLY_KEY;
    }

	Block block(this, it->getAddr());
	block.setDirty(true);
	if(_pHead->_iDirtyTail == block.getBlockHead()->_iSetPrev)
	{
		_pHead->_iDirtyTail = block.getHead();
	}

	return TC_HashMap::RT_OK;
}

int TC_HashMap::setClean(const string& k)
{
	doUpdate();

    if(_pHead->_bReadOnly) return RT_READONLY;

	incGetCount();

	int ret         = TC_HashMap::RT_OK;
	size_t index    = hashIndex(k);
	lock_iterator it= find(k, index, ret);
	if(ret != TC_HashMap::RT_OK)
	{
		return ret;
	}

    //û�����ݻ�ֻ��Key
	if(it == end())
	{
		return TC_HashMap::RT_NO_DATA;
	}

    //ֻ��Key
    if(it->isOnlyKey())
    {
        return TC_HashMap::RT_ONLY_KEY;
    }

	Block block(this, it->getAddr());
	block.setDirty(false);
	block.refreshSetList();

	doUpdate(true);

	return TC_HashMap::RT_OK;
}

int TC_HashMap::get(const string& k, string &v, time_t &iSyncTime)
{
	doUpdate();
	incGetCount();

    int ret             = TC_HashMap::RT_OK;

	size_t index        = hashIndex(k);
	lock_iterator it    = find(k, index, v, ret);

	if(ret != TC_HashMap::RT_OK && ret != TC_HashMap::RT_ONLY_KEY)
	{
		return ret;
	}

    //û������
	if(it == end())
	{
		return TC_HashMap::RT_NO_DATA;
	}

    //ֻ��Key
    if(it->isOnlyKey())
    {
        return TC_HashMap::RT_ONLY_KEY;
    }

    Block block(this, it->getAddr());
    iSyncTime = block.getSyncTime();

    //���ֻ��, ��ˢ��get����
    if(!_pHead->_bReadOnly)
    {
        block.refreshGetList();
    }
	return TC_HashMap::RT_OK;
}

int TC_HashMap::get(const string& k, string &v)
{
    time_t iSyncTime;
    return get(k, v, iSyncTime);
}

int TC_HashMap::set(const string& k, const string& v, bool bDirty, vector<BlockData> &vtData)
{
	doUpdate();
	incGetCount();

    if(_pHead->_bReadOnly) return RT_READONLY;

	int ret             = TC_HashMap::RT_OK;
	size_t index        = hashIndex(k);
	lock_iterator it    = find(k, index, ret);
	bool bNewBlock = false;

	if(ret != TC_HashMap::RT_OK)
	{
		return ret;
	}

	if(it == end())
	{
        TC_PackIn pi;
        pi << k;
        pi << v;
        size_t iAllocSize = sizeof(Block::tagBlockHead) + pi.length();

		//�ȷ���ռ�, �������̭������
		size_t iAddr = _pDataAllocator->allocateMemBlock(index, iAllocSize, vtData);
        if(iAddr == 0)
        {
            return TC_HashMap::RT_NO_MEMORY;
        }

		it = HashMapLockIterator(this, iAddr, HashMapLockIterator::IT_BLOCK, HashMapLockIterator::IT_NEXT);
		bNewBlock = true;

	}

	ret = it->set(k, v, vtData);
	if(ret != TC_HashMap::RT_OK)
	{
        //�¼�¼, дʧ����, Ҫɾ���ÿ�
        if(bNewBlock)
        {
            Block block(this, it->getAddr());
            block.erase();
        }
		return ret;
	}

	Block block(this, it->getAddr());
	if(bNewBlock)
	{
		block.setSyncTime(time(NULL));
	}
	block.setDirty(bDirty);
	block.refreshSetList();

	return TC_HashMap::RT_OK;
}

int TC_HashMap::set(const string& k, vector<BlockData> &vtData)
{
	doUpdate();
	incGetCount();

    if(_pHead->_bReadOnly) return RT_READONLY;

	int ret             = TC_HashMap::RT_OK;
	size_t index        = hashIndex(k);
	lock_iterator it    = find(k, index, ret);
	bool bNewBlock = false;

	if(ret != TC_HashMap::RT_OK)
	{
		return ret;
	}

	if(it == end())
	{
        TC_PackIn pi;
        pi << k;
        size_t iAllocSize = sizeof(Block::tagBlockHead) + pi.length();

		//�ȷ���ռ�, �������̭������
		size_t iAddr = _pDataAllocator->allocateMemBlock(index, iAllocSize, vtData);
        if(iAddr == 0)
        {
            return TC_HashMap::RT_NO_MEMORY;
        }

		it = HashMapLockIterator(this, iAddr, HashMapLockIterator::IT_BLOCK, HashMapLockIterator::IT_NEXT);
		bNewBlock = true;
	}

	ret = it->set(k, vtData);
	if(ret != TC_HashMap::RT_OK)
	{
        //�¼�¼, дʧ����, Ҫɾ���ÿ�
        if(bNewBlock)
        {
            Block block(this, it->getAddr());
            block.erase();
        }
		return ret;
	}

	Block block(this, it->getAddr());
	if(bNewBlock)
	{
		block.setSyncTime(time(NULL));
	}
	block.refreshSetList();

	return TC_HashMap::RT_OK;
}

int TC_HashMap::del(const string& k, BlockData &data)
{
	doUpdate();
	incGetCount();

    if(_pHead->_bReadOnly) return RT_READONLY;

	int    ret      = TC_HashMap::RT_OK;
	size_t index    = hashIndex(k);

    data._key       = k;

	lock_iterator it     = find(k, index, data._value, ret);
	if(ret != TC_HashMap::RT_OK && ret != TC_HashMap::RT_ONLY_KEY)
	{
		return ret;
	}

	if(it == end())
	{
		return TC_HashMap::RT_NO_DATA;
	}

	Block block(this, it->getAddr());
    ret = block.getBlockData(data);
    block.erase();

    return ret;
}

int TC_HashMap::erase(int radio, BlockData &data, bool bCheckDirty)
{
    doUpdate();

    if(_pHead->_bReadOnly) return RT_READONLY;

    if(radio <= 0)   radio = 1;
    if(radio >= 100) radio = 100;

	size_t iAddr    = _pHead->_iGetTail;
    //������ͷ��
	if(iAddr == 0)
	{
		return RT_OK;
	}

    //ɾ����ָ��������
    if((_pHead->_iUsedChunk * 100. / allBlockChunkCount()) < radio)
    {
        return RT_OK;
    }

	Block block(this, iAddr);
	if(bCheckDirty)
	{
		if(block.isDirty())
		{
			if(_pHead->_iDirtyTail == 0)
			{
				_pHead->_iDirtyTail = iAddr;
			}

			// ���������ƶ���get����ͷ����ʹ���Լ�����̭
			if(_pHead->_iGetHead == iAddr)
			{
				// ����ͷҲ��β��ֻ��һ��Ԫ����
				return RT_OK;
			}
			if(block._pMap->_pHead->_iBackupTail == block._iHead)
			{
				update(&block._pMap->_pHead->_iBackupTail, block.getBlockHead()->_iGetPrev);
			}
			// ��GetTail����
			update(&_pHead->_iGetTail, block.getBlockHead()->_iGetPrev);
			// ��Getβ���Ƴ�
			update(&block.getBlockHead(block.getBlockHead()->_iGetPrev)->_iGetNext, (size_t)0);
			update(&block.getBlockHead()->_iGetPrev, (size_t)0);
			// �Ƶ�Getͷ��
			update(&block.getBlockHead()->_iGetNext, _pHead->_iGetHead);
			update(&block.getBlockHead(_pHead->_iGetHead)->_iGetPrev, iAddr);
			update(&_pHead->_iGetHead, iAddr);

			doUpdate(true);

			return RT_DIRTY_DATA;
		}
	}
    int ret = block.getBlockData(data);
    block.erase();
    if(ret == TC_HashMap::RT_OK)
    {
        return TC_HashMap::RT_ERASE_OK;
    }
    return ret;
}

void TC_HashMap::sync()
{
    doUpdate();

    _pHead->_iSyncTail = _pHead->_iDirtyTail;
}

int TC_HashMap::sync(time_t iNowTime, BlockData &data)
{
    doUpdate();

	size_t iAddr    = _pHead->_iSyncTail;

    //������ͷ����, ����RT_OK
	if(iAddr == 0)
	{
		return RT_OK;
	}

	Block block(this, iAddr);

    _pHead->_iSyncTail = block.getBlockHead()->_iSetPrev;

    //��ǰ�����Ǹɾ�����
    if( !block.isDirty())
    {
		if(_pHead->_iDirtyTail == iAddr)
		{
			_pHead->_iDirtyTail = block.getBlockHead()->_iSetPrev;
		}
        return RT_NONEED_SYNC;
    }

    int ret = block.getBlockData(data);
    if(ret != TC_HashMap::RT_OK)
    {
        //û�л�ȡ�����ļ�¼
		if(_pHead->_iDirtyTail == iAddr)
		{
			_pHead->_iDirtyTail = block.getBlockHead()->_iSetPrev;
		}
        return ret;
    }

    //�������ҳ���_pHead->_iSyncTimeû�л�д, ��Ҫ��д
    if(_pHead->_iSyncTime + data._synct < iNowTime && block.isDirty())
    {
    	block.setDirty(false);
        block.setSyncTime(iNowTime);

        if(_pHead->_iDirtyTail == iAddr)
        {
            _pHead->_iDirtyTail = block.getBlockHead()->_iSetPrev;
        }

        return RT_NEED_SYNC;
    }

    //������, ���ǲ���Ҫ��д, ��������ǰ��
	return RT_NONEED_SYNC;
}

void TC_HashMap::backup(bool bForceFromBegin)
{
    doUpdate();
    if(bForceFromBegin || _pHead->_iBackupTail == 0)
    {
        //�ƶ�����ָ�뵽Get��β��, ׼����ʼ��������
        _pHead->_iBackupTail = _pHead->_iGetTail;
    }
}

int TC_HashMap::backup(BlockData &data)
{
    doUpdate();

	size_t iAddr    = _pHead->_iBackupTail;

	//������ͷ����, ����RT_OK
	if(iAddr == 0)
	{
		return RT_OK;
	}

	Block block(this, iAddr);
    int ret = block.getBlockData(data);

    //Ǩ��һ��
    _pHead->_iBackupTail = block.getBlockHead()->_iGetPrev;
	doUpdate(true);

    if(ret == TC_HashMap::RT_OK)
    {
        return TC_HashMap::RT_NEED_BACKUP;
    }
    return ret;
}

TC_HashMap::lock_iterator TC_HashMap::find(const string& k)
{
	size_t index = hashIndex(k);
	int ret = TC_HashMap::RT_OK;

	doUpdate();

	if(item(index)->_iBlockAddr == 0)
	{
		return end();
	}

	Block mb(this, item(index)->_iBlockAddr);
	while(true)
	{
		HashMapLockItem mcmdi(this, mb.getHead());
		if(mcmdi.equal(k, ret))
		{
			incHitCount();
			return lock_iterator(this, mb.getHead(), lock_iterator::IT_BLOCK, lock_iterator::IT_NEXT);
		}

		if (!mb.nextBlock())
		{
			return end();
		}
	}

	return end();
}

TC_HashMap::lock_iterator TC_HashMap::begin()
{
	doUpdate();

	for(size_t i = 0; i < _hash.size(); i++)
	{
		tagHashItem &hashItem = _hash[i];
		if(hashItem._iBlockAddr != 0)
		{
			return lock_iterator(this, hashItem._iBlockAddr, lock_iterator::IT_BLOCK, lock_iterator::IT_NEXT);
		}
	}

	return end();
}

TC_HashMap::lock_iterator TC_HashMap::rbegin()
{
	doUpdate();

	for(size_t i = _hash.size(); i > 0; i--)
	{
		tagHashItem &hashItem = _hash[i-1];
		if(hashItem._iBlockAddr != 0)
		{
			Block block(this, hashItem._iBlockAddr);
			return lock_iterator(this, block.getLastBlockHead(), lock_iterator::IT_BLOCK, lock_iterator::IT_PREV);
		}
	}

	return end();
}

TC_HashMap::lock_iterator TC_HashMap::beginSetTime()
{
	doUpdate();
	return lock_iterator(this, _pHead->_iSetHead, lock_iterator::IT_SET, lock_iterator::IT_NEXT);
}

TC_HashMap::lock_iterator TC_HashMap::rbeginSetTime()
{
	doUpdate();
	return lock_iterator(this, _pHead->_iSetTail, lock_iterator::IT_SET, lock_iterator::IT_PREV);
}

TC_HashMap::lock_iterator TC_HashMap::beginGetTime()
{
	doUpdate();
	return lock_iterator(this, _pHead->_iGetHead, lock_iterator::IT_GET, lock_iterator::IT_NEXT);
}

TC_HashMap::lock_iterator TC_HashMap::rbeginGetTime()
{
	doUpdate();
	return lock_iterator(this, _pHead->_iGetTail, lock_iterator::IT_GET, lock_iterator::IT_PREV);
}

TC_HashMap::lock_iterator TC_HashMap::beginDirty()
{
	doUpdate();
	return lock_iterator(this, _pHead->_iDirtyTail, lock_iterator::IT_SET, lock_iterator::IT_PREV);
}

TC_HashMap::hash_iterator TC_HashMap::hashBegin()
{
    doUpdate();
    return hash_iterator(this, 0);
}

TC_HashMap::hash_iterator TC_HashMap::hashIndex(size_t iIndex)
{
	doUpdate();
	return hash_iterator(this, iIndex);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

string TC_HashMap::desc()
{
	ostringstream s;
	{
		s << "[Version          = "   << (int)_pHead->_cMaxVersion << "." << (int)_pHead->_cMinVersion << "]" << endl;
		s << "[ReadOnly         = "   << _pHead->_bReadOnly    << "]" << endl;
		s << "[AutoErase        = "   << _pHead->_bAutoErase   << "]" << endl;
		s << "[MemSize          = "   << _pHead->_iMemSize          << "]" << endl;
		s << "[Capacity         = "   << _pDataAllocator->getCapacity()  << "]" << endl;
        s << "[SingleBlockCount = "   << TC_Common::tostr(singleBlockChunkCount())         << "]" << endl;
		s << "[AllBlockChunk    = "   << allBlockChunkCount()       << "]" << endl;
        s << "[UsedChunk        = "   << _pHead->_iUsedChunk        << "]" << endl;
        s << "[FreeChunk        = "   << allBlockChunkCount() - _pHead->_iUsedChunk        << "]" << endl;
        s << "[MinDataSize      = "   << _pHead->_iMinDataSize      << "]" << endl;
        s << "[MaxDataSize      = "   << _pHead->_iMaxDataSize      << "]" << endl;
		s << "[HashCount        = "   << _hash.size()               << "]" << endl;
		s << "[HashRadio        = "   << _pHead->_fRadio            << "]" << endl;
		s << "[ElementCount     = "   << _pHead->_iElementCount     << "]" << endl;
		s << "[SetHead          = "   << _pHead->_iSetHead       << "]" << endl;
		s << "[SetTail          = "   << _pHead->_iSetTail       << "]" << endl;
        s << "[GetHead          = "   << _pHead->_iGetHead       << "]" << endl;
        s << "[GetTail          = "   << _pHead->_iGetTail       << "]" << endl;
        s << "[DirtyTail        = "   << _pHead->_iDirtyTail        << "]" << endl;
        s << "[SyncTail         = "   << _pHead->_iSyncTail        << "]" << endl;
        s << "[SyncTime         = "   << _pHead->_iSyncTime        << "]" << endl;
        s << "[BackupTail       = "   << _pHead->_iBackupTail      << "]" << endl;
		s << "[DirtyCount       = "   << _pHead->_iDirtyCount       << "]" << endl;
		s << "[GetCount         = "   << _pHead->_iGetCount         << "]" << endl;
		s << "[HitCount         = "   << _pHead->_iHitCount         << "]" << endl;
		s << "[ModifyStatus     = "   << (int)_pstModifyHead->_cModifyStatus     << "]" << endl;
		s << "[ModifyIndex      = "   << _pstModifyHead->_iNowIndex << "]" << endl;
		s << "[OnlyKeyCount     = "	  << _pHead->_iOnlyKeyCount		<< "]" << endl;
	}

	uint32_t iMaxHash;
	uint32_t iMinHash;
	float fAvgHash;

	analyseHash(iMaxHash, iMinHash, fAvgHash);
	{
		s << "[MaxHash          = "   << iMaxHash      << "]" << endl;
		s << "[MinHash          = "   << iMinHash     << "]" << endl;
		s << "[AvgHash          = "   << fAvgHash     << "]" << endl;
	}

    vector<TC_MemChunk::tagChunkHead> vChunkHead = _pDataAllocator->getBlockDetail();

    s << "*************************************************************************" << endl;
    s << "[DiffBlockCount   = "   << vChunkHead.size()  << "]" << endl;
    for(size_t i = 0; i < vChunkHead.size(); i++)
    {
        s << "[BlockSize        = "   << vChunkHead[i]._iBlockSize      << "]";
        s << "[BlockCount       = "   << vChunkHead[i]._iBlockCount     << "]";
        s << "[BlockAvailable   = "   << vChunkHead[i]._blockAvailable  << "]" << endl;
    }

	return s.str();
}

size_t TC_HashMap::eraseExcept(size_t iNowAddr, vector<BlockData> &vtData)
{
    //���ܱ���̭
    if(!_pHead->_bAutoErase) return 0;

	size_t n = _pHead->_iEraseCount;
	if(n == 0) n = 10;
	size_t d = n;

	while (d != 0)
	{
		size_t iAddr;

        //�жϰ������ַ�ʽ��̭
        if(_pHead->_cEraseMode == TC_HashMap::ERASEBYSET)
        {
            iAddr = _pHead->_iSetTail;
        }
        else
        {
            iAddr = _pHead->_iGetTail;
        }

		if (iAddr == 0)
		{
			break;
		}

		Block block(this, iAddr);

        //��ǰblock���ڷ���ռ�, ����ɾ��, �Ƶ���һ������
		if (iNowAddr == iAddr)
		{
            if(_pHead->_cEraseMode == TC_HashMap::ERASEBYSET)
            {
                iAddr = block.getBlockHead()->_iSetPrev;
            }
            else
            {
                iAddr = block.getBlockHead()->_iGetPrev;
            }
		}
		if(iAddr == 0)
		{
			break;
		}

		Block block1(this, iAddr);
        BlockData data;
        int ret = block1.getBlockData(data);
        if(ret == TC_HashMap::RT_OK)
        {
            vtData.push_back(data);
            d--;
        }
        else if(ret == TC_HashMap::RT_NO_DATA)
        {
            d--;
        }

		//ɾ������
		block1.erase();
	}

	return n-d;
}

size_t TC_HashMap::hashIndex(const string& k)
{
	return _hashf(k) % _hash.size();
}

TC_HashMap::lock_iterator TC_HashMap::find(const string& k, size_t index, string &v, int &ret)
{
	ret = TC_HashMap::RT_OK;

	if(item(index)->_iBlockAddr == 0)
	{
		return end();
	}

	Block mb(this, item(index)->_iBlockAddr);
	while(true)
	{
		HashMapLockItem mcmdi(this, mb.getHead());
		if(mcmdi.equal(k, v, ret))
		{
			incHitCount();
			return lock_iterator(this, mb.getHead(), lock_iterator::IT_BLOCK, lock_iterator::IT_NEXT);
		}

		if (!mb.nextBlock())
		{
			return end();
		}
	}

	return end();
}

TC_HashMap::lock_iterator TC_HashMap::find(const string& k, size_t index, int &ret)
{
	ret = TC_HashMap::RT_OK;

	if(item(index)->_iBlockAddr == 0)
	{
		return end();
	}

	Block mb(this, item(index)->_iBlockAddr);
	while(true)
	{
		HashMapLockItem mcmdi(this, mb.getHead());
		if(mcmdi.equal(k, ret))
		{
			incHitCount();
			return lock_iterator(this, mb.getHead(), lock_iterator::IT_BLOCK, lock_iterator::IT_NEXT);
		}

		if (!mb.nextBlock())
		{
			return end();
		}
	}

	return end();
}

void TC_HashMap::analyseHash(uint32_t &iMaxHash, uint32_t &iMinHash, float &fAvgHash)
{
	iMaxHash = 0;
	iMinHash = (uint32_t)-1;

	fAvgHash = 0;

	uint32_t n = 0;
	for(uint32_t i = 0; i < _hash.size(); i++)
	{
		iMaxHash = max(_hash[i]._iListCount, iMaxHash);
		iMinHash = min(_hash[i]._iListCount, iMinHash);
		//ƽ��ֵֻͳ�Ʒ�0��
		if(_hash[i]._iListCount != 0)
		{
			n++;
			fAvgHash  += _hash[i]._iListCount;
		}
	}

	if(n != 0)
	{
		fAvgHash = fAvgHash / n;
	}
}

void TC_HashMap::doUpdate(bool bUpdate)
{
	if(bUpdate)
	{
		_pstModifyHead->_cModifyStatus = 2;
	}

	//==1, copy������, ����ʧ��, ��Ҫ���״̬
	if(_pstModifyHead->_cModifyStatus == 1)
	{
		_pstModifyHead->_iNowIndex        = 0;
		for(size_t i = 0; i < sizeof(_pstModifyHead->_stModifyData) / sizeof(tagModifyData); i++)
		{
			_pstModifyHead->_stModifyData[i]._iModifyAddr       = 0;
			_pstModifyHead->_stModifyData[i]._cBytes            = 0;
			_pstModifyHead->_stModifyData[i]._iModifyValue      = 0;
		}
		_pstModifyHead->_cModifyStatus    = 0;
	}
	//==2, �Ѿ��޸ĳɹ�, ����û��copy���ڴ���, ��Ҫ���µ��ڴ���
	else if(_pstModifyHead->_cModifyStatus == 2)
	{
		for(size_t i = 0; i < _pstModifyHead->_iNowIndex; i++)
		{
			if(_pstModifyHead->_stModifyData[i]._cBytes == sizeof(size_t))
			{
				*(size_t*)((char*)_pHead + _pstModifyHead->_stModifyData[i]._iModifyAddr) = _pstModifyHead->_stModifyData[i]._iModifyValue;
			}
#if __WORDSIZE == 64
			else if(_pstModifyHead->_stModifyData[i]._cBytes == sizeof(uint32_t))
			{
				*(uint32_t*)((char*)_pHead + _pstModifyHead->_stModifyData[i]._iModifyAddr) = (uint32_t)_pstModifyHead->_stModifyData[i]._iModifyValue;
			}
#endif
			else if(_pstModifyHead->_stModifyData[i]._cBytes == sizeof(bool))
			{
				*(bool*)((char*)_pHead + _pstModifyHead->_stModifyData[i]._iModifyAddr) = (bool)_pstModifyHead->_stModifyData[i]._iModifyValue;
			}
			else
			{
				assert(true);
			}
		}
		_pstModifyHead->_iNowIndex        = 0;
		_pstModifyHead->_cModifyStatus    = 0;
	}
	//==0, ����״̬
	else if(_pstModifyHead->_cModifyStatus == 0)
	{
		return;
	}
}

void TC_HashMap::update(void* iModifyAddr, size_t iModifyValue)
{
	_pstModifyHead->_cModifyStatus = 1;
	_pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._iModifyAddr  = getRelative(iModifyAddr);
	_pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._iModifyValue = iModifyValue;
	_pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._cBytes       = sizeof(iModifyValue);
	_pstModifyHead->_iNowIndex++;

	assert(_pstModifyHead->_iNowIndex < sizeof(_pstModifyHead->_stModifyData) / sizeof(tagModifyData));
}

#if __WORDSIZE == 64
void TC_HashMap::update(void* iModifyAddr, uint32_t iModifyValue)
{
	_pstModifyHead->_cModifyStatus = 1;
	_pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._iModifyAddr  = getRelative(iModifyAddr);
	_pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._iModifyValue = iModifyValue;
	_pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._cBytes       = sizeof(iModifyValue);
	_pstModifyHead->_iNowIndex++;

	assert(_pstModifyHead->_iNowIndex < sizeof(_pstModifyHead->_stModifyData) / sizeof(tagModifyData));
}
#endif

void TC_HashMap::update(void* iModifyAddr, bool bModifyValue)
{
	_pstModifyHead->_cModifyStatus = 1;
	_pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._iModifyAddr  = getRelative(iModifyAddr);
	_pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._iModifyValue = bModifyValue;
	_pstModifyHead->_stModifyData[_pstModifyHead->_iNowIndex]._cBytes       = sizeof(bModifyValue);
	_pstModifyHead->_iNowIndex++;

	assert(_pstModifyHead->_iNowIndex < sizeof(_pstModifyHead->_stModifyData) / sizeof(tagModifyData));
}

size_t TC_HashMap::getMinPrimeNumber(size_t n)
{
    while(true)
    {
        if(TC_Common::isPrimeNumber(n))
        {
            return n;
        }
        ++n;
    }
    return 3;
}

}

