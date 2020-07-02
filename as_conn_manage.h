/******************************************************************************
   ��Ȩ���� (C), 2001-2011, M.Kernel

 ******************************************************************************
  �ļ���          : as_conn_manage.h
  �汾��          : 1.0
  ����            : hexin
  ��������        : 2007-4-02
  ����޸�        :
  ��������        :
  �����б�        :
  �޸���ʷ        :
  1 ����          : 2007-4-02
    ����          : hexin
    �޸�����      : ����
*******************************************************************************/



#ifndef __AS_CONN_MGR_H_INCLUDE__
#define __AS_CONN_MGR_H_INCLUDE__

#ifdef ENV_LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#endif //AS_OS_LINUX

 #ifdef WIN32
 //#include <winsock2.h>
 #endif
//#pragma comment(lib,"ws2_32.lib")

#include <list>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
#include "as.h"

#define InvalidFd -1
#define InvalidSocket -1
#define InvalidIp INADDR_ANY
#define Invalidport 0
#define DEFAULT_TCP_SENDRECV_SIZE (1024 * 1024)
#define DEFAULT_UDP_SENDRECV_SIZE (2 * 1024)

#define SendRecvError       -1//���ͻ���մ���
#define SendRecvErrorTIMEO  -2//���ͻ���ճ�ʱ
#define SendRecvErrorEBADF  -3//socket�������
#define SendRecvErrorEOF    -4//tcp����

#define MAX_LISTEN_QUEUE_SIZE 2000
#define EPOLL_MAX_EVENT (MAX_LISTEN_QUEUE_SIZE + 1000)
#define MAX_EPOLL_FD_SIZE 3000
#define LINGER_WAIT_SECONDS 1 //LINGER�ȴ�ʱ��(seconds)

//�������ӹ������ô�����
#if AS_APP_OS == AS_OS_LINUX
#define CONN_ERR_TIMEO      ETIMEDOUT
#define CONN_ERR_EBADF      EBADF
#elif AS_APP_OS == AS_OS_WIN32
#define CONN_ERR_EBADF      WSAEINTR
#define CONN_ERR_TIMEO      WSAETIMEDOUT
#endif

#if AS_APP_OS == AS_OS_LINUX
#define CLOSESOCK(x) ::close(x)
#define SOCK_OPT_TYPE void
#define CONN_ERRNO errno
#elif AS_APP_OS == AS_OS_WIN32
#define CLOSESOCK(x) closesocket(x)
#define socklen_t int
#define SOCK_OPT_TYPE char
#define CONN_ERRNO WSAGetLastError()
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#endif





#if AS_APP_OS == AS_OS_WIN32
enum tagSockEvent
{
    EPOLLIN  = 0x1,
    EPOLLOUT = 0x2
};

#ifndef INET_ADDRSTRLEN
#define  INET_ADDRSTRLEN 16
#endif

#ifndef MSG_WAITALL
#define MSG_WAITALL 0
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef socklen_t
typedef int socklen_t;
#endif

#endif //#if win32

class as_network_addr
{
  public:
    as_network_addr();
    virtual ~as_network_addr();
  public:
    long m_lIpAddr;
    USHORT m_usPort;
};

typedef enum tagConnStatus
{
    enIdle = 0,
    enConnecting,
    enConnected,
    enConnFailed,
    enClosed
} ConnStatus;

typedef enum tagEnumSyncAsync
{
    enSyncOp = 1,
    enAsyncOp = 0
} EnumSyncAsync;

typedef enum tagEpollEventType
{
    enEpollRead = 0,
    enEpollWrite = 1
} EpollEventType;

class as_handle;
class as_handle_node;
typedef std::list<as_handle_node *> ListOfHandle;
typedef ListOfHandle::iterator ListOfHandleIte;

class as_handle
{
  public:
    as_handle();
    virtual ~as_handle();

  public:
    virtual long initHandle(void);
    virtual void setHandleSend(AS_BOOLEAN bHandleSend);
    virtual void setHandleRecv(AS_BOOLEAN bHandleRecv);
    ULONG getEvents(void)
    {
        if(m_pMutexHandle != NULL)
        {
            (void)as_mutex_lock(m_pMutexHandle);
        }

        ULONG ulEvents = m_ulEvents;

        if(m_pMutexHandle != NULL)
        {
            (void)as_mutex_unlock(m_pMutexHandle);
        }
        return ulEvents;
    };
    virtual void close(void);

