
/******************************************************************************
   ��Ȩ���� (C), 2008-2011, M.Kernel

 *
 *                 ���λ�����
 *
 *    ������
 *      1���������� 2008-08-07
 *
 *    ˵����
 *      1����дλ��˵��
 *         readerΪ������ͷ�����������ݿ�ʼλ��
 *         writerΪ������ͷ������д���ݿ�ʼλ��
 *
 *      2�����ݱ�����Ҫ���������������ͼ��
 *
 *         A��   reader      writer
 *                 ��          ��
 *           ��������������������������������
 *
 *         B��   writer              reader
 *                 ��                  ��
 *           ��������������������������������
 *
 */

#ifndef _RING_CACHE_H_
#define _RING_CACHE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
extern "C"{
#include "as_mutex.h"
}
class as_ring_cache
{
    public:
        as_ring_cache();
        virtual ~as_ring_cache();
    public:

        //��õ�ǰ�����������ݳ��Ⱥͻ��������ȵı����İٷ���
        unsigned long GetUsingPercent() const;

        //���û�������С������������ɺ󻺳�Ĵ�С
        unsigned long SetCacheSize(unsigned long ulCacheSize);

        //��õ�ǰ��������С
        unsigned long GetCacheSize() const;

        //�鿴ָ���������ݣ�����������Ȼ������Щ���ݣ�����ʵ�ʶ�ȡ���ݳ���
        unsigned long Peek(char* pBuf, unsigned long ulPeekLen);

        //��ȡָ���������ݣ�������Щ���ݴӻ����������������ʵ�ʶ�ȡ���ݳ���
        unsigned long Read(char* pBuf, unsigned long ulReadLen);

        //дָ���������ݣ�����ʵ��д���ݳ��ȣ����������ռ䲻������ֹд��
        unsigned long Write(const char* pBuf, unsigned long ulWriteLen);

        //��õ�ǰ���������ݴ�С
        unsigned long GetDataSize() const;

        //��õ�ǰ���໺���С
        unsigned long GetEmptySize() const;

        //�������
        void Clear();
    private:
        as_mutex_t*      m_pMutex;    //���������ʱ���

        char*    m_pBuffer;        //������
        unsigned long    m_ulBufferSize;    //��������С
        unsigned long    m_ulDataSize;    //���ݳ���

        //�������������ֽڴ�0��ʼ��ţ���(m_lBufferSize-1)
        //������ֵ���Ǵ˱�ţ���������ƫ��ֵ
        unsigned long    m_ulReader;        //������ͷ��(���ݶ�ȡ��)
        unsigned long    m_ulWriter;        //������ͷ��(����д���)
};

#endif
