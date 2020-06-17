#include "as_buffer_cache.h"


as_cache::as_cache()
{
    m_pData      = NULL;
    m_rd_ptr     = NULL;
    m_wr_ptr     = NULL;
    m_ulSize     = 0;
    m_ulRefCount = 0;
}
as_cache::as_cache(uint32_t size)
{
    m_ulSize = size;
    char* pData = NULL;
    try {
        pData = new char[size];
    }
    catch(...){
        pData = NULL;
    }
    m_pData = m_rd_ptr = m_wr_ptr = pData;
    m_ulRefCount = 1;
}
as_cache::~as_cache()
{
    if(NULL != m_pData) {
        try {
            delete []m_pData;
        }
        catch(...) {

        }
        m_pData = NULL;
    }
    m_rd_ptr     = NULL;
    m_wr_ptr     = NULL;
    m_ulSize     = 0;
    m_ulRefCount = 0;
}
void*    as_cache::base()
{
    return m_pData;
}
void*    as_cache::rd_ptr()
{
    return m_rd_ptr;
}
void*    as_cache::wr_ptr()
{
    return m_wr_ptr;
}
void     as_cache::rd_ptr(int32_t len)
{
    return;
}
void     as_cache::wr_ptr(int32_t len)
{
    return;
}
uint32_t as_cache::size()
{
    return m_ulSize;
}
uint32_t as_cache::length()
{
    return m_ulSize;
}
int32_t  as_cache::copy(void* data,uint32_t len)
{
    return AS_ERROR_CODE_OK;
}
int32_t  as_cache::copy(as_cache* pCache)
{
    return AS_ERROR_CODE_OK;
}


/*****************************************************************************
 ****************************************************************************/

as_buffer_cache::as_buffer_cache()
{

}
as_buffer_cache::~as_buffer_cache()
{

}
int32_t   as_buffer_cache::init(uint32_t array,uint32_t** config)
{
    return AS_ERROR_CODE_OK;
}
void      as_buffer_cache::release()
{
    return;
}
as_cache* as_buffer_cache::allocate()
{
    return NULL;
}
void      as_buffer_cache::free(as_cache* cache)
{
    return;
}