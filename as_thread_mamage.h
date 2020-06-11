#ifndef __AS_THREAD_MANAGE_INCLUDE_H__
#define __AS_THREAD_MANAGE_INCLUDE_H__

#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
extern "C"{
#include  "as_mutex.h"
#include  "as_thread.h"
}


class as_task_thread
{
public:
    as_task_thread();
    virtual ~as_task_thread();
};


#endif /*__AS_THREAD_MANAGE_INCLUDE_H__*/