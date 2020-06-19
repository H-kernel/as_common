#include "as_buffer_cache.h"


as_data::as_data()
{
    m_pData      = NULL;
    m_rd_ptr     = NULL;
    m_wr_ptr     = NULL;
    m_ulSize     = 0;
    m_ulRefCount = 0;
}
as_data::as_data(uint32_t size)
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
as_data::~as_data()
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
char*    as_data::base()
{
    return m_pData;
}
char*    as_data::rd_ptr()
{
    return m_rd_ptr;
}
char*    as_data::wr_ptr()
{
    return m_wr_ptr;
}
void     as_data::rd_ptr(int32_t len)
{
    m_rd_ptr += len;
    return;
}
void     as_data::wr_ptr(int32_t len)
{
    m_wr_ptr += len;
    return;
}
uint32_t as_data::size()
{
    return m_ulSize;
}
uint32_t as_data::length()
{
    if(m_wr_ptr <= m_rd_ptr) {
        return 0;
    }
    return (m_wr_ptr - m_rd_ptr);
}
int32_t  as_data::copy(char* data,uint32_t len)
{
    uint32_t size = m_ulSize - (m_wr_ptr - m_pData);
    if(size < len) {
        return AS_ERROR_CODE_MEM;
    }
    memcpy(m_wr_ptr,data,len);
    m_wr_ptr += len;
    return AS_ERROR_CODE_OK;
}
void as_data::inc_ref()
{
    m_ulRefCount++;
}
void as_data::dec_ref()
{
    m_ulRefCount--;
}
uint32_t as_data::get_ref()
{
    return m_ulRefCount;
}

as_cache::as_cache()
{
    m_pData      = NULL;
    m_pAllocator = NULL;
}
as_cache::as_cache(uint32_t size)
{
    try {
        m_pData = new as_data(size);
    }
    catch(...) {
        m_pData = NULL;
    }
    m_pAllocator = NULL;
}
as_cache::as_cache(as_data* pData,as_thread_allocator* pAllocator)
{
    m_pData = pData;
    m_pAllocator = pAllocator;
    if(NULL != m_pData) {
        m_pData->inc_ref();
    }
}
void as_cache::set_allocator(as_thread_allocator* pAllocator) 
{
    m_pAllocator = pAllocator;
}
as_cache::~as_cache()
{
    if(NULL == m_pData) {
        return;
    }

    m_pData->dec_ref();
    if(0 ==  m_pData->get_ref()) {
        try {
            delete m_pData;
        }
        catch(...) {

        }
        m_pData = NULL;
    }
}
char*    as_cache::base()
{
    if(NULL == m_pData) {
        return NULL;
    }
    return m_pData->base();
}
char*    as_cache::rd_ptr()
{
    if(NULL == m_pData) {
        return NULL;
    }
    return m_pData->rd_ptr();
}
char*    as_cache::wr_ptr()
{
    if(NULL == m_pData) {
        return NULL;
    }
    return m_pData->wr_ptr();
}
void     as_cache::rd_ptr(int32_t len)
{
    if(NULL == m_pData) {
        return ;
    }
    m_pData->rd_ptr(len);
}
void     as_cache::wr_ptr(int32_t len)
{
    if(NULL == m_pData) {
        return ;
    }
    m_pData->wr_ptr(len);
}
uint32_t as_cache::size()
{
    if(NULL == m_pData) {
        return 0;
    }
    return m_pData->size();
}
uint32_t as_cache::length()
{
    if(NULL == m_pData) {
        return 0;
    }
    return m_pData->length();
}
int32_t  as_cache::copy(char* data,uint32_t len)
{
    if(NULL == m_pData) {
        return AS_ERROR_CODE_MEM;
    }
    return m_pData->copy(data,len);
}
int32_t  as_cache::copy(as_cache* pCache)
{
    if(NULL == m_pData) {
        return AS_ERROR_CODE_MEM;
    }
    return m_pData->copy(pCache->rd_ptr(),pCache->length());
}
as_cache* as_cache::duplicate()
{
    as_cache* pCache = NULL;

    if(NULL == m_pData) {
        return NULL;
    }
    
    try {
        pCache = new as_cache(m_pData,m_pAllocator);
    }
    catch(...) {
        return NULL;
    }
    return pCache;
}
void as_cache::release()
{
    if(NULL == m_pData) {
        return;
    }

    m_pData->dec_ref();
    if(NULL !=m_pAllocator) {
        /* 分配器分配的则由分配器释放 */
        m_pAllocator->free(this);
    }
    else {
        /* 非分配器分配的直接采用逻辑释放 */
        if(0 ==  m_pData->get_ref()) {
            try {
                delete m_pData;
            }
            catch(...) {

            }
            m_pData = NULL;
        }

        delete this;    
    }
}
/*****************************************************************************
 ****************************************************************************/

