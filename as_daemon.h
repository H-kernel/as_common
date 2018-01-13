#ifndef __AS_DAEMON_H__
#define __AS_DAEMON_H__

#define DAEMO_SUCCESS    0
#define DAEMO_FAIL       -1

typedef enum
{
    enForeGround          = 0, //ǰ̨����
    enBackGround          = 1 //��̨����
}RUNNING_MOD;


/*iRunningMod : 1  �ػ�����ģʽ��0 ǰ̨ģʽ*/
void as_run_service(void(*pWorkFunc)(),
                         int32_t iRunningMod,
                         void (*pExitFunc)(),
                         const char* service_conf_path,
            int32_t service_id );
//�˳�
void send_sigquit_to_deamon();

#endif // __AS_DAEMON_H__


