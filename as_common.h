#ifndef __AS_MEDIA_COMMON_H__
#define __AS_MEDIA_COMMON_H__
#include "as_config.h"
#include "as_thread.h"
#include "as_mutex.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>

#define  AS_DEFAULT_STACK_SIZE (128*1024)

enum AS_ERROR_CODE {
    AS_ERROR_CODE_FAIL        = -1,
    AS_ERROR_CODE_OK          = 0x00,
    AS_ERROR_CODE_MEM         = 0x01,
    AS_ERROR_CODE_PARAM       = 0x02,
    AS_ERROR_CODE_SYS         = 0x03,
    AS_ERROR_CODE_TIMEOUT     = 0x04,
    AS_ERROR_CODE_MAX
};


#endif /*__AS_MEDIA_COMMON_H__*/

