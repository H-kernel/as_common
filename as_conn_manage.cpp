/******************************************************************************
   ��Ȩ���� (C), 2001-2011, M.Kernel

 ******************************************************************************
  �ļ���          : as_conn_manage.cpp
  �汾��          : 1.0
  ����            : hexin
  ��������        : 2007-4-10
  ����޸�        :
  ��������        : ���ӹ���ģ��
  �����б�        :
  �޸���ʷ        :
  1 ����          :
    ����          :
    �޸�����      :
*******************************************************************************/

#ifndef WIN32
#include <sys/epoll.h>
#include <netinet/tcp.h>
#endif

#include "as_conn_manage.h"

#include <stdarg.h>

IConnMgrLog *g_pConnMgrLog = NULL;
#define MAX_CONN_LOG_LENTH 512
#define CONN_SECOND_IN_MS   1000
#define CONN_MS_IN_US   1000


//�ļ��к�
#define _FL_ __FILE__, __LINE__

/*******************************************************************************
  Function:       CONN_WRITE_LOG()
  Description:    ��־��ӡ����
  Calls:
  Called By:
  Input:          ��printfһ��
  Output:         ��
  Return:         ��
*******************************************************************************/
void CONN_WRITE_LOG(long lLevel, const char *format, ...)
{
    //���û��ע����־��ӡ����������
    if(NULL == g_pConnMgrLog)
    {
        return;
    }

    //����ӡ������֯���ַ���
    char buff[MAX_CONN_LOG_LENTH + 1];
    buff[0] = '\0';

    va_list args;
    va_start (args, format);
    long lPrefix = snprintf (buff, MAX_CONN_LOG_LENTH, "errno:%d.thread(%lu):",
        CONN_ERRNO, (long)as_thread_self());
    if(lPrefix < MAX_CONN_LOG_LENTH)
    {
        (void)vsnprintf (buff + lPrefix, (ULONG)(MAX_CONN_LOG_LENTH - lPrefix),
                    format, args);
    }
    buff[MAX_CONN_LOG_LENTH] = '\0';

    //������־��ӡ�ӿڴ�ӡ
    g_pConnMgrLog->writeLog(CONN_RUN_LOG, lLevel, buff, (long)strlen(buff));
    va_end (args);
}

/*******************************************************************************
  Function:       CNetworkAddr::CNetworkAddr()
  Description:    ���캯��
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
CNetworkAddr::CNetworkAddr()
{
    m_lIpAddr = InvalidIp;
    m_usPort = Invalidport;
}

/*******************************************************************************
  Function:       CNetworkAddr::~CNetworkAddr()
  Description:    ��������
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
CNetworkAddr::~CNetworkAddr()
{
}

/*******************************************************************************
  Function:       CHandle::CHandle()
  Description:    ���캯��
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
CHandle::CHandle()
{
    m_lSockFD = InvalidSocket;
    m_pHandleNode = NULL;
    m_ulEvents = EPOLLIN;
#if AS_APP_OS == AS_OS_LINUX
    m_lEpfd = InvalidFd;
#endif //#if

#if AS_APP_OS == AS_OS_WIN32
    m_bReadSelected = AS_FALSE;
    m_bWriteSelected = AS_FALSE;
#endif  //#if

    m_pMutexHandle = as_create_mutex();
}

/*******************************************************************************
  Function:       CHandle::~CHandle()
  Description:    ��������
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
CHandle::~CHandle()
{
    try
    {
        if(NULL != m_pHandleNode)
        {
            AS_DELETE(m_pHandleNode);
            m_pHandleNode = NULL;
        }

        if(NULL != m_pMutexHandle)
        {
            (void)as_destroy_mutex(m_pMutexHandle);
            m_pMutexHandle = NULL;
        }
    }
    catch (...)
    {
    }
}

/*******************************************************************************
  Function:       CHandle::initHandle()
  Description:    ��ʼ������
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: init success
  AS_ERROR_CODE_FAIL: init fail
*******************************************************************************/
long CHandle::initHandle(void)
{
    //��ֹ����handleʱδ�ر�����
    if(NULL != m_pMutexHandle)
    {
        this->close();
    }

    m_lSockFD = InvalidSocket;
    m_pHandleNode = NULL;
#if AS_APP_OS == AS_OS_LINUX
    m_lEpfd = InvalidFd;
#endif
    m_ulEvents = EPOLLIN;
    if(NULL == m_pMutexHandle)
    {
        m_pMutexHandle = as_create_mutex();
    }

    if(NULL == m_pMutexHandle)
    {
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}

/*******************************************************************************
  Function:       CHandle::setHandleSend()
  Description:    �����Ƿ���д�¼�
  Calls:
  Called By:
  Input:          bHandleSend: SVS_TRUE��ʾ��⣬SVS_FALSE��ʾ�����
  Output:         ��
  Return:         ��
*******************************************************************************/
void CHandle::setHandleSend(AS_BOOLEAN bHandleSend)
{
    if(m_pMutexHandle != NULL)
    {
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexHandle))
        {
            return;
        }
    }

    //����Ҫ������¼�����
    if(AS_FALSE == bHandleSend)
    {
        m_ulEvents = m_ulEvents & (~EPOLLOUT);
    }
    else
    {
        m_ulEvents = m_ulEvents | EPOLLOUT;
    }

    if((m_pHandleNode != NULL) && (m_lSockFD != InvalidSocket))
    {
#if AS_APP_OS == AS_OS_LINUX
        //��handle��ӵ�epoll�ļ�����
        struct epoll_event epEvent;
        memset(&epEvent, 0, sizeof(epEvent));
        //������Ҫ������¼���ص��ļ�������
        epEvent.data.ptr = (void *)m_pHandleNode;
        //����Ҫ������¼�����
        epEvent.events = m_ulEvents;
        //�޸�ע���epoll�¼�
        if ( 0 != epoll_ctl(m_lEpfd, EPOLL_CTL_MOD, m_lSockFD, &epEvent))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CHandle::setHandleSend: modify event fail, "
                "m_lSockFD = %d", _FL_, m_lSockFD);
        }
#endif
    }

    if(m_pMutexHandle != NULL)
    {
        (void)as_mutex_unlock(m_pMutexHandle);
    }
}

/*******************************************************************************
  Function:       CHandle::setHandleRecv()
  Description:    �����Ƿ�����¼�
  Calls:
  Called By:
  Input:          bHandleRecv: SVS_TRUE��ʾ��⣬SVS_FALSE��ʾ�����
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: init success
  AS_ERROR_CODE_FAIL: init fail
*******************************************************************************/
void CHandle::setHandleRecv(AS_BOOLEAN bHandleRecv)
{
    if(m_pMutexHandle != NULL)
    {
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexHandle))
        {
            return;
        }
    }

    //����Ҫ������¼�����
    if(AS_FALSE == bHandleRecv)
    {
        m_ulEvents = m_ulEvents & (~EPOLLIN);
    }
    else
    {
        m_ulEvents = m_ulEvents | EPOLLIN;
    }

    if((m_pHandleNode != NULL) && (m_lSockFD != InvalidSocket))
    {
#if AS_APP_OS == AS_OS_LINUX
        //��handle��ӵ�epoll�ļ�����
        struct epoll_event epEvent;
        memset(&epEvent, 0, sizeof(epEvent));
        //������Ҫ������¼���ص��ļ�������
        epEvent.data.ptr = (void *)m_pHandleNode;
        //����Ҫ������¼�����
        epEvent.events = m_ulEvents;
        //�޸�ע���epoll�¼�
        if ( 0 != epoll_ctl(m_lEpfd, EPOLL_CTL_MOD, m_lSockFD, &epEvent))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CHandle::setHandleRecv: modify event fail, "
                "m_lSockFD = %d", _FL_, m_lSockFD);
        }
#endif
    }

    if(m_pMutexHandle != NULL)
    {
        (void)as_mutex_unlock(m_pMutexHandle);
    }
}

/*******************************************************************************
  Function:       CHandle::close()
  Description:    �ر���������
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
void CHandle::close(void)
{
    if (InvalidSocket != m_lSockFD)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        m_lSockFD = InvalidSocket;
    }

    return;
}

/*******************************************************************************
  Function:       CNetworkHandle::CNetworkHandle()
  Description:    ���ں���
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
CNetworkHandle::CNetworkHandle()
{
    m_lSockFD = InvalidSocket;
}

/*******************************************************************************
  Function:       CNetworkHandle::initHandle()
  Description:    �ر���������
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
long CNetworkHandle::initHandle(void)
{
    if (AS_ERROR_CODE_OK != CHandle::initHandle())
    {
        return AS_ERROR_CODE_FAIL;
    }
    m_lSockFD = InvalidSocket;

    return AS_ERROR_CODE_OK;
}

#if AS_APP_OS == AS_OS_LINUX
/*******************************************************************************
  Function:       CNetworkHandle::sendMsg()
  Description:    ����ʸ������
  Calls:
  Called By:
  Input:          ʸ������
  Output:         ��
  Return:         �μ�ϵͳ����sendmsg
*******************************************************************************/
long CNetworkHandle::sendMsg(const struct msghdr *pMsg)
{
    if (InvalidSocket == m_lSockFD)
    {
        return SendRecvError;
    }

    return ::sendmsg(m_lSockFD, pMsg, 0);
}
#endif

/*******************************************************************************
  Function:       CTcpConnHandle::CTcpConnHandle()
  Description:    ���캯��
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
CTcpConnHandle::CTcpConnHandle()
{
    m_lConnStatus = enIdle;
}

/*******************************************************************************
  Function:       CTcpConnHandle::~CTcpConnHandle()
  Description:    ��������
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
CTcpConnHandle::~CTcpConnHandle()
{
    try
    {
        if (InvalidSocket != m_lSockFD)
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CHandle::~CHandle: handle not released, "
                "m_lSockFD = %d, peer_ip(0x%x), peer_port(%d)", _FL_, m_lSockFD,
                ntohl((ULONG)(m_peerAddr.m_lIpAddr)), ntohs(m_peerAddr.m_usPort));
            (void)CLOSESOCK((SOCKET)m_lSockFD);
            m_lSockFD = InvalidSocket;
        }
    }
    catch (...)
    {
    }
}

/*******************************************************************************
  Function:       CTcpConnHandle::initHandle()
  Description:    ��ʼ������
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: init success
  AS_ERROR_CODE_FAIL: init fail
*******************************************************************************/
long CTcpConnHandle::initHandle(void)
{
    if (AS_ERROR_CODE_OK != CNetworkHandle::initHandle())
    {
        return AS_ERROR_CODE_FAIL;
    }

    m_lConnStatus = enIdle;
    return AS_ERROR_CODE_OK;
}

