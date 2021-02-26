#ifndef QSERVICEMANAGER_H
#define QSERVICEMANAGER_H

#include <windows.h>
#include <winsvc.h>
#include <string>
#include <vector>

#include <memory>
#include <mmsystem.h>
#include <Shlwapi.h>
#include "Utility.h"
#pragma comment(lib,"winmm.lib")
using namespace std;

enum StartType
{
	Auto_Start,
	Manual_Start,
	Disabled,
	Delayed_Auto
};
#define SERVICE_STOPPED                        0x00000001
#define SERVICE_START_PENDING                  0x00000002
#define SERVICE_STOP_PENDING                   0x00000003
#define SERVICE_RUNNING                        0x00000004
#define SERVICE_CONTINUE_PENDING               0x00000005
#define SERVICE_PAUSE_PENDING                  0x00000006
#define SERVICE_PAUSED                         0x00000007

enum ServiceStatus
{
	Stopped = 1,
	Start_Pending ,
	Stop_Pending ,
	Running,
	Continue_Pending ,
	Pause_Pending,
	Paused 
};


// Alloc space for dest string and then copy the src string
// return
//	0		Failed
//	Nonzero	Succeed
int inline AllocCopyString(char** pszDest, char* pszSrc)
{
	if (pszSrc)
	{
		size_t nSize = strlen(pszSrc);
		if (nSize)
		{
			*pszDest = (char*)malloc(nSize + 1);
#ifdef _DEBUG
			ZeroMemory(*pszDest, nSize + 1);
#endif
			memcpy(*pszDest, pszSrc, nSize);
			(*pszDest)[nSize] = 0;
			return nSize;
		}
		else
		{
			*pszDest = nullptr;
			return 0;
		}			
	}
	else
	{
		*pszDest = nullptr;
		return 0;
	}
}

#define  Safe_Free(p) if (p) {free(p); p = nullptr;}

struct ServiceInformation
{
	ENUM_SERVICE_STATUSA EnumServiceStatus;
	QUERY_SERVICE_CONFIGA ServiceConfig;
	ServiceInformation(ENUM_SERVICE_STATUSA &StatusIn, QUERY_SERVICE_CONFIGA &ConfigIn)
	{
		AllocCopyString(&EnumServiceStatus.lpDisplayName, StatusIn.lpDisplayName);
		AllocCopyString(&EnumServiceStatus.lpServiceName, StatusIn.lpServiceName);
		EnumServiceStatus.ServiceStatus = StatusIn.ServiceStatus;
		AllocCopyString(&ServiceConfig.lpBinaryPathName	   , ConfigIn.lpBinaryPathName	 );
		AllocCopyString(&ServiceConfig.lpLoadOrderGroup	   , ConfigIn.lpLoadOrderGroup	 );
		AllocCopyString(&ServiceConfig.lpDependencies	   , ConfigIn.lpDependencies	 );
		AllocCopyString(&ServiceConfig.lpServiceStartName  , ConfigIn.lpServiceStartName );
		AllocCopyString(&ServiceConfig.lpDisplayName	   , ConfigIn.lpDisplayName	 	 );
		
		ServiceConfig.dwServiceType	 = ConfigIn.dwServiceType;
		ServiceConfig.dwStartType	 = ConfigIn.dwStartType;
		ServiceConfig.dwErrorControl = ConfigIn.dwErrorControl;
		ServiceConfig.dwTagId		 = ConfigIn.dwTagId;

	}
	~ServiceInformation()
	{
		Safe_Free(EnumServiceStatus.lpDisplayName);
		Safe_Free(EnumServiceStatus.lpServiceName);

		Safe_Free(ServiceConfig.lpBinaryPathName);
		Safe_Free(ServiceConfig.lpLoadOrderGroup);
		Safe_Free(ServiceConfig.lpDependencies);
		Safe_Free(ServiceConfig.lpServiceStartName);
		Safe_Free(ServiceConfig.lpDisplayName);
	}
};

