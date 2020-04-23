#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>

#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"

#ifndef uint32_t
typedef u_int32_t  uint32_t;

#endif

#ifndef uint64_t
typedef u_int64_t  uint64_t;

#endif

#ifndef uint16_t
typedef u_int16_t uint16_t;
#endif


#if AS_APP_OS == AS_OS_LINUX
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
//#include <linux/sem.h>
#endif

#if AS_APP_OS == AS_OS_WIN32
#include "Winbase.h"
#include "Windows.h"
#endif

#include "as_daemon.h"

int32_t g_iCfgDaemonlize = 1;
int32_t g_iReStartTimes  = 0;
uint32_t g_ulReStartTime   = 0;



#define RLIMIT (1024 * 1024)

#define SEM_PRMS  0644 //信号量操作权限，0644即主用户(属主)可读写、组成员及其它成员可读不可写


AS_BOOLEAN onlyone_process(const char *strFileName,int32_t key)
{
#if AS_APP_OS == AS_OS_WIN32
    SECURITY_ATTRIBUTES eventAtrributes;
    eventAtrributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    eventAtrributes.bInheritHandle = FALSE;
    eventAtrributes.lpSecurityDescriptor = NULL;

    HANDLE handle = ::CreateEvent(&eventAtrributes, TRUE, FALSE, strFileName);
    if(NULL == handle || (ERROR_ALREADY_EXISTS == ::GetLastError()))
    {
        printf("A instance is running");
        return AS_FALSE;
    }

    return AS_TRUE;
#elif AS_APP_OS == AS_OS_LINUX
    key_t   key_    = 0;
    int32_t sem_id_ = -1;
    const char *fileName = "/dev";
    if(-1 == (key_ = ftok(fileName, key)))
    {
        exit(1);
    }

    sem_id_ = semget(key_, 0, 0);
    if (sem_id_ == -1)
    {
        return AS_FALSE;
    }

    //union semun semctl_arg;
    unsigned short array = NULL;
    if(semctl(sem_id_, 0, GETVAL, array) > 0)
    {
        syslog(LOG_USER|LOG_WARNING,"A instance is running, semaphore ID[%d].", sem_id_);
        return AS_FALSE;
    }

    sem_id_ = semget(key_, 0, 0);
    if (sem_id_ == -1)
    {
        sem_id_ = semget(key_, 1, IPC_CREAT | IPC_EXCL | SEM_PRMS);
    }

    struct sembuf buf[2];

    buf[0].sem_num = 0;
    buf[0].sem_op = 0;
    buf[0].sem_flg = IPC_NOWAIT;
    buf[1].sem_num = 0;
    buf[1].sem_op = 1;
    buf[1].sem_flg = SEM_UNDO;//进程退出时自动回滚

    return semop(sem_id_, &buf[0], 2) == 0;

    if (0 != semop(sem_id_, &buf[0], 2))
    {
        syslog(LOG_USER|LOG_WARNING,"Fail to create semaphore to avoid re-run, semaphore ID[%d].", sem_id_);
        return AS_FALSE;
    }
#endif

    return AS_TRUE;
}


int32_t setResourceLimit()
{
    struct rlimit limit;
    limit.rlim_cur = RLIMIT;
    limit.rlim_max = RLIMIT;

    (void)setrlimit(RLIMIT_NOFILE, &limit);

    limit.rlim_cur = RLIM_INFINITY;
    limit.rlim_max = RLIM_INFINITY;

    (void)setrlimit(RLIMIT_AS, &limit);
    (void)setrlimit(RLIMIT_CORE, &limit);
    (void)setrlimit(RLIMIT_CPU, &limit);
    (void)setrlimit(RLIMIT_DATA, &limit);
    (void)setrlimit(RLIMIT_FSIZE, &limit);
    (void)setrlimit(RLIMIT_LOCKS, &limit);
    (void)setrlimit(RLIMIT_MEMLOCK, &limit);
    (void)setrlimit(RLIMIT_RSS, &limit);
    (void)setrlimit(RLIMIT_STACK, &limit);
    (void)setrlimit(RLIMIT_NPROC, &limit);

    return DAEMO_SUCCESS;
}

static pid_t child_pid; //工作进程的进程ID

//精度为秒
uint32_t as_daemon_get_ticks ( void )
{
    uint32_t ticks = 0 ;

    struct timeval now;

    (void)gettimeofday(&now, NULL);

    ticks = (uint32_t )now.tv_sec;

    return( ticks );
}



void (*workfunc)();

void (*exitfunc)();

void init_daemon( void (*pWorkFunc)(), void (*pExitFunc)())
{
    workfunc = pWorkFunc;
    exitfunc = pExitFunc;
}

#define WAIT_RELAUNCH 1
#define WAIT_PARENT_KILL 1
#define WAIT_DAEMON 1
#define WAIT_KILL_ALL 10