/*******************************************************************************
  Function:       CTcpConnHandle::conn()
  Description:    �������Ӻ���
  Calls:
  Called By:
  Input:          pLocalAddr: ���ص�ַ��pPeerAddr: �Զ˵�ַ��
                  bSyncConn: SVS_TRUE��ʾͬ�����ӣ�SVS_FALSE��ʾ�첽����
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: connect success
  AS_ERROR_CODE_FAIL: connect fail
*******************************************************************************/
long CTcpConnHandle::conn(const CNetworkAddr *pLocalAddr,
    const CNetworkAddr *pPeerAddr, const EnumSyncAsync bSyncConn, ULONG ulTimeOut)
{
    m_lConnStatus = enConnFailed;

    long lSockFd = (long)socket(AF_INET, SOCK_STREAM, 0);
    if(lSockFd < 0)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "opening client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }
    //setSendBufSize
    long lSendBufSize = DEFAULT_TCP_SENDRECV_SIZE;
    socklen_t lSendBufLength = sizeof(lSendBufSize);
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_SNDBUF, (char*)&lSendBufSize,
        lSendBufLength) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setSendBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }
    //setRecBufSize
    long lRecvBufSize = DEFAULT_TCP_SENDRECV_SIZE;
    socklen_t lRecvBufLength = sizeof(lRecvBufSize);
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_RCVBUF, (char*)&lRecvBufSize,
        lRecvBufLength) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setRecvBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

    long flag = 1;
    if(setsockopt((SOCKET)lSockFd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag,
        sizeof(flag)) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "set TCP_NODELAY client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

    //setReuseAddr();
    long lReuseAddrFlag = 1;
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_REUSEADDR, (char*)&lReuseAddrFlag,
        sizeof(lReuseAddrFlag)) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

    //�󶨱��ص�ַ
    if(((ULONG)(pLocalAddr->m_lIpAddr) != InvalidIp)
        && ( pLocalAddr->m_usPort != Invalidport))
    {
        struct sockaddr_in  serverAddr;
        memset((char *)&serverAddr, 0, (long)sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = (unsigned long)pLocalAddr->m_lIpAddr;
        serverAddr.sin_port = pLocalAddr->m_usPort;
        errno = 0;
        if (0 > bind ((SOCKET)lSockFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)))
        {
#if AS_APP_OS == AS_OS_LINUX
            char szServerAddr[INET_ADDRSTRLEN];
            if (NULL != inet_ntop(AF_INET, &serverAddr.sin_addr, szServerAddr,
                sizeof(szServerAddr)))
#elif AS_APP_OS == AS_OS_WIN32
            char *szServerAddr = inet_ntoa(serverAddr.sin_addr);
            if (NULL != szServerAddr)
#endif
            {
                CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                    "Can not Bind Data_Sock %s:%d", _FL_,
                    szServerAddr, ntohs(serverAddr.sin_port));
            }
            else
            {
                CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                    "Can not Bind Data_Sock %d:%d", _FL_,
                    serverAddr.sin_addr.s_addr, ntohs(serverAddr.sin_port));
            }

            (void)CLOSESOCK((SOCKET)lSockFd);
            return AS_ERROR_CODE_FAIL;
        }
    }

    //������첽���ӣ�����Ϊ������ģʽ
    errno = 0;
    if((enAsyncOp == bSyncConn) || (ulTimeOut > 0))
    {
#if AS_APP_OS == AS_OS_LINUX
        //����Ϊ������
        if(fcntl(lSockFd, F_SETFL, fcntl(lSockFd, F_GETFL)|O_NONBLOCK) < 0)
#elif AS_APP_OS == AS_OS_WIN32
        ULONG ulNoBlock = AS_TRUE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)lSockFd,(long)(long)FIONBIO,&ulNoBlock))
#endif
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "fcntl client socket error(%d)", _FL_, CONN_ERRNO);
            (void)CLOSESOCK((SOCKET)lSockFd);
            return AS_ERROR_CODE_FAIL;
        }
        setHandleSend(AS_TRUE);
    }

    //���ӶԶ�
    struct sockaddr_in  peerAddr;
    memset((char *)&peerAddr, 0, (long)sizeof(peerAddr));
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_addr.s_addr = (UINT)pPeerAddr->m_lIpAddr;
    peerAddr.sin_port = pPeerAddr->m_usPort;
    long lRetVal = ::connect((SOCKET)lSockFd,(struct sockaddr*)&peerAddr,
        sizeof(peerAddr));
    if( lRetVal < 0)
    {
        if((enSyncOp == bSyncConn) && (0 == ulTimeOut))
        {
            (void)CLOSESOCK((SOCKET)lSockFd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "SyncConn server fail. error(%d):%s",
                _FL_, CONN_ERRNO, strerror(CONN_ERRNO));
            return AS_ERROR_CODE_FAIL;

        }
        if((EINPROGRESS != CONN_ERRNO) && (EWOULDBLOCK != CONN_ERRNO))
        {
            (void)CLOSESOCK((SOCKET)lSockFd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "AsyncConn server fail. error(%d):%s", _FL_,
                CONN_ERRNO, strerror(CONN_ERRNO));
            return AS_ERROR_CODE_FAIL;
        }

        if(enSyncOp == bSyncConn)
        {
            fd_set    fdWriteReady;
            struct timeval waitTime;
            waitTime.tv_sec = (long)ulTimeOut;
            waitTime.tv_usec = 0;
            FD_ZERO(&fdWriteReady);
            FD_SET((SOCKET)lSockFd, &fdWriteReady);
            long lSelectResult = select(FD_SETSIZE, (fd_set*)0, &fdWriteReady,
                (fd_set*)0, &waitTime);
            if(lSelectResult <= 0)
            {
                CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                    "wait client socket(%d) time out", _FL_, lSockFd);
                (void)CLOSESOCK((SOCKET)lSockFd);
                return AS_ERROR_CODE_FAIL;
            }
            long lErrorNo = 0;
            socklen_t len = sizeof(lErrorNo);
            if (getsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_ERROR,
                (SOCK_OPT_TYPE *)&lErrorNo, &len) < 0)
            {
                CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                    "getsockopt of sockfd(%d) has wrong when wait client",
                    _FL_, lSockFd);
                (void)CLOSESOCK((SOCKET)lSockFd);
                return AS_ERROR_CODE_FAIL;
            }
            else if (lErrorNo != 0)
            {
                CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                    "wait client: socket(%d) connect fail", _FL_, lSockFd);
                (void)CLOSESOCK((SOCKET)lSockFd);
                return AS_ERROR_CODE_FAIL;
            }

            CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
                "connect server OK. socket id = %d", _FL_, lSockFd);
            m_lConnStatus = enConnected;
        }
    }
    else
    {
        CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
            "connect server OK. socket id = %d", _FL_, lSockFd);
    }

    //�������Ϊ������ģʽ���ָ�Ϊ����ģʽ
    if((enAsyncOp == bSyncConn) || (ulTimeOut > 0))
    {
#if AS_APP_OS == AS_OS_LINUX
        //�ָ�Ϊ����ģʽ
        if(fcntl(lSockFd, F_SETFL, fcntl(lSockFd, F_GETFL&(~O_NONBLOCK))) < 0)
#elif AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = 0;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)lSockFd,(long)FIONBIO,&ulBlock))
#endif
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "fcntl client socket error(%d)", _FL_, CONN_ERRNO);
            (void)CLOSESOCK((SOCKET)lSockFd);
            return AS_ERROR_CODE_FAIL;
        }

        if(enAsyncOp == bSyncConn)
        {
            m_lConnStatus = enConnecting;
        }
    }
    else
    {
        m_lConnStatus = enConnected;
    }

    m_lSockFD = lSockFd;

    m_peerAddr.m_lIpAddr = pPeerAddr->m_lIpAddr;
    m_peerAddr.m_usPort= pPeerAddr->m_usPort;

    CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): "
        "CTcpConnHandle::conn: connect success, "
        "m_lSockFD = %d, peer_ip(0x%x), peer_port(%d)", _FL_, m_lSockFD,
        ntohl((ULONG)(m_peerAddr.m_lIpAddr)), ntohs(m_peerAddr.m_usPort));

    return AS_ERROR_CODE_OK;
}

/*******************************************************************************
  Function:       CTcpConnHandle::send()
  Description:    ���ͺ���
  Calls:
  Called By:
  Input:          pArrayData: ����buffer��ulDataSize: ���ݳ��ȣ�
                  bSyncSend: SVS_TRUE��ʾͬ�����ͣ�SVS_FALSE��ʾ�첽����
  Output:         ��
  Return:
  lBytesSent: �����ֽ���(>0)
  SendRecvError: ����ʧ��
*******************************************************************************/
long CTcpConnHandle::send(const char *pArrayData, const ULONG ulDataSize,
    const EnumSyncAsync bSyncSend)
{
    if (InvalidSocket == m_lSockFD)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CTcpConnHandle::send: socket is invalid, send fail", _FL_);
        return SendRecvError;
    }

    errno = 0;
    long lBytesSent = 0;
    if(enSyncOp == bSyncSend)
    {
        //ͬ������
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_FALSE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(long)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket Block fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesSent = ::send((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize, MSG_NOSIGNAL);
    }
    else
    {
        //�첽����
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_TRUE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(long)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket NoBlock fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesSent = ::send((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize,
            MSG_DONTWAIT|MSG_NOSIGNAL);
        //��ʼ����Ƿ���Է�������
        setHandleSend(AS_TRUE);
    }

    //�������ʧ�ܣ������ж��Ƿ�����Ϊ�������ǵĻ����ط���0�ֽڣ�����ر�����
    if (lBytesSent < 0)
    {
#if AS_APP_OS == AS_OS_WIN32
        if ((EWOULDBLOCK == CONN_ERRNO) || (EAGAIN == CONN_ERRNO) || (WSAEWOULDBLOCK == CONN_ERRNO))

#else
        if ((EWOULDBLOCK == CONN_ERRNO) || (EAGAIN == CONN_ERRNO) )
#endif
        {
            return 0;
        }
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CTcpConnHandle::send to peer(IP:0x%x, Port:%d) "
            "Error(%d): %s",  _FL_, ntohl((ULONG)(m_peerAddr.m_lIpAddr)),
           ntohs(m_peerAddr.m_usPort), CONN_ERRNO, strerror(CONN_ERRNO));

        (void)CLOSESOCK((SOCKET)m_lSockFD);
        m_lSockFD = InvalidSocket;
        return SendRecvError;
    }

    return lBytesSent;
}

/*******************************************************************************
  Function:       CTcpConnHandle::recv()
  Description:    ���պ���
  Calls:
  Called By:
  Input:          pArrayData: ����buffer��ulDataSize: ���ݳ��ȣ�
                  bSyncRecv: SVS_TRUE��ʾͬ�����ͣ�SVS_FALSE��ʾ�첽����
  Output:         pArrayData: ����buffer��pPeerAddr: �Զ˵�ַ��
  Return:
  lBytesSent: �����ֽ���(>0)
  SendRecvError: ����ʧ��
*******************************************************************************/
long CTcpConnHandle::recv(char *pArrayData, CNetworkAddr *pPeerAddr,
    const ULONG ulDataSize, const EnumSyncAsync bSyncRecv)
{
    if (InvalidSocket == m_lSockFD)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CTcpConnHandle::recv: socket is invalid, recv fail", _FL_);
        return SendRecvError;
    }

    errno = 0;
    long lBytesRecv = 0;
    if(enSyncOp == bSyncRecv)
    {
        //ͬ������
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_FALSE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(long)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket Block fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesRecv = ::recv((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize, MSG_WAITALL);
    }
    else
    {
        //�첽����
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_TRUE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(long)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket NoBlock fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesRecv = ::recv((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize, MSG_DONTWAIT);
    }

    //�������0����ʾ�Ѿ�����
    if (0 == lBytesRecv)
    {
        CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): recv EOF!",
            _FL_);
        return SendRecvError;
    }

    //���С��0�������ж��Ƿ�����Ϊ�������ǵĻ����ؽ���0�ֽڣ������������
    if (lBytesRecv < 0)
    {
        if((EWOULDBLOCK == CONN_ERRNO) || (EAGAIN == CONN_ERRNO))
        {
            return 0;
        }
        CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
            "recv error. Error(%d): %s", _FL_, CONN_ERRNO, strerror(CONN_ERRNO));
        return SendRecvError;
    }

    pPeerAddr->m_lIpAddr = m_peerAddr.m_lIpAddr;
    pPeerAddr->m_usPort = m_peerAddr.m_usPort;

    return lBytesRecv;
}

