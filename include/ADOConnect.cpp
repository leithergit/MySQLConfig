// ADOConnect.cpp: implementation of the CADOConnection class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
//#define new DEBUG_NEW
#endif

#pragma warning (disable:4100)
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#include "ADOConnect.h"
#include "Runlog.h"
#include "utility.h"
#include "TimeUtility.h"
#include "stdio.h"
#include <TCHAR.h>
#include <time.h>
CRITICAL_SECTION CADOConnection::m_cs;
volatile bool CADOConnection::m_bInitialCS = false;
void CADOConnection::InitLock()
{
	if (!m_bInitialCS)
		InitializeCriticalSection(&m_cs);
}
void CADOConnection::UnInitLock()
{
	if (m_bInitialCS)
		DeleteCriticalSection(&m_cs);
}
CADOConnection::CADOConnection(void): m_ptrDBConnection(NULL), m_ptrCommand(NULL)
{

	::CoInitialize(NULL);
	m_hrExceptionCode = 0;
	ZeroMemory(m_szAdminAccount,256*sizeof(TCHAR));
	ZeroMemory(m_szDatabasePath,MAX_PATH*sizeof(TCHAR));
	ZeroMemory(m_szDBPWD,32*sizeof(TCHAR));
	InitializeCriticalSection(&m_cs);
// 	m_ptrRecordSet = NULL;
// 	m_ptrDBConnection = NULL;
// 	m_ptrCommand = NULL;
}

CADOConnection::~CADOConnection(void)
{
	if (m_ptrDBConnection != NULL && m_ptrDBConnection->State == adStateOpen)
	{
		HRESULT hr;
		TRY_ACCESS
		{
			m_ptrCommand = NULL ;
			m_ptrDBConnection = NULL;
			//m_ptrDBConnection->Release();
			m_ptrRecordSet = NULL;
		}
		CATCH_ACCESS(hr);		
		::CoUninitialize();
	}
	DeleteCriticalSection(&m_cs);
}

//�Ͽ����ݿ������
BOOL CADOConnection::DisConnect()
{
	HRESULT hr;
	TRY_ACCESS
	{
		if (m_ptrDBConnection != NULL && m_ptrDBConnection->State == adStateOpen) 
		{
			if (SUCCEEDED(m_ptrDBConnection->Close()))
			{
				m_ptrCommand = NULL;
				m_ptrDBConnection = NULL;
				m_ptrRecordSet = NULL;				
				return TRUE;
			}
		}
	}
	CATCH_ACCESS(hr);
	return FALSE;
}

//��sql ��������
// pServerName -	����������
// pDataBase		���ݿ�����
// pId				�û���
// pPwd				����
HRESULT CADOConnection::InitialSQLSERVER(LPCTSTR pServerName,LPCTSTR pDataBase,LPCTSTR pId, LPCTSTR pPwd)
{
	if ( !pServerName  ||!pDataBase)
	{
		return S_FALSE;
	}
	HRESULT hr = E_UNEXPECTED;
	TRY_ACCESS
	{		
		TCHAR szConnectstr[512] = {0};
		int nLen = 0;		

		_tcscpy_s(m_szDatabasePath,MAX_PATH,pDataBase);

		//_stprintf_s(szConnectstr, 512, _T("Provider=SQLOLEDB; ")
		//	_T("Server=%s; ")
		//	_T("Database=%s;"),
		//	pServerName, pDataBase);
		
		_stprintf_s(szConnectstr, 512,
			_T("Provider=SQLNCLI11; ")
			_T("Server=%s; ")
			_T("Database=%s;")
			_T("DataTypeCompatibility=80;")
			_T("Connect Timeout=5;")
			_T("AutoTranslate=true;"),
			pServerName, pDataBase);

		nLen = _tcslen(szConnectstr);
		if ( pId != NULL ) 
		{
			nLen = _tcslen(szConnectstr);
			_stprintf_s((TCHAR *)&szConnectstr[nLen],(512 - nLen),_T("uid=%s;"),pId );
			_tcscpy_s(m_szAdminAccount,256,pId);
		}
		else
			_tcscat_s(szConnectstr,512 - nLen,_T("uid=;"));
		nLen = _tcslen(szConnectstr);
		if ( pPwd != NULL && _tcslen(pPwd))
		{
			_stprintf_s((TCHAR *)&szConnectstr[nLen],(512 - nLen),_T("pwd=%s;"),pPwd);
			_tcscpy_s(m_szDBPWD,32,pPwd);
		}
		else
		{
			_tcscat_s(szConnectstr,512 - nLen,_T("pwd=;"));
		}
		
		//strCnn.Assign()
		_bstr_t strConnect(szConnectstr);
		m_hrExceptionCode = 0;
	
		// hr = 0x800401f0 ��δ���� CoInitialize�� 
		hr = m_ptrDBConnection.CreateInstance(__uuidof(Connection));
		m_ptrDBConnection->ConnectionTimeout = 5;
		m_ptrDBConnection->CommandTimeout = 5;

		hr = m_ptrDBConnection->Open(strConnect, _T(""),_T(""),adConnectUnspecified);
		hr = m_ptrCommand.CreateInstance(__uuidof(Command));
		m_ptrCommand->ActiveConnection = m_ptrDBConnection;
		m_ptrCommand->CommandType = adCmdText;
		hr = m_ptrCommand->Parameters->Refresh();
	}
	CATCH_ACCESS(hr);
	if FAILED(hr)
	{
		m_ptrDBConnection = NULL;
		m_ptrCommand = NULL;
	}
	return hr;
}

