/******************************************************************
'' �ļ���:as_ini_config.cpp
'' Copyright (c) .......
'' ��������:��ȡ����ini�ļ�
'' ǣ�����ݱ�����ͼ���洢���̣���
'' �������ļ�:INIConfig.h
'' ������:hexin
'' �ա���: 2008-8-12
'' �޸���:
'' �ա���:
'' �޸�˵��:
'' �桡��:Version 1.0
'' ******************************************************************/

#ifdef WIN32
#pragma warning(disable: 4786 4503)//ȥ��map�ľ���
#endif
#include <string.h>
#include "as_ini_config.h"

/***********************ini�ļ���ʽ*********************************
     [SectionOne]
      Key1 = Value
      Key2 = Value
        .....
     [SectionTwo]
        .....
********************************************************************/

as_ini_config::as_ini_config()
{

}

as_ini_config::~as_ini_config()
{

}


/******************************************************************************
Function:        GetValue 
Description:    ��ȡֵ
Calls:             
Called By:    
Input:            section:��ѯ��section
                key:��ѯ��key            
Output:            value:��Ҫ��ȡ��ֵ
Return:            �ɹ�����SUCCESS ���򷵻ش�����
******************************************************************************/
//PCLINTע��˵���������ڲ������øú���
int32_t as_ini_config::GetValue (const string &section, const string &key, string &value)/*lint -e1714 �ݲ����øú�������ʱ�����ȴ���չʹ��*/
{
    if ( m_SectionMap.find(section) == m_SectionMap.end() )
    {
        return INI_ERROR_NOSECTION;    
    }
  
    if ( m_SectionMap[section].find(key) == m_SectionMap[section].end() )
    {
        return INI_ERROR_NOKEY;  
    }
  
    value = m_SectionMap[section][key];
    return INI_SUCCESS;
}


/******************************************************************************
Function:        SetValue 
Description:    ����һ��ֵ
Calls:             
Called By:    
Input:            section:���õ�section
                key:���õ�key
                value:��Ҫ���õ�ֵ
Output:            ��
Return:            ��
******************************************************************************/
void as_ini_config::SetValue (const string &section, const string &key, const string &value)
{
    m_SectionMap[section][key] = value;
}

/******************************************************************************
Function:         ReadIniFile
Description:    �������ļ�
Calls:             
Called By:    
Input:            filename��Ҫ�򿪵��ļ�
Output:            ��
Return:            �ɹ�����SUCCESS ���򷵻ش�����
******************************************************************************/
int32_t as_ini_config::ReadIniFile (const string &filename)
{
    m_strFilePath = filename;
  
    ifstream input( filename.c_str() );
  
    if ( input.fail() )
    {
        return INI_OPENFILE_FAIL;
    }
  
    string line;
    string section;
    string left_value;
    string right_value;
    string::size_type pos;
    
    while ( !input.eof() )
    {
        (void)getline( input, line );
    
        // ȥע�� 
        pos = line.find('#', 0);
        if (pos != string::npos)
        {
            (void)line.erase(pos);
        }
        
        // ȥ�����˿ո�  
        (void)line.erase(0, line.find_first_not_of("\t "));
        (void)line.erase(line.find_last_not_of("\t\r\n ") + 1);
    
        if ( line == "" )
        {
            continue;
        }  
    
    
        // ����section 
        if ( ( line[0] == '[' ) && ( line[line.length() - 1] == ']' ) )
        {
            section = line.substr(1, line.length() - 2);
            continue;
        }
          
        // �����Ⱥ� 
        pos = line.find('=', 0);
        if ( pos != string::npos )
        {
            left_value = line.substr(0, pos);
            right_value = line.substr(pos + 1, (line.length() - pos) - 1);
      
            m_SectionMap[section][left_value] = right_value;
        }
    }
    
    input.close();
    return INI_SUCCESS;
}