/*******************************************************************************
  Function:       CTcpConnHandle::recvWithTimeout()
  Description:    ���պ���
  Calls:
  Called By:
  Input:          pArrayData: ����buffer��ulDataSize: ���ݳ��ȣ�
                  ulTimeOut: �ȴ�ʱ��, ulSleepTime: �����(ms)
  Output:         pArrayData: ����buffer��pPeerAddr: �Զ˵�ַ��
  Return:
  lBytesSent: �����ֽ���(>0)
  SendRecvError: ����ʧ��
*******************************************************************************/
long CTcpConnHandle::recvWithTimeout(char *pArrayData, CNetworkAddr *pPeerAddr,
    const ULONG ulDataSize, const ULONG ulTimeOut, const ULONG ulSleepTime)
{
    (void)ulSleepTime;//��PC-LINT
    long lRecvBytes = 0;
    ULONG ulTotalRecvBytes = 0;
    ULONG ulWaitTime = ulTimeOut;
    errno = 0;
    //����socket��ʱʱ��
#if AS_APP_OS == AS_OS_WIN32

    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
        (char *) &ulWaitTime, sizeof(int)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }

#elif AS_APP_OS == AS_OS_LINUX

    struct timeval recvWaitTime;
    recvWaitTime.tv_sec = ulWaitTime/CONN_SECOND_IN_MS;
    recvWaitTime.tv_usec = (ulWaitTime%CONN_SECOND_IN_MS)*CONN_MS_IN_US;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
        (char *) &recvWaitTime, sizeof(recvWaitTime)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }

#endif

    //windows����ط��޷�ȫ�������꣬��Ϊѭ������
    //lRecvBytes = this->recv(pArrayData, pPeerAddr, ulDataSize, enSyncOp);

    //windows ѭ������ linux�˴�ֻ����һ��ѭ��
    if(NULL == pArrayData)
    {
        return SendRecvError;
    }

    while(ulTotalRecvBytes<ulDataSize)
    {
        lRecvBytes = this->recv(pArrayData+ulTotalRecvBytes,pPeerAddr, ulDataSize-ulTotalRecvBytes, enSyncOp);
        if(lRecvBytes < 0)
        {
            break;
        }

        ulTotalRecvBytes += (unsigned long)lRecvBytes;
    }


    if(lRecvBytes < 0)
    {
        CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): "
            "CTcpConnHandle::recvWithTimeout: socket closed when receive. "
            "m_lSockFD = %d, peer_ip(0x%x), peer_port(%d) "
            "errno = %d, error: %s", _FL_, m_lSockFD,
            ntohl((ULONG)(m_peerAddr.m_lIpAddr)), ntohs(m_peerAddr.m_usPort),
            CONN_ERRNO, strerror(CONN_ERRNO) );
        if(CONN_ERR_TIMEO == CONN_ERRNO)
        {
            return SendRecvErrorTIMEO;
        }
        if(CONN_ERR_EBADF == CONN_ERRNO)
        {
            return SendRecvErrorEBADF;
        }
        return SendRecvError;
    }

    if(ulTotalRecvBytes <  ulDataSize)//˵�����ܳ�ʱ
    {
        CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): "
            "CTcpConnHandle::recvWithTimeout: recv time out. "
            "m_lSockFD = %d, peer_ip(0x%x), peer_port(%d) recv_msg_len(%lu)"
            "ulDataSize(%lu) errno = %d, error: %s", _FL_, m_lSockFD,
            ntohl((ULONG)(m_peerAddr.m_lIpAddr)), ntohs(m_peerAddr.m_usPort),
            ulTotalRecvBytes, ulDataSize,CONN_ERRNO, strerror(CONN_ERRNO) );
        return SendRecvError;
    }

    //����socket��ʱʱ��Ϊ0��ʾ���õȴ�
#if AS_APP_OS == AS_OS_WIN32

    ulWaitTime = 0;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
        (char *) &ulWaitTime, sizeof(int)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }

#elif AS_APP_OS == AS_OS_LINUX

    //����socket��ʱʱ��Ϊ0��ʾ���õȴ�
    recvWaitTime.tv_sec = 0;
    recvWaitTime.tv_usec = 0;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
        (char *) &recvWaitTime, sizeof(recvWaitTime)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }
#endif

    return (long)ulTotalRecvBytes;

}

/*******************************************************************************
  Function:       CTcpConnHandle::close()
  Description:    �ر�����
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
void CTcpConnHandle::close(void)
{
    if(m_pMutexHandle != NULL)
    {
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexHandle))
        {
            return;
        }
    }

    if (InvalidSocket != m_lSockFD)
    {
        CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): "
            "CTcpConnHandle::close: close connection, "
            "m_lSockFD = %d, peer_ip(0x%x), "
            "peer_port(%d) this(0x%x) m_pHandleNode(0x%x)",
            _FL_, m_lSockFD,
            ntohl((ULONG)(m_peerAddr.m_lIpAddr)), ntohs(m_peerAddr.m_usPort), this,
            this->m_pHandleNode);

        //The close of an fd will cause it to be removed from
        //all epoll sets automatically.
#if AS_APP_OS == AS_OS_LINUX
        struct epoll_event epEvent;
        memset(&epEvent, 0, sizeof(epEvent));
        epEvent.data.ptr = (void *)NULL;
        //����Ҫ������¼�����
        epEvent.events = (EPOLLIN | EPOLLOUT);
        //�޸�ע���epoll�¼�
        if ( 0 != epoll_ctl(m_lEpfd, EPOLL_CTL_MOD, m_lSockFD, &epEvent))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CHandle::setHandleSend: modify event fail, "
                "m_lSockFD = %d", _FL_, m_lSockFD);
        }
        //������Ҫ������¼���ص��ļ�������
        //epEvent.data.ptr = (void *)m_pHandleNode;
        //����Ҫ������¼�����
        //epEvent.events = EPOLLIN;
        if ( 0 != epoll_ctl(m_lEpfd, EPOLL_CTL_DEL, m_lSockFD, &epEvent))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CTcpConnHandle::close: epoll_ctl EPOLL_CTL_DEL fail, "
                "m_lSockFD = %d", _FL_, m_lSockFD);
        }
#endif
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        m_lSockFD = InvalidSocket;
    }
    m_lConnStatus = enClosed;

    CHandle::close();

    if(m_pMutexHandle != NULL)
    {
        (void)as_mutex_unlock(m_pMutexHandle);
    }

    return;
}

/*******************************************************************************
  Function:       CUdpSockHandle::createSock()
  Description:    ����UDP socket
  Calls:
  Called By:
  Input:          pLocalAddr:���ص�ַ
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: init success
  AS_ERROR_CODE_FAIL: init fail
*******************************************************************************/
long CUdpSockHandle::createSock(const CNetworkAddr *pLocalAddr,
                                         const CNetworkAddr *pMultiAddr)
{
    long lSockFd = (long)InvalidSocket;
    if ((lSockFd = (long)socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "create udp socket failed, errno = %d, Msg = %s",
            _FL_, CONN_ERRNO, strerror(CONN_ERRNO));

        return AS_ERROR_CODE_FAIL;
    }

    struct sockaddr_in  localAddr;
    memset((char *)&localAddr, 0, (long)sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    if(NULL != pMultiAddr)
    {
        localAddr.sin_addr.s_addr =  INADDR_ANY;
    }
    else
    {
        localAddr.sin_addr.s_addr = (UINT)pLocalAddr->m_lIpAddr;
    }
    localAddr.sin_port = pLocalAddr->m_usPort;

    //�󶨱��ص�ַ
    if (0 > bind ((SOCKET)lSockFd, (struct sockaddr *) &localAddr, sizeof (localAddr)))
    {
#if AS_APP_OS == AS_OS_LINUX
        char szLocalAddr[INET_ADDRSTRLEN];
        if (NULL != inet_ntop(AF_INET, &localAddr.sin_addr, szLocalAddr,
            sizeof(szLocalAddr)))
#elif AS_APP_OS == AS_OS_WIN32
        char *szLocalAddr = inet_ntoa(localAddr.sin_addr);
        if (NULL == szLocalAddr)
#endif
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Can not Bind Data_Sock %s:%d", _FL_, szLocalAddr,
                ntohs(localAddr.sin_port));
        }
        else
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Can not Bind Data_Sock %d:%d", _FL_,
                localAddr.sin_addr.s_addr, ntohs(localAddr.sin_port));
        }
        (void)CLOSESOCK((SOCKET)lSockFd);
        return AS_ERROR_CODE_FAIL;
    }

#if AS_APP_OS == AS_OS_LINUX
    long lReuseAddrFlag = 1;
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_REUSEADDR, (char*)&lReuseAddrFlag,
        sizeof(lReuseAddrFlag)) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setSendBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }
#endif

    long lSendBufSize = DEFAULT_UDP_SENDRECV_SIZE;
    if(setsockopt ((SOCKET)lSockFd, SOL_SOCKET, SO_SNDBUF, (char*)&lSendBufSize,
        sizeof(lSendBufSize)) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setSendBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }
    //setRecBufSize
    long lRecvBufSize = DEFAULT_UDP_SENDRECV_SIZE;
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_RCVBUF, (char*)&lRecvBufSize,
        sizeof(lRecvBufSize)) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setRecvBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

#if AS_APP_OS == AS_OS_LINUX
    //������鲥��ַ�������Ҫ�����鲥
    if(NULL != pMultiAddr)
    {
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = pMultiAddr->m_lIpAddr;
        mreq.imr_interface.s_addr = pLocalAddr->m_lIpAddr;
        if(setsockopt((SOCKET)lSockFd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                             (char *)&mreq, sizeof(mreq))< 0)
        {
            (void)CLOSESOCK((SOCKET)lSockFd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "set IPPROTO_IP IP_ADD_MEMBERSHIP error(%d)", _FL_, CONN_ERRNO);
            return AS_ERROR_CODE_FAIL;
        }
    }
#endif
    m_lSockFD = lSockFd;

    return AS_ERROR_CODE_OK;

}

/*******************************************************************************
  Function:       CUdpSockHandle::send()
  Description:    ���ͺ���
  Calls:
  Called By:
  Input:          pPeerAddr: �Զ˵�ַ��pArrayData: ���ͻ�������ulDataSize:���ݳ���
                  bSyncSend: SVS_TRUE��ʾͬ�����ͣ�SVS_FALSE��ʾ�첽����
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: connect success
  AS_ERROR_CODE_FAIL: connect fail
*******************************************************************************/
long CUdpSockHandle::send(const CNetworkAddr *pPeerAddr, const char *pArrayData,
         const ULONG ulDataSize, const EnumSyncAsync bSyncSend)
{
    if (InvalidSocket == m_lSockFD)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CUdpSockHandle::send: socket is invalid, send fail", _FL_);
        return SendRecvError;
    }

    struct sockaddr_in peerAddr;

    peerAddr.sin_family = AF_INET;
    peerAddr.sin_addr.s_addr = (UINT)pPeerAddr->m_lIpAddr;
    peerAddr.sin_port = pPeerAddr->m_usPort;

    errno = 0;
    long lBytesSent = 0;
    if(enSyncOp == bSyncSend)
    {
        //ͬ������
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_FALSE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(long)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket Block fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesSent = ::sendto((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize,
            (long)MSG_NOSIGNAL, (const struct sockaddr *)&peerAddr, sizeof(peerAddr));
    }
    else
    {
        //�첽����
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_TRUE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(long)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket NoBlock fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesSent = ::sendto((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize,
            (long) MSG_DONTWAIT|MSG_NOSIGNAL,
            (const struct sockaddr *)&peerAddr, sizeof(peerAddr));
        setHandleSend(AS_TRUE);
    }

    //�������ʧ�ܣ������ж��Ƿ�����Ϊ�������ǵĻ����ط���0�ֽڣ�����ر�����
    if (lBytesSent < 0)
    {
        if((EWOULDBLOCK == CONN_ERRNO) || (EAGAIN == CONN_ERRNO))
        {
            return 0;
        }
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CUdpSockHandle::send (%d)bytes to peer(IP:0x%x, Port:%d) Error(%d): %s",
            _FL_, ulDataSize, ntohl((ULONG)(pPeerAddr->m_lIpAddr)),
            ntohs(pPeerAddr->m_usPort),
            CONN_ERRNO, strerror(CONN_ERRNO));

        return SendRecvError;
    }

    return lBytesSent;
}

