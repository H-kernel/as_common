
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
#include "as_config.h"
#include "as_basetype.h"

#define ASLOG_API

typedef enum _ASLogLevel
{
    AS_LOG_EMERGENCY   = 0,
    AS_LOG_ALERT       = 1, 
    AS_LOG_CRITICAL    = 2,
    AS_LOG_ERROR       = 3,
    AS_LOG_WARNING     = 4, 
    AS_LOG_NOTICE      = 5,
    AS_LOG_INFO        = 6, 
    AS_LOG_DEBUG       = 7
}ASLogLevel;

ASLOG_API void ASSetLogLevel(int32_t lLevel);

ASLOG_API bool ASSetLogFilePathName(const char* szPathName);

ASLOG_API void ASSetLogFileLengthLimit(uint32_t ulLimitLengthKB);

ASLOG_API void ASStartLog(void);

ASLOG_API void __ASWriteLog(const char* szFileName, int32_t lLine,
                             int32_t lLevel, const char* format, va_list argp);
ASLOG_API void ASStopLog(void);

class CWriter
{
    public:
        CWriter(const char* file, int32_t line)
        {
            m_file_ = file;
            m_line_ = line;
        }
        void operator()(int32_t level, const char* format, ...)
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
        int32_t m_line_;
};

#define AS_LOG (CWriter(__FILE__, __LINE__))

#endif//_AS_LOG_H_

