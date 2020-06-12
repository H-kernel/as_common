#include "as_thread_mamage.h"
#include "as_lock_guard.h"
#include "as_log.h"

as_task_manage::as_task_manage()
{
    m_ulMaxThreadCheckInterval = DEFAULT_THREAD_CHECK_INTERVAL;
    m_ulRestartServer          = 0;
    m_ulCoreDump               = 0;
    m_nThreadFlag              = false;
    m_FreeIndexListMutex       = NULL;
    m_tvLastCheckTime          = time(NULL);
    m_RegisteredThreadNum      = 0;
}

as_task_manage::~as_task_manage()
{
    try
    {
        m_FreeIndexList.clear();
    }
    catch(...)
    {
    }

    if(NULL != m_FreeIndexListMutex) {
        as_destroy_mutex(m_FreeIndexListMutex);
        m_FreeIndexListMutex = NULL;
    }
}

int32_t as_task_manage::Init(uint32_t ulMaxCheckInterval,
                             uint32_t ulRestartServer,
                             uint32_t ulCoreDump)
{
    m_tvLastCheckTime          = time(NULL);
    m_nThreadFlag              = false;
    m_ulMaxThreadCheckInterval = ulMaxCheckInterval;
    m_ulRestartServer          = ulRestartServer;
    m_ulCoreDump               = ulCoreDump;

    if ((MIN_INTERVAL > m_ulMaxThreadCheckInterval)
          || (MAX_INTERVAL < m_ulMaxThreadCheckInterval))
    {
        m_ulMaxThreadCheckInterval = DEFAULT_INTERVAL;
    }

    m_FreeIndexListMutex = as_create_mutex();
    if(NULL == m_FreeIndexListMutex) {
        return AS_ERROR_CODE_SYS;
    }


    ThreadInfo *  pReporter = NULL;
    as_lock_guard locker(m_FreeIndexListMutex);
    for (int32_t i = 0; i < MAX_THREAD_NUM; i++)
    {
        pReporter = m_ThreadArray + i;
        pReporter->m_ulThreadID      = 0;
        pReporter->m_nThreadIndex    = 0;
        pReporter->m_ulProcessNum    = 0;
        memset(pReporter->m_szThreadName, 0x0, MAX_THREAD_NAME + 1);
        pReporter->m_pReporter = NULL;
        m_FreeIndexList.push_back(i);
    }

    int32_t nRet = open(NULL);
    if (0 != nRet)
    {
        AS_LOG(AS_LOG_ERROR, "[deamon thread]Init SVS_Daemon_Thread fail.");
        return nRet;
    }

    AS_LOG(AS_LOG_INFO, "[deamon thread]Init SVS_Daemon_Thread success.");
    return AS_ERROR_CODE_OK;
}


void as_task_manage::Destroy()
{
    as_lock_guard locker(m_FreeIndexListMutex);
    m_FreeIndexList.clear();

    (void) close(0);
    AS_LOG(AS_LOG_INFO, "[deamon thread]Destroy SVS_Daemon_Thread success.");
}


int32_t as_task_manage::RegistThread(as_thread_reporter* pReporter,const char* pszThreadName)
{
    if((NULL == pReporter) || (NULL == pszThreadName))
    {
        AS_LOG(AS_LOG_ERROR,
            "Thread Regist to Daemon failed.The parameter is invalid."
            "pReporter[0x%08x] pszThreadName[0x%08x]",
            pReporter,
            pszThreadName);

        return -1;
    }
    int32_t nThreadIndex = 0;
    {
        as_lock_guard locker(m_FreeIndexListMutex);
        if (m_FreeIndexList.empty())
        {
            return -1;
        }

        ++m_RegisteredThreadNum;

        nThreadIndex = m_FreeIndexList.front();
        m_FreeIndexList.pop_front();
    }

    m_ThreadArray[nThreadIndex].m_ulThreadID   = as_get_threadid();
    m_ThreadArray[nThreadIndex].m_nThreadIndex = nThreadIndex;
    m_ThreadArray[nThreadIndex].m_ulProcessNum = 0;
    (void)strncpy(m_ThreadArray[nThreadIndex].m_szThreadName,
                  pszThreadName,
                  MAX_THREAD_NAME);
    m_ThreadArray[nThreadIndex].m_tvStartTime  = time(NULL);
    m_ThreadArray[nThreadIndex].m_tvAliveTime  = m_ThreadArray[nThreadIndex].m_tvStartTime;
    m_ThreadArray[nThreadIndex].m_pReporter    = pReporter;

    AS_LOG(AS_LOG_INFO, "[deamon thread]regist thread[%s:%u], index[%d],pReporter[0x%08x]",
              m_ThreadArray[nThreadIndex].m_szThreadName,
              m_ThreadArray[nThreadIndex].m_ulThreadID,
              m_ThreadArray[nThreadIndex].m_nThreadIndex,
              pReporter);
    return nThreadIndex;
}