#define PROCESS_MASK 022

void  rrs_run()
{
    (void)sleep(10);
}

void sigquit_handle(int32_t signum)
{
    syslog(LOG_USER|LOG_WARNING,
        "daemon(pid=%d) recive signal SIGQUIT.\n", getpid());

    pid_t pid;

    signum = signum;

    //不再接受SIGCHLD信号，防止重复拉起
    (void)signal(SIGCHLD, SIG_IGN);

    (void)system("echo \" fail to start service, please check log files!\" | wall");

    //工作进程组全部杀掉
    (void)kill(0 - child_pid, SIGTERM);
    (void)usleep(WAIT_KILL_ALL);
    (void)kill(0 - child_pid, SIGKILL);

    //释放子进程占用的系统资源
    do
    {
        pid = waitpid(-1, NULL, WNOHANG | __WALL);
    }
    while (pid > 0);

    syslog(LOG_USER|LOG_WARNING,
        "daemon(pid=%d) exit.\n", getpid());

    //daemon自己也退出
    exit(1);
}


void sigchld_handle(int32_t signum)
{
    g_iReStartTimes++ ;

    syslog(LOG_USER|LOG_WARNING,
        "daemon(pid=%d) recive signal SIGCHLD "
        "g_iReStartTimes = %d g_ulReStartTime = %lu.\n",
         getpid(), g_iReStartTimes, g_ulReStartTime);

    uint32_t ulTempReStartTime = 0;

    /*10秒之内重启了三次，则守护进程退出*/
    if(g_iReStartTimes > 3)
    {
        ulTempReStartTime = as_daemon_get_ticks( );

        syslog(LOG_USER|LOG_WARNING,
            "daemon(pid=%d)  g_ulReStartTime = %lu ulTempReStartTime = %lu.\n",
             getpid(), g_ulReStartTime, ulTempReStartTime);

        //重启动开始时间算起，10秒之内超过三次则daemon进程退出
        if( (ulTempReStartTime - g_ulReStartTime) < 10 )
        {
            syslog(LOG_USER|LOG_WARNING,
                "daemon(pid=%d) exit. "
                "because daemon restarted too many times in time(%lu).\n",
                getpid(), (ulTempReStartTime - g_ulReStartTime));

            //daemon退出
            exit(1);
        }

        //超过三次后，重新计算重启时间
        g_iReStartTimes = 0;
        g_ulReStartTime = as_daemon_get_ticks();

    }

    pid_t pid;

    signum = signum;

    //工作进程组全部杀掉
    (void)kill(0 - child_pid, SIGTERM);
    (void)usleep(WAIT_KILL_ALL);
    (void)kill(0 - child_pid, SIGKILL);

    //释放子进程占用的系统资源
    do
    {
        pid = waitpid(-1, NULL, WNOHANG | __WALL);
    }
    while (pid > 0);

    //先还原SIGCHLD处理函数，否则fork出来就是僵尸
    (void)signal(SIGCHLD, SIG_DFL);
    //休眠2秒，防止频发启停
    sleep(WAIT_RELAUNCH);

    //daemon进程派生出工作进程，此时daemon进程将监控工作进程
    pid = fork();

    switch (pid)
    {
        case -1:
        {
            //fork失败
            syslog(LOG_USER|LOG_ERR,
               "Unable to fork worker process, exit.\n");
            exit(1);
            break;
        }
        case 0:
        {
            //工作进程，继续运行

            (void)umask(PROCESS_MASK);

            (void)setpgid(0, getpid());

            (void)signal(SIGCHLD, SIG_DFL);
            (void)signal(SIGQUIT, SIG_IGN);
            (void)signal(SIGPIPE, SIG_IGN);

            workfunc();
            exit(0);
        }
        default:
        {
            //守护进程
            child_pid = pid;

            syslog(LOG_USER|LOG_WARNING,
                   "respawed new worker pid is  %d , daemon pid is %d\n",
                    child_pid, getpid());

            //重新注册SIGCHLD处理函数
            (void)signal(SIGCHLD, sigchld_handle);

            //抑制一下再次拉起的速度，防止反复拉起
            (void)sleep(WAIT_RELAUNCH);
            break;
        }
    }
}

void send_sigquit_to_deamon()
{
    int32_t enback = 1;
    if (enback == g_iCfgDaemonlize)
    {
        //给父进程发一个SIGQUIT，表示不用拉起，一起退出
        (void)kill(getppid(), SIGQUIT);

        (void)sleep(WAIT_PARENT_KILL);
    }
    exit(1);
}

