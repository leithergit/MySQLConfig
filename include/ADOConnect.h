// ADOSQLConnect.h: interface for the CADOConnect class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#pragma warning(disable:4146) 

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*#if WINVER >= 0x0610*/
//#import "c:\program files\common files\system\adoold\msado15.dll"  no_namespace rename("EOF", "adoEOF")
// #else
#import "c:\program files\common files\system\ado\msado15.dll"  no_namespace rename("EOF", "adoEOF")
//#import "C:\Program Files\Common Files\System\ado\msado60.tlb" no_namespace rename("EOF","adoEOF")
// #endif
#include <afxcom_.h>
#include <winbase.h>
inline void TESTHR(HRESULT _hr) { if FAILED(_hr) _com_issue_error(_hr); }

#ifdef _UNICODE
#define CATCH_ACCESS		CATCH_ACCESSW
#else
#define CATCH_ACCESS		CATCH_ACCESSA
#endif

#define TRY_ACCESS	try
#define CATCH_ACCESSA(hr)		catch(_com_error e)\
							{\
								hr = e.Error();\
								TraceMsg(_T("Exception occured At %s Line %d\n"),__FILE__,__LINE__);\
								TraceMsg(_T("Code = %08lx\n"), e.Error());\
								TraceMsg(_T("Meaning = %s\n"), e.ErrorMessage());\
								TraceMsg(_T("Source = %s\n"), (LPCSTR) e.Source());	\
								TraceMsg(_T("Description = %s\n"), (LPCSTR) e.Description());\
							};
#define CATCH_ACCESSW(hr)		catch(_com_error e)\
							{\
								hr = e.Error();\
								WCHAR szText[1024] = {0};\
								A2WHelper(__FILE__,szText,1024);\
								TraceMsg(_T("The Exception Occured at %s Line %d\n"),szText,__LINE__);\
								TraceMsg(_T("Code = %08lx\n"), e.Error());\
								TraceMsg(_T("Meaning = %s\n"),e.ErrorMessage());\
								A2WHelper((LPCSTR)e.Source(),szText,1024);\
								TraceMsg(_T("Source = %s\n"), szText);\
								A2WHelper((LPCSTR)e.Description(),szText,1024);\
								TraceMsg(_T("Description = %s\n"), szText);\
							};

class CAdoLock
{
private:
	CRITICAL_SECTION *m_pCS;
public:
	CAdoLock()
	{
		m_pCS = NULL;
	};
	CAdoLock(CRITICAL_SECTION cs)
	{			
		EnterCriticalSection(&cs);
		m_pCS = &cs;		
	};
	~CAdoLock()
	{
		if (m_pCS)
			LeaveCriticalSection(m_pCS);
	};
};


class CADOConnection 
{
public:
	static CRITICAL_SECTION	m_cs;
	volatile static bool m_bInitialCS;
	static void InitLock();
	static void UnInitLock();
public:

	BOOL DisConnect();
	TCHAR m_szDatabasePath[MAX_PATH];
	TCHAR m_szAdminAccount[256];
	TCHAR m_szDBPWD[32];
	BOOL ChangeAccessPassword(LPCTSTR lpszOldPass,LPCTSTR lpszNewPass);
	long GetFieldActueSize(LPCTSTR);
	_RecordsetPtr ExecuteTransQuery(LPCTSTR pSQLStatement,bool bHandleException = false);
	HRESULT CommitTrans();
	HRESULT RollbackTrans();
	HRESULT BeginTrans();
	CADOConnection();
	HRESULT	m_hrExceptionCode;
	virtual ~CADOConnection();
	//��sql ��������
	HRESULT InitialSQLSERVER(LPCTSTR pServerName,LPCTSTR pDataBase,LPCTSTR pId, LPCTSTR pPwd);
	//��access��������
	HRESULT InitialAccess( LPCTSTR pDataBase, LPCTSTR pId = NULL, LPCTSTR pPwd = NULL);
	//��Excel��������
	HRESULT InitialExcel(LPCTSTR pExcelFile, bool bCatchExcption = true);