/*******************************************************************************
  Function:       CUdpSockHandle::recv()
  Description:    ���պ���
  Calls:
  Called By:
  Input:          pArrayData: ����buffer��ulDataSize: ���ݳ��ȣ�
                  bSyncRecv: SVS_TRUE��ʾͬ�����ͣ�SVS_FALSE��ʾ�첽����
  Output:         pArrayData: ����buffer��pPeerAddr: �Զ˵�ַ��
  Return:
  lBytesSent: �����ֽ���(>0)
  SendRecvError: ����ʧ��
*******************************************************************************/
long CUdpSockHandle::recv(char *pArrayData, CNetworkAddr *pPeerAddr,
    const ULONG ulDataSize, const EnumSyncAsync bSyncRecv)
{
    if (InvalidSocket == m_lSockFD)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CUdpSockHandle::recv: socket is invalid, recv fail", _FL_);
        return SendRecvError;
    }

    errno = 0;
    struct sockaddr_in  peerAddr;
    socklen_t iFromlen = sizeof(peerAddr);
    long lBytesRecv = 0;
    if(enSyncOp == bSyncRecv)
    {
        //ͬ������
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_FALSE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(long)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket Block fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesRecv = recvfrom((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize,
            MSG_WAITALL, (struct sockaddr *)&peerAddr, &iFromlen);
    }
    else
    {
        //�첽����
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_TRUE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(long)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket NoBlock fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesRecv = recvfrom((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize, 0,
            (struct sockaddr *)&peerAddr, &iFromlen);
    }

    //�������0����ʾ�Ѿ�����
    if (0 == lBytesRecv)
    {
        CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): recv EOF!", _FL_);
        return SendRecvError;
    }

    //���С��0�������ж��Ƿ�����Ϊ�������ǵĻ����ؽ���0�ֽڣ������������
    if (lBytesRecv < 0)
    {
        if((EWOULDBLOCK == CONN_ERRNO) || (EAGAIN == CONN_ERRNO))
        {
            return 0;
        }
        CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
            "recv error. Error(%d): %s", _FL_, CONN_ERRNO, strerror(CONN_ERRNO));
        return SendRecvError;
    }

    pPeerAddr->m_lIpAddr = (LONG)peerAddr.sin_addr.s_addr;
    pPeerAddr->m_usPort = peerAddr.sin_port;

    //���ӹ�����ý��ղ������ٻָ������¼�����Ϊ��Ӧ�ó���ָ�
    //setHandleRecv(AS_TRUE);

    return lBytesRecv;
}

/*******************************************************************************
  Function:       CUdpSockHandle::recvWithTimeout()
  Description:    ���պ���
  Calls:
  Called By:
  Input:          pArrayData: ����buffer��ulDataSize: ���ݳ��ȣ�
                  ulTimeOut: �ȴ�ʱ��, ulSleepTime: �����(ms)
  Output:         pArrayData: ����buffer��pPeerAddr: �Զ˵�ַ��
  Return:
  lBytesSent: �����ֽ���(>0)
  SendRecvError: ����ʧ��
*******************************************************************************/
long CUdpSockHandle::recvWithTimeout(char *pArrayData, CNetworkAddr *pPeerAddr,
    const ULONG ulDataSize, const ULONG ulTimeOut, const ULONG ulSleepTime)
{
    (void)ulSleepTime;//��PC-LINT
    long lRecvBytes = 0;
    ULONG ulTotalRecvBytes = 0;
    ULONG ulWaitTime = ulTimeOut;
    errno = 0;
    //����socket��ʱʱ��
#if AS_APP_OS == AS_OS_WIN32

    ULONG recvWaitTime;
    recvWaitTime = ulWaitTime;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
                  (char *) &recvWaitTime, sizeof(recvWaitTime)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }

#elif AS_APP_OS == AS_OS_LINUX

    struct timeval recvWaitTime;
    recvWaitTime.tv_sec = ulWaitTime/CONN_SECOND_IN_MS;
    recvWaitTime.tv_usec = (ulWaitTime%CONN_SECOND_IN_MS)*CONN_MS_IN_US;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
                  (char *) &recvWaitTime, sizeof(recvWaitTime)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }

#endif

    lRecvBytes = this->recv(pArrayData, pPeerAddr, ulDataSize, enSyncOp);
    if(lRecvBytes < 0)
    {
        CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): "
            "CUdpSockHandle::recvWithTimeout: socket closed when receive. "
            "m_lSockFD = %d, peer_ip(0x%x), peer_port(%d) "
            "errno = %d, error: %s", _FL_, m_lSockFD,
            ntohl((ULONG)(pPeerAddr->m_lIpAddr)), ntohs(pPeerAddr->m_usPort),
            CONN_ERRNO, strerror(CONN_ERRNO) );
        return SendRecvError;
    }

    ulTotalRecvBytes += (ULONG)lRecvBytes;

    //����socket��ʱʱ��Ϊ0��ʾ���õȴ�
#if AS_APP_OS == AS_OS_WIN32

    recvWaitTime = 0;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
                  (char *) &recvWaitTime, sizeof(recvWaitTime)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }

#elif AS_APP_OS == AS_OS_LINUX

    //����socket��ʱʱ��Ϊ0��ʾ���õȴ�
    recvWaitTime.tv_sec = 0;
    recvWaitTime.tv_usec = 0;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
                  (char *) &recvWaitTime, sizeof(recvWaitTime)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }
#endif

    return (long)ulTotalRecvBytes;
}

/*******************************************************************************
  Function:       CUdpSockHandle::close()
  Description:    �ر�����
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
void CUdpSockHandle::close(void)
{
    if(m_pMutexHandle != NULL)
    {
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexHandle))
        {
            return;
        }
    }
    if (InvalidSocket != m_lSockFD)
    {
        //The close of an fd will cause it to be removed from
        //all epoll sets automatically.
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        m_lSockFD = InvalidSocket;
    }

    CHandle::close();

    if(m_pMutexHandle != NULL)
    {
        (void)as_mutex_unlock(m_pMutexHandle);
    }

    return;
}

/*******************************************************************************
  Function:       CTcpServerHandle::listen()
  Description:    �����ȴ��Զ�����
  Calls:
  Called By:
  Input:          pLocalAddr: ���ص�ַ
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: listen success
  AS_ERROR_CODE_FAIL: listen fail
*******************************************************************************/
long CTcpServerHandle::listen(const CNetworkAddr *pLocalAddr)
{
    long lSockFd = (long)socket(AF_INET, SOCK_STREAM, 0);
    if(lSockFd < 0)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "opening client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

    //setSendBufSize
    long lSendBufSize = DEFAULT_TCP_SENDRECV_SIZE;
    socklen_t lSendBufLength = sizeof(lSendBufSize);
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_SNDBUF, (char*)&lSendBufSize,
        lSendBufLength) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setSendBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

    //setRecBufSize
    long lRecvBufSize = DEFAULT_TCP_SENDRECV_SIZE;
    socklen_t lRecvBufLength = sizeof(lRecvBufSize);
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_RCVBUF, (char*)&lRecvBufSize,
        lRecvBufLength) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setRecvBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

#if AS_APP_OS == AS_OS_LINUX
    //setReuseAddr();
    long lReuseAddrFlag = 1;
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_REUSEADDR, (char*)&lReuseAddrFlag,
        sizeof(lReuseAddrFlag)) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "setsockopt client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }
#endif
    //�󶨱��ص�ַ
    struct sockaddr_in  serverAddr;
    memset((char *)&serverAddr, 0, (long)sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = (UINT)pLocalAddr->m_lIpAddr;
    serverAddr.sin_port = pLocalAddr->m_usPort;
    errno = 0;
    if (0 > bind ((SOCKET)lSockFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)))
    {
#if AS_APP_OS == AS_OS_LINUX
        char szServerAddr[INET_ADDRSTRLEN];
        if (NULL != inet_ntop(AF_INET, &serverAddr.sin_addr, szServerAddr,
            sizeof(szServerAddr)))
#elif AS_APP_OS == AS_OS_WIN32
        char *szServerAddr = inet_ntoa(serverAddr.sin_addr);
        if (NULL != szServerAddr)
#endif
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "Can not Bind Data_Sock %s:%d", _FL_,
                szServerAddr, ntohs(serverAddr.sin_port));
        }
        else
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "Can not Bind Data_Sock %d:%d", _FL_,
                serverAddr.sin_addr.s_addr, ntohs(serverAddr.sin_port));
        }
        (void)CLOSESOCK((SOCKET)lSockFd);
        return AS_ERROR_CODE_FAIL;
    }

    //��������
    errno = 0;
    if(::listen((SOCKET)lSockFd, MAX_LISTEN_QUEUE_SIZE) < 0)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "listen Error(%d):%s.", _FL_, CONN_ERRNO, strerror(CONN_ERRNO));
        (void)CLOSESOCK((SOCKET)lSockFd);
        return AS_ERROR_CODE_FAIL;
    }

    m_lSockFD = lSockFd;

    return AS_ERROR_CODE_OK;
}

