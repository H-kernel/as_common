/******************************************************************************
   ��Ȩ���� (C), 2001-2011, M.Kernel

 ******************************************************************************
  �ļ���          : as_timer.cpp
  �汾��          : 1.0
  ����            :
  ��������        : 2007-4-02
  ����޸�        :
  ��������        :
  �����б�        :
  �޸���ʷ        :
  1 ����          : 2007-4-02
    ����          :
    �޸�����      : ����
*******************************************************************************/

#include <stdarg.h>
extern "C"{
#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
#include "as_time.h"
}
#include "as_timer.h"
#include "as_mem.h"

ITimerLog *g_pTimerLog = NULL;
#define MAX_TIMER_LOG_LENTH 512

#ifndef WIN32   //��PC-LINT
#define TIMER_SECOND_IN_MS 1000
#define TIMER_MS_IN_US 1000
#endif

#define _TIMER_FL_ "as_timer.cpp", __LINE__
/*******************************************************************************
  Function:       TIMER_WRITE_LOG()
  Description:    ��־��ӡ����
  Calls:
  Called By:
  Input:          ��printfһ��
  Output:         ��
  Return:         ��
*******************************************************************************/
void TIMER_WRITE_LOG(int32_t lLevel, const char *format, ...)
{
    if(NULL == g_pTimerLog)
    {
        return;
    };

    char buff[MAX_TIMER_LOG_LENTH + 1];
    buff[0] = '\0';

    va_list args;
    va_start (args, format);
    int32_t lPrefix = snprintf (buff, MAX_TIMER_LOG_LENTH, "errno:%d.thread(%u):",
        errno, (int32_t)as_thread_self());
    if(lPrefix < MAX_TIMER_LOG_LENTH)
    {
        (void)vsnprintf (buff + lPrefix,
            (uint32_t)(MAX_TIMER_LOG_LENTH - lPrefix), format, args);
    }
    buff[MAX_TIMER_LOG_LENTH] = '\0';
    g_pTimerLog->writeLog(TIMER_RUN_LOG, lLevel, buff, (int32_t)strlen(buff));
    va_end (args);

};
as_timer::as_timer()
{
    m_plistTrigger = NULL;
    m_ulTimerScale = DefaultTimerScale;
    m_ullRrsAbsTimeScales = 0;
    m_pMutexListOfTrigger = NULL;
    m_pASThread = NULL;
    m_bExit = AS_FALSE;
};

/*******************************************************************************
  Function:       as_timer::~as_timer()
  Description:    ��������
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
as_timer::~as_timer()
{
    try
    {
        if(NULL != m_plistTrigger)
        {
            ListOfTriggerIte itListOfTrigger = m_plistTrigger->begin();
            TIMER_WRITE_LOG(TIMER_DEBUG, "FILE(%s)LINE(%d): as_timer::~as_timer: thread = %u",
                _TIMER_FL_, as_thread_self());
            while(itListOfTrigger != m_plistTrigger->end())
            {
                AS_DELETE((*itListOfTrigger).second);
                ++itListOfTrigger;
            };
            m_plistTrigger->clear();
            AS_DELETE(m_plistTrigger);
            m_plistTrigger = NULL;
        }

        if(m_pASThread != NULL)
        {
            free(m_pASThread);
        }
        m_pASThread = NULL;

        if(m_pMutexListOfTrigger != NULL)
        {
            (void)as_destroy_mutex(m_pMutexListOfTrigger);
        }
        m_pMutexListOfTrigger = NULL;
    }
    catch (...)
    {
    }
};

/*******************************************************************************
  Function:       as_timer::init()
  Description:    ��ʼ������
  Calls:
  Called By:
  Input:          ulTimerScale: ��ʱ������
  Output:         ��
  Return:
  AS_SUCCESS: init success
  AS_FAIL: init fail
*******************************************************************************/
int32_t as_timer::init(ULONG ulTimerScale)
{
    if (ulTimerScale < MinTimerScale)
    {
        m_ulTimerScale = MinTimerScale;
    }
    else
    {
        m_ulTimerScale = ulTimerScale;
    }

    m_pMutexListOfTrigger = as_create_mutex();
    if(NULL == m_pMutexListOfTrigger)
    {
        TIMER_WRITE_LOG(TIMER_ERROR,
            "FILE(%s)LINE(%d): as_timer::init: create m_pMutexListOfTrigger fail.",
            _TIMER_FL_);
        return AS_FAIL;
    }

    m_ullRrsAbsTimeScales = as_get_ticks() / m_ulTimerScale;

    (void)AS_NEW( m_plistTrigger );
    if( NULL == m_plistTrigger )
    {
        TIMER_WRITE_LOG(TIMER_ERROR,
            "FILE(%s)LINE(%d): m_plistTrigger is NULL.",
            _TIMER_FL_);
        return AS_FAIL;
    }

    return AS_SUCCESS;
};

