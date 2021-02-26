#pragma once
#include <vector>
#include <string>
#include <windows.h>
typedef  std::vector<std::string> StringArray;
class CIniFile
{
public:
	CIniFile() : m_strFile("") {}
	CIniFile(const std::string &strFile) :m_strFile(strFile)
	{
		SetFileAttributesA( m_strFile.c_str(), FILE_ATTRIBUTE_NORMAL );
	}
	~CIniFile() 
	{
		if (m_pCurrentSection)
			delete[]m_pCurrentSection;
	}

	void SetFileName(const std::string &strFile)
	{ 
		m_strFile = strFile;
		SetFileAttributesA( m_strFile.c_str(), FILE_ATTRIBUTE_NORMAL );
	}
	const std::string GetFileName() const { return m_strFile; }

	void ReadSection(StringArray& setArray);
	bool ReadSectionKey(StringArray& setArray,const char* pSection = nullptr );
	bool ReadSectionString(StringArray& setArray, const char* pSection = nullptr);
	bool ReadKey(const char* pKey, std::string &strItem, const char* pSection = nullptr);
	bool ReadKey(std::string& pKey, std::string& strItem, const char* pSection = nullptr);
	bool ReadKey(const char* pKey, int& nValue, const char* pSection = nullptr);
	bool ReadKey(const char* pKey, short& nValue, const char* pSection = nullptr);
	bool WriteKey(const char* pKey, const char* pItem, const char* pSection = nullptr);
	bool WriteKey(const char* pKey, std::string pItem, const char* pSection = nullptr);
	bool WriteKey(const char* pKey, int nValue, const char* pSection = nullptr);
	
	bool EraseKey(const char* pKey, const char* pSection = nullptr);
	void EraseSection(const char* pSection);
	void EnterSection(const char* pSection)
	{
		if (pSection)
		{
			m_pCurrentSection = new char[strlen(pSection) + 1];
			strcpy(m_pCurrentSection, pSection);
		}
		else
		{
			if (m_pCurrentSection)
				delete[]m_pCurrentSection;
			m_pCurrentSection = nullptr;
		}
	}
	void LeaveSection()
	{
		if (m_pCurrentSection)
			delete[]m_pCurrentSection;
		m_pCurrentSection = nullptr;
	}

protected:
	std::string m_strFile;
	char* m_pCurrentSection = nullptr;
};