/*******************************************************************************
  Function:       CTcpServerHandle::close()
  Description:    �ر�����
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
void CTcpServerHandle::close(void)
{
    if(m_pMutexHandle != NULL)
    {
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexHandle))
        {
            return;
        }
    }
    if (InvalidSocket != m_lSockFD)
    {
        //The close of an fd will cause it to be removed from
        //all epoll sets automatically.
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        m_lSockFD = InvalidSocket;
    }

    CHandle::close();

    if(m_pMutexHandle != NULL)
    {
        (void)as_mutex_unlock(m_pMutexHandle);
    }

    return;
}

/*******************************************************************************
  Function:       CHandleManager::CHandleManager()
  Description:    ���캯��
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
CHandleManager::CHandleManager()
{
    m_pMutexListOfHandle = NULL;
#if AS_APP_OS == AS_OS_LINUX
    m_lEpfd = InvalidFd;
    memset(m_epEvents, 0, sizeof(m_epEvents));
#elif AS_APP_OS == AS_OS_WIN32
    //��ʼ��select��
    FD_ZERO(&m_readSet);
    FD_ZERO(&m_writeSet);
    m_stSelectPeriod.tv_sec = 0;
    m_stSelectPeriod.tv_usec = 0;
#endif
    m_ulSelectPeriod = DEFAULT_SELECT_PERIOD;
    m_pSVSThread = NULL;
    m_bExit = AS_FALSE;
    memset(m_szMgrType, 0, sizeof(m_szMgrType));
}

/*******************************************************************************
  Function:       CHandleManager::~CHandleManager()
  Description:    ��������
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
CHandleManager::~CHandleManager()
{
    try
    {
    #if AS_APP_OS == AS_OS_LINUX
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::~CHandleManager: "
            "manager type: %s. thread = %d, m_lEpfd = %d",
            _FL_, m_szMgrType, as_thread_self(), m_lEpfd);
    #elif AS_APP_OS == AS_OS_WIN32
        CONN_WRITE_LOG(CONN_WARNING,   (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::~CHandleManager: "
            "manager type: %s. thread = %d ", _FL_, m_szMgrType, as_thread_self());
    #endif

        ListOfHandleIte itListOfHandle = m_listHandle.begin();
        while(itListOfHandle != m_listHandle.end())
        {
            if((*itListOfHandle)->m_pHandle != NULL)
            {
                (*itListOfHandle)->m_pHandle->close();
            }
    #if AS_APP_OS == AS_OS_LINUX

    #endif
            //����Ӧ��HandleNodeɾ��
            AS_DELETE(*itListOfHandle);
            ++itListOfHandle;
        }

    #if AS_APP_OS == AS_OS_LINUX
        if (m_lEpfd != InvalidFd)
        {
            (void)CLOSESOCK(m_lEpfd);
            m_lEpfd = InvalidFd;
        }
    #endif

        if(m_pSVSThread != NULL)
        {
            free(m_pSVSThread);
        }
        m_pSVSThread = NULL;

        if(m_pMutexListOfHandle != NULL)
        {
            if(AS_ERROR_CODE_OK == as_destroy_mutex(m_pMutexListOfHandle))
            {
                m_pMutexListOfHandle = NULL;
            }
        }

        m_pMutexListOfHandle = NULL;
    }
    catch (...)
    {
    }

}

/*******************************************************************************
  Function:       CHandleManager::init()
  Description:    ��ʼ������
  Calls:
  Called By:
  Input:          ulSelectPeriod: �¼����������λΪms
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: init success
  AS_ERROR_CODE_FAIL: init fail
*******************************************************************************/
long CHandleManager::init(const ULONG ulSelectPeriod)
{
    if (0 == ulSelectPeriod)
    {
        m_ulSelectPeriod = DEFAULT_SELECT_PERIOD;
    }
    else
    {
        m_ulSelectPeriod = ulSelectPeriod;
    }

#if AS_APP_OS == AS_OS_LINUX
    m_lEpfd = epoll_create(MAX_EPOLL_FD_SIZE);

    if(m_lEpfd < 0)
    {
        m_lEpfd = InvalidFd;
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::init: create file handle for epoll fail. "
            "manager type: %s", _FL_, m_szMgrType);
        return AS_ERROR_CODE_FAIL;
    }
#elif AS_APP_OS == AS_OS_WIN32
    //��ulSelectPeriodת����timeval�ṹ
    m_stSelectPeriod.tv_sec = (long)(ulSelectPeriod / CONN_SECOND_IN_MS);
    m_stSelectPeriod.tv_usec = (ulSelectPeriod % CONN_SECOND_IN_MS) * CONN_MS_IN_US;

    //��ʼ��select��
    FD_ZERO(&m_readSet);
    FD_ZERO(&m_writeSet);
#endif

    m_pMutexListOfHandle = as_create_mutex();
    if(NULL == m_pMutexListOfHandle)
    {

#if AS_APP_OS == AS_OS_LINUX
        close(m_lEpfd);
        m_lEpfd = InvalidFd;
#endif
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::init: create m_pMutexListOfHandle fail. "
            "manager type: %s", _FL_, m_szMgrType);
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}

/*******************************************************************************
  Function:       CHandleManager::run()
  Description:    �����̣߳������¼������ѭ��
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: init success
  AS_ERROR_CODE_FAIL: init fail
*******************************************************************************/
long CHandleManager::run()
{
    errno = 0;
    if (AS_ERROR_CODE_OK != as_create_thread((AS_THREAD_FUNC)invoke, (void *)this,
        &m_pSVSThread, AS_DEFAULT_STACK_SIZE))
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "Create play thread failed. manager type: %s. error(%d):%s",
            _FL_, m_szMgrType, CONN_ERRNO, strerror(CONN_ERRNO));
        return AS_ERROR_CODE_FAIL;
    }
    CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
        "as_create_thread: manager type: %s. create thread %d", _FL_,
        m_szMgrType, m_pSVSThread->pthead);

    return AS_ERROR_CODE_OK;
}

/*******************************************************************************
  Function:       CHandleManager::invoke()
  Description:    �����̣߳������¼������ѭ��
  Calls:
  Called By:
  Input:          argc: �������ʵ��ָ��
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: init success
  AS_ERROR_CODE_FAIL: init fail
*******************************************************************************/
void *CHandleManager::invoke(void *argc)
{
    CHandleManager *pHandleManager = (CHandleManager *)argc;
    CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): %s invoke mainLoop",
         _FL_, pHandleManager->m_szMgrType);
    pHandleManager->mainLoop();
    as_thread_exit(NULL);

    return NULL;
}
#if AS_APP_OS == AS_OS_WIN32
/*******************************************************************************
  Function:       CHandleManager::mainLoop()
  Description:    �¼������ѭ��
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
void CHandleManager::mainLoop()
{
    while(AS_FALSE == m_bExit)
    {
        errno = 0;
        long lWaitFds = 0;

        //����
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfHandle))
        {
           break;
        }

        FD_ZERO(&m_readSet);
        FD_ZERO(&m_writeSet);

        //��ulSelectPeriodת����timeval�ṹ
        m_stSelectPeriod.tv_sec =  (long)(m_ulSelectPeriod / CONN_SECOND_IN_MS);
        m_stSelectPeriod.tv_usec =
            (m_ulSelectPeriod % CONN_SECOND_IN_MS) * CONN_MS_IN_US;

        ListOfHandleIte itListOfHandle = m_listHandle.begin();
        while(itListOfHandle != m_listHandle.end())
        {
            CHandleNode *pHandleNode = NULL;
            CHandle *pHandle = NULL;
            long lSockFd = InvalidSocket;

            if(AS_TRUE == (*itListOfHandle)->m_bRemoved)
            {
                itListOfHandle = m_listHandle.erase(itListOfHandle);
                AS_DELETE(pHandleNode);
                continue;
            }
            else
            {
                pHandleNode = *itListOfHandle;
                pHandle = pHandleNode->m_pHandle;
                lSockFd = pHandle->m_lSockFD;
                pHandle->m_bReadSelected = AS_FALSE;
                pHandle->m_bWriteSelected = AS_FALSE;
                if(lSockFd != InvalidSocket)
                {
                    ULONG ulEvent = pHandle->getEvents();
                    if (EPOLLIN == (ulEvent & EPOLLIN))
                    {
                        if(!FD_ISSET(lSockFd,&m_readSet))
                        {
                            FD_SET((SOCKET)lSockFd,&m_readSet);
                            pHandle->m_bReadSelected = AS_TRUE;
                        }
                    }

                    if (EPOLLOUT == (ulEvent & EPOLLOUT))
                    {
                        if(!FD_ISSET(lSockFd,&m_writeSet))
                        {
                            FD_SET((SOCKET)lSockFd,&m_writeSet);
                            pHandle->m_bWriteSelected = AS_TRUE;
                        }
                    }
                }

            }
            ++itListOfHandle;
        }
        //����
        (void)as_mutex_unlock(m_pMutexListOfHandle);


        //��û��Ҫ����socket
        if ((0 == m_readSet.fd_count) && (0 == m_writeSet.fd_count))
        {
            Sleep(1);
            continue;
        }
        else
        {
            if (0 == m_readSet.fd_count)
            {
                lWaitFds = select(0,NULL,&m_writeSet,NULL,&m_stSelectPeriod);
            }
            else
            {
                if (0 == m_writeSet.fd_count)
                {
                    lWaitFds = select(0,&m_readSet,NULL,NULL,&m_stSelectPeriod);
                }
                else
                {
                    lWaitFds = select(0,&m_readSet,&m_writeSet,NULL,&m_stSelectPeriod);
                }
            }
        }

        if (0 == lWaitFds)
        {
            continue;
        }
        if (SOCKET_ERROR == lWaitFds)
        {
            CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
                "select failed: manager type: %s. errno = %d",
                _FL_, m_szMgrType, CONN_ERRNO);
            //�����select����֮ǰFD_SET�����е�socket��close��
            //select�����ᱨWSAENOTSOCK��10038������, break�ᵼ���߳��˳�
            //break;
            continue;
        }

        //����
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfHandle))
        {
           break;
        }

        CHandleNode *pHandleNode = NULL;
        for(ListOfHandleIte it = m_listHandle.begin();
        it != m_listHandle.end(); ++it)
        {
            pHandleNode = *it;
            if (AS_TRUE == pHandleNode->m_bRemoved)
            {
                continue;
            }

            //����Ƿ�Ϊ������
            CHandle *pHandle = pHandleNode->m_pHandle;
            if (pHandle->m_lSockFD != InvalidSocket)
            {
                if (FD_ISSET(pHandle->m_lSockFD,&m_readSet)
                    && (AS_TRUE == pHandle->m_bReadSelected))

                {
                    this->checkSelectResult(enEpollRead, pHandleNode->m_pHandle);
                }

                //����Ƿ�Ϊд����
                if (FD_ISSET(pHandle->m_lSockFD,&m_writeSet)
                    && (AS_TRUE == pHandle->m_bWriteSelected))
                {
                    this->checkSelectResult(enEpollWrite, pHandleNode->m_pHandle);
                }
            }
        }
        //����
        (void)as_mutex_unlock(m_pMutexListOfHandle);
    }

    return;
}
#endif

#if AS_APP_OS == AS_OS_LINUX
/*******************************************************************************
  Function:       CHandleManager::mainLoop()
  Description:    �¼������ѭ��
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
void CHandleManager::mainLoop()
{
    while(AS_FALSE == m_bExit)
    {
        errno = 0;
        long lWaitFds = epoll_wait(m_lEpfd, m_epEvents, EPOLL_MAX_EVENT,
            (long)m_ulSelectPeriod);
        if (0 == lWaitFds )
        {
            continue;
        }

        if (0 > lWaitFds )
        {
            if(EINTR == CONN_ERRNO)
            {
                continue;
            }

            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "epoll_wait failed: manager type: %s. errno(%d):%s",
                _FL_, m_szMgrType, CONN_ERRNO, strerror(CONN_ERRNO));
            break;
        }

        //����
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfHandle))
        {
           break;
        }

        CHandleNode *pHandleNode = NULL;
        for(long i = 0; i < lWaitFds; ++i)
        {
            pHandleNode = (CHandleNode *)(m_epEvents[i].data.ptr);
            if(NULL == pHandleNode)
            {
                CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                    "pHandleNode is NULL, sequence = %d", _FL_, i);
                continue;
            }
            //���������m_bRemoved��ֵֻ��2�֣�Ϊ�˷�ֹ�ڴ汻��д������ֵ��
            //��������:(AS_FALSE != pHandleNode->m_bRemoved)
            if((AS_TRUE == pHandleNode->m_bRemoved) ||
                (AS_FALSE != pHandleNode->m_bRemoved))
            {
                continue;
            }
            CHandle *pHandle = pHandleNode->m_pHandle;
            if(NULL == pHandle)
            {
                CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                    "pHandle is NULL, sequence = %d", _FL_, i);
                continue;
            }

            //ͨ���¼����ͼ���Ƿ�Ϊ������
            if(m_epEvents[i].events & EPOLLIN)
            {
                this->checkSelectResult(enEpollRead, pHandle);
            }

            //ͨ���¼����ͼ���Ƿ�Ϊд����
            if(m_epEvents[i].events & EPOLLOUT)
            {
                this->checkSelectResult(enEpollWrite, pHandle);
            }
        }

        ListOfHandleIte itListOfHandle = m_listHandle.begin();
        while(itListOfHandle != m_listHandle.end())
        {
            if(AS_TRUE == (*itListOfHandle)->m_bRemoved)
            {
                CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
                    "(*itListOfHandle) removed, pHandleNode = 0x%x", _FL_,
                    (long)(*itListOfHandle));

                //����Ӧ��HandleNodeɾ��
                pHandleNode = *itListOfHandle;
                itListOfHandle = m_listHandle.erase(itListOfHandle);
                AS_DELETE(pHandleNode);
                continue;
            }
            ++itListOfHandle;
        }

        //����
        (void)as_mutex_unlock(m_pMutexListOfHandle);
    }
    return;
}
#endif

/*******************************************************************************
  Function:       CHandleManager::exit()
  Description:    �����˳�֪ͨ���¼������ѭ�����˳��߳�
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
void CHandleManager::exit()
{
    if(NULL == m_pSVSThread)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::exit: m_pSVSThread is null", _FL_);
        return;
    }

    this->m_bExit = AS_TRUE;
    errno = 0;
    long ret_val = as_join_thread(m_pSVSThread);
    if (ret_val != AS_ERROR_CODE_OK)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "Wait play thread exit failed. ret_val(%d). error(%d):%s",
            _FL_, ret_val, CONN_ERRNO, strerror(CONN_ERRNO));
    }

    CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
        "CHandleManager::exit: manager type: %s. exit complete."
        "Thread = %d", _FL_, m_szMgrType, m_pSVSThread->pthead);

    return;
}

/*******************************************************************************
  Function:       CHandleManager::addHandle()
  Description:    ע����Ҫ����¼���handle
  Calls:
  Called By:
  Input:          pHandle: ��Ҫ����¼���handle
  Output:         ��
  Return:         ��
*******************************************************************************/
long CHandleManager::addHandle(CHandle *pHandle,
                                  AS_BOOLEAN bIsListOfHandleLocked)
{
    if (NULL == pHandle )
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::addHandle: pHandle is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }
    if(InvalidSocket == pHandle->m_lSockFD)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::addHandle: pHandle's socket is invalid", _FL_);
        return AS_ERROR_CODE_FAIL;
    }
    CHandleNode *pHandleNode = NULL;
    (void)AS_NEW(pHandleNode);
    if (NULL == pHandleNode )
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::addHandle: new pHandleNode fail", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    //����(�����mainloop����ͬһ�̲߳���Ҫ����)
    AS_BOOLEAN bNeedLock = AS_FALSE;
    AS_BOOLEAN bLocked = AS_FALSE;
    if(AS_FALSE == bIsListOfHandleLocked)//û�мӹ�����Ҫ����
    {
        if (NULL == m_pSVSThread)
        {
            bNeedLock = AS_TRUE;
        }
        else
        {
            if(as_thread_self() != m_pSVSThread->pthead)
            {
                bNeedLock = AS_TRUE;
            }
        }

        if(AS_TRUE == bNeedLock)
        {
            if (AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfHandle))
            {
                CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "CHandleManager::removeHandle: get lock failed", _FL_);
            }
            else
            {
               bLocked = AS_TRUE;
            }
        }
    }

#if AS_APP_OS == AS_OS_LINUX
    //��handle��ӵ�epoll�ļ�����
    struct epoll_event epEvent;
    memset(&epEvent, 0, sizeof(epEvent));
    //������Ҫ������¼���ص��ļ�������
    epEvent.data.ptr = (void *)pHandleNode;
    //����Ҫ������¼�����
    epEvent.events = pHandle->getEvents();
    //ע��epoll�¼�
    errno = 0;
    if ( 0 != epoll_ctl(m_lEpfd, EPOLL_CTL_ADD, pHandle->m_lSockFD, &epEvent))
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::addHandle: add event fail, "
            "errno = %d, error: %s", _FL_, CONN_ERRNO, strerror(CONN_ERRNO));
        AS_DELETE(pHandleNode);

        //����
        if(AS_TRUE == bLocked)
        {
            if (AS_ERROR_CODE_OK != as_mutex_unlock(m_pMutexListOfHandle))
            {
                CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                    "CHandleManager::addHandle: release lock failed", _FL_);
            }
        }

        return AS_ERROR_CODE_FAIL;
    }
#endif
    pHandle->m_pHandleNode = pHandleNode;

#if AS_APP_OS == AS_OS_LINUX
    pHandle->m_lEpfd = m_lEpfd;
#endif
    pHandleNode->m_pHandle = pHandle;
#if AS_APP_OS == AS_OS_LINUX
    CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::addHandle: "
            "new pHandleNode(0x%x) m_pHandle(0x%x) fd(%d) Epfd(%d)"
            "peer_ip(0x%x) peer_port(%d)",
            _FL_, pHandleNode, pHandleNode->m_pHandle,
            pHandleNode->m_pHandle->m_lSockFD, pHandleNode->m_pHandle->m_lEpfd,
            pHandleNode->m_pHandle->m_localAddr.m_lIpAddr,
            pHandleNode->m_pHandle->m_localAddr.m_usPort);
#elif AS_APP_OS == AS_OS_WIN32
    CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
        "CHandleManager::addHandle: "
        "new pHandleNode(0x%x) m_pHandle(0x%x) fd(%d) "
        "peer_ip(0x%x) peer_port(%d)",
        _FL_, pHandleNode, pHandleNode->m_pHandle,
        pHandleNode->m_pHandle->m_lSockFD,
        pHandleNode->m_pHandle->m_localAddr.m_lIpAddr,
            pHandleNode->m_pHandle->m_localAddr.m_usPort);
#endif
    m_listHandle.push_back(pHandleNode);

    //����
    if(AS_TRUE == bLocked)
    {
        if (AS_ERROR_CODE_OK != as_mutex_unlock(m_pMutexListOfHandle))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CHandleManager::addHandle: release lock failed", _FL_);
        }
    }

    return AS_ERROR_CODE_OK;
}

/*******************************************************************************
  Function:       CHandleManager::removeHandle()
  Description:    ȡ��ע����Ҫ����¼���handle
  Calls:
  Called By:
  Input:          pHandle: ��Ҫ����¼���handle
  Output:         ��
  Return:         ��
*******************************************************************************/
void CHandleManager::removeHandle(CHandle *pHandle)
{
    if(NULL == pHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::removeHandle: pHandle is NULL", _FL_);
        return;
    }

    //����(�����mainloop����ͬһ�̲߳���Ҫ����)
    AS_BOOLEAN bNeedLock = AS_FALSE;
    AS_BOOLEAN bLocked = AS_FALSE;
    if (NULL == m_pSVSThread)
    {
        bNeedLock = AS_TRUE;
    }
    else
    {
        if(as_thread_self() != m_pSVSThread->pthead)
        {
            bNeedLock = AS_TRUE;
        }
    }

    if(AS_TRUE == bNeedLock)
    {
        if (AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfHandle))
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "CHandleManager::removeHandle: get lock failed", _FL_);
        }
        else
        {
            bLocked = AS_TRUE;
        }
    }

    if(NULL == pHandle->m_pHandleNode)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "pHandle->m_pHandleNode is NULL", _FL_);
    }
    else
    {
        if(pHandle->m_pHandleNode->m_bRemoved != AS_TRUE)
        {
            pHandle->close();
            pHandle->m_pHandleNode->m_bRemoved = AS_TRUE;
            pHandle->m_pHandleNode->m_pHandle = NULL;
        }
        else
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "pHandle removed more than once", _FL_);
        }

        pHandle->m_pHandleNode = NULL;
    }

    //����(�������ͬһ�߳�)
    if(AS_TRUE == bLocked)
    {
        if (AS_ERROR_CODE_OK != as_mutex_unlock(m_pMutexListOfHandle))
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "CHandleManager::removeHandle: release lock failed", _FL_);
        }
    }

    return;
}

/*******************************************************************************
  Function:       CTcpConnMgr::lockListOfHandle()
  Description:    ��List Handle ����
  Calls:
  Called By:
  Input:          NA
  Output:         ��
  Return:         ��
*******************************************************************************/
void CTcpConnMgr::lockListOfHandle()
{
    if (AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfHandle))
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CTcpConnMgr::lockListOfHandle: get lock failed", _FL_);
    }
}
/*******************************************************************************
  Function:       CTcpConnMgr::unlockListOfHandle()
  Description:    ��List Handle ����
  Calls:
  Called By:
  Input:          NA
  Output:         ��
  Return:         ��
*******************************************************************************/
void CTcpConnMgr::unlockListOfHandle()
{
    if (AS_ERROR_CODE_OK != as_mutex_unlock(m_pMutexListOfHandle))
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CTcpConnMgr::unlockListOfHandle: release lock failed", _FL_);
    }
}

/*******************************************************************************
  Function:       CTcpConnMgr::checkSelectResult()
  Description:    ���ݵõ����¼�������Ӧ��tcp handle�����¼�
  Calls:
  Called By:
  Input:          enEpEvent:��⵽���¼���pHandle: ��Ҫ����¼���handle
  Output:         ��
  Return:         ��
*******************************************************************************/
void CTcpConnMgr::checkSelectResult(const EpollEventType enEpEvent,
    CHandle *pHandle)
{
    if(NULL == pHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::checkSelectResult: pHandle is NULL", _FL_);
        return;
    }

    CTcpConnHandle *pTcpConnHandle = dynamic_cast<CTcpConnHandle *>(pHandle);
    if(NULL == pTcpConnHandle)
    {
        return;
    }

    //������¼�
    if(enEpollRead == enEpEvent)
    {
        //������¼����
        pTcpConnHandle->setHandleRecv(AS_FALSE);
        //����handle��������¼�
        pTcpConnHandle->handle_recv();
    }

    //����д�¼�
    if(enEpollWrite == enEpEvent)
    {
        //���д�¼����
        pTcpConnHandle->setHandleSend(AS_FALSE);

        //����Ƿ����ӳɹ�
        if(pTcpConnHandle->getStatus() == enConnecting)
        {
            long lErrorNo = 0;
            socklen_t len = sizeof(lErrorNo);
            if (getsockopt((SOCKET)(pTcpConnHandle->m_lSockFD), SOL_SOCKET, SO_ERROR,
                (SOCK_OPT_TYPE *)&lErrorNo, &len) < 0)
            {
                CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                    "getsockopt of sockfd has wrong", _FL_);
                pTcpConnHandle->close();
                pTcpConnHandle->m_lConnStatus = enConnFailed;
            }
            else if (lErrorNo != 0)
            {
                CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                    "getsockopt says sockfd is wrong", _FL_);
                pTcpConnHandle->close();
                pTcpConnHandle->m_lConnStatus = enConnFailed;
            }

            pTcpConnHandle->m_lConnStatus = enConnected;
        }

        //����handle����д�¼�
        pTcpConnHandle->handle_send();
    }

}

