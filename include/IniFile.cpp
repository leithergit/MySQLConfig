#include "IniFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

////////////////////////////////////////////////////////////////////////////////////////
void CIniFile::ReadSection(StringArray& setArray)
{
	char szBuf[1024*64];
	memset(szBuf, 0, sizeof(szBuf));
	char* p = szBuf;
	int nLen = 0;

	if (GetPrivateProfileStringA(NULL, NULL, "", szBuf, sizeof(szBuf)/sizeof(char), m_strFile.c_str()) > 0)
	{
		while (*p != '\0')
		{
			setArray.push_back(p);
			nLen = (int)strlen(p) + 1;
			p += nLen;
		}  
	}
}

bool CIniFile::ReadSectionKey(StringArray& setArray, const char* pSection)
{
    char szBuf[1024*64] = {0};
	char* p = szBuf;
	int nLen = 0;
	if (!pSection)
	{
		if (!m_pCurrentSection)
			return false;
		else
			pSection = m_pCurrentSection;
	}
		
	if (GetPrivateProfileStringA(pSection, NULL, "", szBuf, sizeof(szBuf)/sizeof(char), m_strFile.c_str()) > 0)
	{
		while (*p != '\0')
		{
			setArray.push_back(p);
			nLen = (int)strlen(p) + 1;
			p += nLen;
		}  
	}
	return true;
}

bool CIniFile::ReadSectionString(StringArray& setArray, const char* pSection)
{
	if (!pSection)
	{
		if (!m_pCurrentSection)
			return false;
		else
			pSection = m_pCurrentSection;
	}
	StringArray ayKey;
	std::string strItem;
	ReadSectionKey(ayKey,pSection);
	for (int i = 0; i< ayKey.size(); ++i)
	{
		ReadKey(ayKey[i], strItem,pSection);
		if (strItem.size())
			setArray.push_back(strItem);
	}
	return true;
}

bool CIniFile::ReadKey(const char* pKey,std::string& strItem, const char* pSection)
{
	if (!pSection)
	{
		if (!m_pCurrentSection)
			return false;
		else
			pSection = m_pCurrentSection;
	}
	char szReturn[1024*4];
	memset(szReturn, 0, sizeof(szReturn));

	strItem.clear();
	if (GetPrivateProfileStringA(pSection, pKey, "", szReturn, _countof(szReturn), m_strFile.c_str()) > 0)
	{
		strItem = szReturn;
	}
	
	return true;
}

bool CIniFile::ReadKey(std::string& pKey, std::string& strItem, const char* pSection)
{
	if (!pSection)
	{
		if (!m_pCurrentSection)
			return false;
		else
			pSection = m_pCurrentSection;
	}
	char szReturn[1024 * 4];
	memset(szReturn, 0, sizeof(szReturn));

	strItem.clear();
	if (GetPrivateProfileStringA(pSection, pKey.c_str(), "",szReturn, _countof(szReturn),m_strFile.c_str()) > 0)
	{
		strItem = szReturn;
	}
	return true;
}
bool CIniFile::ReadKey(const char* pKey, int& nValue, const char* pSection)
{
	if (!pSection)
	{
		if (!m_pCurrentSection)
			return false;
		else
			pSection = m_pCurrentSection;
	}
	char szReturn[32];
	memset(szReturn, 0, sizeof(szReturn));
	if (GetPrivateProfileStringA(pSection, pKey, "", szReturn, _countof(szReturn),m_strFile.c_str()) > 0)
		nValue = strtol(szReturn,nullptr,10);
	else
		nValue = 0;
	return true;
}

bool CIniFile::ReadKey(const char* pKey, short& nValue, const char* pSection)
{
	if (!pSection)
	{
		if (!m_pCurrentSection)
			return false;
		else
			pSection = m_pCurrentSection;
	}
	int nIntValue = 0;
	ReadKey( pKey, nIntValue,pSection);
	nValue = (short)nIntValue;
	return true;
}

bool CIniFile::WriteKey(const char* pKey, const char* pItem, const char* pSection)
{
	if (!pSection)
	{
		if (!m_pCurrentSection)
			return false;
		else
			pSection = m_pCurrentSection;
	}
	WritePrivateProfileStringA(pSection, pKey, pItem, m_strFile.c_str());
	return true;
}

bool CIniFile::WriteKey(const char* pKey, std::string pItem, const char* pSection)
{
	if (!pSection)
	{
		if (!m_pCurrentSection)
			return false;
		else
			pSection = m_pCurrentSection;
	}
	WritePrivateProfileStringA(pSection, pKey, pItem.c_str(), m_strFile.c_str());
	return true;
}

bool CIniFile::EraseKey(const char* pKey, const char* pSection)
{
	if (!pSection)
	{
		if (!m_pCurrentSection)
			return false;
		else
			pSection = m_pCurrentSection;
	}
	WritePrivateProfileStringA(pSection, pKey, NULL, m_strFile.c_str());
	return true;
}

bool CIniFile::WriteKey(const char* pKey, int nValue, const char* pSection)
{
	if (!pSection)
	{
		if (!m_pCurrentSection)
			return false;
		else
			pSection = m_pCurrentSection;
	}
	char szValue[32];
	sprintf(szValue, "%d", nValue);
	WriteKey( pKey, szValue,pSection);
	return true;
}

void CIniFile::EraseSection(const char* pSection)
{
	WritePrivateProfileStructA(pSection, NULL, NULL, 0, m_strFile.c_str());
}


////////////////////////////////////////////////////////////////////////////////////////
