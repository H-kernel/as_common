#ifndef SVS_DLL_H_INCLUDE
#define SVS_DLL_H_INCLUDE

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#include "as_config.h"
#if AS_APP_OS == AS_OS_LINUX
#include <dlfcn.h>
#endif


#if AS_APP_OS == AS_OS_LINUX
typedef struct tag_ASDllHandle
{
    void* hDllInst;
}as_dll_handle_t;
#elif AS_APP_OS == AS_OS_WIN32
typedef struct tag_ASDllHandle
{
    HINSTANCE hDllInst;
}as_dll_handle_t;
#endif


as_dll_handle_t* SVS_LoadLibrary(const char* pszPath);
void* SVS_GetProcAddress(as_dll_handle_t* pHandle, const char* pszName);
void SVS_FreeLoadLib(as_dll_handle_t* pHandle);


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */


#endif // SVS_H_INCLUDE