/*******************************************************************************
  Function:       CUdpSockMgr::checkSelectResult()
  Description:    ���ݵõ����¼�������Ӧ��udp handle�����¼�
  Calls:
  Called By:
  Input:          enEpEvent:��⵽���¼���pHandle: ��Ҫ����¼���handle
  Output:         ��
  Return:         ��
*******************************************************************************/
void CUdpSockMgr::checkSelectResult(const EpollEventType enEpEvent,
    CHandle *pHandle)
{
    if(NULL == pHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::checkSelectResult: pHandle is NULL", _FL_);
        return;
    }

    CUdpSockHandle *pUdpSockHandle = dynamic_cast<CUdpSockHandle *>(pHandle);
    if(NULL == pUdpSockHandle)
    {
        return;
    }

    //������¼�
    if(enEpollRead == enEpEvent)
    {
        //������¼����
        pUdpSockHandle->setHandleRecv(AS_FALSE);
        //����handle��������¼�
        pUdpSockHandle->handle_recv();
    }

    //����д�¼�
    if(enEpollWrite == enEpEvent)
    {
        //���д�¼����
        pUdpSockHandle->setHandleSend(AS_FALSE);
        //����handle����д�¼�
        pUdpSockHandle->handle_send();
    }
}

/*******************************************************************************
  Function:       CTcpServerMgr::checkSelectResult()
  Description:    ���ݵõ����¼�������Ӧ��handle�����¼�
  Calls:
  Called By:
  Input:          enEpEvent:��⵽���¼���pHandle: ��Ҫ����¼���handle
  Output:         ��
  Return:         ��
*******************************************************************************/
void CTcpServerMgr::checkSelectResult(const EpollEventType enEpEvent,
    CHandle *pHandle)
{
    if(NULL == pHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CHandleManager::checkSelectResult: pHandle is NULL", _FL_);
        return;
    }

    if(NULL == m_pTcpConnMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CTcpServerMgr::checkSelectResult: m_pTcpConnMgr is NULL.", _FL_);
        return;
    }

    CTcpServerHandle *pTcpServerHandle = dynamic_cast<CTcpServerHandle *>(pHandle);
    if(NULL == pTcpServerHandle)
    {
        return;
    }

    //�������ӵ����¼�
    if(enEpollRead == enEpEvent)
    {
        struct sockaddr_in peerAddr;
        memset(&peerAddr, 0, sizeof(struct sockaddr_in));

        //��������
        socklen_t len = sizeof(struct sockaddr_in);
        long lClientSockfd = InvalidFd;
        errno = 0;
        lClientSockfd = (long)::accept((SOCKET)(pTcpServerHandle->m_lSockFD),
            (struct sockaddr *)&peerAddr, &len);
        if( 0 > lClientSockfd)
        {
            if((EWOULDBLOCK != CONN_ERRNO) && (CONN_ERRNO != EAGAIN))
            {
                CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                    "accept Error(%d): %s. ", _FL_, CONN_ERRNO, strerror(CONN_ERRNO));
            }
            return;
        }
        //setSendBufSize
        long lSendBufSize = DEFAULT_TCP_SENDRECV_SIZE;
        socklen_t lSendBufLength = sizeof(lSendBufSize);
        if(setsockopt((SOCKET)lClientSockfd, SOL_SOCKET, SO_SNDBUF, (char*)&lSendBufSize,
            lSendBufLength) < 0)
        {
            (void)CLOSESOCK((SOCKET)lClientSockfd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "setSendBufSize client socket error(%d)", _FL_, CONN_ERRNO);
            return;
        }

        //setRecBufSize
        long lRecvBufSize = DEFAULT_TCP_SENDRECV_SIZE;
        socklen_t lRecvBufLength = sizeof(lRecvBufSize);
        if(setsockopt((SOCKET)lClientSockfd, SOL_SOCKET, SO_RCVBUF, (char*)&lRecvBufSize,
            lRecvBufLength) < 0)
        {
            (void)CLOSESOCK((SOCKET)lClientSockfd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "setRecvBufSize client socket error(%d)", _FL_, CONN_ERRNO);
            return;
        }
        long flag = 1;
        if(setsockopt((SOCKET)lClientSockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag,
            sizeof(flag)) < 0)
        {
            (void)CLOSESOCK((SOCKET)lClientSockfd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "set TCP_NODELAY client socket error(%d)", _FL_, CONN_ERRNO);
            return;
        }
        //setReuseAddr();
        long lReuseAddrFlag = 1;
        if(setsockopt((SOCKET)lClientSockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&lReuseAddrFlag,
            sizeof(lReuseAddrFlag)) < 0)
        {
            (void)CLOSESOCK((SOCKET)lClientSockfd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "setsockopt client socket error(%d)", _FL_, CONN_ERRNO);
            return;
        }
        //����server handle�������ӵ���
        CNetworkAddr clientAddr;
        clientAddr.m_lIpAddr = (LONG)peerAddr.sin_addr.s_addr;
        clientAddr.m_usPort = peerAddr.sin_port;
        CTcpConnHandle *pTcpConnHandle = NULL;

        /*�˴�����,ʹ�������ɵ�pTcpConnHandle,��removeTcpClient����*/
        m_pTcpConnMgr->lockListOfHandle();
        if (AS_ERROR_CODE_OK != pTcpServerHandle->handle_accept(&clientAddr, pTcpConnHandle))
        {
            (void)CLOSESOCK((SOCKET)lClientSockfd);
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "CTcpServerMgr::checkSelectResult: accept fail.", _FL_);
            m_pTcpConnMgr->unlockListOfHandle();
            return;
        }

        if (NULL == pTcpConnHandle)
        {
            (void)CLOSESOCK((SOCKET)lClientSockfd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CTcpServerMgr::checkSelectResult: "
                "return NULL arg.", _FL_);
            m_pTcpConnMgr->unlockListOfHandle();
            return;
        }
        if(AS_ERROR_CODE_OK != pTcpConnHandle->initHandle())
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "CTcpServerMgr::checkSelectResult: "
                "pTcpConnHandle init fail", _FL_);
            m_pTcpConnMgr->unlockListOfHandle();
            return;
        }
        pTcpConnHandle->m_lSockFD = lClientSockfd;
        pTcpConnHandle->m_localAddr.m_lIpAddr = pTcpServerHandle->m_localAddr.m_lIpAddr;
        pTcpConnHandle->m_localAddr.m_usPort = pTcpServerHandle->m_localAddr.m_usPort;
        pTcpConnHandle->m_lConnStatus = enConnected;
        pTcpConnHandle->m_peerAddr.m_lIpAddr = clientAddr.m_lIpAddr;
        pTcpConnHandle->m_peerAddr.m_usPort = clientAddr.m_usPort;

        AS_BOOLEAN bIsListOfHandleLocked = AS_TRUE;
        if (AS_ERROR_CODE_OK != m_pTcpConnMgr->addHandle(pTcpConnHandle,
                                                    bIsListOfHandleLocked))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CTcpServerMgr::checkSelectResult: addHandle fail.", _FL_);
            pTcpConnHandle->close();
        }
        m_pTcpConnMgr->unlockListOfHandle();
        CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): "
            "CTcpServerMgr::checkSelectResult: accept connect, "
            "m_lSockFD = %d, peer_ip(0x%x), peer_port(%d)", _FL_,
            pTcpConnHandle->m_lSockFD,
            ntohl((ULONG)(pTcpConnHandle->m_peerAddr.m_lIpAddr)),
            ntohs(pTcpConnHandle->m_peerAddr.m_usPort));

    }

    //��Ӧ�ü�⵽д�¼�
    if(enEpollWrite == enEpEvent)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CTcpServerMgr should not process write event", _FL_);
    }
}

/*******************************************************************************
  Function:       CConnMgr::CConnMgr()
  Description:    ���캯��
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
CConnMgr::CConnMgr()
{
    m_lLocalIpAddr = InvalidIp;
    m_pTcpConnMgr = NULL;
    m_pUdpSockMgr = NULL;
    m_pTcpServerMgr = NULL;

}

/*******************************************************************************
  Function:       CConnMgr::~CConnMgr()
  Description:    ��������
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
CConnMgr::~CConnMgr()
{
    try
    {
        AS_DELETE(m_pTcpConnMgr);
        AS_DELETE(m_pUdpSockMgr);
        AS_DELETE(m_pTcpServerMgr);
    }
    catch (...)
    {
    }
}

/*******************************************************************************
  Function:       CConnMgr::init()
  Description:    ��ʼ������
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: init success
  AS_ERROR_CODE_FAIL: init fail
*******************************************************************************/
long CConnMgr::init(const ULONG ulSelectPeriod, const AS_BOOLEAN bHasUdpSock,
            const AS_BOOLEAN bHasTcpClient, const AS_BOOLEAN bHasTcpServer)
{
#if AS_APP_OS == AS_OS_WIN32
    WSAData wsaData;
    if (SOCKET_ERROR == WSAStartup(MAKEWORD(2,2),&wsaData))
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "WSAStartup error.", _FL_);
        return SVS_ERR_SYS;
    }
#endif
    if(AS_TRUE == bHasUdpSock)
    {
        (void)AS_NEW(m_pUdpSockMgr);
        if(NULL == m_pUdpSockMgr)
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CConnMgr::init: create m_pUdpSockMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
        if(AS_ERROR_CODE_OK != m_pUdpSockMgr->init(ulSelectPeriod))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CConnMgr::init: init m_pUdpSockMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
    }

    if((AS_TRUE == bHasTcpClient) || (AS_TRUE == bHasTcpServer))
    {
        (void)AS_NEW(m_pTcpConnMgr);
        if(NULL == m_pTcpConnMgr)
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CConnMgr::init: create m_pTcpConnMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
        if(AS_ERROR_CODE_OK != m_pTcpConnMgr->init(ulSelectPeriod))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CConnMgr::init: init m_pTcpConnMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
    }

    if(AS_TRUE == bHasTcpServer)
    {
        (void)AS_NEW(m_pTcpServerMgr);
        if(NULL == m_pTcpServerMgr)
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "CConnMgr::init: create m_pTcpServerMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
        if(AS_ERROR_CODE_OK != m_pTcpServerMgr->init(ulSelectPeriod))
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "CConnMgr::init: init m_pTcpServerMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
        m_pTcpServerMgr->setTcpClientMgr(m_pTcpConnMgr);
    }

    return AS_ERROR_CODE_OK;
}

/*******************************************************************************
  Function:       CConnMgr::run()
  Description:    ��������manager
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: start success
  AS_ERROR_CODE_FAIL: start fail
*******************************************************************************/
long CConnMgr::run(void)
{
    if(NULL != m_pUdpSockMgr)
    {
        if(AS_ERROR_CODE_OK != m_pUdpSockMgr->run())
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CConnMgr::run: run m_pUdpSockMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
    }

    if(NULL != m_pTcpConnMgr)
    {
        if(AS_ERROR_CODE_OK != m_pTcpConnMgr->run())
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "CConnMgr::run: run m_pTcpConnMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
    }

    if(NULL != m_pTcpServerMgr)
    {
        if(AS_ERROR_CODE_OK != m_pTcpServerMgr->run())
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "CConnMgr::run: run m_pTcpServerMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
    }

    return AS_ERROR_CODE_OK;

}