/*******************************************************************************
  Function:       as_timer::run()
  Description:    ������ʱ����߳�
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:
  AS_SUCCESS: init success
  AS_FAIL: init fail
*******************************************************************************/
int32_t as_timer::run()
{
    errno = 0;
    if (AS_ERROR_CODE_OK != as_create_thread((AS_THREAD_FUNC)invoke, (void *)this,
                                    &m_pASThread, AS_DEFAULT_STACK_SIZE))
    {
        TIMER_WRITE_LOG(TIMER_ERROR,
            "FILE(%s)LINE(%d): Create timer thread failed. error(%d):%s",
            _TIMER_FL_, errno, strerror(errno));
        return AS_FAIL;
    };
    TIMER_WRITE_LOG(TIMER_DEBUG,
        "FILE(%s)LINE(%d): AS_CreateThread: create timer thread(%d) OK.",
        _TIMER_FL_, m_pASThread->pthead);

    return AS_SUCCESS;
};

/*******************************************************************************
  Function:       as_timer::exit()
  Description:    �˳���ʱ����߳�
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
void as_timer::exit()
{
    if(NULL == m_pASThread)
    {
        TIMER_WRITE_LOG(TIMER_ERROR,
            "FILE(%s)LINE(%d): as_timer::exit: m_pASThread is null", _TIMER_FL_);
        return;
    }

    this->m_bExit = AS_TRUE;

    clearTimer();


    errno = 0;
    int32_t ret_val = as_join_thread(m_pASThread);
    if (ret_val != AS_ERROR_CODE_OK)
    {
        TIMER_WRITE_LOG(TIMER_ERROR,
            "FILE(%s)LINE(%d): Wait timer thread exit failed. ret_val(%d). error(%d):%s",
            _TIMER_FL_, ret_val, errno, strerror(errno));
    }

    TIMER_WRITE_LOG(TIMER_DEBUG,
        "FILE(%s)LINE(%d): as_timer::exit: exit complete. Thread = %d",
        _TIMER_FL_, m_pASThread->pthead);

    return;
};

/*******************************************************************************
  Function:       as_timer::registerTimer()
  Description:    ע�ᶨʱ��
  Calls:
  Called By:
  Input:          pTrigger: ��ʱ��������ʵ��, void *pArg: ��ʱ��������,
                  nScales: ��ʱ�¼�(��initʱָ����scaleΪ��λ)
                  enStyle: �������    enOneShot: ����һ��, enRepeated: ѭ������
  Output:         ��
  Return:
  AS_SUCCESS: init success
  AS_FAIL: init fail
*******************************************************************************/
int32_t as_timer::registerTimer(ITrigger *pTrigger, void *pArg, ULONG nScales,
    TriggerStyle enStyle)
{
    if (NULL == pTrigger )
    {
        TIMER_WRITE_LOG(TIMER_ERROR,
            "FILE(%s)LINE(%d): as_timer::registerTimer: pTrigger is NULL",
            _TIMER_FL_);
        return AS_FAIL;
    }

    if (0 == nScales )
    {
        TIMER_WRITE_LOG(TIMER_ERROR,
            "FILE(%s)LINE(%d): as_timer::registerTimer: nScales is zero",
            _TIMER_FL_);
        return AS_FAIL;
    }

    if(  AS_TRUE == m_bExit)
    {
        TIMER_WRITE_LOG(TIMER_ERROR,
                        "FILE(%s)LINE(%d): CTimer::registerTimer: "
                        "m_bExit is AS_TRUE, thread exit\n",
            _TIMER_FL_);
        return AS_FAIL;
    }

    CTimerItem *pTimerItem = NULL;
    (void)AS_NEW(pTimerItem);
    if (NULL == pTimerItem )
    {
        TIMER_WRITE_LOG(TIMER_ERROR,
            "FILE(%s)LINE(%d): as_timer::registerTimer: new pTimerItem fail",
            _TIMER_FL_);
        return AS_FAIL;
    }

    pTrigger->m_pTimerItem = pTimerItem;

    pTimerItem->m_pTrigger = pTrigger;
    pTimerItem->m_pArg = pArg;
    pTimerItem->m_ulInitialScales = nScales;
    pTimerItem->m_ullCurScales = m_ullRrsAbsTimeScales + nScales;
    pTimerItem->m_enStyle = enStyle;

    //����(�����mainloop����ͬһ�̲߳���Ҫ����)
    AS_BOOLEAN bNeedLock = AS_FALSE;
    AS_BOOLEAN bLocked = AS_FALSE;
    if (NULL == m_pASThread)
    {
        bNeedLock = AS_TRUE;
    }
    else
    {
        if(as_thread_self() != m_pASThread->pthead)
        {
            bNeedLock = AS_TRUE;
        }
    }

    if(AS_TRUE == bNeedLock)
    {
        if (AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfTrigger))
        {
            TIMER_WRITE_LOG(TIMER_ERROR,
                "FILE(%s)LINE(%d):as_timer::registerTimer: get lock failed",
                _TIMER_FL_);
        }
        else
        {
            bLocked = AS_TRUE;
        }
    }

    (void)(m_plistTrigger->insert(ListOfTriggerPair(pTimerItem->m_ullCurScales, pTimerItem)));

    //����
    if(AS_TRUE == bLocked)
    {
        if (AS_ERROR_CODE_OK != as_mutex_unlock(m_pMutexListOfTrigger))
        {
            TIMER_WRITE_LOG(TIMER_ERROR,
                "FILE(%s)LINE(%d): as_timer::registerTimer: release lock failed",
                _TIMER_FL_);
        }
    }

    return AS_SUCCESS;
};

