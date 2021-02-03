#ifndef __AS_TIME_H_INCLUDE
#define __AS_TIME_H_INCLUDE

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"

void  as_sleep(uint32_t ulMs );
uint32_t as_get_ticks (void);
uint32_t as_get_cur_msecond(void);
void  as_strftime(char * pszTimeStr, uint32_t ulLens, char* pszFormat, time_t ulTime);
time_t as_str2time(const char *pStr);
struct tm* as_Localtime(time_t* ulTime);


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */


#endif // __AS_TIME_H_INCLUDE

