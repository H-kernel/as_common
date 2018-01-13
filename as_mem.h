#ifndef SVS_COMMON_H_INCLUDE
#define SVS_COMMON_H_INCLUDE

#include <stdlib.h>

template<class T>
T* AS_NEW(T* &m, unsigned long ulMuili = 0)
{
    try
    {
        if (ulMuili == 0)
        {
            m = new T;
        }
        else
        {
            m = new T[ulMuili];
        }
        return m;
    }
    catch(...)
    {
        m = NULL;
        return NULL;
    }
};

enum DELETE_MULTI
{
    SINGLE = 0,
    MULTI = 1
};

template<class T>
void AS_DELETE(T* &m, unsigned long ulMuili = 0)
{
    if(NULL == m)
    {
        return;
    }

    try
    {
        if (0 == ulMuili)
        {
            delete m;
        }
        else
        {
            delete[] m;
        }
    }
    catch(...)
    {
    }

    m = NULL;
};

//��ȫ��̬���䵥ʵ���ڴ棬���ɵ������࣬�����ص��Ǹ���ָ��
//���ͽ����ڼ̳���
template<class TBASE, class TREAL>
TBASE* AS_NEW_REAL(TBASE* &m)
{
    try
    {
        m = new TREAL;
    }
    catch(...)
    {
        m = NULL;
    }

    return m;
};

//��ȫ�ͷŵ�ʵ���ڴ棬�����ǻ������ָ�룬��ʵ��ɾ�������������
//���ͽ����ڼ̳���
//������������������麯�������ֱ��ʹ��AS_DELETE
template<class TBASE, class TREAL>
void AS_DELETE_REAL(TBASE* &m)
{
    try
    {
        TREAL* p = (TREAL*)m; /*lint !e1774*///ģ��,��ȫת��
        delete p;
    }
    catch(...)
    {
        m = NULL;
    }

    m = NULL;
};

//����ָ�밲ȫת��Ϊ��ʵָ��
template<class T>
T* AS_CAST(void* pVoid)
{
    T* pReal = NULL;

    try
    {
        pReal = dynamic_cast<T*>((T*)pVoid);
    }
    catch(...)
    {
        pReal = NULL;
    }

    return pReal;
};

#endif //SVS_COMMON_H_INCLUDE
