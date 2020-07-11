
/******************************************************************************
   ��Ȩ���� (C), 2008-2011, M.Kernel

 ******************************************************************************
  �ļ���          : ASLog.cpp
  �汾��          : 1.0
  ����            :
  ��������        : 2008-08-17
  ����޸�        :
  ��������        : AS��־ģ��
  �����б�        :
  �޸���ʷ        :
  1 ����          :
    ����          :
    �޸�����      :
*******************************************************************************/


#include <stdio.h>
#include "as_ring_cache.h"
#include "as_log.h"
#include <time.h>

#ifdef WIN32
#include "atlstr.h"
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



//extern HANDLE g_hDLLModule;                     //��̬���ӿ���

/************************** Begin ��־����ʵ�� ********************************/

#define ASLOG_ERROR_OK             0       //�ɹ�
#define ASLOG_ERROR_INIT_CACHE     (-1)    //��ʼ������������
#define ASLOG_ERROR_FILE_NAME      (-2)    //�Զ�������־�ļ�������
#define ASLOG_ERROR_OPEN_FILE      (-3)    //���ļ�����
#define ASLOG_ERROR_CREATE_EVENT   (-4)    //�����¼�����
#define ASLOG_ERROR_CREATE_THREAD  (-5)    //�����̳߳���

//Ĭ�ϻ�������С1M
#define DEFAULT_CACHE_SIZE          (1024*1024)
//Ĭ���ļ��л����ȣ���λByte
#define DEFAULT_CHANGE_FILE_LEN     (10*1024*1024)
//Ĭ������ļ��л����ȣ���λByte
#define MAX_CHANGE_FILE_LEN         (100*1024*1024)
//Ĭ����С�ļ��л����ȣ���λByte
#define MIN_CHANGE_FILE_LEN         (100*1024)
//��־�ļ�·��������
#define MAX_LOG_FILE_PATH_NAME_LEN  1024
//������־��󳤶�
#define MAX_LEN_OF_SINGLE_LOG       2048


//�ȴ��˳��¼�����ʱ��
#define LOG_WAIT_FOR_EXIT_EVENT     5000
//�ȴ��������
#define LOG_WAIT_FOR_EXIT_EVENT_INTERVAL 50
//�ȴ�д���
#define LOG_WAIT_FOR_WRITE_OVER_INTERVAL 10


class as_log
{
    private:    //��ʵ��
        as_log();
    public:
        virtual ~as_log();

    public:
        static as_log* GetASLogInstance();    //��ȡ��־����
        static void DeleteASLogInstance();        //ɾ����־����

    public:
        //������־
        int32_t Start();
        //������־����
        void SetLevel(int32_t lLevel);
        //���õ�ǰд����־�ļ�·����
        bool SetLogFilePathName(const char* szPathName);
        //������־�ļ��������ƣ������˳���ʱ�������ļ���������λKB
        void SetFileLengthLimit(uint32_t ulLimitLengthKB);
        //д��־
        int32_t Write(const char* szFile, int32_t lLine,
            int32_t lLevel, const char* format, va_list argp);
        //ֹͣ��־
        int32_t Stop();

    private:
        //������д�ļ��߳�
#if AS_APP_OS == AS_OS_WIN32
        //��־д�߳����
        static uint32_t __stdcall ThreadEntry(VOID* lpvoid);
#else
        static VOID* ThreadEntry(VOID* lpvoid);
#endif
        void WriteLogFileThread();

        const char* GetLevelString(int32_t lLevel) const;

    private:
        //��־ģ���Ƿ�����
        bool    m_bRun;

        //�Ƿ�����д��־
        bool    m_bAllowWrite;

        //�Ƿ������������־ֹͣ
        bool    m_bDiskFull;
        //����⵽���̿ռ����1Mʱ���ָ���־��ӡ
        #define MIN_DISK_SPACE_FOR_LOG      (1024*1024) //���̿ռ����1Mʱ�ָ���־
        #define DISK_SPACE_CHECK_INTERVAL   10000       //���̿ռ�����10����
        //�ϴμ���������ʱ��(���ڴ�����������־ֹͣʱ������)
        uint32_t   m_dwLastCheckDiskSpaceTime;

        //��־����Ĭ��ΪINFO����
        int32_t    m_lLevel;

