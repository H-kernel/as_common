#ifndef __AS_SYNCHRONIZED_INCLUDE_H__
#define __AS_SYNCHRONIZED_INCLUDE_H__
#include "as.h" 
class as_synchronized
{
public:
    as_synchronized();
    virtual ~as_synchronized();

    long   popWait(long timeout);    
    long   pushWait(long timeout);    

    long   start();

    long   notifyRead();
    long   notifyWrite();    
    long   notify_all();
    long  lock();
    AS_BOOLEAN  trylock();
    long  unlock();

#ifdef WIN32
    long     wait(HANDLE,HANDLE,long)const;
    char    numNotifies;
    HANDLE  semEvent;
    HANDLE  semMutex;
    HANDLE  semPushEvent;
    HANDLE  semPushMutex;    
#else
    pthread_cond_t  pop_cond;
    pthread_cond_t  push_cond;    
    pthread_mutex_t monitor;
    pthread_mutex_t push_monitor;    
    long  wait(pthread_cond_t *,pthread_mutex_t *,long );
    long  cond_timed_wait( pthread_cond_t * ,pthread_mutex_t *,timespec *);    
#endif
};

#endif /* __AS_SYNCHRONIZED_INCLUDE_H__ */
