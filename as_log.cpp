#include <stdio.h>
#include "as_ring_cache.h"
#include "as_log.h"
#include <time.h>

#ifdef WIN32
#include "atlstr.h"
#include <windows.h>
#endif
extern "C"{
#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
#include "as_mutex.h"
#include "as_event.h"
#include "as_thread.h"
#include "as_time.h"
#include "as_json.h"
}


#define ASLOG_ERROR_OK             0    
#define ASLOG_ERROR_INIT_CACHE     (-1)    
#define ASLOG_ERROR_FILE_NAME      (-2)    
#define ASLOG_ERROR_OPEN_FILE      (-3)    
#define ASLOG_ERROR_CREATE_EVENT   (-4)    
#define ASLOG_ERROR_CREATE_THREAD  (-5)   

#define DEFAULT_CACHE_SIZE          (1024*1024)

#define DEFAULT_CHANGE_FILE_LEN     (10*1024*1024)

#define MAX_CHANGE_FILE_LEN         (100*1024*1024)

#define MIN_CHANGE_FILE_LEN         (100*1024)

#define MAX_LOG_FILE_PATH_NAME_LEN  1024

#define MAX_LEN_OF_SINGLE_LOG       2048


#define LOG_WAIT_FOR_EXIT_EVENT     5000

#define LOG_WAIT_FOR_EXIT_EVENT_INTERVAL 50

#define LOG_WAIT_FOR_WRITE_OVER_INTERVAL 10


class as_log
{
private: 
    as_log();
public:
    static as_log& Instance()
    {
        static as_log objAsLog;
        return objAsLog;
    };
    virtual ~as_log();
public:
    int32_t Start();
    void SetLevel(int32_t lLevel);
    int32_t GetLevel();
    bool SetLogFilePathName(const char* szPathName);
    void SetFileLengthLimit(uint32_t ulLimitLengthKB);
    int32_t Write(const char* szFile, int32_t lLine,int32_t lLevel, const char* format, va_list argp);
    int32_t Write(int32_t lLevel,const char* szlog);
    int32_t Stop();
private:
#if AS_APP_OS == AS_OS_WIN32
    static uint32_t __stdcall ThreadEntry(VOID* lpvoid);
#else
    static VOID* ThreadEntry(VOID* lpvoid);
#endif
    void WriteLogFileThread();

    const char* GetLevelString(int32_t lLevel) const;

private:
    bool             m_bRun;

    bool             m_bAllowWrite;

    bool             m_bDiskFull;
#define MIN_DISK_SPACE_FOR_LOG      (1024*1024) 
#define DISK_SPACE_CHECK_INTERVAL   10000 
    uint32_t         m_dwLastCheckDiskSpaceTime;

    int32_t          m_lLevel;

    as_thread_t*     m_hWriteThread;

    as_event_t*      m_hWriteEvent;

    as_event_t*      m_hThreadExitEvent;

    as_ring_cache    m_Cache;

    FILE*            m_pLogFile;

    uint32_t         m_ulFileLenLimit;

    char             m_szLogFilePathName[MAX_LOG_FILE_PATH_NAME_LEN];
};

as_log::as_log()
{
    m_bRun = false;
    m_bAllowWrite = false;
    m_bDiskFull = false;
    m_dwLastCheckDiskSpaceTime = 0;
    m_lLevel = AS_LOG_INFO;
    m_hWriteThread = NULL;
    m_hWriteEvent = NULL;
    m_hThreadExitEvent = NULL;

    m_pLogFile = NULL;
    m_ulFileLenLimit = DEFAULT_CHANGE_FILE_LEN;
    ::memset(m_szLogFilePathName,0,MAX_LOG_FILE_PATH_NAME_LEN);
}

as_log::~as_log()
{
    try
    {
        (void)Stop();

        if(NULL != m_hWriteThread)
        {
            m_hWriteThread = NULL;
        }
        if(NULL != m_hWriteEvent)
        {
            as_destroy_event(m_hWriteEvent);
            m_hWriteEvent = NULL;
        }
        if(NULL != m_hThreadExitEvent)
        {
            as_destroy_event(m_hThreadExitEvent);
            m_hThreadExitEvent = NULL;
        }
        if(NULL != m_pLogFile)
        {
            (void)::fclose(m_pLogFile);
            m_pLogFile = NULL;
        }

    }
    catch(...)
    {
    }
}


int32_t as_log::Start()
{
    if(m_bRun)
    {
        m_bAllowWrite = true;
        return ASLOG_ERROR_OK;
    }

    uint32_t ulCacheSize = m_Cache.SetCacheSize(DEFAULT_CACHE_SIZE);
    if(DEFAULT_CACHE_SIZE != ulCacheSize)
    {
        return ASLOG_ERROR_INIT_CACHE;
    }

    if(::strlen(m_szLogFilePathName) == 0)
    {
#if AS_APP_OS == AS_OS_WIN32
        (void)::GetModuleFileName(NULL, CA2W(m_szLogFilePathName), MAX_LOG_FILE_PATH_NAME_LEN - 1);
        char* pszFind = ::strrchr(m_szLogFilePathName, '.');
        if (NULL == pszFind)
        {
            return ASLOG_ERROR_FILE_NAME;
        }
        (void)::sprintf(pszFind, ".log");
#elif AS_APP_OS == AS_OS_LINUX
        (void)::sprintf(m_szLogFilePathName, "AS_Module.log");
#endif
    }

    m_pLogFile = ::fopen(m_szLogFilePathName, "a+");
    if(NULL == m_pLogFile)
    {
        return ASLOG_ERROR_OPEN_FILE;
    }

    m_hWriteEvent = as_create_event();
    if (NULL == m_hWriteEvent)
    {
        (void)::fclose(m_pLogFile);
        m_pLogFile = NULL;
        return ASLOG_ERROR_CREATE_EVENT;
    }

    m_hThreadExitEvent = as_create_event();
    if (NULL == m_hThreadExitEvent)
    {
        (void)::fclose(m_pLogFile);
        m_pLogFile = NULL;

        as_destroy_event(m_hWriteEvent);
        m_hWriteEvent = NULL;
        return ASLOG_ERROR_CREATE_EVENT;
    }

    m_bRun = true;

    int32_t lResult = as_create_thread(ThreadEntry, this, &m_hWriteThread, AS_DEFAULT_STACK_SIZE);
    if((AS_ERROR_CODE_OK != lResult)
        ||(NULL == m_hWriteThread))
    {
        m_bRun = false;

        (void)::fclose(m_pLogFile);
        m_pLogFile = NULL;


        as_destroy_event(m_hWriteEvent);
        m_hWriteEvent = NULL;

        as_destroy_event(m_hThreadExitEvent);
        m_hThreadExitEvent = NULL;
        return ASLOG_ERROR_CREATE_THREAD;
    }

    m_bAllowWrite = true;

    return ASLOG_ERROR_OK;
}
void as_log::SetLevel(int32_t lLevel)
{
    switch(lLevel)
    {
        case AS_LOG_EMERGENCY:
        case AS_LOG_ALERT:
        case AS_LOG_CRITICAL:
        case AS_LOG_ERROR:
        case AS_LOG_WARNING:
        case AS_LOG_NOTICE:
        case AS_LOG_INFO:
        case AS_LOG_DEBUG:
            m_lLevel = lLevel;
            break;
        default:
            break;
    }
}
int32_t as_log::GetLevel()
{
    return m_lLevel;
}

bool as_log::SetLogFilePathName(const char* szPathName)
{
    if(m_bRun)
    {
        return false;
    }
    bool bSetOk = false;
    if((NULL!=szPathName) && ::strlen(szPathName)<MAX_LOG_FILE_PATH_NAME_LEN)
    {
        (void)::sprintf(m_szLogFilePathName, "%s", szPathName);
        bSetOk = true;
    }

    return bSetOk;
}
void as_log::SetFileLengthLimit(uint32_t ulLimitLengthKB)
{
    uint32_t ulNewLimitLength = ulLimitLengthKB * 1024;

    if(ulNewLimitLength < MIN_CHANGE_FILE_LEN)
    {
        m_ulFileLenLimit = MIN_CHANGE_FILE_LEN;
    }
    else if(ulNewLimitLength < MAX_CHANGE_FILE_LEN)
    {
        m_ulFileLenLimit = ulNewLimitLength;
    }
    else
    {
        m_ulFileLenLimit = MAX_CHANGE_FILE_LEN;
    }
}

