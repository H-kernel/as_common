#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#include "as_config.h"
#include <time.h>
#include <errno.h>
#include "as_time.h"


#if AS_APP_OS == AS_OS_WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment( lib,"winmm.lib" )
#elif AS_APP_OS == AS_OS_LINUX
#include <sys/time.h>
#endif


static uint32_t g_ulSysStart = 0 ;


uint32_t as_get_ticks ()
{
    ULONG ticks = 0 ;

#ifdef WIN32
    ticks = timeGetTime()/1000;
#else
    struct timeval now;
    gettimeofday(&now, AS_NULL);
    ticks=now.tv_sec;
#endif

    return( ticks );
}

uint32_t as_get_cur_msecond()
{
    ULONG ticks = 0;

#ifdef WIN32
    ticks = timeGetTime();
#else
    struct timeval now;
    gettimeofday(&now, AS_NULL);
    ticks = now.tv_sec*1000+now.tv_usec/1000;
#endif

    return(ticks);
}

void  as_delay (uint32_t ulDelayTimeMs)
{
    LONG was_error;

    struct timeval tv;

    ULONG then, now, elapsed;

    then = as_get_ticks();

    do
    {
        errno = 0;
        /* Calculate the time LONGerval left (in case of LONGerrupt) */
        now = as_get_ticks();
        elapsed = (now-then);
        then = now;
        if ( elapsed >= ulDelayTimeMs )
        {
            break;
        }

        ulDelayTimeMs -= elapsed;
        tv.tv_sec = (int32_t)(ulDelayTimeMs/1000);
        tv.tv_usec = (ulDelayTimeMs%1000)*1000;

        was_error = select(0, AS_NULL, AS_NULL, AS_NULL, &tv);

    } while ( was_error && (errno == EINTR) );
}

/*1000 = 1second*/
void  as_sleep(uint32_t ulMs )
{
#if AS_APP_OS == AS_OS_LINUX
    as_delay( ulMs );
#elif AS_APP_OS == AS_OS_WIN32
    Sleep(ulMs);
#endif

return ;
}

void  as_strftime(char * pszTimeStr, uint32_t ulLens, char* pszFormat, time_t ulTime)
{
    struct tm* tmv;
    time_t uTime = (time_t)ulTime;
    tmv = (struct tm*)localtime(&uTime);

    strftime(pszTimeStr, ulLens, pszFormat, tmv);
    return;
}

struct tm* as_Localtime(time_t* ulTime)
{
    return (struct tm*)localtime(ulTime);/*0~6���� ���յ�����*/
}


//����20040629182030��ʱ�䴮ת��������Ϊ��λ������ʱ��,
//���Թ��ʱ�׼ʱ�乫Ԫ1970��1��1��00:00:00����������������
time_t as_str2time(const char *pStr)
{
    struct tm tmvalue;

    (void)memset(&tmvalue, 0, sizeof(tmvalue));

    const char *pch = pStr;
    char tmpstr[8];
    memcpy(tmpstr, pch, 4);
    tmpstr[4] = '\0';
    tmvalue.tm_year = atoi(tmpstr) - 1900;
    pch += 4;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_mon = atoi(tmpstr) - 1;
    pch += 2;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_mday = atoi(tmpstr);
    pch += 2;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_hour = atoi(tmpstr);
    pch += 2;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_min = atoi(tmpstr);
    pch += 2;

    memcpy(tmpstr, pch, 2);
    tmpstr[2] = '\0';
    tmvalue.tm_sec = atoi(tmpstr);

    return mktime(&tmvalue);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */



