// INIConfig.h: interface for the as_ini_config class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INICONFIG_H__C94A8432_F6BA_4C88_99BA_7C5CA0139EC7__INCLUDED_)
#define AFX_INICONFIG_H__C94A8432_F6BA_4C88_99BA_7C5CA0139EC7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
#include <fstream>
#include <map>

using namespace std;



//key = value,key=value .........
typedef map<string , string> items; 
//[section] ---> key key key .........
typedef map<string , items> sections;


enum ErrorType
{
    INI_SUCCESS         =  0,
    INI_OPENFILE_FAIL   = -1,
    INI_SAVEFILE_FAIL   = -2,
    INI_ERROR_NOSECTION = -3,
    INI_ERROR_NOKEY     = -4
};

#define     INI_CONFIG_MAX_SECTION_LEN        63

#define     INI_CONFIG_LINE_SIZE_MAX          2048

class as_ini_config  
{
public:
    as_ini_config();
    virtual ~as_ini_config();
public:
    //��ini�ļ�
    virtual long ReadIniFile(const string &filename);
    //����ini�ļ�(�Ḳ��������Ϣ)
    long SaveIniFile(const string & filename = "");
    //����Section��key��Ӧ��value
    void SetValue(const string & section, const string & key, 
        const string & value);
    //��ȡSection��key��Ӧ��value
    virtual long GetValue(const string & section, const string & key, 
        string & value);

    //��ȡһ����������Ϣ
    long GetSection(const string & section, items & keyValueMap);
    //���������õ�����ĳ���Ѿ����ڵ������ļ�
    //��SaveIniFile�������ǣ�ֻ���޸���Ӧֵ�������Ḳ��
    long ExportToFile(const string &filename);

    //���������õ�����ĳ���Ѿ����ڵ������ļ�
    //��ExportToFile�������ǣ�ָ����Section����Ҫ����,ԭ�ļ����ļ����������Ҳ�ᱣ������
    long ExportToFileExceptPointed(const string &filename, 
                            unsigned long ulSectionCont, const char szSection[][INI_CONFIG_MAX_SECTION_LEN+1]);
                            
private:

     string m_strFilePath;//���ļ�·��      
     sections m_SectionMap; //�����ļ�����,map

};

#endif // !defined(AFX_INICONFIG_H__C94A8432_F6BA_4C88_99BA_7C5CA0139EC7__INCLUDED_)