        //д�ļ��̵߳ľ��
        as_thread_t* m_hWriteThread;

        //д��־�¼��ľ��
        as_event_t*  m_hWriteEvent;

        //д�߳��˳��¼��ľ��
        as_event_t*    m_hThreadExitEvent;

        //��־����
        as_ring_cache    m_Cache;

        //��־�ļ�
        FILE*    m_pLogFile;

        //��־�ļ��������ƣ���λByte
        uint32_t    m_ulFileLenLimit;

        //��־�ļ�·��������
        char    m_szLogFilePathName[MAX_LOG_FILE_PATH_NAME_LEN];

    private:
        //��ʵ����־����
        static as_log*    g_pASLog;
};

as_log* as_log::g_pASLog = NULL;

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
        //ֹͣ��־ģ��
        (void)Stop();

        //������Ϊ��PC-LINT
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

//��ȡ��־ģ���Ψһʵ��
as_log* as_log::GetASLogInstance()
{
    //����־ʵ��δ�������������
    if(NULL == g_pASLog)
    {
        g_pASLog = new as_log;
    }

    return g_pASLog;
}

//ɾ����־ģ���ʵ��
void as_log::DeleteASLogInstance()
{
    //���Ѿ�����������ͷ�
    if(NULL == g_pASLog)
    {
        //����Ҫ��ֹͣ����
        (void)(g_pASLog->Stop());

        //ɾ������
        delete g_pASLog;
        g_pASLog = NULL;
    }
}

//������־ģ��
int32_t as_log::Start()
{
    //����Ѿ�������ֱ�ӷ���
    if(m_bRun)
    {
        m_bAllowWrite = true;
        return ASLOG_ERROR_OK;
    }

    //��ʼ��������Ϊ2M
    uint32_t ulCacheSize = m_Cache.SetCacheSize(DEFAULT_CACHE_SIZE);
    if(DEFAULT_CACHE_SIZE != ulCacheSize)
    {
        //����������ʧ��
        return ASLOG_ERROR_INIT_CACHE;
    }

    //����ļ���
    if(::strlen(m_szLogFilePathName) == 0)
    {
#if AS_APP_OS == AS_OS_WIN32
        //�����м��ģ�����õ�·��
        (void)::GetModuleFileName(NULL, CA2W(m_szLogFilePathName), MAX_LOG_FILE_PATH_NAME_LEN - 1);
        //    (void)::GetModuleFileName((HMODULE)g_hDLLModule, m_szLogFilePathName, MAX_LOG_FILE_PATH_NAME_LEN-1);
        char* pszFind = ::strrchr(m_szLogFilePathName, '.');
        if (NULL == pszFind)
        {
            //�쳣
            return ASLOG_ERROR_FILE_NAME;
        }
        //���Ӻ�׺
        (void)::sprintf(pszFind, ".log");
#elif AS_APP_OS == AS_OS_LINUX
        (void)::sprintf(m_szLogFilePathName, "AS_Module.log");
#endif
    }
    //���ļ�
    m_pLogFile = ::fopen(m_szLogFilePathName, "a+");
    if(NULL == m_pLogFile)
    {
        //�ļ��޷���
        return ASLOG_ERROR_OPEN_FILE;
    }

    //��ʼ��д�¼�
    m_hWriteEvent = as_create_event();
    if (NULL == m_hWriteEvent)
    {
        //д�¼�����ʧ��

        //�ر��ļ�
        (void)::fclose(m_pLogFile);
        m_pLogFile = NULL;
        return ASLOG_ERROR_CREATE_EVENT;
    }

    //��ʼ��д�߳��˳��¼�
    m_hThreadExitEvent = as_create_event();
    if (NULL == m_hThreadExitEvent)
    {
        //д�߳��˳��¼�����ʧ��

        //�ر��ļ�
        (void)::fclose(m_pLogFile);
        m_pLogFile = NULL;

        //�ر�д�¼����
        as_destroy_event(m_hWriteEvent);
        m_hWriteEvent = NULL;
        return ASLOG_ERROR_CREATE_EVENT;
    }

    //������־������־
    m_bRun = true;

    //����������д�ļ��߳�
    int32_t lResult = as_create_thread(ThreadEntry, this, &m_hWriteThread, AS_DEFAULT_STACK_SIZE);
    if((AS_ERROR_CODE_OK != lResult)
        ||(NULL == m_hWriteThread))
    {
        //д�̴߳���ʧ�ܣ�������Դ
        m_bRun = false;

        //�ر��ļ�
        (void)::fclose(m_pLogFile);
        m_pLogFile = NULL;

        //�ر�д�¼����
        as_destroy_event(m_hWriteEvent);
        m_hWriteEvent = NULL;

        //�ر�д�߳��˳��¼����
        as_destroy_event(m_hThreadExitEvent);
        m_hThreadExitEvent = NULL;
        return ASLOG_ERROR_CREATE_THREAD;
    }

    //��ʼ������־
    m_bAllowWrite = true;

    return ASLOG_ERROR_OK;
}
//������־����
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

