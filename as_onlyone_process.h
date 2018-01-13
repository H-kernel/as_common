#ifndef _AS_Onlyone_Process_h
#define _AS_Onlyone_Process_h

#include <stdint.h>
#include <sys/ipc.h>

const int32_t SEM_PRMS = 0644;//�ź�������Ȩ�ޣ�0644�����û�(����)�ɶ�д�����Ա��������Ա�ɶ�����д

class as_onlyone_process
{
protected:
    as_onlyone_process();
    virutal ~as_onlyone_process();
    as_onlyone_process(const as_onlyone_process& obj);
    as_onlyone_process& operator=(const as_onlyone_process& obj);
public:
    static bool onlyone(const char *strFileName,int32_t key =0);

    /**
    * �������Ƿ���Ҫ��������.
    * �����Ҫ������������ô����true�����򷵻�false.
    */
    static bool need_restart(const char *strFileName, int32_t key=0);
protected:
    int32_t init(const char *strFileName, int32_t key);
    bool exists();  //����ź����Ƿ��Ѵ���
    bool mark();    //�����ź���
    bool unmark();  //����ź���
private:
    key_t key_;
    int32_t sem_id_;
};

#endif //_AS_Onlyone_Process_h