  public:
    long m_lSockFD;
    as_handle_node *m_pHandleNode;
    as_network_addr m_localAddr;

#if AS_APP_OS == AS_OS_WIN32
    AS_BOOLEAN m_bReadSelected;
    AS_BOOLEAN m_bWriteSelected;
#endif  //#if

#if AS_APP_OS == AS_OS_LINUX
    long m_lEpfd;
#endif  //#if
    ULONG m_ulEvents;
    as_mutex_t *m_pMutexHandle;
};

class as_handle_node
{
  public:
    as_handle_node()
    {
        m_pHandle = NULL;
        m_bRemoved = AS_FALSE;
    };

  public:
    as_handle *m_pHandle;
    AS_BOOLEAN m_bRemoved;
};

class as_network_handle : public as_handle
{
  public:
    as_network_handle();
    virtual ~as_network_handle(){};

  public:
    virtual long initHandle(void);
    long getSockFD(void) const    /*lint -e1714*///�ӿں��������಻����
    {
        return m_lSockFD;
    };
    void setSockFD(long lSockFD)
    {
        m_lSockFD = lSockFD;
    };
#if AS_APP_OS == AS_OS_LINUX
    long sendMsg(const struct msghdr *pMsg);
#endif
    virtual long recv(char *pArrayData, as_network_addr *pPeerAddr, const ULONG ulDataSize,
        const EnumSyncAsync bSyncRecv) = 0;

  public:
    virtual void handle_recv(void) = 0;
    virtual void handle_send(void) = 0;
};

class as_tcp_conn_handle : public as_network_handle
{
  public:
    as_tcp_conn_handle();
    virtual ~as_tcp_conn_handle();

  public:
    virtual long initHandle(void);
    virtual long conn( const as_network_addr *pLocalAddr, const as_network_addr *pPeerAddr,
        const EnumSyncAsync bSyncConn, ULONG ulTimeOut);
    virtual long send(const char *pArrayData, const ULONG ulDataSize,
        const EnumSyncAsync bSyncSend);
    virtual long recv(char *pArrayData, as_network_addr *pPeerAddr, const ULONG ulDataSize,
        const EnumSyncAsync bSyncRecv);
    virtual long recvWithTimeout(char *pArrayData, as_network_addr *pPeerAddr,
        const ULONG ulDataSize, const ULONG ulTimeOut, const ULONG ulSleepTime);
    virtual ConnStatus getStatus(void) const
    {
        return m_lConnStatus;
    };
    virtual void close(void);

  public:
    ConnStatus m_lConnStatus;
    as_network_addr m_peerAddr;
};

class as_udp_sock_handle : public as_network_handle
{
  public:
    virtual long createSock(const as_network_addr *pLocalAddr,
                               const as_network_addr *pMultiAddr);
    virtual long send(const as_network_addr *pPeerAddr, const char *pArrayData,
         const ULONG ulDataSize, const EnumSyncAsync bSyncSend);
    virtual long recv(char *pArrayData, as_network_addr *pPeerAddr, const ULONG ulDataSize,
        const EnumSyncAsync bSyncRecv);
    virtual void close(void);
    virtual long recvWithTimeout(char *pArrayData,
                                        as_network_addr *pPeerAddr,
                                        const ULONG ulDataSize,
                                        const ULONG ulTimeOut,
                                        const ULONG ulSleepTime);
};

class as_tcp_server_handle : public as_handle
{
  public:
    long listen(const as_network_addr *pLocalAddr);

  public:
    virtual long handle_accept(const as_network_addr *pRemoteAddr,
        as_tcp_conn_handle *&pTcpConnHandle) = 0;
    virtual void close(void);
};


#define MAX_HANDLE_MGR_TYPE_LEN 20
class  as_handle_manager
{
  public:
    as_handle_manager();
    virtual ~as_handle_manager();

  public:
    long init(const ULONG ulMilSeconds);
    long run();
    void exit();

  public:
    long addHandle(as_handle *pHandle,
                      AS_BOOLEAN bIsListOfHandleLocked = AS_FALSE);
    void removeHandle(as_handle *pHandle);
    virtual void checkSelectResult(const EpollEventType enEpEvent,
        as_handle *pHandle) = 0;

  protected:
    static void *invoke(void *argc);
    void mainLoop();

  protected:
    ListOfHandle m_listHandle;
    as_mutex_t *m_pMutexListOfHandle;

#if AS_APP_OS == AS_OS_LINUX
    long m_lEpfd; //����epoll�ľ��
    struct epoll_event m_epEvents[EPOLL_MAX_EVENT];
#elif AS_APP_OS == AS_OS_WIN32
    fd_set m_readSet;
    fd_set m_writeSet;
    timeval m_stSelectPeriod;            //select����
#endif