typedef std::shared_ptr<ServiceInformation> ServiceInformationPtr;
typedef std::vector<ServiceInformationPtr>ServiceInformationArray;
class CWinService 
{
public:

	explicit CWinService()
	{
		m_hSC = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!m_hSC)
		{
			DWORD dwError = GetLastError();
			char szError[1024] = { 0 };
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				dwError,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPSTR)szError,
				1024,
				NULL);
			throw std::exception(szError);
		}
	}

	~CWinService()
	{
		if (m_hSC)
		{
			CloseServiceHandle(m_hSC);
			m_hSC = nullptr;
		}
	}

	LONG Start(LPCSTR lpszService, SERVICE_STATUS_PROCESS& ssStatus, DWORD dwTimeout = 15000)
	{
		DWORD dwOldCheckPoint;
		DWORD dwStartTime = timeGetTime();
		DWORD dwWaitTime;
		DWORD dwBytesNeeded;
		SC_HANDLE schService = nullptr;
		LONG dwError = -1;
		__try
		{
			if (!m_hSC)
				__leave;
			if (!lpszService || !strlen(lpszService))
				__leave;
			schService = OpenServiceA(m_hSC, lpszService, SERVICE_ALL_ACCESS);

			if (!schService)
			{
				dwError = GetLastError();
				__leave;
			}

			// Check the status in case the service is not stopped. 

			if (!QueryServiceStatusEx(
				schService,                     // handle to service 
				SC_STATUS_PROCESS_INFO,         // information level
				(LPBYTE)&ssStatus,             // address of structure
				sizeof(SERVICE_STATUS_PROCESS), // size of structure
				&dwBytesNeeded))              // size needed if buffer is too small
			{
				dwError = GetLastError();
				__leave;
			}
			TraceMsgA("\n%s %d Service %s is %s.\n", __FUNCTION__,__LINE__, lpszService, szServiceStatus[ssStatus.dwCurrentState]);
			// Check if the service is already running. It would be possible 
			// to stop the service here, but for simplicity this example just returns. 
			if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
			{
				dwError = GetLastError();
				__leave;
			}

			// Wait for the service to stop before attempting to start it.
			/*
			* SERVICE_STATUS_PROCESS::dwCheckPoint

			The check-point value that the service increments periodically to report its progress during a lengthy start, 
			stop, pause, or continue operation. For example, the service should increment this value as it completes each 
			step of its initialization when it is starting up. The user interface program that invoked the operation on 
			the service uses this value to track the progress of the service during a lengthy operation. This value is not
			valid and should be zero when the service does not have a start, stop, pause, or continue operation pending.

			SERVICE_STATUS_PROCESS::dwWaitHint

			The estimated time required for a pending start, stop, pause, or continue operation, in milliseconds. Before 
			the specified amount of time has elapsed, the service should make its next call to the SetServiceStatus function 
			with either an incremented dwCheckPoint value or a change in dwCurrentState. If the amount of time specified 
			by dwWaitHint passes, and dwCheckPoint has not been incremented or dwCurrentState has not changed, the service 
			control manager or service control program can assume that an error has occurred and the service should be stopped.
			However, if the service shares a process with other services, the service control manager cannot terminate the 
			service application because it would have to terminate the other services sharing the process as well.
			*/
			// Save the time and initial checkpoint.
			dwStartTime = timeGetTime();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
			while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
			{
				TraceMsgA("%s %d Service %s is %s.\n", __FUNCTION__, __LINE__, lpszService, szServiceStatus[ssStatus.dwCurrentState]);
				dwWaitTime = ssStatus.dwWaitHint / 10;

				if (dwWaitTime < 200)
					dwWaitTime = 200;
				else if (dwWaitTime > 1000)
					dwWaitTime = 1000;
				Sleep(dwWaitTime);

				// Check the status until the service is no longer stop pending. 

				if (!QueryServiceStatusEx(
					schService,                     // handle to service 
					SC_STATUS_PROCESS_INFO,         // information level
					(LPBYTE)&ssStatus,             // address of structure
					sizeof(SERVICE_STATUS_PROCESS), // size of structure
					&dwBytesNeeded))              // size needed if buffer is too small
				{
					dwError = GetLastError();
					__leave;
				}

				TraceMsgA("%s %d Service %s is %s.\n", __FUNCTION__, __LINE__, lpszService, szServiceStatus[ssStatus.dwCurrentState]);
				if (ssStatus.dwCheckPoint > dwOldCheckPoint)
				{
					// Continue to wait and check.
					dwStartTime = timeGetTime();
					dwOldCheckPoint = ssStatus.dwCheckPoint;
				}
				else
				{
					if (timeGetTime() - dwStartTime > ssStatus.dwWaitHint)
					{
						TraceMsgA("%s %d Service %s Wait for stopping timeout.\n", __FUNCTION__,__LINE__,lpszService);
						dwError = WAIT_TIMEOUT;
						__leave;
					}
				}
			}

			// Attempt to start the service.
			if (!StartService(
				schService,  // handle to service 
				0,           // number of arguments 
				NULL))       // no arguments 
			{
				dwError = GetLastError();
				__leave;
			}

			// Check the status until the service is no longer start pending. 

			if (!QueryServiceStatusEx(
				schService,                     // handle to service 
				SC_STATUS_PROCESS_INFO,         // info level
				(LPBYTE)&ssStatus,             // address of structure
				sizeof(SERVICE_STATUS_PROCESS), // size of structure
				&dwBytesNeeded))              // if buffer too small
			{
				dwError = GetLastError();
				__leave;
			}

			TraceMsgA("%s %d Service %s is %s.\n", __FUNCTION__, __LINE__, lpszService, szServiceStatus[ssStatus.dwCurrentState]);
			// Save the tick count and initial checkpoint.
			if (ssStatus.dwCurrentState == SERVICE_RUNNING)
			{
				dwError = 0;
				__leave;
			}
				
			dwStartTime = timeGetTime();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
			TraceMsgA("%s %d OldCheckPoint:%d.\n", __FUNCTION__, __LINE__, ssStatus.dwCheckPoint);
			while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
			{
				if (dwWaitTime < 200)
					dwWaitTime = 200;
				else if (dwWaitTime > 1000)
					dwWaitTime = 1000;
				Sleep(dwWaitTime);

				// Check the status again. 
				if (!QueryServiceStatusEx(
					schService,             // handle to service 
					SC_STATUS_PROCESS_INFO, // info level
					(LPBYTE)&ssStatus,             // address of structure
					sizeof(SERVICE_STATUS_PROCESS), // size of structure
					&dwBytesNeeded))              // if buffer too small
				{
					dwError = GetLastError();
					__leave;
				}
				TraceMsgA("%s %d Service %s is %s.\n", __FUNCTION__, __LINE__, lpszService, szServiceStatus[ssStatus.dwCurrentState]);
				if (ssStatus.dwCurrentState == SERVICE_RUNNING)
					break;
				if (ssStatus.dwCheckPoint > dwOldCheckPoint)
				{
					TraceMsgA("%s %d CheckPoint Updated:%d,Timespan = %d.\n", __FUNCTION__, __LINE__, ssStatus.dwCheckPoint,timeGetTime() - dwStartTime);
					dwStartTime = timeGetTime();
					dwOldCheckPoint = ssStatus.dwCheckPoint;
				}
				else
				{
					if (timeGetTime() - dwStartTime > ssStatus.dwWaitHint)
					{
						DWORD dwWaitTime = timeGetTime() - dwStartTime;
						TraceMsgA("%s %d Service Started timeout.\n", __FUNCTION__, __LINE__, lpszService);
						dwError = WAIT_TIMEOUT;
						break;
					}
				}
			}

			// Determine whether the service is running.

			if (ssStatus.dwCurrentState == SERVICE_RUNNING)
				dwError = 0;
			else
				dwError = GetLastError();
		}
		__finally
		{
			if (schService)
			{
				CloseServiceHandle(schService);
				schService = nullptr;
			}
		}
		return dwError;
	}

	LONG Stop(LPCSTR lpszService, SERVICE_STATUS_PROCESS& ssStatus, DWORD dwTimeout = 15000)
	{
		DWORD dwStartTime = timeGetTime();
		DWORD dwBytesNeeded;
		DWORD dwWaitTime;
		SC_HANDLE  schService = nullptr;
		LONG  dwError = -1;
		// Get a handle to the SCM database. 

		__try
		{
			if (!m_hSC)
				__leave;
			if (!lpszService || !strlen(lpszService))
				__leave;
			// Get a handle to the service.

			schService = OpenServiceA(
				m_hSC,         // SCM 
				lpszService,         // name of service 
				SERVICE_STOP |
				SERVICE_QUERY_STATUS |
				SERVICE_ENUMERATE_DEPENDENTS);

			if (schService == NULL)
			{
				dwError = GetLastError();
				__leave;
			}

			// Make sure the service is not already stopped.

			if (!QueryServiceStatusEx(
				schService,
				SC_STATUS_PROCESS_INFO,
				(LPBYTE)&ssStatus,
				sizeof(SERVICE_STATUS_PROCESS),
				&dwBytesNeeded))
			{
				dwError = GetLastError();
				__leave;
			}
			TraceMsgA("%s %d Service %s is %s.\n", __FUNCTION__, __LINE__, lpszService, szServiceStatus[ssStatus.dwCurrentState]);

			if (ssStatus.dwCurrentState == SERVICE_STOPPED)
			{
				dwError = 0;
				__leave;
			}

			// If a stop is pending, wait for it.
			while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
			{
				// Do not wait longer than the wait hint. A good interval is 
				// one-tenth of the wait hint but not less than 1 second  
				// and not more than 10 seconds. 

				dwWaitTime = ssStatus.dwWaitHint / 10;

				if (dwWaitTime < 200)
					dwWaitTime = 200;
				else if (dwWaitTime > 1000)
					dwWaitTime = 1000;
				Sleep(dwWaitTime);

				if (!QueryServiceStatusEx(
					schService,
					SC_STATUS_PROCESS_INFO,
					(LPBYTE)&ssStatus,
					sizeof(SERVICE_STATUS_PROCESS),
					&dwBytesNeeded))
				{
					dwError = GetLastError();
					__leave;
				}

				TraceMsgA("%s %d Service %s is %s.\n", __FUNCTION__, __LINE__, lpszService, szServiceStatus[ssStatus.dwCurrentState]);
				if (ssStatus.dwCurrentState == SERVICE_STOPPED)
				{
					dwError = 0;
					__leave;
				}

				if (timeGetTime() - dwStartTime > ssStatus.dwWaitHint)
				{
					TraceMsgA("%s %d Service Wait for stopping timeout.\n", __FUNCTION__, __LINE__, lpszService);
					dwError = WAIT_TIMEOUT;
					__leave;
				}
			}

			// If the service is running, dependencies must be stopped first.
			StopDependentServices(schService);

			// Send a stop code to the service.

			if (!ControlService(
				schService,
				SERVICE_CONTROL_STOP,
				(LPSERVICE_STATUS)&ssStatus))
			{
				dwError = GetLastError();
				__leave;
			}

			// Wait for the service to stop.

			while (ssStatus.dwCurrentState != SERVICE_STOPPED)
			{
				TraceMsgA("%s %d Service %s is %s.\n", __FUNCTION__, __LINE__, lpszService, szServiceStatus[ssStatus.dwCurrentState]);

				dwWaitTime = ssStatus.dwWaitHint / 10;
				if (dwWaitTime < 200)
					dwWaitTime = 200;
				else if (dwWaitTime > 1000)
					dwWaitTime = 1000;
				Sleep(dwWaitTime);

				if (!QueryServiceStatusEx(
					schService,
					SC_STATUS_PROCESS_INFO,
					(LPBYTE)&ssStatus,
					sizeof(SERVICE_STATUS_PROCESS),
					&dwBytesNeeded))
				{
					dwError = GetLastError();
					__leave;
				}

				if (ssStatus.dwCurrentState == SERVICE_STOPPED)
					break;

				if (timeGetTime() - dwStartTime > ssStatus.dwWaitHint)
				{
					TraceMsgA("%s %d Service Wait for stopping timeout.\n", __FUNCTION__, __LINE__, lpszService);
					dwError = WAIT_TIMEOUT;
					__leave;
				}
			}
			dwError = 0;
		}
		__finally
		{
			if (schService)
			{
				CloseServiceHandle(schService);
				schService = nullptr;
			}
		}
		return dwError;
	}

	LONG Restart(LPCSTR lpszService, SERVICE_STATUS_PROCESS& ssStatus,DWORD dwTimeout = 15000)
	{
		int nResult = Stop(lpszService, ssStatus, dwTimeout);
		if (nResult || ssStatus.dwCurrentState != Stopped)
			return nResult;
		nResult = Start(lpszService, ssStatus);
		if (nResult || ssStatus.dwCurrentState != Running)
			return nResult;
		else
			return 0;
	}

	LONG Delete(LPCSTR lpszService)
	{
		if (!m_hSC)
			return -1;
		if (!lpszService || !strlen(lpszService))
			return -1;

		SC_HANDLE schService;
		SERVICE_STATUS ssStatus;

		schService = OpenServiceA(
			m_hSC,       // SCM database 
			lpszService,          // name of service 
			DELETE);            // need delete access 

		if (schService == NULL)
		{
			return GetLastError();
		}

		// Delete the service.

		if (!DeleteService(schService))
		{
			long nError = GetLastError();
			CloseServiceHandle(schService);
			return nError;
		}
		else
		{
			CloseServiceHandle(schService);
			return 0;
		}
	}

	LONG Query(LPCSTR lpszService, SERVICE_STATUS& ssStatus)
	{
		if (!m_hSC)
			return -1;
		if (!lpszService || !strlen(lpszService))
			return -1;
		// 打开服务。
		SC_HANDLE schService = OpenServiceA(m_hSC, lpszService, SERVICE_QUERY_STATUS);
		if (schService)
		{
			if (QueryServiceStatus(schService, &ssStatus))
			{
				CloseServiceHandle(schService);
				return 0;
			}
			else
			{
				DWORD dwError = GetLastError();
				CloseServiceHandle(schService);
				return dwError;
			}
		}
		else
			return GetLastError();
	}

	LONG Create(LPCSTR lpszService, LPCSTR lpszProcessPath, StartType = Auto_Start)
	{
		if (!m_hSC)
			return -1;

		if (!lpszService || !strlen(lpszService))
			return -1;

		if (!lpszProcessPath || !strlen(lpszProcessPath))
			return -1;

		if (!PathFileExistsA(lpszProcessPath))
			return -1;
		long dwError = -1;
		SC_HANDLE schService = schService = CreateServiceA(
			m_hSC,						// SCM database 
			lpszService,				// name of service 
			lpszService,				// service name to display 
			SERVICE_ALL_ACCESS,			// desired access 
			SERVICE_WIN32_OWN_PROCESS,	// service type 
			SERVICE_DEMAND_START,		// start type 
			SERVICE_ERROR_NORMAL,		// error control type 
			lpszProcessPath,			// path to service's binary 
			NULL,						// no load ordering group 
			NULL,						// no tag identifier 
			NULL,						// no dependencies 
			NULL,						// LocalSystem account 
			NULL);						// no password 

		if (schService == NULL)
		{
			return GetLastError();
		}
		else
		{
			CloseServiceHandle(schService);
			return 0;
		}
	}

	LONG Enable(LPCSTR lpszService, bool bEnable = true)
	{
		if (!m_hSC)
			return -1;
		if (!lpszService || !strlen(lpszService))
			return -1;

		SC_HANDLE schService = OpenServiceA(
			m_hSC,            // SCM database 
			lpszService,     // name of service 
			SERVICE_CHANGE_CONFIG);  // need change config access 

		if (schService == NULL)
		{
			DWORD dwError = GetLastError();
			CloseServiceHandle(schService);
			return dwError;
		}

		// Change the service start type.
		if (!ChangeServiceConfig(
			schService,        // handle of service 
			SERVICE_NO_CHANGE, // service type: no change 
			bEnable ? SERVICE_DEMAND_START : SERVICE_DISABLED,  // service start type 
			SERVICE_NO_CHANGE, // error control: no change 
			NULL,              // binary path: no change 
			NULL,              // load order group: no change 
			NULL,              // tag ID: no change 
			NULL,              // dependencies: no change 
			NULL,              // account name: no change 
			NULL,              // password: no change 
			NULL))            // display name: no change
		{
			DWORD dwError = GetLastError();
			CloseServiceHandle(schService);
			return dwError;
		}
		CloseServiceHandle(schService);
		return 0;
	}

	LONG GetAllServiceInformation(ServiceInformationArray &SvrConfigArray)
	{
		LONG dwError = -1;
		size_t nBufferSize = sizeof(ENUM_SERVICE_STATUSA) * 4096;
		size_t nServiceInfoSize = 1024 * 8;
		LPENUM_SERVICE_STATUSA pService_status = nullptr; //保存系统服务的结构
		LPQUERY_SERVICE_CONFIGA pServiceConfig = nullptr;
		SC_HANDLE hSC = nullptr;
		DWORD cbBytesNeeded = NULL;
		DWORD ServicesReturned = NULL;
		DWORD ResumeHandle = NULL;
	
		if (!m_hSC)
			goto finally;
			
		pService_status = (LPENUM_SERVICE_STATUSA)LocalAlloc(LPTR, nBufferSize); 
		pServiceConfig = (LPQUERY_SERVICE_CONFIGA)LocalAlloc(LPTR, nServiceInfoSize);
		if (!pService_status || !pServiceConfig)
			goto finally;
		//获取系统服务的简单信息
		if (!EnumServicesStatusA(m_hSC,		//系统服务句柄
			/*SERVICE_TYPE_ALL*/SERVICE_WIN32,	//服务的类型
			SERVICE_STATE_ALL,				//服务的状态
			(LPENUM_SERVICE_STATUSA)pService_status,  //输出参数，系统服务的结构
			nBufferSize,					// 结构的大小
			&cbBytesNeeded,					//输出参数，接收返回所需的服务
			&ServicesReturned,				//输出参数，接收返回服务的数量
			&ResumeHandle))					//输入输出参数，第一次调用必须为0，返回为0代表成功
		{
			dwError = GetLastError();
			goto finally;
		}
		for (int i = 0; i < ServicesReturned; i++)
		{
			if (hSC)
			{
				CloseServiceHandle(hSC);
				hSC = nullptr;
			}
				
			hSC = OpenServiceA(m_hSC, pService_status[i].lpServiceName, SERVICE_QUERY_CONFIG); 
			if (!QueryServiceConfigA(hSC, pServiceConfig, nServiceInfoSize, &ResumeHandle))
			{
				continue;
			}
			SvrConfigArray.push_back(make_shared<ServiceInformation>(pService_status[i], *pServiceConfig));
		}
		dwError = 0;
	finally:
		
		if (pService_status)
			LocalFree((HLOCAL)pService_status);
		if (pServiceConfig)
			LocalFree((HLOCAL)pServiceConfig);
		if (hSC)
			CloseServiceHandle(hSC);
		
		return dwError;
	}
	LONG Update()
	{
		//BOOL ChangeServiceConfig2(SC_HANDLE hService, DWORD     dwInfoLevel, LPVOID    lpInfo);
		//dwInfoLevel
		//	The configuration information to be changed.This parameter can be one of the following values.
		//	TABLE 1
		//	Value									Meaning
		//	SERVICE_CONFIG_DELAYED_AUTO_START_INFO	The lpInfo parameter is a pointer to a SERVICE_DELAYED_AUTO_START_INFO structure.
		//	3										Windows Server 2003 and Windows XP : This value is not supported.

		//	SERVICE_CONFIG_DESCRIPTION				The lpInfo parameter is a pointer to a SERVICE_DESCRIPTION structure.
		//	1

		//	SERVICE_CONFIG_FAILURE_ACTIONS			The lpInfo parameter is a pointer to a SERVICE_FAILURE_ACTIONS structure.
		//	2										If the service controller handles the SC_ACTION_REBOOT action, the caller must have the SE_SHUTDOWN_NAME privilege.For more information, see Running with Special Privileges.

		//	SERVICE_CONFIG_FAILURE_ACTIONS_FLAG		The lpInfo parameter is a pointer to a SERVICE_FAILURE_ACTIONS_FLAG structure.
		//	4										Windows Server 2003 and Windows XP : This value is not supported.

		//	SERVICE_CONFIG_PREFERRED_NODE			The lpInfo parameter is a pointer to a SERVICE_PREFERRED_NODE_INFO structure.
		//	9										Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP : This value is not supported.

		//	SERVICE_CONFIG_PRESHUTDOWN_INFO			The lpInfo parameter is a pointer to a SERVICE_PRESHUTDOWN_INFO structure.
		//	7										Windows Server 2003 and Windows XP : This value is not supported.

		//	SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO	The lpInfo parameter is a pointer to a SERVICE_REQUIRED_PRIVILEGES_INFO structure.
		//	6										Windows Server 2003 and Windows XP : This value is not supported.

		//	SERVICE_CONFIG_SERVICE_SID_INFO			The lpInfo parameter is a pointer to a SERVICE_SID_INFO structure.
		//	5										

		//	SERVICE_CONFIG_TRIGGER_INFO				The lpInfo parameter is a pointer to a SERVICE_TRIGGER_INFO structure.This value is not supported by the ANSI version of ChangeServiceConfig2.
		//	8										Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP : This value is not supported until Windows Server 2008 R2.

		//	SERVICE_CONFIG_LAUNCH_PROTECTED			The lpInfo parameter is a pointer a SERVICE_LAUNCH_PROTECTED_INFO structure.
		//	12										Note  This value is supported starting with Windows 8.1.


		//if (!m_hSC)
		//	return -1;
		//if (!m_lpszService.size())
		//	return -1;

		//SC_HANDLE schService;
		//SERVICE_DESCRIPTION sd;
		//LPTSTR szDesc = TEXT("This is a test description");

		//// Get a handle to the service.

		//schService = OpenServiceA(
		//	m_hSC,            // SCM database 
		//	m_lpszService,        // name of service 
		//	SERVICE_CHANGE_CONFIG);  // need change config access 

		//if (schService == NULL)
		//{
		//	DWORD dwError = GetLastError();
		//	CloseServiceHandle(schService);
		//	return dwError;
		//}

		//// Change the service description.

		//sd.lpDescription = szDesc;

		//if (!ChangeServiceConfig2(
		//	schService,                 // handle to service
		//	SERVICE_CONFIG_DESCRIPTION, // change: description
		//	&sd))                      // new description
		//{
		//	printf("ChangeServiceConfig2 failed\n");
		//}
		//else printf("Service description updated successfully.\n");

		//CloseServiceHandle(schService);
		//CloseServiceHandle(schSCManager);
	}
	LONG IsInstalled(LPCSTR lpszService)
	{
		if (!m_hSC)
			return -1;
		if (!lpszService || !strlen(lpszService))
			return -1;

		// 打开服务。
		SC_HANDLE hSvc = OpenServiceA(m_hSC, lpszService, SERVICE_QUERY_STATUS);
		if (hSvc)
		{
			CloseServiceHandle(hSvc);
			return 0;
		}
		else
			return GetLastError();
	}
