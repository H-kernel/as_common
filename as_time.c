#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
#include "as_time.h"


#if AS_APP_OS == AS_OS_WIN32
#include <time.h>
#include <windows.h>
#include <mmsystem.h>
#pragma comment( lib,"winmm.lib" )
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
#include <time.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#endif


#include <errno.h>

static uint32_t g_ulSysStart = 0 ;


uint32_t as_get_ticks (void)
{
    ULONG ticks = 0 ;

#if AS_APP_OS ==  AS_OS_WIN32
    ticks = timeGetTime()/1000;
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    struct timeval now;
    gettimeofday(&now, AS_NULL);
    ticks=now.tv_sec;
#endif

    return( ticks );
}

uint32_t as_get_cur_msecond(void)
{
    ULONG ticks = 0;

#if AS_APP_OS ==  AS_OS_WIN32
    ticks = timeGetTime();
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    struct timeval now;
    gettimeofday(&now, AS_NULL);
    ticks = now.tv_sec*1000+now.tv_usec/1000;
#endif

    return(ticks);
}

/*1000 = 1second*/
void  as_sleep(uint32_t ulMs )
{
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    usleep( ulMs*1000 );
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