int32_t as_log::Write(const char* szFile, int32_t lLine,
                int32_t lLevel, const char* format, va_list argp)
{
    if(!m_bRun)
    {
        return AS_ERROR_CODE_OK;
    }

    if(!m_bAllowWrite)
    {
        m_dwLastCheckDiskSpaceTime = 0;
        m_bDiskFull = false;
        m_bAllowWrite = true;

        if(NULL == m_pLogFile)
        {
            m_pLogFile = ::fopen(m_szLogFilePathName, "a+");
            if(NULL == m_pLogFile)
            {
                m_bDiskFull = true;
                m_bAllowWrite = false;
                return AS_ERROR_CODE_OK;
            }
            as_set_event(m_hWriteEvent);
        }
    }

    if(lLevel > m_lLevel)
    {
        return AS_ERROR_CODE_OK;
    }


    time_t ulTick = time(NULL);

    const char* pszLevel = GetLevelString(lLevel);

    const char* pszFileName = ::strrchr(szFile, '\\');
    if(NULL != pszFileName)
    {
        pszFileName += 1;
    }
    else
    {
        pszFileName = szFile;
    }

    uint32_t ulThreadID = as_get_threadid();
#if AS_APP_OS == AS_OS_LINUX
    uint32_t ulErr = 0;
#elif AS_APP_OS == AS_OS_WIN32
    uint32_t ulErr = GetLastError();
#endif

    char szLogTmp [MAX_LEN_OF_SINGLE_LOG] = {0};
    char szLogTime[MAX_LEN_OF_SINGLE_LOG] = { 0 };
    as_strftime(szLogTime, MAX_LEN_OF_SINGLE_LOG, "%Y-%m-%d %H:%M:%S", ulTick);
    (void)::sprintf(szLogTmp,"%s[%s|%20s:%05d|TID:0x%04X|Err:0x%04X] ",
        szLogTime, pszLevel, pszFileName, lLine, ulThreadID, ulErr);

    uint32_t ulLen = ::strlen(szLogTmp);
#if AS_APP_OS == AS_OS_LINUX
    (void)::vsnprintf(szLogTmp + ulLen, (MAX_LEN_OF_SINGLE_LOG - ulLen) - 1, format, argp);
#elif AS_APP_OS == AS_OS_WIN32
    (void)::_vsnprintf(szLogTmp + ulLen, (MAX_LEN_OF_SINGLE_LOG - ulLen) - 1, format, argp);
#endif
    szLogTmp[MAX_LEN_OF_SINGLE_LOG-1] = '\0';
    ulLen = ::strlen(szLogTmp);
    if((ulLen+2) < MAX_LEN_OF_SINGLE_LOG)
    {
        szLogTmp[ulLen] = '\n';
        szLogTmp[ulLen+1] = '\0';
    }

    uint32_t ulWriteLen = m_Cache.Write(szLogTmp, ::strlen(szLogTmp));
    while(0 == ulWriteLen)
    {
        as_set_event(m_hWriteEvent);
        as_sleep(LOG_WAIT_FOR_WRITE_OVER_INTERVAL);
        ulWriteLen = m_Cache.Write(szLogTmp, ::strlen(szLogTmp));
    }

    as_set_event(m_hWriteEvent);
    return AS_ERROR_CODE_OK;
}

int32_t as_log::Write(int32_t lLevel,const char* szlog)
{
    if(!m_bRun)
    {
        return AS_ERROR_CODE_OK;
    }

    if(!m_bAllowWrite)
    {
        m_dwLastCheckDiskSpaceTime = 0;
        m_bDiskFull = false;
        m_bAllowWrite = true;

        if(NULL == m_pLogFile)
        {
            m_pLogFile = ::fopen(m_szLogFilePathName, "a+");
            if(NULL == m_pLogFile)
            {
                m_bDiskFull = true;
                m_bAllowWrite = false;
                return AS_ERROR_CODE_OK;
            }
            as_set_event(m_hWriteEvent);
        }
    }

    if(lLevel > m_lLevel)
    {
        return AS_ERROR_CODE_OK;
    }

    uint32_t ulLen = ::strlen(szlog);

    uint32_t ulWriteLen = m_Cache.Write(szlog, ulLen);
    while(0 == ulWriteLen)
    {
        as_set_event(m_hWriteEvent);
        as_sleep(LOG_WAIT_FOR_WRITE_OVER_INTERVAL);
        ulWriteLen = m_Cache.Write(szlog, ulLen);
    }

    as_set_event(m_hWriteEvent);
    return AS_ERROR_CODE_OK;
}