	//ͨ�������ַ���sql��������
	HRESULT InitialServerByConnectString( LPCTSTR pStr, BOOL bShowErrMsg = FALSE);
	HRESULT ConnnectServerByString(LPCTSTR szConnect);	//Ĭ��1������,�����������ӳ�
	//ִ��sql���
	_RecordsetPtr RunSQL(LPCTSTR pSQLStatement,bool bCatchException = false,void *pRunlog = NULL);
	_RecordsetPtr OpenTable(TCHAR *szTable,
							CursorTypeEnum CursorType = adOpenStatic, 
							LockTypeEnum LockType = adLockOptimistic, 
							CommandTypeEnum  Options = adCmdTable);
	_RecordsetPtr GetRecordsetObj(LPCTSTR pSQLStatement,LPCTSTR szConnectionString = NULL,enum LockTypeEnum LockType = adLockOptimistic);
	LONG GetMaxField(LPCTSTR pTable, LPCTSTR pField);
	LONG GetMatchField(LPCTSTR pTable, LPCTSTR pField, LPCTSTR WhereExpr);

	// �����������������¼
	_RecordsetPtr	OpenTableBatch(TCHAR *szTable,
						enum CursorTypeEnum CursorType = adOpenStatic, 
						enum LockTypeEnum LockType = adLockBatchOptimistic, 
						CommandTypeEnum Options = adCmdTable);
	// ����ole�����ֶ�szField��
	static HRESULT InsertOleObj(_RecordsetPtr pRecordSet,TCHAR *szField,void *pOleObject,int nObjSize);
	// ȡ��Ole�ֶεĶ�������
	static HRESULT GetOleObj(_RecordsetPtr pRecordSet,TCHAR *szField,void *pObject,int &nObjSize);
	//��ʾ������Ϣ
	void PrintComError(_com_error &e);
	_ConnectionPtr m_ptrDBConnection;

	_RecordsetPtr  m_ptrRecordSet;
	_CommandPtr		m_ptrCommand;
	void OutputException(HRESULT &hr,LPCTSTR szSQL,_com_error &e,LPCSTR szFile,int nLine);
};


class CADOConnectionEx:public CADOConnection
{
private:
	_ConnectionPtr	m_ptrRemoteDB;
	TCHAR 			m_szRemoteConnectString[1024];
	TCHAR 			m_szLocalConnectString[1024];
	_ConnectionPtr	m_ptrLocalDB;
	_CommandPtr		m_ptrRemoteCommand;
	_CommandPtr		m_ptrLocalCommand;
	TCHAR			m_strLinkMissedStringArray[8][1024];
	int				m_nLinkMissedString;
	volatile bool	m_bRemoteDBLinkStauts;		// Զ��������·״̬
	HANDLE			m_hLinkDetectThread;		// Զ�����Ӽ���߳�
	volatile bool	m_bLinkDetectThreadRun;	
	static  DWORD	WINAPI LinkDetectThread(void *p);
	CRITICAL_SECTION m_csLinkStatus;
public:
	CADOConnectionEx();
	~CADOConnectionEx();
	_RecordsetPtr ExecuteTransQuery(LPCTSTR pSQLStatement,bool bHandleException = false);	
	HRESULT ConnnectRemoteServerByString(LPCTSTR szConnect,long nTimeout = 5);	// ����Զ��
	HRESULT ConnnectLocalServerByString(LPCTSTR szConnect);
	bool	AddLinkMissedString(LPCTSTR strLinkMissedString);

	void	DisConnectRemote();
	void	DisConnectLocal();
	void	DisConnectAll();
	bool	GetRemoteLinkStatus();
	void	SetRemoteLinkStatus(bool bStatus);
	bool	GetRemnoteConnectString(LPTSTR szConnectString,int nBuffsize);
	bool	GetLocalConnectString(LPTSTR szConnectString,int nBuffsize);
	_com_ptr_t<_com_IIID<_Connection,0x0>> ::Interface *GetRemoteConnectionPtr() 
	{
		if (GetRemoteLinkStatus()) 
			return m_ptrRemoteDB.GetInterfacePtr();
		else
			return NULL;
	};
	_com_ptr_t<_com_IIID<_Connection,0x0>> ::Interface *GetLocalConnectionPtr(){return m_ptrLocalDB.GetInterfacePtr();};

};