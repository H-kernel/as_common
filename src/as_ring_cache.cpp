
/******************************************************************************
   ��Ȩ���� (C), 2008-2011, M.Kernel

 ******************************************************************************
  �ļ���          : as_ring_cache.cpp
  �汾��          : 1.0
  ����            : hexin
  ��������        : 2008-08-07
  ����޸�        :
  ��������        : ���λ�����
  �����б�        :
  �޸���ʷ        :
  1 ����          :
    ����          :
    �޸�����      :
*******************************************************************************/


#include "as_ring_cache.h"
extern "C"{
#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
#include "as_mutex.h"
#include "as_event.h"
#include "as_thread.h"
#include "as_time.h"
#include "as_json.h"
}

as_ring_cache::as_ring_cache()
{
    m_pBuffer = NULL;
    m_ulBufferSize = 0;
    m_ulReader = 0;
    m_ulWriter = 0;
    m_ulDataSize = 0;

    m_pMutex = as_create_mutex();
}

as_ring_cache::~as_ring_cache()
{
    if(NULL != m_pBuffer)
    {
        try
        {
            delete[] m_pBuffer;
            m_pBuffer = NULL;
        }
        catch(...)
        {
            m_pBuffer = NULL;
        }
        m_pBuffer = NULL;
    }
    m_ulBufferSize = 0;
    m_ulReader = 0;
    m_ulWriter = 0;
    m_ulDataSize = 0;

    as_destroy_mutex(m_pMutex);
}

//���û�������С������������ɺ󻺳�Ĵ�С
uint32_t as_ring_cache::SetCacheSize(uint32_t ulCacheSize)
{
    as_mutex_lock(m_pMutex);
    //�������
    m_ulReader = 0;
    m_ulWriter = 0;
    m_ulDataSize = 0;

    //��������Сδ�����仯������Ҫ���������ڴ�
    if(ulCacheSize == m_ulBufferSize)
    {
        as_mutex_unlock(m_pMutex);
        return m_ulBufferSize;
    }

    //��������С�����仯����Ҫ���������ڴ�
    //�ͷŵ�ǰ�����ڴ�
    if(NULL != m_pBuffer)
    {
        try
        {
            delete[] m_pBuffer;
            m_pBuffer = NULL;
        }
        catch(...)
        {
            m_pBuffer = NULL;
        }
        m_pBuffer = NULL;
    }

    //�����»����ڴ�
    m_ulBufferSize = ulCacheSize;
    if(m_ulBufferSize > 0)
    {
        try
        {
            m_pBuffer = new char[m_ulBufferSize];
        }
        catch(...)
        {
        }

        if(NULL == m_pBuffer)
        {//����ʧ��
            m_ulBufferSize = 0;
        }
    }

    as_mutex_unlock(m_pMutex);
    return m_ulBufferSize;
}

//��õ�ǰ��������С
uint32_t as_ring_cache::GetCacheSize() const
{
    return m_ulBufferSize;
}

//�鿴ָ���������ݣ�����������Ȼ������Щ���ݣ�����ʵ�ʶ�ȡ���ݳ���
//PCLINTע��˵�����ú����ڱ����ڲ���ʹ��
uint32_t as_ring_cache::Peek(char* pBuf, uint32_t ulPeekLen)/*lint -e1714*/
{
    uint32_t ulResult = 0;

    as_mutex_lock(m_pMutex);

    //����ʵ�ʿɶ�ȡ�ĳ���
    ulResult = m_ulDataSize>ulPeekLen?ulPeekLen:m_ulDataSize;
    if(0 == ulResult)
    {
        as_mutex_unlock(m_pMutex);
        return ulResult;
    }

    //���ݳʵ��ηֲ�
    //PCLINTע��˵������������Ҫ���ǵ���Ч�ʣ��ʲ�������Ĳ�����飬�ɵ����߱�֤
    if(m_ulReader < m_ulWriter)/*lint -e613*/
    {//ooo********ooooo
        //PCLINTע��˵������Ҫ���ǵ���Ч�ʣ�����������
        ::memcpy(pBuf, m_pBuffer+m_ulReader, ulResult);/*lint -e670*/
        as_mutex_unlock(m_pMutex);
        return ulResult;
    }

    //���ݳ����ηֲ���m_ulReader����m_ulWriterʱ��������Ҳ������
    //*B*oooooooo**A**
    uint32_t ulASectionLen = m_ulBufferSize - m_ulReader;//A�����ݳ���
    if(ulResult <= ulASectionLen)//A�����ݳ����㹻
    {
        ::memcpy(pBuf, m_pBuffer+m_ulReader, ulResult);
    }
    else//A�����ݳ��Ȳ���������Ҫ��B�ζ�ȡ
    {/*lint -e668*/
        //PCLINTע��˵������Ҫ���ǵ���Ч�ʣ�����������
        //�ȶ�A�Σ��ٴ�B�β���
        ::memcpy(pBuf, m_pBuffer+m_ulReader, ulASectionLen);
        ::memcpy(pBuf+ulASectionLen, m_pBuffer, ulResult-ulASectionLen);
    }

    as_mutex_unlock(m_pMutex);
    return ulResult;
}