//���õ�ǰд����־�ļ�·����
bool as_log::SetLogFilePathName(const char* szPathName)
{
    bool bSetOk = false;
    //�������
    if((NULL!=szPathName) && ::strlen(szPathName)<MAX_LOG_FILE_PATH_NAME_LEN)
    {
        (void)::sprintf(m_szLogFilePathName, "%s", szPathName);
        //�ļ�Ŀ¼�������
        bSetOk = true;
    }

    return bSetOk;
}

//������־�ļ��������ƣ������˳���ʱ�������ļ���������λKB
void as_log::SetFileLengthLimit(uint32_t ulLimitLengthKB)
{
    //KBתΪByte
    uint32_t ulNewLimitLength = ulLimitLengthKB * 1024;

    //��Χȷ��
    if(ulNewLimitLength < MIN_CHANGE_FILE_LEN)
    {
        //С����Сֵʹ����Сֵ
        m_ulFileLenLimit = MIN_CHANGE_FILE_LEN;
    }
    else if(ulNewLimitLength < MAX_CHANGE_FILE_LEN)
    {
        //������Χ
        m_ulFileLenLimit = ulNewLimitLength;
    }
    else
    {
        //�������ֵʹ�����ֵ
        m_ulFileLenLimit = MAX_CHANGE_FILE_LEN;
    }
}

//дһ����־��������������:
//2008-08-17 10:45:45.939[DEBUG|Log.cpp:152|PID:772|TID:2342|Err:0]��������...
int32_t as_log::Write(const char* szFile, int32_t lLine,
                int32_t lLevel, const char* format, va_list argp)
{
    if(!m_bRun)
    {
        //δ����
        return 0;
    }

    //δ����д
    if(!m_bAllowWrite)
    {
        //���̿ռ�ﵽ�ָ���׼���ָ���־���룬�����������־
        m_dwLastCheckDiskSpaceTime = 0;
        m_bDiskFull = false;
        m_bAllowWrite = true;

        //����ļ�δ�򿪣���Ҫ���´���־�ļ���д������־��Ϣ
        if(NULL == m_pLogFile)
        {
            m_pLogFile = ::fopen(m_szLogFilePathName, "a+");
            if(NULL == m_pLogFile)
            {
                //��Ȼ�����⣬�ָ������ǣ��´��ٳ��Դ�
                m_bDiskFull = true;
                m_bAllowWrite = false;
                return 0;
            }
            //֪ͨд�߳�д��־
            as_set_event(m_hWriteEvent);
        }
    }

    //��־��������
    if(lLevel > m_lLevel)
    {
        return 0;
    }

//������Ϣ׼��
    //��־ʱ��
    time_t ulTick = time(NULL);
    //��־����
    const char* pszLevel = GetLevelString(lLevel);
    //�ļ���
    const char* pszFileName = ::strrchr(szFile, '\\');
    if(NULL != pszFileName)
    {
        //Խ��б��
        pszFileName += 1;
    }
    else
    {
        pszFileName = szFile;
    }

    //�߳�ID
    uint32_t ulThreadID = as_get_threadid();
    //��ǰ������
#if AS_APP_OS == AS_OS_LINUX
    uint32_t ulErr = 0;
#elif AS_APP_OS == AS_OS_WIN32
    uint32_t ulErr = GetLastError();
#endif
//������Ϣ׼�����

    //����ʹ�ó�Ա���������̻߳��������
    char szLogTmp [MAX_LEN_OF_SINGLE_LOG] = {0};
    char szLogTime[MAX_LEN_OF_SINGLE_LOG] = { 0 };
    as_strftime(szLogTime, MAX_LEN_OF_SINGLE_LOG, "%Y-%m-%d %H:%M:%S", ulTick);
    //������������־��Ϣ�м��������Ϣ
    (void)::sprintf(szLogTmp,"%s[%s|%20s:%05d|TID:0x%04X|Err:0x%04X] ",
        szLogTime, pszLevel, pszFileName, lLine, ulThreadID, ulErr);

    //���û���Ϣ���ڶ�����Ϣ��д����־��
    uint32_t ulLen = ::strlen(szLogTmp);
#if AS_APP_OS == AS_OS_LINUX
    (void)::vsnprintf(szLogTmp + ulLen, (MAX_LEN_OF_SINGLE_LOG - ulLen) - 1, format, argp);
#elif AS_APP_OS == AS_OS_WIN32
    (void)::_vsnprintf(szLogTmp + ulLen, (MAX_LEN_OF_SINGLE_LOG - ulLen) - 1, format, argp);
#endif
    szLogTmp[MAX_LEN_OF_SINGLE_LOG-1] = '\0';
    ulLen = ::strlen(szLogTmp);
    if((ulLen+2) < MAX_LEN_OF_SINGLE_LOG)
    {//�Զ�����һ������
        szLogTmp[ulLen] = '\n';
        szLogTmp[ulLen+1] = '\0';
    }

    //����־д��������
    uint32_t ulWriteLen = m_Cache.Write(szLogTmp, ::strlen(szLogTmp));
    while(0 == ulWriteLen)
    {
        //֪ͨд�߳�д��־
        as_set_event(m_hWriteEvent);
        //�ȴ�����д�뻺��
        as_sleep(LOG_WAIT_FOR_WRITE_OVER_INTERVAL);
        ulWriteLen = m_Cache.Write(szLogTmp, ::strlen(szLogTmp));
    }

    //֪ͨд�߳�д��־
    as_set_event(m_hWriteEvent);
    return 0;
}

