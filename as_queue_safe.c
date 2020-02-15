#include "as_thread.h"
#include "as_config.h"
#include "as_common.h"
#if AS_APP_OS == AS_OS_LINUX
#include <pthread.h>
#elif AS_APP_OS == AS_OS_WIN32
#endif
#include "as_queue_safe.h"

typedef struct as_safe_node_s {
    as_queue_t   node;
    void        *data;
}as_safe_node_t;

as_safe_queue_t* as_safe_queue_create()
{
    as_safe_queue_t* pQueue = NULL;
    do {
        pQueue = (as_safe_queue_t *)(void*) malloc(sizeof(as_safe_queue_t));
        if (NULL == pQueue) {
            break;
        }
        as_queue_init(&pQueue->head);
        pQueue->lock = as_create_mutex();
        if (NULL == pQueue->lock) {
            break;
        }
        queue->size = 0;
        return pQueue;
    }while (0);

    if(NULL != pQueue) {
        if(NULL != pQueue->lock) {
            as_destroy_mutex(pQueue->lock);
            pQueue->lock = NULL;
        }
        free( pQueue );
        pQueue = NULL;
    }
    return pQueue;
}
void    as_safe_queue_destory(as_safe_queue_t* queue)
{
    if(NULL != queue) {
        if(NULL != queue->lock) {
            as_destroy_mutex(queue->lock);
            queue->lock = NULL;
        }
        free( queue );
    }
    return;
}
int32_t as_safe_queue_push_back(as_safe_queue_t* queue,void* data)
{
    as_safe_node_t* pNode = NULL;
    int32_t nRet = AS_ERROR_CODE_FAIL;
    if(AS_ERROR_CODE_OK != as_mutex_lock(queue->lock)) {
        return AS_ERROR_CODE_FAIL;
    }

    do {
        pNode = (as_safe_node_t *)(void*) malloc(sizeof(as_safe_node_t));
        if (NULL == pNode) {
            break;
        }
        pNode->data = data;
        pNode->node.next = NULL;
        pNode->node.prev = NULL;
        as_queue_insert_tail(&queue->head,&pNode->node);
        queue->size++;
        nRet = AS_ERROR_CODE_OK;
    }while (0);

    as_mutex_unlock(queue->lock);
    return nRet;
}
int32_t as_safe_queue_pop_back(as_safe_queue_t* queue,void** data)
{
    as_safe_node_t* pNode = NULL;
    int32_t nRet = AS_ERROR_CODE_FAIL;
    if(AS_ERROR_CODE_OK != as_mutex_lock(queue->lock)) {
        return AS_ERROR_CODE_FAIL;
    }
    do{
        if(0 == queue->size) {
            break;
        }
        pNode = as_queue_last(&queue->head);
        as_queue_remove(pNode);
        *data = pNode->data;
        queue->size--;
        free(pNode);
        nRet = AS_ERROR_CODE_OK;
    }while (0);    
    
    as_mutex_unlock(queue->lock);

    return nRet;
}
int32_t as_safe_queue_push_front(as_safe_queue_t* queue,void* data)
{
    as_safe_node_t* pNode = NULL;
    int32_t nRet = AS_ERROR_CODE_FAIL;
    if(AS_ERROR_CODE_OK != as_mutex_lock(queue->lock)) {
        return AS_ERROR_CODE_FAIL;
    }

    do {
        pNode = (as_safe_node_t *)(void*) malloc(sizeof(as_safe_node_t));
        if (NULL == pNode) {
          
            break;
        }
        pNode->node.next = NULL;
        pNode->node.prev = NULL;
        pNode->data = data;
        as_queue_insert_head(&queue->head,&pNode->node);
        queue->size++;
        nRet = AS_ERROR_CODE_OK;
    }while (0);

    as_mutex_unlock(queue->lock);
}
int32_t as_safe_queue_pop_front(as_safe_queue_t* queue,void** data)
{
    as_safe_node_t* pNode = NULL;
    int32_t nRet = AS_ERROR_CODE_FAIL;
    if(AS_ERROR_CODE_OK != as_mutex_lock(queue->lock)) {
        return AS_ERROR_CODE_FAIL;
    }
    do{
        if(0 == queue->size) {
            break;
        }
        pNode = as_queue_head(&queue->head);
        as_queue_remove(pNode);
        *data = pNode->data;
        queue->size--;
        free(pNode);
        nRet = AS_ERROR_CODE_OK;
    }while (0);    
    
    as_mutex_unlock(queue->lock);

    return nRet;
}