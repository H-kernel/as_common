
/******************************************************************************
   ��Ȩ���� (C), 2008-2011, M.Kernel

 ******************************************************************************
  �ļ���          : as_lock_guard.cpp
  �汾��          : 1.0
  ����            :
  ��������        : 2008-8-17
  ����޸�        :
  ��������        : ʵ������������
  �����б�        :
  �޸���ʷ        :
  1 ����          :
    ����          :
    �޸�����      :
*******************************************************************************/



#include "as_lock_guard.h"

as_lock_guard::as_lock_guard(as_mutex_t *pMutex)
{
    m_pMutex = NULL;

    if(NULL == pMutex)
    {
        return;
    }

    m_pMutex = pMutex;

    (void)as_mutex_lock(m_pMutex);
}

as_lock_guard::~as_lock_guard()
{
    if(NULL == m_pMutex)
    {
        return;
    }
    (void)as_mutex_unlock(m_pMutex);

    m_pMutex = NULL;
}

/*******************************************************************************
Function:       // as_lock_guard::lock
Description:    // ����
Calls:          //
Data Accessed:  //
Data Updated:   //
Input:          // as_mutex_t *pMutex
Output:         // ��
Return:         // ��
Others:         // ��
*******************************************************************************/
void as_lock_guard::lock(as_mutex_t *pMutex)
{
    if(NULL == pMutex)
    {
        return;
    }
    (void)as_mutex_lock(pMutex);
}

/*******************************************************************************
Function:       // as_lock_guard::unlock
Description:    // �ͷ���
Calls:          //
Data Accessed:  //
Data Updated:   //
Input:          // AS_Mutex *pMutex
Output:         // ��
Return:         // ��
Others:         // ��
*******************************************************************************/
void as_lock_guard::unlock(as_mutex_t *pMutex)
{
    if(NULL == pMutex)
    {
        return;
    }
    (void)as_mutex_unlock(pMutex);
}