/*******************************************************************************
  Function:       CConnMgr::exit()
  Description:    �˳�����manager
  Calls:
  Called By:
  Input:          ��
  Output:         ��
  Return:         ��
*******************************************************************************/
void CConnMgr::exit(void)
{
    if(NULL != m_pUdpSockMgr)
    {
        m_pUdpSockMgr->exit();
        AS_DELETE(m_pUdpSockMgr);
    }

    if(NULL != m_pTcpServerMgr)
    {
        m_pTcpServerMgr->exit();
        AS_DELETE(m_pTcpServerMgr);
    }

    if(NULL != m_pTcpConnMgr)
    {
        m_pTcpConnMgr->exit();
        AS_DELETE(m_pTcpConnMgr);
    }

#if AS_APP_OS == AS_OS_WIN32
    (void)WSACleanup();
#endif
    return;
}

/*******************************************************************************
  Function:       CConnMgr::setDefaultLocalAddr()
  Description:    ���ñ���ȱʡ��ַ
  Calls:
  Called By:
  Input:          szLocalIpAddr: ���ص�ַ
  Output:         ��
  Return:         ��
*******************************************************************************/
void CConnMgr::setDefaultLocalAddr(const char *szLocalIpAddr)
{
    if(szLocalIpAddr != NULL)
    {
        long lLocalIp = (long)inet_addr(szLocalIpAddr);
        if ((ULONG)lLocalIp != InvalidIp)
        {
            m_lLocalIpAddr = (long)inet_addr(szLocalIpAddr);
        }
    }

    return;
}

/*******************************************************************************
  Function:       CConnMgr::regTcpClient()
  Description:    ����TCP�ͻ���
  Calls:
  Called By:
  Input:          pLocalAddr: ���ص�ַ��pPeerAddr: �Զ˵�ַ��
                  pTcpConnHandle: ���Ӷ�Ӧ��handle
                  bSyncConn: SVS_TRUE��ʾͬ�����ӣ�SVS_FALSE��ʾ�첽����
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: connect success
  AS_ERROR_CODE_FAIL: connect fail
*******************************************************************************/
long CConnMgr::regTcpClient( const CNetworkAddr *pLocalAddr,
    const CNetworkAddr *pPeerAddr, CTcpConnHandle *pTcpConnHandle,
    const EnumSyncAsync bSyncConn, ULONG ulTimeOut)
{
    if(NULL == pLocalAddr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpClient: pLocalAddr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == pPeerAddr)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpClient: pPeerAddr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == pTcpConnHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpClient: pTcpConnHandle is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(AS_ERROR_CODE_OK != pTcpConnHandle->initHandle())
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpClient: pTcpConnHandle init fail", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == m_pTcpConnMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpClient: m_pTcpConnMgr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    CNetworkAddr localAddr;
    if (InvalidIp == (ULONG)(pLocalAddr->m_lIpAddr))
    {
        localAddr.m_lIpAddr = this->m_lLocalIpAddr;
    }
    else
    {
        localAddr.m_lIpAddr = pLocalAddr->m_lIpAddr;
    }
    localAddr.m_usPort = pLocalAddr->m_usPort;

    pTcpConnHandle->m_localAddr.m_lIpAddr = pLocalAddr->m_lIpAddr;
    pTcpConnHandle->m_localAddr.m_usPort = pLocalAddr->m_usPort;

    long lRetVal = pTcpConnHandle->conn(&localAddr, pPeerAddr, bSyncConn, ulTimeOut);

    if(lRetVal != AS_ERROR_CODE_OK)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpClient: connect peer fail(0x%x:%d)", _FL_,
            ntohl((ULONG)(pPeerAddr->m_lIpAddr)), ntohs(pPeerAddr->m_usPort));
        return lRetVal;
    }

    lRetVal = m_pTcpConnMgr->addHandle(pTcpConnHandle);
    if(lRetVal != AS_ERROR_CODE_OK)
    {
        pTcpConnHandle->close();
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpClient: register connection fail", _FL_);
        return lRetVal;
    }

    return AS_ERROR_CODE_OK;
}

/*******************************************************************************
  Function:       CConnMgr::removeTcpClient()
  Description:    ע�����Ӻ���
  Calls:
  Called By:
  Input:          pTcpConnHandle: ���Ӷ�Ӧ��handle
  Output:         ��
  Return:         ��
*******************************************************************************/
void CConnMgr::removeTcpClient(CTcpConnHandle *pTcpConnHandle)
{
    if(NULL == pTcpConnHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::removeTcpClient: pTcpConnHandle is NULL", _FL_);
        return;
    }
    CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::removeTcpClient: "
            "remove pTcpConnHandle(0x%x) pHandleNode(0x%x) fd(%d)"
            "m_lIpAddr(0x%x) m_usPort(%d)",
            _FL_, pTcpConnHandle, pTcpConnHandle->m_pHandleNode,
            pTcpConnHandle->m_lSockFD, pTcpConnHandle->m_localAddr.m_lIpAddr,
            pTcpConnHandle->m_localAddr.m_usPort);

    //�˴����ܹر�socket��ԭ������:
    //����ͨ��ƽ̨�е�CConnMgr::removeTcpClient����ʱ��
    //�����ȹر���socket������ͨ��ƽ̨��socketɨ���̼߳�ص�socket�ж��¼���
    //���Ǵ�ʱsocket�Ѿ����ر���,socket �ϱ��Ķ��¼��ǷǷ��ġ�
    //���Թر�socket����Ҫ�ͼ��socket�¼���������ʱ����Ҫ����.
    //pTcpConnHandle->close();

    if(NULL == m_pTcpConnMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::removeTcpClient: m_pTcpConnMgr is NULL", _FL_);
        return;
    }

    m_pTcpConnMgr->removeHandle(pTcpConnHandle);

    return;
}

/*******************************************************************************
  Function:       CConnMgr::regTcpServer()
  Description:    ����TCP������
  Calls:
  Called By:
  Input:          pLocalAddr: ���ص�ַ
                  pTcpServerHandle: TCP��������Ӧ��handle
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: listen success
  AS_ERROR_CODE_FAIL: listen fail
*******************************************************************************/
long CConnMgr::regTcpServer(const CNetworkAddr *pLocalAddr,
    CTcpServerHandle *pTcpServerHandle)
{
    if(NULL == pLocalAddr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpServer: pLocalAddr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == pTcpServerHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpServer: pTcpServerHandle is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(AS_ERROR_CODE_OK != pTcpServerHandle->initHandle())
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpServer: pTcpServerHandle init fail", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == m_pTcpConnMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpServer: m_pTcpConnMgr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == m_pTcpServerMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpServer: m_pTcpServerMgr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    CNetworkAddr localAddr;
    if (InvalidIp == (ULONG)(pLocalAddr->m_lIpAddr))
    {
        localAddr.m_lIpAddr = this->m_lLocalIpAddr;
    }
    else
    {
        localAddr.m_lIpAddr = pLocalAddr->m_lIpAddr;
    }
    localAddr.m_usPort = pLocalAddr->m_usPort;

    pTcpServerHandle->m_localAddr.m_lIpAddr = pLocalAddr->m_lIpAddr;
    pTcpServerHandle->m_localAddr.m_usPort = pLocalAddr->m_usPort;

    long lRetVal = pTcpServerHandle->listen(&localAddr);

    if(lRetVal != AS_ERROR_CODE_OK)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpServer: listen fail", _FL_);
        return lRetVal;
    }

    lRetVal = m_pTcpServerMgr->addHandle(pTcpServerHandle);
    if(lRetVal != AS_ERROR_CODE_OK)
    {
        pTcpServerHandle->close();
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regTcpClient: register tcp server fail", _FL_);
        return lRetVal;
    }

    return AS_ERROR_CODE_OK;
}

/*******************************************************************************
  Function:       CConnMgr::regTcpClient()
  Description:    ע��TCP������
  Calls:
  Called By:
  Input:          pTcpServerHandle: TCP��������Ӧ��handle
  Output:         ��
  Return:         ��
*******************************************************************************/
void CConnMgr::removeTcpServer(CTcpServerHandle *pTcpServerHandle)
{
    if(NULL == pTcpServerHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::removeTcpServer: pTcpServerHandle is NULL", _FL_);
        return;
    }

    //�˴����ܹر�socket��ԭ������:
    //����ͨ��ƽ̨�е�CConnMgr::removeTcpClient����ʱ��
    //�����ȹر���socket������ͨ��ƽ̨��socketɨ���̼߳�ص�socket�ж��¼���
    //���Ǵ�ʱsocket�Ѿ����ر���,socket �ϱ��Ķ��¼��ǷǷ��ġ�
    //���Թر�socket����Ҫ�ͼ��socket�¼���������ʱ����Ҫ����.
    //pTcpServerHandle->close();

    if(NULL == m_pTcpServerMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::removeTcpServer: m_pTcpServerMgr is NULL", _FL_);
        return;
    }

    m_pTcpServerMgr->removeHandle(pTcpServerHandle);

    return;
}

/*******************************************************************************
  Function:       CConnMgr::regUdpSocket()
  Description:    ����UDP socket
  Calls:
  Called By:
  Input:          pLocalAddr: ���ص�ַ��
                  pUdpSockHandle: ���Ӷ�Ӧ��handle
  Output:         ��
  Return:
  AS_ERROR_CODE_OK: create success
  AS_ERROR_CODE_FAIL: create fail
*******************************************************************************/
long CConnMgr::regUdpSocket(const CNetworkAddr *pLocalAddr,
                                 CUdpSockHandle *pUdpSockHandle,
                                 const CNetworkAddr *pMultiAddr)
{
    if(NULL == pLocalAddr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regUdpSocket: pUdpSockHandle is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == pUdpSockHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regUdpSocket: pUdpSockHandle is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(AS_ERROR_CODE_OK != pUdpSockHandle->initHandle())
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regUdpSocket: pUdpSockHandle init fail", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == m_pUdpSockMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regUdpSocket: m_pUdpSockMgr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    CNetworkAddr localAddr;
    if (InvalidIp == (ULONG)(pLocalAddr->m_lIpAddr))
    {
        localAddr.m_lIpAddr = this->m_lLocalIpAddr;
    }
    else
    {
        localAddr.m_lIpAddr = pLocalAddr->m_lIpAddr;
    }
    localAddr.m_usPort = pLocalAddr->m_usPort;

    pUdpSockHandle->m_localAddr.m_lIpAddr = pLocalAddr->m_lIpAddr;
    pUdpSockHandle->m_localAddr.m_usPort = pLocalAddr->m_usPort;

    long lRetVal = pUdpSockHandle->createSock(&localAddr, pMultiAddr);
    if(lRetVal != AS_ERROR_CODE_OK)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regUdpSocket: create UDP socket fail", _FL_);
        return lRetVal;
    }

    lRetVal = m_pUdpSockMgr->addHandle(pUdpSockHandle);
    if(lRetVal != AS_ERROR_CODE_OK)
    {
        pUdpSockHandle->close();
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::regUdpSocket: register UDP socket fail", _FL_);
        return lRetVal;
    }

    return AS_ERROR_CODE_OK;
}

/*******************************************************************************
  Function:       CConnMgr::removeUdpSocket()
  Description:    ɾ��UDP socket
  Calls:
  Called By:
  Input:          pUdpSockHandle: ���Ӷ�Ӧ��handle
  Output:         ��
  Return:         ��
*******************************************************************************/
void CConnMgr::removeUdpSocket(CUdpSockHandle *pUdpSockHandle)
{
    if(NULL == pUdpSockHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::removeUdpSocket: pUdpSockHandle is NULL", _FL_);
        return;
    }
    pUdpSockHandle->close();

    if(NULL == m_pUdpSockMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "CConnMgr::removeUdpSocket: m_pUdpSockMgr is NULL", _FL_);
        return;
    }

    m_pUdpSockMgr->removeHandle(pUdpSockHandle);

    return;
}