    ULONG m_ulSelectPeriod;
    as_thread_t *m_pSVSThread;
    AS_BOOLEAN m_bExit;
    char m_szMgrType[MAX_HANDLE_MGR_TYPE_LEN+1];
};

class as_tcp_conn_mgr : public as_handle_manager
{
  public:
    as_tcp_conn_mgr()
    {
        (void)strncpy(m_szMgrType, "as_tcp_conn_mgr", MAX_HANDLE_MGR_TYPE_LEN);
    };
    void lockListOfHandle();
    void unlockListOfHandle();

  protected:
    virtual void checkSelectResult(const EpollEventType enEpEvent,
                            as_handle *pHandle);  /*lint !e1768*///��Ҫ�������θýӿ�
};

class as_udp_sock_mgr : public as_handle_manager
{
  public:
    as_udp_sock_mgr()
    {
        (void)strncpy(m_szMgrType, "as_udp_sock_mgr", MAX_HANDLE_MGR_TYPE_LEN);
    };

  protected:
    virtual void checkSelectResult(const EpollEventType enEpEvent,
        as_handle *pHandle);  /*lint !e1768*///��Ҫ�������θýӿ�
};

class as_tcp_server_mgr : public as_handle_manager
{
  public:
    as_tcp_server_mgr()
    {
        m_pTcpConnMgr = NULL;
        (void)strncpy(m_szMgrType, "as_tcp_server_mgr", MAX_HANDLE_MGR_TYPE_LEN);
    };

  public:
    void setTcpClientMgr(as_tcp_conn_mgr *pTcpConnMgr)
    {
        m_pTcpConnMgr = pTcpConnMgr;
    };

  protected:
    virtual void checkSelectResult(const EpollEventType enEpEvent,
        as_handle *pHandle);  /*lint !e1768*///��Ҫ�������θýӿ�

  protected:
    as_tcp_conn_mgr *m_pTcpConnMgr;
};

#define DEFAULT_SELECT_PERIOD 20

// 4����־����
#define    CONN_OPERATOR_LOG    16
#define    CONN_RUN_LOG         17
#define    CONN_SECURITY_LOG    20
#define    CONN_USER_LOG        19


// 4����־����
enum CONN_LOG_LEVEL
{
    CONN_EMERGENCY = 0,
    CONN_ERROR = 3,
    CONN_WARNING = 4,
    CONN_DEBUG = 6
};

class as_conn_mgr_log
{
  public:
    virtual void writeLog(long lType, long llevel,
        const char *szLogDetail, const long lLogLen) = 0;
};

extern as_conn_mgr_log *g_pAsConnMgrLog;

class as_conn_mgr
{
  public:
    as_conn_mgr();
    virtual ~as_conn_mgr();
protected:
    as_conn_mgr();
public:
    virtual long init(const ULONG ulSelectPeriod, const AS_BOOLEAN bHasUdpSock,
        const AS_BOOLEAN bHasTcpClient, const AS_BOOLEAN bHasTcpServer);
    virtual void setLogWriter(as_conn_mgr_log *pConnMgrLog) const
    {
        g_pAsConnMgrLog = pConnMgrLog;
    };
    virtual long run(void);
    virtual void exit(void);

public:
    virtual void setDefaultLocalAddr(const char *szLocalIpAddr);
    virtual long regTcpClient( const as_network_addr *pLocalAddr,
        const as_network_addr *pPeerAddr, as_tcp_conn_handle *pTcpConnHandle,
        const EnumSyncAsync bSyncConn, ULONG ulTimeOut);
    virtual void removeTcpClient(as_tcp_conn_handle *pTcpConnHandle);
    virtual long regTcpServer(const as_network_addr *pLocalAddr,
        as_tcp_server_handle *pTcpServerHandle);
    virtual void removeTcpServer(as_tcp_server_handle *pTcpServerHandle);
    virtual long regUdpSocket(const as_network_addr *pLocalAddr,
                                 as_udp_sock_handle *pUdpSockHandle,
                                 const as_network_addr *pMultiAddr= NULL);
    virtual void removeUdpSocket(as_udp_sock_handle *pUdpSockHandle);

protected:
    long               m_lLocalIpAddr;
    as_tcp_conn_mgr   *m_pTcpConnMgr;
    as_udp_sock_mgr   *m_pUdpSockMgr;
    as_tcp_server_mgr *m_pTcpServerMgr;
};

#endif //__AS_CONN_MGR_H_INCLUDE__

