#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "as_dlfcn.h"

#define AS_FILE_FILE_SIZE 256

as_dll_handle_t* as_load_library(const char* pszPath)
{
    as_dll_handle_t* phandle = NULL;
    phandle = (as_dll_handle_t *)malloc(sizeof(as_dll_handle_t));
    if (NULL == phandle)
    {
        return NULL;
    }

#if AS_APP_OS == AS_OS_WIN32
    char szFullPath[AS_FILE_FILE_SIZE] = { 0 }; 
    char szLoadPath[AS_FILE_FILE_SIZE] = { 0 };
    HANDLE hDLLModule = NULL;

    if (0 == GetModuleFileName((HMODULE)hDLLModule, szFullPath, sizeof(szFullPath)))
    {
        SVS_free(phandle);
        return NULL;
    }

    char* pszFind = strrchr(szFullPath, '\\');
    if (NULL == pszFind)
    {
        SVS_free(phandle);
        return NULL;
    }

    *(pszFind + 1) = '\0';    //�滻Ϊ������

    strncat(szFullPath, pszPath, AS_FILE_FILE_SIZE);

    phandle->hDllInst = LoadLibraryEx(szFullPath, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
#elif AS_APP_OS == AS_OS_LINUX
    //phandle->hDllInst = dlopen(pszPath,RTLD_NOW);
    phandle->hDllInst = dlopen(pszPath,RTLD_LAZY);
#endif
    if (NULL == phandle->hDllInst)
    {
        free(phandle);
        return NULL;
    }

    return phandle;
}
void* as_get_proc_addr(as_dll_handle_t* pHandle, const char* pszName)
{
    if (NULL == pHandle)
    {
        return NULL;
    }
#if AS_APP_OS == AS_OS_WIN32
    return GetProcAddress(pHandle->hDllInst, pszName);
#elif AS_APP_OS == AS_OS_LINUX
    return dlsym(pHandle->hDllInst, pszName);
#else
    return NULL;
#endif
}

void as_free_library(as_dll_handle_t* pHandle)
{
    if (NULL == pHandle)
    {
        return ;
    }
#if AS_APP_OS == AS_OS_WIN32
    (void)FreeLibrary(pHandle->hDllInst);
#elif AS_APP_OS == AS_OS_LINUX
    dlclose(pHandle->hDllInst);
#endif
    free(pHandle);
    return;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */