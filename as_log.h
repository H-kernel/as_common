
/******************************************************************************
   ��Ȩ���� (C), 2008-2011, M.Kernel

 ******************************************************************************
  �ļ���          : ASLog.h
  �汾��          : 1.0
  ����            :
  ��������        : 2008-8-17
  ����޸�        :
  ��������        : AS��־ģ���û��ӿ�
  �����б�        :
  �޸���ʷ        :
  1 ����          :
    ����          :
    �޸�����      :
*******************************************************************************/


#ifndef _AS_LOG_H_
#define _AS_LOG_H_

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>




#define ASLOG_API

//��־�ȼ�
typedef enum _ASLogLevel
{
    AS_LOG_EMERGENCY   = 0,    //ϵͳ������
    AS_LOG_ALERT       = 1,    //�������̲�ȡ�ж�������ϵͳ������
    AS_LOG_CRITICAL    = 2,    //���ش���
    AS_LOG_ERROR       = 3,    //һ�����
    AS_LOG_WARNING     = 4,    //����
    AS_LOG_NOTICE      = 5,    //��Ҫ��������Ϣ
    AS_LOG_INFO        = 6,    //һ���������Ϣ
    AS_LOG_DEBUG       = 7,    //������Ϣ
}ASLogLevel;

//������־����Ĭ����:LOG_INFO
ASLOG_API void ASSetLogLevel(long lLevel);

//���õ�ǰд����־�ļ�·����(����·�������·��)
//Ĭ���ǵ�ǰ·����:exename.log
ASLOG_API bool ASSetLogFilePathName(const char* szPathName);

//������־�ļ��������ƣ������˳���ʱ�������ļ�����λKB(100K-100M,Ĭ����10M)
ASLOG_API void ASSetLogFileLengthLimit(unsigned long ulLimitLengthKB);

//��������������ɺ󣬿�������AS��־ģ��
ASLOG_API void ASStartLog(void);

//дһ����־(�����涨���ASWriteLog����д��־)
ASLOG_API void __ASWriteLog(const char* szFileName, long lLine,
                             long lLevel, const char* format, va_list argp);
//ֹͣAS��־ģ��
ASLOG_API void ASStopLog(void);

//vc6�Լ�vc7.1����֧��C99(��g++֧��)
//�������ﲻ��ʹ�ÿɱ�����궨�壬���ö�()������������ʵ��
class CWriter
{
    public:
        CWriter(const char* file, long line)
        {
            m_file_ = file;
            m_line_ = line;
        }
        void operator()(long level, const char* format, ...)
        {
            va_list argp;
            va_start(argp, format);
            __ASWriteLog(m_file_,m_line_,level,format,argp);
            va_end(argp);
        }
    private:
        CWriter()   //��PC-LINT
        {
            m_file_ = NULL;
            m_line_ = 0;
        }
        const char* m_file_;
        long m_line_;
};

//������ʹ�����º�д��־
#define AS_LOG (CWriter(__FILE__, __LINE__))
//���磺AS_LOG(LOG_INFO, "Recv=%d,Send=%d", nRecv,nSend);


#endif//_AS_LOG_H_