int32_t as_task_manage::UnregistThread(int32_t nThreadIndex)
{
    if ((0 > nThreadIndex)
         || (MAX_THREAD_NUM <= nThreadIndex))
    {
        return -1;
    }


    int32_t nIndex = m_ThreadArray[nThreadIndex].m_nThreadIndex;
    if (nIndex != nThreadIndex)
    {
        return -1;
    }

    AS_LOG(AS_LOG_INFO, "[deamon thread]unregist thread[%s:%u], index[%d]",
              m_ThreadArray[nThreadIndex].m_szThreadName,
              m_ThreadArray[nThreadIndex].m_ulThreadID,
              m_ThreadArray[nThreadIndex].m_nThreadIndex);


    m_ThreadArray[nThreadIndex].m_ulThreadID   = 0;
    m_ThreadArray[nThreadIndex].m_nThreadIndex = 0;
    m_ThreadArray[nThreadIndex].m_ulProcessNum = 0;
    memset(m_ThreadArray[nThreadIndex].m_szThreadName, 0x0, MAX_THREAD_NAME + 1);
    m_ThreadArray[nThreadIndex].m_tvStartTime  = time(NULL);
    m_ThreadArray[nThreadIndex].m_tvAliveTime  = time(NULL);
    m_ThreadArray[nThreadIndex].m_pReporter = NULL;


    as_lock_guard locker(m_FreeIndexListMutex);
    m_FreeIndexList.push_back(nThreadIndex);

    --m_RegisteredThreadNum;
    return 0;
}


int32_t as_task_manage::open(void *args)
{
    if (NULL == args)
    {
       ; // only for compile warning
    }


    size_t stack_size = 128 * 1024;
    int32_t nRet = activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,
                        1,
                        0,
                        ACE_DEFAULT_THREAD_PRIORITY,
                        -1,
                        0,
                        0,
                        0,
                        &stack_size,
                        0 );
    if (0 != nRet)
    {
        AS_LOG(AS_LOG_ERROR, "[deamon thread]open deamon thread fail.");
    }

    return nRet;
}