int32_t create_daemon( const char* service_conf_path, int32_t service_id )
{
    int32_t fdnull;
    pid_t pid;

    //fork出deamon进程
    pid = fork();

    switch (pid)
    {
        case -1:
        {
            //fork失败
            printf("Unable to fork()!\n");
            exit(1);
            break;
        }
        case 0:
        {
            //子进程（deemon进程），继续运行
            break;
        }
        default:
        {
            //daemon进程的父进程，退出
            exit(0);
        }
    }

    if(AS_TRUE != onlyone_process( service_conf_path, service_id))  //保证只有一个实例运行
    {
        printf( "\nA instance is running[%s].\n\n", service_conf_path );
        exit(0);
    }
    (void)setsid();

    fdnull = open("/dev/null", O_RDWR);
    if (fdnull > 0)
    {
        (void)dup2(fdnull, STDIN_FILENO);
        (void)dup2(fdnull, STDOUT_FILENO);
        (void)dup2(fdnull, STDERR_FILENO);
    }

    (void)signal(SIGCHLD, sigchld_handle);
    (void)signal(SIGQUIT, sigquit_handle);

    //deamon进程派生出工作进程，此时deamon进程将监控工作进程
    pid = fork();

    switch (pid)
    {
        case -1:
        {
            //fork失败
            printf("Unable to fork()!\n");
            exit(1);
            break;
        }
        case 0:
        {
            //子进程（deemon进程），继续运行
            break;
        }
        default:
        {
            //守护进程
            child_pid = pid;
            syslog(LOG_USER|LOG_WARNING,
                "create daemon pid=%d, worker pid=%d.\n", getpid(), child_pid);

            //屏蔽信号 ctrl+c
            (void)signal(SIGINT, SIG_IGN);
            (void)signal(SIGPIPE, SIG_IGN);

            g_ulReStartTime = as_daemon_get_ticks();

            syslog(LOG_USER|LOG_WARNING,
                "g_ulReStartTime =%lu.\n", g_ulReStartTime);

            //(void)signal(SIGCHLD, sigchld_handle);
            //(void)signal(SIGQUIT, sigquit_handle);

            for(; ;)
            {
                //空循环，降低cpu使用率
                (void)sleep(WAIT_DAEMON);
            }
        }
    }

    (void)signal(SIGCHLD, SIG_DFL);
    (void)signal(SIGQUIT, SIG_DFL);

    (void)sleep(WAIT_DAEMON);

    (void)umask(PROCESS_MASK);

    (void)setpgid(0, getpid());

    (void)signal(SIGCHLD, SIG_DFL);

    (void)signal(SIGQUIT, SIG_DFL);

    //执行工作进程中的主函数
    workfunc();

    return DAEMO_SUCCESS;
}

typedef void sigfunc(int32_t, siginfo_t *, void *);


void sigprocess(int32_t iSignal, const siginfo_t *pSiginfo, void *procContext)
{
    printf("Receove FPE signal. signal = %d, SIGFPE = %d\n", iSignal, SIGFPE);

    procContext = procContext;

    if (pSiginfo != NULL)
    {
        printf("pSiginfo->si_signo = %d\n", pSiginfo->si_signo);

        printf("pSiginfo->si_errno = %d\n", pSiginfo->si_errno);
    }
    else
    {
        printf("pSiginfo is NULL\n");
    }


    exitfunc();

    return;
}

int32_t sigRegister(int32_t iSigNo, sigfunc *pFunc)
{
    struct sigaction newAct;

    newAct.sa_sigaction = pFunc;
    (void)sigemptyset(&newAct.sa_mask);

    newAct.sa_flags = 0;
    newAct.sa_flags |= SA_SIGINFO;

    if (sigaction(iSigNo, &newAct, NULL) < 0)
    {
        return DAEMO_FAIL;
    }

    return DAEMO_SUCCESS;
}

void as_run_service(void (*pWorkFunc)(),
                          int32_t iRunningMod, void (*pExitFunc)(),
                           const char* service_conf_path, int32_t service_id)
{
    printf("server is starting\n");

    if (DAEMO_FAIL == setResourceLimit())
    {
        printf("set resource limit failed, abort\n");
        return ;
    }

    g_iCfgDaemonlize = iRunningMod;

     //初始化daemon， 注册回调函数
    init_daemon(pWorkFunc, pExitFunc);
    if (enBackGround == g_iCfgDaemonlize)
    {
        //启动程序
        (void)create_daemon( service_conf_path, service_id );
    }
    else
    {
         if(AS_TRUE !=  onlyone_process( service_conf_path, service_id))  //保证只有一个实例运行
         {
             printf( "\nA instance is running[%s].\n\n", service_conf_path );
             exit(0);
         }
        if (DAEMO_SUCCESS != sigRegister(SIGINT, (sigfunc *)sigprocess))
        {
            printf("SigRegister SIGINT fail, abort\n");
            return ;
        }
        workfunc();
    }

    return ;

}



