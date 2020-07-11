#ifndef __AS_URL_H_INCLUDE
#define __AS_URL_H_INCLUDE

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#include "as_config.h"
#include "as_common.h"
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#define AS_ULR_PROTOCOL_LEN  8
#define AS_URL_MAX_LEN       128
#define AS_URL_ARG_NAME_LEN  64
#define AS_URL_ARG_VALUE_LEN 128

typedef struct tagASUrl
{
    char     protocol[AS_ULR_PROTOCOL_LEN];
    char     host[AS_URL_MAX_LEN];
    uint16_t port;
    char     uri[AS_URL_MAX_LEN];
    char     args[AS_URL_MAX_LEN];
}as_url_t;

typedef struct tagASUrlArg
{
    char     name[AS_URL_ARG_NAME_LEN];
    char     value[AS_URL_ARG_VALUE_LEN];
    char*    next;
}as_url_arg_t;

void       as_init_url(as_url_t* url);
int32_t    as_parse_url(const char* url,as_url_t* info);
int32_t    as_first_arg(as_url_t* url,as_url_arg_t* arg);
int32_t    as_next_arg(as_url_arg_t* arg,as_url_arg_t* next);
int32_t    as_find_arg(as_url_t* url,const char* name,as_url_arg_t* arg);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */


#endif // __AS_URL_H_INCLUDE