/*******************************************************************************
  Function:       as_timer::clearTimer
  Description:    ɾ����ʱ���������е�TimerItem
  Calls:
  Called By:
  Input:         ��
  Output:         ��
  Return:
*******************************************************************************/
void as_timer::clearTimer( )
{
    CTimerItem *pTimerItem = NULL;

    if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfTrigger))
    {
        return;
    };
    ListOfTriggerIte itListOfTrigger = m_plistTrigger->begin();
    ListOfTriggerIte itCurrentTrigger = m_plistTrigger->begin();
    while(itListOfTrigger != m_plistTrigger->end())
    {
        pTimerItem = (*itListOfTrigger).second;
        itCurrentTrigger = itListOfTrigger;
        ++itListOfTrigger;
        (void)(m_plistTrigger->erase(itCurrentTrigger));

        TIMER_WRITE_LOG(TIMER_DEBUG,
            "FILE(%s)LINE(%d): clearTimer erase pTimerItem(0x%x) .\n",
                        _TIMER_FL_, pTimerItem);

        continue;
    }
    (void)as_mutex_unlock(m_pMutexListOfTrigger);

}

/*******************************************************************************
  Function:       as_timer::cancelTimer()
  Description:    ע����ʱ��
  Calls:
  Called By:
  Input:          pTrigger: ��ʱ��������ʵ��
  Output:         ��
  Return:
  AS_SUCCESS: init success
  AS_FAIL: init fail
*******************************************************************************/
int32_t as_timer::cancelTimer(ITrigger *pTrigger)
{
    if(NULL == pTrigger)
    {
        TIMER_WRITE_LOG(TIMER_ERROR,
            "FILE(%s)LINE(%d): as_timer::cancelTimer: pTrigger is NULL",
            _TIMER_FL_);
        return AS_FAIL;
    };

    //����(�����mainloop����ͬһ�̲߳���Ҫ����)
    AS_BOOLEAN bNeedLock = AS_FALSE;
    AS_BOOLEAN bLocked = AS_FALSE;
    if (NULL == m_pASThread)
    {
        bNeedLock = AS_TRUE;
    }
    else
    {
        if(as_thread_self() != m_pASThread->pthead)
        {
            bNeedLock = AS_TRUE;
        }
    }

    if(AS_TRUE == bNeedLock)
    {
        if (AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfTrigger))
        {
            TIMER_WRITE_LOG(TIMER_ERROR,
                "FILE(%s)LINE(%d): as_timer::cancelTimer: get lock failed",
                _TIMER_FL_);
        }
        else
        {
            bLocked = AS_TRUE;
        }
    }

    if(pTrigger->m_pTimerItem != NULL)
    {
        pTrigger->m_pTimerItem->m_bRemoved = AS_TRUE;
        pTrigger->m_pTimerItem->m_pTrigger = NULL;
        pTrigger->m_pTimerItem = NULL;
    }

    TIMER_WRITE_LOG(TIMER_DEBUG,
                "FILE(%s)LINE(%d): cancelTimer set m_bRemoved=AS_TRUE"
                "  pTimerItem(0x%x) pTrigger(0x%x) .\n",
                _TIMER_FL_, pTrigger->m_pTimerItem, pTrigger);


    //����(�������ͬһ�߳�)
    if(AS_TRUE == bLocked)
    {
        if (AS_ERROR_CODE_OK != as_mutex_unlock(m_pMutexListOfTrigger))
        {
            TIMER_WRITE_LOG(TIMER_ERROR,
                "FILE(%s)LINE(%d): as_timer::cancelTimer: release lock failed",
                _TIMER_FL_);
        }
    }

    return AS_SUCCESS;
};

