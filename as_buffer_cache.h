#ifndef __AS_BUFFER_CACHE_INCLUDE_H__
#define __AS_BUFFER_CACHE_INCLUDE_H__

#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
extern "C"{
#include  "as_mutex.h"
#include  "as_thread.h"
}

class as_data
{
public:
   friend class as_cache;
public:
    virtual ~as_data();
    void*    base();
    void*    rd_ptr();
    void*    wr_ptr();
    void     rd_ptr(int32_t len);
    void     wr_ptr(int32_t len);
    uint32_t size();
    uint32_t length();
    int32_t  copy(void* data,uint32_t len);
protected:
    as_data(uint32_t size);
    as_data();
private:
    void*    m_pData;
    void*    m_rd_ptr;
    void*    m_wr_ptr;
    uint32_t m_ulSize;
    uint32_t m_ulRefCount;
};

class as_cache
{
public:
    as_cache(uint32_t size);
    virtual ~as_cache();
    void*    base();
    void*    rd_ptr();
    void*    wr_ptr();
    void     rd_ptr(int32_t len);
    void     wr_ptr(int32_t len);
    uint32_t size();
    uint32_t length();
    int32_t  copy(void* data,uint32_t len);
    int32_t  copy(as_cache* pCache);
protected:
    as_cache();
private:
    as_data*    m_pData;
};


class as_buffer_cache
{
public:
    static as_buffer_cache& instance()
    {
        static as_buffer_cache objBufferCache;
        return objBufferCache;
    };
    virtual ~as_buffer_cache();
    /*
     * uint32_t config [][] = {
     *     {128  byte, count},
     *     {512  byte, count},
     *     {1024 byte, count},
     *     {2048 byte, count},
     *     ...........
     * }
     * 
     */
    int32_t   init(uint32_t array,uint32_t** config);
    void      release();
    as_cache* allocate();
    void      free(as_cache* cache);
protected:
    as_buffer_cache();
};


#endif /* __AS_BUFFER_CACHE_INCLUDE_H__ */