private:
	BOOL StopDependentServices(SC_HANDLE schService)
	{
		DWORD i;
		DWORD dwBytesNeeded;
		DWORD dwCount;

		LPENUM_SERVICE_STATUS   lpDependencies = NULL;
		ENUM_SERVICE_STATUS     ess;
		SC_HANDLE               hDepService;
		SERVICE_STATUS_PROCESS  ssp;

		DWORD dwStartTime = timeGetTime();
		DWORD dwTimeout = 15000; // 30-second time-out

		// Pass a zero-length buffer to get the required buffer size.
		if (EnumDependentServices(schService, SERVICE_ACTIVE,
			lpDependencies, 0, &dwBytesNeeded, &dwCount))
		{
			// If the Enum call succeeds, then there are no dependent
			// services, so do nothing.
			return TRUE;
		}
		else
		{
			if (GetLastError() != ERROR_MORE_DATA)
				return FALSE; // Unexpected error

			// Allocate a buffer for the dependencies.
			lpDependencies = (LPENUM_SERVICE_STATUS)HeapAlloc(
				GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded);

			if (!lpDependencies)
				return FALSE;

			__try {
				// Enumerate the dependencies.
				if (!EnumDependentServices(schService, SERVICE_ACTIVE,
					lpDependencies, dwBytesNeeded, &dwBytesNeeded,
					&dwCount))
					return FALSE;

				for (i = 0; i < dwCount; i++)
				{
					ess = *(lpDependencies + i);
					// Open the service.
					hDepService = OpenService(m_hSC,
						ess.lpServiceName,
						SERVICE_STOP | SERVICE_QUERY_STATUS);

					if (!hDepService)
						return FALSE;

					__try
					{
						// Send a stop code.
						if (!ControlService(hDepService,
							SERVICE_CONTROL_STOP,
							(LPSERVICE_STATUS)&ssp))
							return FALSE;

						// Wait for the service to stop.
						while (ssp.dwCurrentState != SERVICE_STOPPED)
						{
							Sleep(ssp.dwWaitHint);
							if (!QueryServiceStatusEx(
								hDepService,
								SC_STATUS_PROCESS_INFO,
								(LPBYTE)&ssp,
								sizeof(SERVICE_STATUS_PROCESS),
								&dwBytesNeeded))
								return FALSE;

							if (ssp.dwCurrentState == SERVICE_STOPPED)
								break;

							if (timeGetTime() - dwStartTime > dwTimeout)
								return FALSE;
						}
					}
					__finally
					{
						// Always release the service handle.
						CloseServiceHandle(hDepService);
					}
				}
			}
			__finally
			{
				// Always free the enumeration buffer.
				HeapFree(GetProcessHeap(), 0, lpDependencies);
			}
		}
		return TRUE;
	}
	static constexpr  char* szServiceStatus[] = {
		"",
	"Stopped",
	"Start_Pending",
	"Stop_Pending",
	"Running",
	"Continue_Pending",
	"Pause_Pending",
	"Paused"
	};
	SC_HANDLE	m_hSC = nullptr;
	
};

#endif // QSERVICEMANAGER_H
