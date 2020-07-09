#ifndef __AS_URL_H_INCLUDE
#define __AS_URL_H_INCLUDE

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#include "as_config.h"
#include <stdint.h>
#include <stdio.h>
#include <signal.h>

#define AS_URL_MAX_LEN 128

typedef struct tagASUrl
{
    char     host[AS_URL_MAX_LEN];
    uint16_t port;
}as_url_t;

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */


#endif // __AS_URL_H_INCLUDE