//ֹͣ��־����ֹд�߳�
int32_t as_log::Stop()
{
    //������д��־
    m_bAllowWrite = false;
    m_bDiskFull = false;
    m_dwLastCheckDiskSpaceTime = 0;

    //���Ѿ�ֹͣ����δ������ֱ���˳�
    if(!m_bRun)
    {
        return ASLOG_ERROR_OK;
    }

    //�Ȼ��������ݶ�д���ļ���(�ȴ�5��)
    int32_t lWaitTime = LOG_WAIT_FOR_EXIT_EVENT;
    while(lWaitTime >= 0)
    {
        if(0 == m_Cache.GetDataSize())
        {
            //��������־�Ѿ���д���ļ�����
            break;
        }

        //����д�¼�
        as_set_event(m_hWriteEvent);

        as_sleep(LOG_WAIT_FOR_WRITE_OVER_INTERVAL);
        lWaitTime -= LOG_WAIT_FOR_WRITE_OVER_INTERVAL;
    }

    //������־��־Ϊֹͣ������д�¼������߳��Լ��˳�
    //�ȴ�5�룬����δ�˳�����ǿ����ֹ
    m_bRun = false;
    lWaitTime = LOG_WAIT_FOR_EXIT_EVENT;
    while(lWaitTime >= 0)
    {
        //����д�¼������߳��Լ��˳�
        as_set_event(m_hWriteEvent);

        if (AS_ERROR_CODE_TIMEOUT != as_wait_event(m_hThreadExitEvent, LOG_WAIT_FOR_EXIT_EVENT_INTERVAL))
        {
            //�߳̽���
            //begin delete for AQ1D01618 by xuxin
            //m_hThreadExitEvent = NULL; //������CloseHandle���������ﲻ���ÿ�
            //end delete for AQ1D01618 by xuxin
            break;
        }

        lWaitTime -= LOG_WAIT_FOR_EXIT_EVENT_INTERVAL;
    }

    if(NULL != m_hWriteThread)
    {
        //ǿ����ֹд�߳�
        (void)::as_thread_exit(m_hWriteThread);
        m_hWriteThread = NULL;
    }

    //����д�¼�
    if(NULL != m_hWriteEvent)
    {
        (void)as_destroy_event(m_hWriteEvent);
        m_hWriteEvent = NULL;
    }

    //����д�߳��˳��¼�
    if(NULL != m_hThreadExitEvent)
    {
        as_destroy_event(m_hThreadExitEvent);
        m_hThreadExitEvent = NULL;
    }

    //�ر��ļ�
    if(NULL != m_pLogFile)
    {
        (void)::fclose(m_pLogFile);
        m_pLogFile = NULL;
    }

    //��ջ���
    m_Cache.Clear();

    return ASLOG_ERROR_OK;
}