_RecordsetPtr CADOConnection::OpenTable(TCHAR *szTable,
										CursorTypeEnum CursorType, 
										LockTypeEnum LockType, 
										CommandTypeEnum  Options)
{
	if (m_ptrDBConnection == NULL || szTable == NULL)
		return S_FALSE;
	HRESULT hr = E_UNEXPECTED;
	_RecordsetPtr rsptr;
	TRY_ACCESS
	{		
		hr = rsptr.CreateInstance(__uuidof(Recordset));
		if (FAILED(hr))
		{
			TraceMsg(_T("%s ����_RecordsetPtr����ʧ��.\r\n"));
			return NULL;
		}

		hr = rsptr->Open(_variant_t(szTable),
					m_ptrDBConnection.GetInterfacePtr(),
					CursorType,
					LockType,
					Options);
		if (FAILED(hr))
		{			
			TraceMsg(_T("%s �����ݱ�%sʧ��.\r\n"),szTable);
			return NULL;
		}
		return rsptr;
	}
	CATCH_ACCESS(hr);
	if (SUCCEEDED(hr))
		return rsptr;
	else
	{
		rsptr = NULL;
		return NULL;
	}
}
_RecordsetPtr	CADOConnection::OpenTableBatch(TCHAR *szTable,
									   CursorTypeEnum CursorType, 
									   LockTypeEnum LockType, 
									   CommandTypeEnum Options )
{
	if (m_ptrDBConnection == NULL || szTable == NULL)
		return nullptr;
	HRESULT hr = E_UNEXPECTED;
	_RecordsetPtr pRecordPtr;
	TRY_ACCESS
	{
		hr = pRecordPtr.CreateInstance(_T("ADODB.Recordset"));
		if (FAILED(hr))
		{
			TraceMsg(_T("������¼��ʧ��.\r\n"));
			return nullptr;
		}
		hr = pRecordPtr->Open(_variant_t(szTable), m_ptrDBConnection.GetInterfacePtr(), CursorType, LockType, Options);
		if (FAILED(hr))
		{
			TraceMsg(_T("%s �����ݱ�%sʧ��.\r\n"),szTable);
			return nullptr;
		}
	}
	CATCH_ACCESS(hr);
	if (FAILED(hr))
	{
		pRecordPtr = NULL;
	}
	return pRecordPtr;
}

//��access��������
// pDataBase		���ݿ�·��
// pId				�û���
// pPwd				����
HRESULT CADOConnection::InitialAccess(LPCTSTR pDataBase, LPCTSTR pId, LPCTSTR pPwd)
{
	if ( pDataBase == NULL ) 
		return S_FALSE;	
	HRESULT hr = E_UNEXPECTED;
	TRY_ACCESS
	{
		TCHAR szConnectStr[512] = {0};			
		int nLen = 0;

		_tcscpy_s(m_szDatabasePath,MAX_PATH,pDataBase);
		_stprintf_s(szConnectStr,512,_T("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%s;"),pDataBase);
		nLen = _tcslen(szConnectStr);
		if ( pId != NULL ) 
		{
			_stprintf_s((TCHAR *)&szConnectStr[nLen],512 - nLen,_T("User ID=%s;"),pId );
			_tcscpy_s(m_szAdminAccount,256,(LPTSTR)pId);			
		}
		else
		{
			_tcscat_s(szConnectStr,512 - nLen,_T("User ID=Admin;"));		
			_tcscpy_s(m_szAdminAccount, 256, _T("Admin"));
		}
		nLen = _tcslen(szConnectStr);
		if ( pPwd != NULL )
		{
			_stprintf_s((TCHAR *)&szConnectStr[nLen],512 - nLen,_T("Jet OLEDB:DataBase Password=%s;"),pPwd);
			_tcscpy_s(m_szDBPWD,32,pPwd);

		}
		else		
			_tcscat_s(szConnectStr,512 - nLen,_T("Password=\"\";"));	

		_bstr_t strConnect;
		strConnect = szConnectStr;	
		m_hrExceptionCode = 0;
		TraceMsg(_T("Access DB Connect string = %s\r\n"),szConnectStr);
		hr = m_ptrDBConnection.CreateInstance(__uuidof(Connection));
		hr = m_ptrDBConnection->Open(strConnect, "","",adConnectUnspecified);
		hr = m_ptrCommand.CreateInstance(__uuidof(Command));
		m_ptrCommand->ActiveConnection = m_ptrDBConnection;		
		m_ptrCommand->CommandType = adCmdText;
		m_ptrCommand->Parameters->Refresh();		
	}
	CATCH_ACCESS(hr);
	if (FAILED(hr))
	{
		m_ptrDBConnection = NULL;
		m_ptrCommand = NULL;
	}
	
	return hr;
}

