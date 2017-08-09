#include <errno.h>
#include <string.h>
#include "tc_sem_mutex.h"

namespace taf
{

TC_SemMutex::TC_SemMutex():_semID(-1),_semKey(-1)
{

}

TC_SemMutex::TC_SemMutex(key_t iKey)
{
    init(iKey);
}

void TC_SemMutex::init(key_t iKey)
{
    #if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
    /* union semun is defined by including <sys/sem.h> */
    #else
    /* according to X/OPEN we have to define it ourselves */
    union semun
    {
         int val;                  /* value for SETVAL */
         struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
         unsigned short *array;    /* array for GETALL, SETALL */
                                   /* Linux specific part: */
         struct seminfo *__buf;    /* buffer for IPC_INFO */
    };
    #endif

    int  iSemID;
    union semun arg;
    u_short array[2] = { 0, 0 };

    //�����ź�����, ���������ź���
    if ( (iSemID = semget( iKey, 2, IPC_CREAT | IPC_EXCL | 0666)) != -1 )
    {
        arg.array = &array[0];

        //�������ź�����ֵ����Ϊ0
        if ( semctl( iSemID, 0, SETALL, arg ) == -1 )
        {
            throw TC_SemMutex_Exception("[TC_SemMutex::init] semctl error:" + string(strerror(errno)));
        }
    }
    else
    {
        //�ź����Ѿ�����
        if ( errno != EEXIST )
        {
            throw TC_SemMutex_Exception("[TC_SemMutex::init] sem has exist error:" + string(strerror(errno)));
        }

        //�����ź���
        if ( (iSemID = semget( iKey, 2, 0666 )) == -1 )
        {
            throw TC_SemMutex_Exception("[TC_SemMutex::init] connect sem error:" + string(strerror(errno)));
        }
    }

    _semKey = iKey;
    _semID  = iSemID;
}

int TC_SemMutex::rlock() const
{
    //���빲����, �ڶ����ź�����ֵ��ʾ��ǰʹ���ź����Ľ��̸���
    //�ȴ���һ���ź�����Ϊ0(������û��ʹ��)
    //ռ�õڶ����ź���(�ڶ����ź���ֵ+1, ��ʾ��������ʹ��)
    struct sembuf sops[2] = { {0, 0, SEM_UNDO}, {1, 1, SEM_UNDO} };
    size_t nsops = 2;
    int ret = -1;

    do
    {
        ret=semop(_semID,&sops[0],nsops);

    } while ((ret == -1) &&(errno==EINTR));

    return ret;

    //return semop( _semID, &sops[0], nsops);
}

int TC_SemMutex::unrlock( ) const
{
    //���������, �н���ʹ�ù��ڶ����ź���
    //�ȵ��ڶ����ź�������ʹ��(�ڶ����ź�����ֵ>=1)
    struct sembuf sops[1] = { {1, -1, SEM_UNDO} };
    size_t nsops = 1;

    int ret = -1;

    do
    {
        ret=semop(_semID,&sops[0],nsops);

    } while ((ret == -1) &&(errno==EINTR));

    return ret;

    //return semop( _semID, &sops[0], nsops);
}

bool TC_SemMutex::tryrlock() const
{
    struct sembuf sops[2] = { {0, 0, SEM_UNDO|IPC_NOWAIT}, {1, 1, SEM_UNDO|IPC_NOWAIT}};
    size_t nsops = 2;

    int iRet = semop( _semID, &sops[0], nsops );
    if(iRet == -1)
    {
        if(errno == EAGAIN)
        {
            //�޷������
            return false;
        }
        else
        {
            throw TC_SemMutex_Exception("[TC_SemMutex::tryrlock] semop error : " + string(strerror(errno)));
        }
    }
    return true;
}

int TC_SemMutex::wlock() const
{
    //����������, ��һ���ź����͵ڶ����źŶ�û�б�ʹ�ù�(��, ��������û�б�ʹ��)
    //�ȴ���һ���ź�����Ϊ0
    //�ȴ��ڶ����ź�����Ϊ0
    //�ͷŵ�һ���ź���(��һ���ź���+1, ��ʾ��һ������ʹ�õ�һ���ź���)
    struct sembuf sops[3] = { {0, 0, SEM_UNDO}, {1, 0, SEM_UNDO}, {0, 1, SEM_UNDO} };
    size_t nsops = 3;

    int ret = -1;

    do
    {
        ret=semop(_semID,&sops[0],nsops);

    } while ((ret == -1) &&(errno==EINTR));

    return ret;
    //return semop( _semID, &sops[0], nsops);
}

int TC_SemMutex::unwlock() const
{
    //���������, �н���ʹ�ù���һ���ź���
    //�ȴ���һ���ź���(�ź���ֵ>=1)
    struct sembuf sops[1] = { {0, -1, SEM_UNDO} };
    size_t nsops = 1;

    int ret = -1;

    do
    {
        ret=semop(_semID,&sops[0],nsops);

    } while ((ret == -1) &&(errno==EINTR));

    return ret;

    //return semop( _semID, &sops[0], nsops);

}

bool TC_SemMutex::trywlock() const
{
    struct sembuf sops[3] = { {0, 0, SEM_UNDO|IPC_NOWAIT}, {1, 0, SEM_UNDO|IPC_NOWAIT}, {0, 1, SEM_UNDO|IPC_NOWAIT} };
    size_t nsops = 3;

    int iRet = semop( _semID, &sops[0], nsops );
    if(iRet == -1)
    {
        if(errno == EAGAIN)
        {
            //�޷������
            return false;
        }
        else
        {
            throw TC_SemMutex_Exception("[TC_SemMutex::trywlock] semop error : " + string(strerror(errno)));
        }
    }

    return true;
}

}

