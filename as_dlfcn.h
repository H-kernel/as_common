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
typedef struct tagSVSDLLHandle
{
    void* hDllInst;
}SVSDLLHandle;
#elif AS_APP_OS == AS_OS_WIN32
typedef struct tagSVSDLLHandle
{
    HINSTANCE hDllInst;
}SVSDLLHandle;
#endif


SVSDLLHandle* SVS_LoadLibrary(const char* pszPath);
void* SVS_GetProcAddress(SVSDLLHandle* pHandle, const char* pszName);
void SVS_FreeLoadLib(SVSDLLHandle* pHandle);


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */


#endif // SVS_H_INCLUDE