HRESULT CADOConnection::InitialExcel(LPCTSTR pExcelFile, bool bCatchExcption)
{
	if (pExcelFile == NULL)
		return S_FALSE;
	HRESULT hr = E_UNEXPECTED;
	TCHAR szConnectStr[512] = { 0 };
	int nLen = 0;

	_tcscpy_s(m_szDatabasePath, MAX_PATH, pExcelFile);
	// �����ļ���չ��������ʹ����һ����������
	// 
	// 
	TCHAR *szExtendedProperties = _T("Provider=Microsoft.JET.OLEDB.4.0;Extended Properties=\"Excel 8.0;HDR=No;IMEX=1\"");
	//TCHAR *szExtendedProperties = _T("Provider=MSDASQL.1;Persist Security Info=False;DRIVER={Microsoft Excel Driver (*.xls)};Extended Properties=\"Excel 8.0\"");
	//TCHAR *szConnectStr = _T("Driver={Microsoft Excel Driver (*.xls)};DriverId=790;Dbq = D:\Mydocuments\T.xls; DefaultDir = D:\Mydocuments;");
	//LPCTSTR pDest = _tcsstr(pExcelFile, _T(".xlsx"));
	//if (_tcsstr(pExcelFile, _T(".xlsx")) > 0)
	//	szExtendedProperties = _T("Provider=Microsoft.ACE.OLEDB.12.0;Extended Properties = \"Excel 12.0 Xml\"");

	_stprintf_s(szConnectStr, 512, _T("%s;Data Source=\"%s\";"), szExtendedProperties, pExcelFile);
	nLen = _tcslen(szConnectStr);
	if (bCatchExcption)
	{
		TRY_ACCESS
		{
			_bstr_t strConnect;
			strConnect = szConnectStr;
			m_hrExceptionCode = 0;
			TraceMsg(_T("Access DB Connect string = %s\r\n"), szConnectStr);
			hr = m_ptrDBConnection.CreateInstance(__uuidof(Connection));
			hr = m_ptrDBConnection->Open(strConnect, "", "", adConnectUnspecified);
			hr = m_ptrCommand.CreateInstance(__uuidof(Command));
			m_ptrCommand->ActiveConnection = m_ptrDBConnection;
			m_ptrCommand->CommandType = adCmdText;
			m_ptrCommand->Parameters->Refresh();
		}
		CATCH_ACCESS(hr);
		if (FAILED(hr))
		{
			m_ptrDBConnection = NULL;
			m_ptrCommand = NULL;
		}
	}
	else
	{
		_bstr_t strConnect;
		strConnect = szConnectStr;
		m_hrExceptionCode = 0;
		TraceMsg(_T("Access DB Connect string = %s\r\n"), szConnectStr);
		hr = m_ptrDBConnection.CreateInstance(__uuidof(Connection));
		hr = m_ptrDBConnection->Open(strConnect, "", "", adConnectUnspecified);
		hr = m_ptrCommand.CreateInstance(__uuidof(Command));
		m_ptrCommand->ActiveConnection = m_ptrDBConnection;
		m_ptrCommand->CommandType = adCmdText;
		hr = m_ptrCommand->Parameters->Refresh();
	}
	
	return hr;
}
//ִ��sql���
_RecordsetPtr CADOConnection::RunSQL(LPCTSTR pSQLStatement,  bool bCatchException , void *plog)
{
	if (!bCatchException)
	{
		m_ptrCommand->CommandText = pSQLStatement;
		m_ptrCommand->Parameters->Refresh();
		m_hrExceptionCode = 0;
		return m_ptrCommand->Execute(NULL, NULL, adCmdText);
	}
	else
	try
	{
		m_ptrCommand->CommandText = pSQLStatement;		
		m_ptrCommand->Parameters->Refresh();
		m_hrExceptionCode = 0;
		return m_ptrCommand->Execute(NULL,NULL,adCmdText);	
	}
	
	catch(_com_error e)
	{
		m_hrExceptionCode = e.Error();
#ifndef _DEBUG
		if (plog != NULL)
		{
			CRunlog *pRunlog = (CRunlog *)plog;
			int nLen = _tcslen(pSQLStatement);		//�ֶ���ʾ,����TraceMsg���������������쳣
			int nSegment = nLen / 255;
			char szTemp[256] = {0};
			pRunlog->Runlog(_T("The Exception Occured at :File %s Line %d\r\n"),__FILE__,__LINE__);		
			pRunlog->Runlog(_T("SQLStatement = "));
			int i = 0;
			for ( i = 0;i < nSegment; i ++)
			{		
				memcpy(szTemp,&pSQLStatement[i*255],255);
				pRunlog->Runlog(_T("%s"),szTemp);
			}
			ZeroMemory(szTemp,256);
			memcpy(szTemp,&pSQLStatement[i*255],nLen - 255*i);
			pRunlog->Runlog(_T("%s\r\n"),szTemp);		
			pRunlog->Runlog(_T("Code = %08lx\tMeaning = %s\n"), e.Error(),e.ErrorMessage());
			pRunlog->Runlog(_T("Source = %s\n"), (LPCSTR) e.Source());
			pRunlog->Runlog(_T("Description = %s\n"), (LPCSTR) e.Description());

		}
		//MessageBox(NULL,e.Description(),_T("AdoError"),MB_OK);
#endif
		int nLen = _tcslen(pSQLStatement);		//�ֶ���ʾ,����TraceMsg���������������쳣
		int nSegment = nLen / 255;
		TCHAR szTemp[256] = {0};
		TraceMsg(_T("The Exception Occured at :File %s Line %d\r\n"),__FILE__,__LINE__);
		TraceMsg(_T("SQLStatement = "));
		int i = 0;
		for ( i = 0;i < nSegment; i ++)
		{		
			memcpy(szTemp,&pSQLStatement[i*255],255);
			TraceMsg(_T("%s"),szTemp);
		}
		ZeroMemory(szTemp,256);
		memcpy(szTemp,&pSQLStatement[i*255],nLen - 255*i);
		TraceMsg(_T("%s\r\n"),szTemp);		
		TraceMsg(_T("Code = %08lx\tMeaning = %s\n"), e.Error(),e.ErrorMessage());
		TraceMsg(_T("Source = %s\n"), (LPCSTR) e.Source());
		TraceMsg(_T("Description = %s\n"), (LPCSTR) e.Description());
	}
	return NULL;
}