as_thread_allocator::as_thread_allocator(uint32_t ulThreadId)
{
    m_ulRefCount = 1;
    m_ulThreadId = ulThreadId;
    for(uint32_t i = 0 ; i < AS_CACHE_SIZE_DEFINE_MAX;i++) {
        m_Cachelist[i].clear();
    }
}
as_thread_allocator::~as_thread_allocator()
{
    for(uint32_t i = 0 ; i < AS_CACHE_SIZE_DEFINE_MAX;i++) {
        while (0 < m_Cachelist[i].size())
        {
            /* code */
        }
        
    }
}
uint32_t  as_thread_allocator::threadId()
{
    return m_ulThreadId;
}

as_cache* as_thread_allocator::allocate(uint32_t ulSize)
{
}
void      as_thread_allocator::free(as_cache* cache)
{

}

as_buffer_allocator::as_buffer_allocator()
{
    m_pThreadAllocator = NULL;
}
as_buffer_allocator::~as_buffer_allocator()
{
    if(NULL != m_pThreadAllocator) {
        as_buffer_cache::instance().release_thread_allocator(m_pThreadAllocator);
        m_pThreadAllocator = NULL;
    }
}
as_cache* as_buffer_allocator::allocate(uint32_t ulSize)
{
    if(NULL == m_pThreadAllocator) {
        m_pThreadAllocator = as_buffer_cache::instance().find_thread_allocator();
    }

    if(NULL == m_pThreadAllocator) {
        return NULL;
    }

    return m_pThreadAllocator->allocate(ulSize);
}


as_buffer_cache::as_buffer_cache()
{

}
as_buffer_cache::~as_buffer_cache()
{

}
int32_t   as_buffer_cache::init(uint32_t config[],uint32_t size)
{
    return AS_ERROR_CODE_OK;
}
void      as_buffer_cache::release()
{
    return;
}

as_cache* as_buffer_cache::allocate(uint32_t ulSize)
{
    return NULL;
}
void      as_buffer_cache::free(as_cache* cache)
{
    return;
}
as_thread_allocator* as_buffer_cache::find_thread_allocator()
{
    uint32_t ulCurThreadId = as_get_threadid();
    as_thread_allocator* pAllocator = NULL;
    THREAD_ALLOCATOR_MAP::iterator iter = m_ThreadAllocMap.find(ulCurThreadId);
    if(iter != m_ThreadAllocMap.end()) {
        pAllocator = iter->second;
        pAllocator->m_ulRefCount++; 
    }
    try {
        pAllocator = new as_thread_allocator(ulCurThreadId);
    }
    catch(...) {
        pAllocator = NULL;
    }
    if(NULL != pAllocator) {
        m_ThreadAllocMap.insert(THREAD_ALLOCATOR_MAP::value_type(ulCurThreadId,pAllocator));
    }
    return pAllocator;
}
void as_buffer_cache::release_thread_allocator(as_thread_allocator* pAllocator)
{
    uint32_t ulThreadId = pAllocator->threadId();
    pAllocator->m_ulRefCount--;
    if(0 < pAllocator->m_ulRefCount) {
        return;
    }

    THREAD_ALLOCATOR_MAP::iterator iter = m_ThreadAllocMap.find(ulThreadId);
    if(iter != m_ThreadAllocMap.end()) {
        m_ThreadAllocMap.erase(iter);
    }
    try {
        delete pAllocator ;
    }
    catch(...) {
        
    }
    pAllocator = NULL;
    return;
}