#if AS_APP_OS == AS_OS_WIN32
//��־д�߳����
uint32_t __stdcall as_log::ThreadEntry(VOID* lpvoid)
#else
VOID* as_log::ThreadEntry(VOID* lpvoid)
#endif
{
    if(NULL != lpvoid)
    {
        //����д�̺߳�����
        as_log* pASLog = (as_log *)lpvoid;
        pASLog->WriteLogFileThread();
    }

    return 0;
}

//д��־�߳�
void as_log::WriteLogFileThread()
{
    //�߳��б���׼��
    char szNewFileName[MAX_LOG_FILE_PATH_NAME_LEN] = {0};
    uint32_t ulLogDataLen = 0;
    uint32_t ulCurFileLen = 0;
    char* pLogInfo = NULL;
    try
    {
        //��������ݿռ�
        pLogInfo = new char[m_Cache.GetCacheSize()];
    }
    catch(...)
    {
    }
    if(NULL == pLogInfo)
    {
        //�ͷŲ�����ֵ
        as_thread_exit(m_hWriteThread);
        m_hWriteThread = NULL;

        //�߳��˳��¼�֪ͨ
        as_set_event(m_hThreadExitEvent);

        return ;
    }

    //����������ѭ��
    while(m_bRun)
    {
        //�ȴ�д�ļ��¼�
        as_wait_event(m_hWriteEvent, 0);

        //�������ļ�δ��
        if(m_bDiskFull || NULL == m_pLogFile)
        {
            //�����Ǵ��������ȴ�
            if(m_bDiskFull)
            {
                continue;
            }
            else
            {   //������Ϊ��������ɵģ��˳�
                break;
            }
        }

        //������
        ulLogDataLen = m_Cache.GetCacheSize();
        ulLogDataLen= m_Cache.Read(pLogInfo, ulLogDataLen);
        if(0 == ulLogDataLen)
        {
            //����Ϊ��δ��������
            continue;
        }

        //д���ݵ��ļ�
        if(1 != fwrite(pLogInfo, ulLogDataLen, 1, m_pLogFile))
        {
#if AS_APP_OS == AS_OS_WIN32
            //д���ݵ��ļ��г����쳣�����Ǵ���������ͣ��־���ȴ��ռ�
            if(ERROR_DISK_FULL == ::GetLastError())
            {
                m_bAllowWrite = false;
                m_bDiskFull = true;
                continue;
            }
            else
            {//�����������˳�д��־
                break;
            }
#elif AS_APP_OS == AS_OS_LINUX
            break;
#endif
        }
        //���ļ�����������д��Ӳ��
        if(fflush(m_pLogFile) != 0)
        {
#if AS_APP_OS == AS_OS_WIN32
            //д���ݵ�Ӳ���г����쳣�����Ǵ���������ͣ��־���ȴ��ռ�
            if(ERROR_DISK_FULL == ::GetLastError())
            {
                m_bAllowWrite = false;
                m_bDiskFull = true;
                continue;
            }
            else
            {//�����������˳�д��־
                break;
            }
#elif AS_APP_OS == AS_OS_LINUX
            break;
#endif
        }

        //����ļ������Ƿ񳬹�����
        ulCurFileLen = (uint32_t)::ftell(m_pLogFile);
        if(ulCurFileLen < m_ulFileLenLimit)
        {
            continue;
        }

        if(::strlen(m_szLogFilePathName) >= (MAX_LOG_FILE_PATH_NAME_LEN-20))
        {
            //�ļ���̫�����޷��޸��ļ���Ϊ�����ļ���
            continue;
        }

        //�رյ�ǰ��־�ļ�
        (void)::fclose(m_pLogFile);
        m_pLogFile = NULL;

        ULONG ulTick = as_get_ticks();

        //���ɱ����ļ���
        (void)::sprintf(szNewFileName, "%s.%4d",m_szLogFilePathName, ulTick);

        //�޸ĵ�ǰ��־�ļ�Ϊ�����ļ����޸��ļ���ʧ��Ҳû�й�ϵ�������ٴ������������ļ���д
        (void)::rename(m_szLogFilePathName, szNewFileName);

        //���´���־�ļ�
        m_pLogFile = ::fopen(m_szLogFilePathName, "a+");
        if(NULL != m_pLogFile)
        {
            //�ļ�������
            continue;
        }
#if AS_APP_OS == AS_OS_WIN32
        //����־�ļ���ʧ��
        if(ERROR_DISK_FULL != ::GetLastError())
        {
            //�쳣��ֹͣ��־
            break;
        }
#endif
        //���Ǵ���������ͣ��־���ȴ��ռ�
        m_bAllowWrite = false;
        m_bDiskFull = true;
    }

    //�ָ�״̬
    m_bRun = false;
    m_bAllowWrite = false;
    m_bDiskFull = false;
    m_dwLastCheckDiskSpaceTime = 0;

    //�ͷŲ�����ֵ
    as_thread_exit(m_hWriteThread);
    m_hWriteThread = NULL;

    //�رյ�ǰ��־�ļ�
    if(NULL != m_pLogFile)
    {
        (void)::fclose(m_pLogFile);
        m_pLogFile = NULL;
    }

    //�ͷ���ʱ�ռ�
   try
    {
        delete[] pLogInfo;
    }
    catch(...)
    {
    }
    pLogInfo = NULL;

    //�߳��˳��¼�֪ͨ
    as_set_event(m_hThreadExitEvent);
}