LONG CADOConnection::GetMaxField(LPCTSTR pTable, LPCTSTR pField)
{	
	int id = -1;
	HRESULT hr;
	TRY_ACCESS
	{
		TCHAR szSQL[512];
		ZeroMemory(szSQL,512*sizeof(TCHAR));
		_stprintf_s(szSQL,512,_T("SELECT MAX(%s) AS maxid ")
					_T("FROM %s"),pField,pTable);

		m_hrExceptionCode = 0;
		m_hrExceptionCode = 0;
		m_ptrCommand->CommandText = szSQL;
		m_ptrCommand->Parameters->Refresh();
		_RecordsetPtr res = m_ptrCommand->Execute(NULL,NULL,adCmdText);
		
		if ( !res->adoEOF )
		{
			_variant_t vId = res->GetCollect("maxid");
			if ( vId.vt != VT_NULL){
				id = atoi((LPCSTR)(_bstr_t)vId);
				id++;
			}
		}
		res->Close();
		res = NULL;	
		return id;
	}
	CATCH_ACCESS(hr);
	return -1;
}
/************************************************************************/
/*  return -1 not find                                                                     */
/************************************************************************/
LONG CADOConnection::GetMatchField(LPCTSTR pTable, LPCTSTR pField, LPCTSTR  WhereExpr)
{
	LONG nID = -1;
	TCHAR szSQL[512] = {0}	;
	HRESULT hr;
	CAdoLock(m_cs);
	TRY_ACCESS
	{		
		_stprintf_s(szSQL,512,_T("SELECT %s FROM %s %s "), pField, pTable, WhereExpr );
		m_ptrCommand->CommandText = szSQL;
		m_ptrCommand->Parameters->Refresh();
		_RecordsetPtr res = m_ptrCommand->Execute(NULL,NULL,adCmdText);
		if ( !res->adoEOF )
		{
			_variant_t vId = res->GetCollect(pField);
			if ( vId.vt != VT_NULL)
			{
				nID = atoi((LPCSTR)(_bstr_t)vId);
			}
		}
		res->Close();
		res = NULL;
		return nID;
	}	
	CATCH_ACCESS(hr);
	return nID;
}
_RecordsetPtr CADOConnection::GetRecordsetObj(LPCTSTR pSQLStatement,LPCTSTR szConnectionString,enum LockTypeEnum LockType)
{
	_RecordsetPtr rst;
	try
	{
		rst.CreateInstance(__uuidof(Recordset));
		rst->CursorType = adOpenStatic;
		TCHAR szConnectStr[255] = {0};
		if (!szConnectionString)		
			_tcscpy_s(szConnectStr,255,m_ptrDBConnection->GetConnectionString());
		else
			_tcscpy_s(szConnectStr,255,szConnectionString);
		rst->Open(pSQLStatement,szConnectStr,adOpenStatic,LockType,adCmdText);
		return rst;
	}
	catch(_com_error e)
	{
#ifndef _DEBUG
		MessageBox(NULL,e.Description(),_T("AdoError"),MB_OK);
#endif
		int nLen = _tcslen(pSQLStatement);		//�ֶ���ʾ,����TraceMsg���������������쳣
		int nSegment = nLen / 255;
		char szTemp[256] = {0};
		TraceMsg(_T("The Exception Occured at :File %s Line %d\r\n"),__FILE__,__LINE__);		
		TraceMsg(_T("SQLStatement = "));
		int i = 0;
		for (i = 0;i < nSegment; i ++)
		{		
			memcpy(szTemp,&pSQLStatement[i*255],255);
			TraceMsg(_T("%s"),szTemp);
		}
		ZeroMemory(szTemp,256);
		memcpy(szTemp,&pSQLStatement[i*255],nLen - 255*i);
		TraceMsg(_T("%s\r\n"),szTemp);		
		TraceMsg(_T("Code = %08lx\tMeaning = %s\n"), e.Error(),e.ErrorMessage());
		TraceMsg(_T("Source = %s\n"), (LPCSTR) e.Source());
		TraceMsg(_T("Description = %s\n"), (LPCSTR) e.Description());
	}
	return NULL;
}