/*******************************************************************************
  Function:       as_timer::mainLoop()
  Description:    �߳�ִ�еĶ�ʱ�����ѭ��
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
void as_timer::mainLoop()
{
    ULONGLONG ullCurrentScales = 0;
    while(AS_FALSE == m_bExit)
    {

#ifdef WIN32
        Sleep(m_ulTimerScale);
#else
        struct timeval tv;
        tv.tv_sec = (int32_t)(m_ulTimerScale/TIMER_SECOND_IN_MS);
        tv.tv_usec = (m_ulTimerScale%TIMER_SECOND_IN_MS)*TIMER_MS_IN_US;
        (void)select(0, AS_NULL, AS_NULL, AS_NULL, &tv);
#endif

        //����
        CTimerItem *pTimerItem = NULL;
        ITrigger *pTrigger = NULL;
        ++m_ullRrsAbsTimeScales ;//�ڲ�ʱ���׼����һ���̶�
        ullCurrentScales = m_ullRrsAbsTimeScales;

        if (AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfTrigger))
        {
            break;
        };
        ListOfTriggerIte itListOfTrigger = m_plistTrigger->begin();
        ListOfTriggerIte itCurrentTrigger = m_plistTrigger->begin();
        while(itListOfTrigger != m_plistTrigger->end())
        {
            pTimerItem = (*itListOfTrigger).second;
            if(NULL == pTimerItem)
            {
                TIMER_WRITE_LOG(TIMER_ERROR,
                    "FILE(%s)LINE(%d): pTimerItem is NULL.", _TIMER_FL_);
                itCurrentTrigger = itListOfTrigger;
                ++itListOfTrigger;
                (void)(m_plistTrigger->erase(itCurrentTrigger));
                continue;
            }

            pTrigger = (ITrigger *)pTimerItem->m_pTrigger;

            //�����ʱ����ע����ɾ����ʱ��������������һ��
            if((NULL == pTrigger) || (AS_TRUE == pTimerItem->m_bRemoved))
            {
                TIMER_WRITE_LOG(TIMER_DEBUG,
                    "FILE(%s)LINE(%d): Timer(0x%x) removed.", _TIMER_FL_, pTimerItem);
                itCurrentTrigger = itListOfTrigger;
                ++itListOfTrigger;
                (void)(m_plistTrigger->erase(itCurrentTrigger));
                AS_DELETE(pTimerItem);
                continue;
            };

            if(ullCurrentScales < pTimerItem->m_ullCurScales)
            {
                break;
            }

            itCurrentTrigger = itListOfTrigger;
            ++itListOfTrigger;
            (void)(m_plistTrigger->erase(itCurrentTrigger));

            //����trigger��onTrigger����
            pTrigger->onTrigger(pTimerItem->m_pArg,
                ullCurrentScales, pTimerItem->m_enStyle);

            //���ֻ����һ�Σ�ɾ����ʱ��
            if(enOneShot == pTimerItem->m_enStyle)
            {
                TIMER_WRITE_LOG(TIMER_DEBUG,
                    "FILE(%s)LINE(%d): Timer(0x%x) remove trigger once timer.",
                    _TIMER_FL_, pTimerItem);
                //��ֹpTrigger->onTrigger��ɾ����ʱ��
                pTrigger->m_pTimerItem = NULL;
                AS_DELETE(pTimerItem);
                continue;
            }

            //�����ظ�������ʱ������Ҫ�޸��´γ�ʱ�¼������¼���multimap
            pTimerItem->m_ullCurScales = ullCurrentScales
                + pTimerItem->m_ulInitialScales;
            (void)(m_plistTrigger->insert(ListOfTriggerPair(pTimerItem->m_ullCurScales,
                pTimerItem)));
        };
        (void)as_mutex_unlock(m_pMutexListOfTrigger);
    }

    return;
}