//��ȡ��־�����ַ���
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
/*************************** End ��־����ʵ�� *********************************/


/********************** Begin ��־ģ���û��ӿ�ʵ�� ****************************/
//����AS��־ģ��
ASLOG_API void ASStartLog(void)
{
    //��ȡ��־ʵ��
    as_log* pASLog = as_log::GetASLogInstance();
    //��ȡ��־ʵ��
    (void)(pASLog->Start());
    va_list arg;
    va_end(arg);
    //д������Ϣ
    (void)(pASLog->Write(__FILE__, __LINE__,
        AS_LOG_INFO, "AS Log Module Start!", arg));
}

//дһ����־
ASLOG_API void __ASWriteLog(const char* szFileName, int32_t lLine,
                      int32_t lLevel, const char* format, va_list argp)
{
    //��ȡ��־ʵ��
    as_log* pASLog = as_log::GetASLogInstance();
    //дһ����־
    (void)(pASLog->Write(szFileName, lLine, lLevel, format, argp));
}

//ֹͣAS��־ģ��
ASLOG_API void ASStopLog(void)
{
    //��ȡ��־ʵ��
    as_log* pASLog = as_log::GetASLogInstance();

    va_list arg;
    va_end(arg);
    //дֹͣ��Ϣ
    (void)(pASLog->Write(__FILE__, __LINE__,
        AS_LOG_INFO, "AS Log Module Stop!\n\n\n\n", arg));
    //ֹͣ��־
    (void)(pASLog->Stop());
    //ɾ����־����
    as_log::DeleteASLogInstance();
}

//������־����
ASLOG_API void ASSetLogLevel(int32_t lLevel)
{
    //��ȡ��־ʵ��
    as_log* pASLog = as_log::GetASLogInstance();
    //������־����
    pASLog->SetLevel(lLevel);
}

//���õ�ǰд����־�ļ�·����(����·�������·��)
ASLOG_API bool ASSetLogFilePathName(const char* szPathName)
{
    //��ȡ��־ʵ��
    as_log* pASLog = as_log::GetASLogInstance();
    //�����ļ���
    bool bSetOk = pASLog->SetLogFilePathName(szPathName);
    return bSetOk;
}

//������־�ļ��������ƣ������˳���ʱ�������ļ�����λKB(100K��100M֮��,Ĭ����10M)
ASLOG_API void ASSetLogFileLengthLimit(uint32_t ulLimitLengthKB)
{
    //��ȡ��־ʵ��
    as_log* pASLog = as_log::GetASLogInstance();
    //�����ļ���������
    pASLog->SetFileLengthLimit(ulLimitLengthKB);
}
/************************ End ��־ģ���û��ӿ�ʵ�� ****************************/