void CADOConnection::PrintComError(_com_error &e)
{
	_bstr_t bstrSource(e.Source());
	_bstr_t bstrDescription(e.Description());

	// Print Com errors.  
// 	TRACE("Error\n");
// 	TRACE("\tCode = %08lx\n", e.Error());
// 	TRACE("\tCode meaning = %s\n", e.ErrorMessage());
// 	TRACE("\tSource = %s\n", (LPCSTR) bstrSource);
// 	TRACE("\tDescription = %s\n", (LPCSTR) bstrDescription);
}

//ͨ�������ַ���sql��������
// pStr		�����ַ���
// bShowErrMsg	�Ƿ���ʾ������Ϣ Ĭ���Ƿ�
HRESULT CADOConnection::InitialServerByConnectString(LPCTSTR pStr, BOOL bShowErrMsg)
{
	HRESULT hr = E_UNEXPECTED;
	TRY_ACCESS
	{
		_bstr_t strCnn(pStr);
		m_hrExceptionCode = 0;
		hr = m_ptrDBConnection.CreateInstance(__uuidof(Connection));
		hr = m_ptrDBConnection->Open(strCnn, "","",adConnectUnspecified);
		m_ptrCommand.CreateInstance(__uuidof(Command));
		m_ptrCommand->ActiveConnection = m_ptrDBConnection;		
		m_ptrCommand->CommandType = adCmdText;
		m_ptrCommand->Parameters->Refresh();
		
	}
	CATCH_ACCESS(hr);
	return hr;
}

HRESULT CADOConnection::ConnnectServerByString(LPCTSTR szConnect)
{
	HRESULT hr = E_UNEXPECTED;
	_bstr_t strCnn(szConnect);
	TRY_ACCESS
	{
		hr = m_ptrDBConnection.CreateInstance(__uuidof(Connection));
		hr = m_ptrDBConnection->Open(strCnn, "","",adConnectUnspecified);
		hr = m_ptrCommand.CreateInstance(__uuidof(Command));
		m_ptrCommand->ActiveConnection = m_ptrDBConnection;	
		m_ptrCommand->CommandType = adCmdText;
		m_ptrCommand->Parameters->Refresh();	
	}
	CATCH_ACCESS(hr);
	return hr;
}

// ��һ�ε���ʱ�Ͱ�pObject ��ΪNULL,�Ի�ȡ����ĳߴ�
HRESULT CADOConnection::GetOleObj(_RecordsetPtr pRecordSet,TCHAR *szField,void *pObject,int &nObjSize)
{	
	if (!pRecordSet ||
		!szField||
		pRecordSet->adoEOF||
		!pObject )		
		return S_FALSE;
	HRESULT hr = E_UNEXPECTED;	
	TRY_ACCESS
	{
		nObjSize = pRecordSet->GetFields()->GetItem(szField)->ActualSize;
		
		if(nObjSize > 0)
		{
			_variant_t	varBLOB;
			varBLOB = pRecordSet->GetFields()->GetItem(szField)->GetChunk(nObjSize);
			if(varBLOB.vt == (VT_ARRAY | VT_UI1))
			{				
				BSTR HUGEP *pbstr = NULL;
				SafeArrayAccessData(varBLOB.parray,(void HUGEP**)&pbstr);
				memcpy(pObject,pbstr,nObjSize);				///�������ݵ�������m_pBMPBuffer
				SafeArrayUnaccessData (varBLOB.parray);					
				return S_OK;
			}			
		}
		return S_FALSE;
	}
	CATCH_ACCESS(hr);
	return hr;
}
HRESULT CADOConnection::InsertOleObj(_RecordsetPtr pRecordSet,TCHAR *szField,void *pObject,int nObjSize)
{
	if (!pRecordSet ||
		!szField||
		pRecordSet->adoEOF||
		!pObject )		
		return S_FALSE;

	CAdoLock(m_cs);
	BYTE *pBuf = (byte *)pObject;     
	VARIANT			varBLOB;
	SAFEARRAY		*psa;
	SAFEARRAYBOUND	rgsabound[1];
	HRESULT hr;
	TRY_ACCESS
	{
		rgsabound[0].lLbound = 0;
		rgsabound[0].cElements = nObjSize;
		psa = SafeArrayCreate(VT_UI1, 1, rgsabound);
		for (long i = 0; i < (long)nObjSize; i++)
			SafeArrayPutElement (psa, &i, pBuf++);
		varBLOB.vt = VT_ARRAY | VT_UI1;
		varBLOB.parray = psa;
		return pRecordSet->GetFields()->GetItem(szField)->AppendChunk(varBLOB);	// in Sql Server db
		//return pRecordSet->Update(szField,varBLOB);			// in Access db
	}
	CATCH_ACCESS(hr);
	return hr;
}
HRESULT CADOConnection::BeginTrans()
{
	return m_ptrDBConnection->BeginTrans();
}

