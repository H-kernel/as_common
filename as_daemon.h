#ifndef __AS_DAEMON_H__
#define __AS_DAEMON_H__
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#define DAEMO_SUCCESS    0
#define DAEMO_FAIL       -1

typedef enum
{
    enForeGround          = 0, 
    enBackGround          = 1 
}RUNNING_MOD;

void as_run_service(void(*pWorkFunc)(),
                    int32_t iRunningMod,
                    void (*pExitFunc)(),
                    const char* service_conf_path,
                    int32_t service_id );
//�˳�
void send_sigquit_to_deamon();

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif // __AS_DAEMON_H__