/******************************************************************************
Function:        RegisterCallBack 
Description:    ע��ص�����
Calls:             
Called By:    
Input:            pCallBackInfo ע��ص���������Ϣ
Output:            ��
Return:            �ɹ�����SUCCESS ���򷵻ش�����
******************************************************************************/
int32_t as_ini_config::SaveIniFile ( const string & filename )
{
    if ( filename != "" )
    {
        m_strFilePath = filename;
    }

    ofstream output( m_strFilePath.c_str() );

    if ( output.fail() )
    {
        return INI_OPENFILE_FAIL;
    }

    for ( sections::iterator it_section = m_SectionMap.begin();
            it_section != m_SectionMap.end(); ++it_section)
    {
        if ( it_section->first != "" )
        {
            output << "[" << it_section->first << "]" << endl;
            if(output.fail())
            {
                return INI_SAVEFILE_FAIL;
            }
        }
    
        for ( items::iterator it_item = (it_section->second).begin();
                it_item != (it_section->second).end(); ++it_item )
        {
            output << it_item->first << "=" << it_item->second << endl;
            if(output.fail())
            {
                return INI_SAVEFILE_FAIL;
            }
        }
    
        output << endl;
    }
   
    return INI_SUCCESS;
}


//��ȡһ����������Ϣ
int32_t as_ini_config::GetSection(const string & section, items & keyValueMap)
{
    sections::iterator iter = m_SectionMap.find(section);
    if ( iter == m_SectionMap.end() )
    {
        return INI_ERROR_NOSECTION;    
    }
    
    items& foundItems = iter->second;
    items::iterator Itemiter = foundItems.begin();
    items::iterator ItemEnd = foundItems.end();
    while(Itemiter != ItemEnd)
    {
        keyValueMap[Itemiter->first] = Itemiter->second;        
        ++Itemiter;
    }

    return INI_SUCCESS;
}

//���������õ�����ĳ���Ѿ����ڵ������ļ�
//��SaveIniFile�������ǣ�ֻ���޸���Ӧֵ�������Ḳ�ǣ�ԭ�ļ����ļ����������Ҳ�ᱣ������
int32_t as_ini_config::ExportToFile(const string &filename)
{
    as_ini_config iniRecv;     //�������õ��ļ�
    int32_t lResult = iniRecv.ReadIniFile(filename);
    if(lResult != INI_SUCCESS)
    {
        return lResult;
    }

    for(sections::iterator it_section = m_SectionMap.begin();
            it_section != m_SectionMap.end(); ++it_section)
    {
        for(items::iterator it_item = (it_section->second).begin();
                it_item != (it_section->second).end(); ++it_item )
        {
            iniRecv.SetValue(it_section->first, it_item->first, it_item->second);
        }
    }

    //����
    lResult = iniRecv.SaveIniFile();

    return lResult;
}

//���������õ�����ĳ���Ѿ����ڵ������ļ�
//��ExportToFile�������ǣ�ָ����Section����Ҫ����
int32_t as_ini_config::ExportToFileExceptPointed(const string &filename, 
                            uint32_t ulSectionCont, const char szSection[][INI_CONFIG_MAX_SECTION_LEN+1])
{
    as_ini_config iniRecv;     //�������õ��ļ�
    int32_t lResult = iniRecv.ReadIniFile(filename);
    if(lResult != INI_SUCCESS)
    {
        return lResult;
    }

    for(sections::iterator it_section = m_SectionMap.begin();
            it_section != m_SectionMap.end(); ++it_section)
    {
        bool bPointed = false;
        //�ж��Ƿ�ָ���Ĳ���Ҫ������Section
        for(uint32_t i=0; i<ulSectionCont; i++)
        {
            if(0 == strcmp(it_section->first.c_str(), szSection[i]))
            {
                bPointed = true;
                break;
            }            
        }

        //�����ָ����Section��������һ��Section
        if(true == bPointed)
        {
            continue;
        }
        
        
        for(items::iterator it_item = (it_section->second).begin();
                it_item != (it_section->second).end(); ++it_item )
        {
            iniRecv.SetValue(it_section->first, it_item->first, it_item->second);
        }
    }

    //����
    lResult = iniRecv.SaveIniFile();

    return lResult;
}