HRESULT CADOConnection::RollbackTrans()
{
	HRESULT hr;
	TRY_ACCESS
	{	
		return m_ptrDBConnection->RollbackTrans();
	}
	CATCH_ACCESS(hr);
	return E_FAIL;
}

HRESULT CADOConnection::CommitTrans()
{	
	return m_ptrDBConnection->CommitTrans();	
}

#define A2W(AnsiStr)	L##AnsiStr
_RecordsetPtr CADOConnection::ExecuteTransQuery(LPCTSTR pSQLStatement,bool bHandleException)
{
	HRESULT hr;
	CAdoLock(m_cs);
	if (bHandleException)
	{
		try
		{
			_variant_t RecordsAffected;
			m_hrExceptionCode = 0;
			return m_ptrDBConnection->Execute(pSQLStatement,&RecordsAffected,adCmdText);
			//m_ptrDBConnection->GetState() != adStateClosed;
		}
		catch( _com_error e)
		{
			OutputException(hr,pSQLStatement,e,__FILE__,__LINE__);
		}
		return NULL;
	}
	else
	{
		_variant_t RecordsAffected;		
		return m_ptrDBConnection->Execute(pSQLStatement,&RecordsAffected,adCmdText);
	}
}

void CADOConnection::OutputException(HRESULT &hr,LPCTSTR pSQLStatement,_com_error &e,LPCSTR szFile,int nLine)
{
	m_hrExceptionCode = e.Error();
	int nLen = _tcslen(pSQLStatement);		//�ֶ���ʾ,����TraceMsg���������������쳣
	int nSegment = nLen / 512;
	TCHAR szTemp[520] = {0};
	TCHAR szText[1024] = {0};
#ifdef _UNICODE		
	A2WHelper(szFile,szText,1024);
	TraceMsg(_T("The Exception Occured at %s Line %d\n"),szText,nLine);
#else
	TraceMsg(_T("The Exception Occured at :File %s Line %d\r\n"),szFile,nLine);
#endif
	TraceMsg(_T("SQLStatement = "));
	int i = 0;
	for (i = 0;i < nSegment; i ++)
	{		
		_tcsncpy_s(szTemp,520,(TCHAR *)&pSQLStatement[i*512],512);
		TraceMsg(_T("%s"),szTemp);
	}
	ZeroMemory(szTemp,520);
	_tcsncpy_s(szTemp,520,(TCHAR *)&pSQLStatement[i*512],nLen - 512*i);
	TraceMsg(_T("%s\r\n"),szTemp);		

#ifdef _UNICODE	
	TraceMsg(_T("Code = %08lx\n"), e.Error());		
	TraceMsg(_T("Meaning = %s\n"),e.ErrorMessage());
	A2WHelper((LPCSTR)e.Source(),szText,1024);
	TraceMsg(_T("Source = %s\n"), szText);
	A2WHelper((LPCSTR)e.Description(),szText,1024);
	TraceMsg(_T("Description = %s\n"), szText);
#else
	TraceMsg(_T("Code = %08lx\n"), e.Error());
	TraceMsg(_T("Meaning = %s\n"), e.ErrorMessage());
	TraceMsg(_T("Source = %s\n"), (LPCSTR) e.Source());
	TraceMsg(_T("Description = %s\n"), (LPCSTR) e.Description());
#endif
}

long CADOConnection::GetFieldActueSize(LPCTSTR)
{
	return 0;
}

//�޸�Access���ݿ�����
BOOL CADOConnection::ChangeAccessPassword(LPCTSTR lpszOldPass, LPCTSTR lpszNewPass)
{	
	HRESULT hr = E_UNEXPECTED;
	TRY_ACCESS
	{
		m_hrExceptionCode = 0;
		//�ر����ݿ�
		if ( m_ptrDBConnection != NULL )
		{
			hr = m_ptrDBConnection->Close();
			m_ptrDBConnection = NULL;
			m_ptrCommand = NULL;
		}
		int nLen = 0;
		TCHAR szConnect[512] = {0};
		_stprintf_s(szConnect,512,_T("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%s;"),m_szDatabasePath);
		nLen = _tcslen(szConnect);
		_stprintf_s((TCHAR *)&szConnect[nLen],512 - nLen,_T("User ID=%s;"),m_szAdminAccount );
		nLen = _tcslen(szConnect);
		//�Զ�ռ��ʽ��
		if ( lpszOldPass != NULL )		
			_stprintf_s((TCHAR *)&szConnect[nLen],512 - nLen,_T("Jet OLEDB:DataBase Password=%s;Mode=Share Deny Read|Share Deny Write"),lpszOldPass);
		else		
			_tcscat_s(szConnect,512 - nLen,_T("Jet OLEDB:DataBase Password=NULL;Mode=Share Deny Read|Share Deny Write"));
		_bstr_t strCnn;
		strCnn = szConnect;
		hr = m_ptrDBConnection.CreateInstance(__uuidof(Connection));
		hr = m_ptrDBConnection->Open(strCnn, "","",adConnectUnspecified);
		hr = m_ptrCommand.CreateInstance(__uuidof(Command));
		m_ptrCommand->ActiveConnection = m_ptrDBConnection;			
		m_ptrCommand->CommandType = adCmdText;
		m_ptrCommand->Parameters->Refresh();
		_RecordsetPtr rs = NULL;
		TCHAR szSQLStatements[512] = {0};
		_stprintf_s(szSQLStatements,512,_T("ALTER DATABASE PASSWORD [%s] [%s]"),lpszNewPass,lpszOldPass);
		rs = m_ptrDBConnection->Execute(szSQLStatements,NULL,adCmdText);
		if (rs != NULL)	//�����޸ĳɹ�,�����Թ���ʽ�����ݿ�
		{			 
			 hr = m_ptrDBConnection->Close();
			 m_ptrDBConnection = NULL;
			 m_ptrCommand = NULL;
			 hr = InitialAccess((LPCTSTR)m_szDatabasePath,(LPCTSTR)m_szAdminAccount,lpszNewPass);
			 return TRUE;
		}
	}
	CATCH_ACCESS(hr);
	return hr;
}