int32_t as_log::Stop()
{
    m_bAllowWrite = false;
    m_bDiskFull   = false;
    m_dwLastCheckDiskSpaceTime = 0;

    if(!m_bRun)
    {
        return ASLOG_ERROR_OK;
    }

    int32_t lWaitTime = LOG_WAIT_FOR_EXIT_EVENT;
    while(lWaitTime >= 0)
    {
        if(0 == m_Cache.GetDataSize())
        {
            break;
        }

        as_set_event(m_hWriteEvent);

        as_sleep(LOG_WAIT_FOR_WRITE_OVER_INTERVAL);
        lWaitTime -= LOG_WAIT_FOR_WRITE_OVER_INTERVAL;
    }

    m_bRun = false;
    lWaitTime = LOG_WAIT_FOR_EXIT_EVENT;
    while(lWaitTime >= 0)
    {
        as_set_event(m_hWriteEvent);

        if (AS_ERROR_CODE_TIMEOUT != as_wait_event(m_hThreadExitEvent, LOG_WAIT_FOR_EXIT_EVENT_INTERVAL))
        {
            break;
        }

        lWaitTime -= LOG_WAIT_FOR_EXIT_EVENT_INTERVAL;
    }

    if(NULL != m_hWriteThread)
    {
        (void)::as_thread_exit(m_hWriteThread);
        m_hWriteThread = NULL;
    }

    if(NULL != m_hWriteEvent)
    {
        (void)as_destroy_event(m_hWriteEvent);
        m_hWriteEvent = NULL;
    }

    if(NULL != m_hThreadExitEvent)
    {
        as_destroy_event(m_hThreadExitEvent);
        m_hThreadExitEvent = NULL;
    }

    if(NULL != m_pLogFile)
    {
        (void)::fclose(m_pLogFile);
        m_pLogFile = NULL;
    }

    m_Cache.Clear();

    return ASLOG_ERROR_OK;
}

#if AS_APP_OS == AS_OS_WIN32
uint32_t __stdcall as_log::ThreadEntry(VOID* lpvoid)
#else
VOID* as_log::ThreadEntry(VOID* lpvoid)
#endif
{
    if(NULL != lpvoid)
    {
        as_log* pASLog = (as_log *)lpvoid;
        pASLog->WriteLogFileThread();
    }
#if AS_APP_OS == AS_OS_WIN32
    return AS_ERROR_CODE_OK;
#else
    return NULL;
#endif
}