//��ȡָ���������ݣ�����ʵ�ʶ�ȡ���ݳ���
uint32_t as_ring_cache::Read(char* pBuf, uint32_t ulReadLen)
{
    uint32_t ulResult = 0;

    as_mutex_lock(m_pMutex);

    //����ʵ�ʿɶ�ȡ�ĳ���
    ulResult = m_ulDataSize>ulReadLen?ulReadLen:m_ulDataSize;
    if(0 == ulResult)
    {
        as_mutex_unlock(m_pMutex);
        return ulResult;
    }

    //���ݳʵ��ηֲ�
    if(m_ulReader < m_ulWriter)
    {//ooo********ooooo
        ::memcpy(pBuf, m_pBuffer+m_ulReader, ulResult);

        //���ݱ���ȡ�����¶�ȡλ��
        m_ulReader += ulResult;/*lint -e414*/
        //PCLINTע��˵������Ҫ���ǵ���Ч�ʣ�����������
        m_ulReader %= m_ulBufferSize;
        //�����ѱ���ȡ�����»��������ݳ���
        m_ulDataSize -= ulResult;

        as_mutex_unlock(m_pMutex);
        return ulResult;
    }

    //���ݳ����ηֲ���m_ulReader����m_ulWriterʱ��������Ҳ������
    //*B*oooooooo**A**
    uint32_t ulASectionLen = m_ulBufferSize - m_ulReader;//A�����ݳ���
    if(ulResult <= ulASectionLen)//A�����ݳ����㹻
    {
        //PCLINTע��˵������Ҫ���ǵ���Ч�ʣ����������飬ָ��ʹ��������
        ::memcpy(pBuf, m_pBuffer+m_ulReader, ulResult);/*lint -e826*/

        //���ݱ���ȡ�����¶�ȡλ��
        m_ulReader += ulResult;
        m_ulReader %= m_ulBufferSize;
    }
    else//A�����ݳ��Ȳ���������Ҫ��B�ζ�ȡ
    {
        //�ȶ�A�Σ��ٴ�B�β���
        ::memcpy(pBuf, m_pBuffer+m_ulReader, ulASectionLen);
        //PCLINTע��˵������Ҫ���ǵ���Ч�ʣ����������飬��ȷ������Ч��
        ::memcpy(pBuf+ulASectionLen, m_pBuffer, ulResult-ulASectionLen);/*lint -e429*/
        m_ulReader = ulResult - ulASectionLen;//���ݱ���ȡ�����¶�ȡλ��
    }
    //�����ѱ���ȡ�����»��������ݳ���
    m_ulDataSize -= ulResult;

    as_mutex_unlock(m_pMutex);
    return ulResult;
}

//дָ���������ݣ�����ʵ��д���ݳ��ȣ����������ռ䲻������ֹд��
uint32_t as_ring_cache::Write(const char* pBuf, uint32_t ulWriteLen)
{
    uint32_t ulResult = 0;

    as_mutex_lock(m_pMutex);

    //����ʵ�ʿ�д�볤�ȣ������໺������������д������
    ulResult = (m_ulBufferSize-m_ulDataSize)<ulWriteLen?0:ulWriteLen;
    if(0 == ulResult)
    {
        as_mutex_unlock(m_pMutex);
        return ulResult;
    }

    //����ռ�ʵ��ηֲ�
    if(m_ulReader > m_ulWriter)
    {//***oooooooo*****
        ::memcpy(m_pBuffer+m_ulWriter, pBuf, ulResult);

        //������д�룬����д��λ��
        m_ulWriter += ulResult;
        m_ulWriter %= m_ulBufferSize;
        //������д�룬���»��������ݳ���
        m_ulDataSize += ulResult;

        as_mutex_unlock(m_pMutex);
        return ulResult;
    }

    //����ռ�����ηֲ���m_ulReader����m_ulWriterʱ�����ݣ�Ҳ�����ηֲ�
    //oBo********ooAoo
    uint32_t ulASectionLen = m_ulBufferSize - m_ulWriter;//A�ο��໺�峤��
    if(ulResult <= ulASectionLen)//A�ο��໺�峤���㹻
    {/*lint -e669*/
        //PCLINTע��˵������Ҫ���ǵ���Ч�ʣ�����������
        ::memcpy(m_pBuffer+m_ulWriter, pBuf, ulResult);

        //������д�룬����д��λ��
        m_ulWriter += ulResult;
        m_ulWriter %= m_ulBufferSize;
    }
    else//A�ο��໺�峤�Ȳ�������Ҫ��B��д��
    {
        ::memcpy(m_pBuffer+m_ulWriter, pBuf, ulASectionLen);
        //PCLINTע��˵������Ҫ���ǵ���Ч�ʣ�����������
        ::memcpy(m_pBuffer, pBuf+ulASectionLen, ulResult-ulASectionLen);/*lint !e662*/
        m_ulWriter = ulResult - ulASectionLen;//������д�룬����д��λ��
    }

    //������д�룬���»��������ݳ���
    m_ulDataSize += ulResult;

    as_mutex_unlock(m_pMutex);
    return ulResult;
}

//��õ�ǰ���������ݴ�С
uint32_t as_ring_cache::GetDataSize() const
{
    return m_ulDataSize;
}

//��õ�ǰ���໺���С
uint32_t as_ring_cache::GetEmptySize() const
{
    return (m_ulBufferSize - m_ulDataSize);
}

//�������
void as_ring_cache::Clear()
{
    as_mutex_lock(m_pMutex);
    m_ulReader = 0;
    m_ulWriter = 0;
    m_ulDataSize = 0;
    as_mutex_unlock(m_pMutex);
}

//��õ�ǰ�����������ݳ��Ⱥͻ��������ȵı����İٷ���
uint32_t as_ring_cache::GetUsingPercent() const
{
    //��ֹ����Ϊ0���쳣����
    if (0 == m_ulBufferSize)
    {
        return 0;
    }

    uint32_t ulCurrentUsingPercent = (m_ulDataSize*100)/m_ulBufferSize;

    return ulCurrentUsingPercent;
}