int32_t as_task_manage::svc(void)
{
    ACE_thread_t threadID = ACE_OS::thr_self();
    AS_LOG(AS_LOG_INFO, "[deamon thread]deamon thread[%u] run.", threadID);

    ACE_Time_Value delay(1);
    ACE_Time_Value curtime;
    ThreadInfo *pThreadInfo = NULL;
    m_tvLastCheckTime         = ACE_OS::gettimeofday();

    while (!m_nThreadFlag)
    {
        ACE_Time_Value maxinterval((int32_t)m_ulMaxThreadCheckInterval);
        for (int32_t i = 0; i < DEFAULT_THREAD_CHECK_INTERVAL; i++)
        {
             (void)ACE_OS::sleep(delay);

             if (m_nThreadFlag)
             {
                 return 0;
             }
        }

        curtime = ACE_OS::gettimeofday();
        if ((curtime < m_tvLastCheckTime)
            || (curtime - m_tvLastCheckTime > maxinterval))
        {
            AS_LOG(AS_LOG_WARNING, "[deamon thread]systime is abnormal.");
            m_tvLastCheckTime = curtime;
            continue;
        }

        AS_LOG(AS_LOG_INFO, "[deamon thread]deamon thread begin");
        ACE_Time_Value tvLastReportInterval(DEFAULT_THREAD_CHECK_INTERVAL);

        for (int32_t nIndex = 0; nIndex < MAX_THREAD_NUM; nIndex++)
        {
            pThreadInfo = &m_ThreadArray[nIndex];

            if ((0 == pThreadInfo->m_ulThreadID) || (NULL == pThreadInfo->m_pReporter))
            {
                continue;
            }

            ACE_Time_Value lastReportTime = pThreadInfo->m_pReporter->getLastReportTime();
            AS_LOG(AS_LOG_INFO,
                "[deamon thread]Check thread active status."
                "thread[%s:%u], index[%d],last report at last check period, thread last report[%d].",
                pThreadInfo->m_szThreadName,
                pThreadInfo->m_ulThreadID,
                pThreadInfo->m_nThreadIndex,
                pThreadInfo->m_tvAliveTime.sec(),
                lastReportTime.sec());

            pThreadInfo->m_tvAliveTime = lastReportTime;
            pThreadInfo->m_ulProcessNum = pThreadInfo->m_pReporter->getProcessNum();
            if (curtime - pThreadInfo->m_tvAliveTime > tvLastReportInterval)
            {
                AS_LOG(AS_LOG_WARNING, "[deamon thread]thread[%s:%u], index[%d] has"
                               " not update stat 60s, last report[%d].",
                               pThreadInfo->m_szThreadName,
                               pThreadInfo->m_ulThreadID,
                               pThreadInfo->m_nThreadIndex,
                               pThreadInfo->m_tvAliveTime.sec());
            }

            if (curtime - pThreadInfo->m_tvAliveTime > maxinterval)
            {
                if (ACE_Time_Value::zero != pThreadInfo->m_tvAliveTime)
                {
                    AS_LOG(AS_LOG_CRITICAL, "[deamon thread]thread[%s:%u], index[%d] has"
                               " not update stat too long time, last report[%d],"
                               "restart flag[%d] dump flag[%d].",
                               pThreadInfo->m_szThreadName,
                               pThreadInfo->m_ulThreadID,
                               pThreadInfo->m_nThreadIndex,
                               pThreadInfo->m_tvAliveTime.sec(),
                               m_ulRestartServer,
                               m_ulCoreDump);

                    (void)system("dmesg -c");
                    (void)system("echo \"1\" >/proc/sys/kernel/sysrq");
                    (void)system("echo \"m\" >/proc/sysrq-trigger");
                    (void)system("dmesg");

                    if (1 <= m_ulRestartServer)
                    {
                        if (1 <= m_ulCoreDump)
                        {
                            (void)kill(getpid(), SIGABRT);
                        }
                        else
                        {
                            (void)kill(getpid(), SIGKILL);
                        }
                    }
                }
            }
        }

        AS_LOG(AS_LOG_INFO, "[deamon thread]deamon thread end");
        m_tvLastCheckTime = curtime;
    }

    AS_LOG(AS_LOG_INFO, "[deamon thread]deamon thread[%u] exit.", threadID);
    return 0;
}


int32_t as_task_manage::close(u_long flags)
{
    m_nThreadFlag = true;
    (void)wait();
    AS_LOG(AS_LOG_INFO, "[deamon thread]close deamon thread, flags[%d]", flags);
    return 0;
}

as_thread_reporter::as_thread_reporter()
{
    m_ulProcessNum       = 0;
    m_nThreadIndex       = -1;
    m_tvLastReportTime   = ACE_Time_Value::zero;
}


as_thread_reporter::as_thread_reporter(const char *pszThreadName)
{
    m_ulProcessNum       = 0;
    m_tvLastReportTime   = ACE_Time_Value::zero;

    m_nThreadIndex       = as_task_manage::instance()->RegistThread(this,pszThreadName);
    AS_LOG(AS_LOG_INFO, "as_thread_reporter::as_thread_reporter(), index[%d]",
                  m_nThreadIndex);
}

as_thread_reporter::~as_thread_reporter()
{
    try
    {
        (void)as_task_manage::instance()->UnregistThread(m_nThreadIndex);
        AS_LOG(AS_LOG_INFO, "as_thread_reporter::~as_thread_reporter(), index[%d]",
                  m_nThreadIndex);
    }
    catch(...)
    {
    }
}


void as_thread_reporter::ReportStat(uint32_t ulProcessNum)
{
    ACE_Time_Value curtime = ACE_OS::gettimeofday();
    m_tvLastReportTime = curtime;
    m_ulProcessNum += ulProcessNum ;
}


ACE_Time_Value as_thread_reporter::getLastReportTime()const
{
    return m_tvLastReportTime;
}


uint32_t as_thread_reporter::getProcessNum()const
{
    return m_ulProcessNum;
}