void as_log::WriteLogFileThread()
{
    char szNewFileName[MAX_LOG_FILE_PATH_NAME_LEN] = {0};
    uint32_t ulLogDataLen = 0;
    uint32_t ulCurFileLen = 0;
    char* pLogInfo = NULL;
    try
    {
        pLogInfo = new char[m_Cache.GetCacheSize()];
    }
    catch(...)
    {
    }
    if(NULL == pLogInfo)
    {
        as_thread_exit(m_hWriteThread);
        m_hWriteThread = NULL;

        as_set_event(m_hThreadExitEvent);

        return ;
    }

    while(m_bRun)
    {
        as_wait_event(m_hWriteEvent, 0);

        if(m_bDiskFull || NULL == m_pLogFile)
        {
            if(m_bDiskFull)
            {
                continue;
            }
            else
            {
                break;
            }
        }

        ulLogDataLen = m_Cache.GetCacheSize();
        ulLogDataLen= m_Cache.Read(pLogInfo, ulLogDataLen);
        if(0 == ulLogDataLen)
        {
            continue;
        }

        if(1 != fwrite(pLogInfo, ulLogDataLen, 1, m_pLogFile))
        {
#if AS_APP_OS == AS_OS_WIN32
            if(ERROR_DISK_FULL == ::GetLastError())
            {
                m_bAllowWrite = false;
                m_bDiskFull = true;
                continue;
            }
            else
            {
                break;
            }
#elif AS_APP_OS == AS_OS_LINUX
            break;
#endif
        }

        if(fflush(m_pLogFile) != 0)
        {
#if AS_APP_OS == AS_OS_WIN32
            if(ERROR_DISK_FULL == ::GetLastError())
            {
                m_bAllowWrite = false;
                m_bDiskFull = true;
                continue;
            }
            else
            {
                break;
            }
#elif AS_APP_OS == AS_OS_LINUX
            break;
#endif
        }

        ulCurFileLen = (uint32_t)::ftell(m_pLogFile);
        if(ulCurFileLen < m_ulFileLenLimit)
        {
            continue;
        }

        if(::strlen(m_szLogFilePathName) >= (MAX_LOG_FILE_PATH_NAME_LEN-20))
        {
            continue;
        }

        (void)::fclose(m_pLogFile);
        m_pLogFile = NULL;

        ULONG ulTick = as_get_ticks();

        (void)::sprintf(szNewFileName, "%s.%4d",m_szLogFilePathName, ulTick);

        (void)::rename(m_szLogFilePathName, szNewFileName);

        m_pLogFile = ::fopen(m_szLogFilePathName, "a+");
        if(NULL != m_pLogFile)
        {
            continue;
        }
#if AS_APP_OS == AS_OS_WIN32
        if(ERROR_DISK_FULL != ::GetLastError())
        {
            break;
        }
#endif
        m_bAllowWrite = false;
        m_bDiskFull = true;
    }


    m_bRun = false;
    m_bAllowWrite = false;
    m_bDiskFull = false;
    m_dwLastCheckDiskSpaceTime = 0;


    as_thread_exit(m_hWriteThread);
    m_hWriteThread = NULL;


    if(NULL != m_pLogFile)
    {
        (void)::fclose(m_pLogFile);
        m_pLogFile = NULL;
    }

   try
    {
        delete[] pLogInfo;
    }
    catch(...)
    {
    }
    pLogInfo = NULL;


    as_set_event(m_hThreadExitEvent);
}


const char* as_log::GetLevelString(int32_t lLevel) const
{
    const char* pstrLevel = NULL;
    switch(lLevel)
    {
        case AS_LOG_EMERGENCY:
            pstrLevel = "EMERGENCY";
            break;
        case AS_LOG_ALERT:
            pstrLevel = "ALERT";
            break;
        case AS_LOG_CRITICAL:
            pstrLevel = "CRITICAL";
            break;
        case AS_LOG_ERROR:
            pstrLevel = "ERROR";
            break;
        case AS_LOG_WARNING:
            pstrLevel = "WARNING";
            break;
        case AS_LOG_NOTICE:
            pstrLevel = "NOTICE";
            break;
        case AS_LOG_INFO:
            pstrLevel = "INFO";
            break;
        case AS_LOG_DEBUG:
            pstrLevel = "DEBUG";
            break;
        default:
            pstrLevel = "NOLEVEL";
            break;
    }

    return pstrLevel;
}

ASLOG_API void ASStartLog(void)
{
    (void)(as_log::Instance().Start());
    va_list arg;
    va_end(arg);

    (void)(as_log::Instance().Write(__FILE__, __LINE__,
        AS_LOG_INFO, "AS Log Module Start!", arg));
}


ASLOG_API void ASWrite(const char* szFileName, int32_t lLine,
                      int32_t lLevel, const char* format, va_list argp)
{

    (void)(as_log::Instance().Write(szFileName, lLine, lLevel, format, argp));
}

ASLOG_API void ASWriteLog(int32_t lLevel,const char* szLog)
{
    (void)(as_log::Instance().Write(lLevel, szLog));
}

ASLOG_API void ASStopLog(void)
{

    va_list arg;
    va_end(arg);

    (void)(as_log::Instance().Write(__FILE__, __LINE__,
        AS_LOG_INFO, "AS Log Module Stop!\n\n\n\n", arg));

    (void)(as_log::Instance().Stop());
}

ASLOG_API void ASSetLogLevel(int32_t lLevel)
{
    as_log::Instance().SetLevel(lLevel);
}

ASLOG_API int32_t ASGetLogLevel()
{
    return as_log::Instance().GetLevel();
}

ASLOG_API bool ASSetLogFilePathName(const char* szPathName)
{
    bool bSetOk = as_log::Instance().SetLogFilePathName(szPathName);
    return bSetOk;
}

ASLOG_API void ASSetLogFileLengthLimit(uint32_t ulLimitLengthKB)
{
    as_log::Instance().SetFileLengthLimit(ulLimitLengthKB);
}