CADOConnectionEx::CADOConnectionEx(void)
: m_ptrLocalDB(NULL)
, m_ptrRemoteDB(NULL)
, m_bRemoteDBLinkStauts(false)
, m_hLinkDetectThread(NULL)
, m_bLinkDetectThreadRun(false)
{
	::CoInitialize(NULL);
	::InitializeCriticalSection(&m_csLinkStatus);
	m_hrExceptionCode = 0;
	ZeroMemory(m_szLocalConnectString,sizeof(m_szLocalConnectString));
	ZeroMemory(m_szRemoteConnectString,sizeof(m_szRemoteConnectString));
	_tcscpy_s(m_strLinkMissedStringArray[0],1024, _T("Query execution was interrupted"));
	_tcscpy_s(m_strLinkMissedStringArray[1],1024, _T("Lost connection to MySQL server during query"));
	_tcscpy_s(m_strLinkMissedStringArray[2],1024, _T("MySQL server has gone away"));
	m_nLinkMissedString = 3;
	
}

CADOConnectionEx::~CADOConnectionEx(void)
{
	HRESULT hr;
	TRY_ACCESS
	{				
		DisConnectAll();
		::DeleteCriticalSection(&m_csLinkStatus);
	}
	CATCH_ACCESS(hr);		
	::CoUninitialize();
}

bool CADOConnectionEx::AddLinkMissedString(LPCTSTR strLinkMissedString)
{
	if (m_nLinkMissedString < 8)
	{
		_tcscpy_s(m_strLinkMissedStringArray[m_nLinkMissedString ++],1024,strLinkMissedString);
		return true;
	}
	else
		return false;
}

_RecordsetPtr CADOConnectionEx::ExecuteTransQuery(LPCTSTR pSQLStatement,bool bHandleException)
{
	HRESULT hr = E_UNEXPECTED;
	_variant_t RecordsAffected;
	m_hrExceptionCode = 0;	
	CAdoLock(m_cs);
	if (GetRemoteLinkStatus())
	{
		try
		{
			return m_ptrRemoteDB->Execute(pSQLStatement,&RecordsAffected,adCmdText);
		}
		catch( _com_error e)
		{
			OutputException(hr,pSQLStatement,e,__FILE__,__LINE__);
			TCHAR szDescription[1024] =  {0};
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)e.Description(), -1, (LPWSTR)szDescription, 1024);
			// ��������쳣,�Ƿ�Ϊ��·�쳣
			for (int i = 0;i < m_nLinkMissedString;i ++)
			{
				if (_tcsstr(szDescription,(LPCTSTR)m_strLinkMissedStringArray[i]))
				{// ȷ��Ϊ�����쳣
					SetRemoteLinkStatus(false);
					break;
				}
			}				
			if (!GetRemoteLinkStatus())	// Զ�������Ѿ��Ͽ�
			{// ʹ�ñ������ݿ�
				// ������·����߳�
				if (!m_hLinkDetectThread)
				{
					m_bLinkDetectThreadRun = true;
					m_hLinkDetectThread = CreateThread(NULL,0,LinkDetectThread,this,THREAD_PRIORITY_NORMAL,NULL);					
				}
			}
			else	// ������·�Ͽ������������׳��쳣
			{
				if (!bHandleException)	// �����������쳣,������׳�ԭ�쳣
				{
					throw e;
					return NULL;
				}
			}
		}			
	}
	// ��Զ�����Ӳ�����ʱ,���������ݿ����,��ִ�е��������ݿ�
	if (!GetRemoteLinkStatus()  && m_ptrLocalDB)
	{
		if (bHandleException)
		{
			try
			{
				return m_ptrLocalDB->Execute(pSQLStatement,&RecordsAffected,adCmdText);
			}
			catch( _com_error e)
			{
				OutputException(hr,pSQLStatement,e,__FILE__,__LINE__);
			}
			return NULL;
		}
		else
			return m_ptrLocalDB->Execute(pSQLStatement,&RecordsAffected,adCmdText);				
	}
	else 
		return NULL;
}


HRESULT CADOConnectionEx::ConnnectRemoteServerByString(LPCTSTR szConnect,long nTimeOut)
{
	HRESULT hr = E_UNEXPECTED;
	_bstr_t strCnn(szConnect);
	try
	{
		_tcscpy_s(m_szRemoteConnectString,1024,szConnect);
		hr = m_ptrRemoteDB.CreateInstance(__uuidof(Connection));
		m_ptrRemoteDB->ConnectionTimeout = nTimeOut;
		hr = m_ptrRemoteDB->Open(strCnn, "","",adConnectUnspecified);
		hr = m_ptrRemoteCommand.CreateInstance(__uuidof(Command));
		m_ptrRemoteCommand->ActiveConnection = m_ptrRemoteDB;	
		m_ptrRemoteCommand->CommandType = adCmdText;
		m_ptrRemoteCommand->Parameters->Refresh();
		SetRemoteLinkStatus(true);
	}	
	catch( _com_error e)
	{
		// ����ʧ��
		OutputException(hr,szConnect,e,__FILE__,__LINE__);
		hr = E_UNEXPECTED;
		// �û������������
		// Access denied for user 'root'@'localhost' (using password: YES)	
		// δ֪�����ݿ�
		// Unknown database
		// �޷�����
		// Can't connect to MySQL server		
	}	
	return hr;
}

HRESULT CADOConnectionEx::ConnnectLocalServerByString(LPCTSTR szConnect)
{
	HRESULT hr = E_UNEXPECTED;
	_bstr_t strCnn(szConnect);
	TRY_ACCESS
	{
		hr = m_ptrLocalDB.CreateInstance(__uuidof(Connection));
		hr = m_ptrLocalDB->Open(strCnn, "","",adConnectUnspecified);
		hr = m_ptrLocalCommand.CreateInstance(__uuidof(Command));
		m_ptrLocalCommand->ActiveConnection = m_ptrLocalDB;	
		m_ptrLocalCommand->CommandType = adCmdText;
		m_ptrLocalCommand->Parameters->Refresh();
		_tcscpy_s(m_szLocalConnectString,1024,szConnect);
	}
	CATCH_ACCESS(hr);
	return hr;
}

void CADOConnectionEx::DisConnectRemote()
{	
	m_ptrRemoteDB = NULL;
	m_ptrRemoteCommand = NULL;
}

void CADOConnectionEx::DisConnectLocal()
{
	m_ptrRemoteDB = NULL;
	m_ptrLocalCommand = NULL;
}

void CADOConnectionEx::DisConnectAll()
{
	m_ptrRemoteDB = NULL;
	m_ptrRemoteDB = NULL;
	m_ptrRemoteCommand = NULL;
	m_ptrLocalCommand = NULL;
	if (m_hLinkDetectThread)
	{
		m_bLinkDetectThreadRun = false;
		TerminateThread(m_hLinkDetectThread,0xFF);
		CloseHandle(m_hLinkDetectThread);
		m_hLinkDetectThread = NULL;
	}	
}

DWORD WINAPI  CADOConnectionEx::LinkDetectThread(void *p)
{
	CADOConnectionEx *pAdoEx = (CADOConnectionEx *)p;
	HRESULT hr = S_OK;
	time_t tRuning = time(NULL) - 5;
	while(pAdoEx->m_bLinkDetectThreadRun)
	{
		if (TimeSpan(tRuning) >= 5)
		{
			hr = pAdoEx->ConnnectServerByString(pAdoEx->m_szRemoteConnectString);
			if (SUCCEEDED(hr))
			{
				// ��·�Ѿ��ָ�,�˳�����߳�
				pAdoEx->SetRemoteLinkStatus(true);
				break;
			}
			tRuning = time(NULL);
		}
		Sleep(5000);
	}
	if (pAdoEx->m_bLinkDetectThreadRun)		// �߳����б�־��Ϊtrue,��Ϊ�����˳�
	{
		CloseHandle(pAdoEx->m_hLinkDetectThread);
		pAdoEx->m_hLinkDetectThread = NULL;
	}
	return 0;
}

bool CADOConnectionEx::GetRemoteLinkStatus()
{
	bool bStatus = false;
	EnterCriticalSection(&m_csLinkStatus);
	bStatus = m_bRemoteDBLinkStauts;
	::LeaveCriticalSection(&m_csLinkStatus);
	return bStatus;
}
void CADOConnectionEx::SetRemoteLinkStatus(bool bStatus)
{
	EnterCriticalSection(&m_csLinkStatus);
	m_bRemoteDBLinkStauts = bStatus;
	::LeaveCriticalSection(&m_csLinkStatus);
}

bool CADOConnectionEx::GetRemnoteConnectString(LPTSTR szConnectString,int nBuffsize)
{
	if (szConnectString  && nBuffsize)
	{
		_tcscpy_s(szConnectString,nBuffsize,m_szRemoteConnectString);
		return false;
	}
	else
		return false;

}

bool CADOConnectionEx::GetLocalConnectString(LPTSTR szConnectString,int nBuffsize)
{
	if (szConnectString  && nBuffsize)
	{
		_tcscpy_s(szConnectString,nBuffsize,m_szLocalConnectString);
		return false;
	}
	else
		return false;

}