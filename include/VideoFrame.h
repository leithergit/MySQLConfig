#pragma once

#include <assert.h>
#include <vector>
#include <map>
#include <list>
//#include <memory>
#ifdef _STD_SMARTPTR				// ʹ��STL������ָ��
#include <memory>
using namespace std;
using namespace std::tr1;
#else
#include <boost/shared_ptr.hpp>
using namespace boost;
#endif

#include <math.h>
#include "AutoLock.h"
#include "Utility.h"
using namespace  std;

#pragma warning(disable:4244 4018)
enum FrameStyle
{
	StyleNormal = 0,
	Style_2P1,
	Style_5P1,
	Style_7P1,
	Style_9P1,
	Style_11P1,
	Style_13P1,
	Style_15P1,
};
#define		_GRID_LINE_WIDTH	1
// CVideoFrame
#define WM_TROGGLEFULLSCREEN		WM_USER + 4096
// ֪ͨ�������Ҽ���Ϣ,WPARAM Ϊ�����������,x = LOWROD(wParam),y = HIWORD(wParam),LPARAMΪ��������
#define WM_FRAME_RBUTTONDOWN		WM_USER + 4097		

#define RectWidth(rt)	(rt.right - rt.left)
#define RectHeight(rt)	(rt.bottom - rt.top)

struct _Line
{
	_Line()
	{
		ZeroMemory(this,sizeof(_Line));
	}
	POINT ptStart;
	POINT ptStop;
};

typedef shared_ptr<_Line> LinePtr;
struct PanelInfo
{
	PanelInfo()
	{
		ZeroMemory(this, sizeof(PanelInfo));
	}
	PanelInfo(int nRowIn, int nColIn)
	{
		ZeroMemory(this, sizeof(PanelInfo));
		nRow = nRowIn;
		nCol = nColIn;
	}
	
	~PanelInfo()
	{
		DestroyWindow(hWnd);
	}
	void UpdateWindow()
	{
		MoveWindow(hWnd, rect.left, rect.top, (rect.right - rect.left), (rect.bottom - rect.top), TRUE);	
	}
	HWND hWnd;
	RECT rect;
	int nIndex;
	int nRow;
	int nCol;
	void *pCustumData;
};
typedef shared_ptr<PanelInfo> PanelInfoPtr;

class CriticalSectionWrap;
typedef shared_ptr<CriticalSectionWrap>CriticalSectionPtr;
class CriticalSectionWrap
{
public:
	CriticalSectionWrap()
	{
		InitializeCriticalSection(&m_cs);
	}
	~CriticalSectionWrap()
	{
		DeleteCriticalSection(&m_cs);
	}
	inline BOOL TryLock()
	{
		return TryEnterCriticalSection(&m_cs);
	}
	inline void Lock()
	{
		EnterCriticalSection(&m_cs);
	}
	void Unlock()
	{
		LeaveCriticalSection(&m_cs);
	}
	inline CRITICAL_SECTION *Get()
	{
		return &m_cs;
	}

private:
	CRITICAL_SECTION	m_cs;
};

class CVideoFrame : public CWnd
{
	DECLARE_DYNAMIC(CVideoFrame)
public:
	CVideoFrame();
	virtual ~CVideoFrame();
	static map<HWND, HWND> m_PanelMap;
	static CriticalSectionPtr m_csPannelMap;
	
protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
private:
	static CVideoFrame *m_pCurrentFrame;
	UINT	m_nCols /*= 1*/, m_nRows/* = 1*/;
	UINT	m_nNewCols,m_nNewRows;
	vector	<PanelInfoPtr>m_vecPanel;
	list	<PanelInfoPtr>m_listRecyclePanel;
	int		m_nPannelUsed/* = 0*/;		//  ���ÿո�����
	LPRECT  m_pCurSelectRect;
	LPRECT  m_pLastSelectRect;
	int		m_nCurPanel;		// ��ǰ��ѡ�е�������
	FrameStyle	m_nFrameStyle;
//	list<LinePtr> m_listLine;
	
private:
	void ResizePanel(int nWidth = 0, int nHeight = 0);

	// ���ƴ���
	void DrawGrid(CDC *pdc);

	// ����������崰��
	HWND CreatePanel(int nRow, int nCol)
	{
		WNDCLASSEX wcex;
		TCHAR *szWindowClass = _T("PanelWnd");
		TCHAR szWndName[256] = { 0 };
		_stprintf_s(szWndName, 256, _T("Panel(%d,%d)"), nRow, nCol);
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wcex.lpfnWndProc = PanelWndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = NULL;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = szWindowClass;
		wcex.hIconSm = NULL;
		RegisterClassEx(&wcex);
		RECT* pRtWnd = GetPanelRect(nRow, nCol);
		_TraceMsgA("Rect(%d,%d) = (%d,%d,%d,%d).\n", nRow, nCol, pRtWnd->left, pRtWnd->right, pRtWnd->top, pRtWnd->bottom);
		HWND hPannel =  ::CreateWindow(szWindowClass,	// ������
									szWndName,							// ���ڱ��� 
									WS_CHILD,							// ���ڷ��
									pRtWnd->left, 						// �������Ͻ�X����
									pRtWnd->top, 						// �������Ͻ�Y����
									(pRtWnd->right - pRtWnd->left), 	// ���ڿ��
									(pRtWnd->bottom - pRtWnd->top), 	// ���ڸ߶�
									m_hWnd, 							// �����ھ��
									NULL,								// �˵����
									NULL,
									this);
		if (hPannel)
		{
			DWORD dwExStype = ((DWORD)GetWindowLong(hPannel, GWL_EXSTYLE));
			dwExStype |=WS_EX_TRANSPARENT;
			SetWindowLong(hPannel, GWL_EXSTYLE,dwExStype);
			ModifyWndStyle(hPannel, GWL_STYLE, 0, WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 0);
			m_csPannelMap->Lock();
			map<HWND, HWND>::iterator it = m_PanelMap.find(hPannel);
			if (it != m_PanelMap.end())
				m_PanelMap.erase(it);
			m_PanelMap.insert(pair<HWND, HWND>(hPannel, m_hWnd));
			m_csPannelMap->Unlock();
		}
		return hPannel;
	}
	HWND CreatePanel(int nIndex)
	{
		WNDCLASSEX wcex;
		TCHAR *szWindowClass = _T("PanelWnd");
		TCHAR szWndName[256] = { 0 };
		_stprintf_s(szWndName, 256, _T("Panel(%d)"), nIndex);
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wcex.lpfnWndProc = PanelWndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = NULL;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = szWindowClass;
		wcex.hIconSm = NULL;
		RegisterClassEx(&wcex);
		RECT* pRtWnd = GetPanelRect(nIndex);
		_TraceMsgA("Rect[%d] = (%d,%d,%d,%d).\n", nIndex, pRtWnd->left, pRtWnd->right, pRtWnd->top, pRtWnd->bottom);
		HWND hPannel =  ::CreateWindow(szWindowClass,	// ������
			szWndName,							// ���ڱ��� 
			WS_CHILD,							// ���ڷ��
			pRtWnd->left, 						// �������Ͻ�X����
			pRtWnd->top, 						// �������Ͻ�Y����
			(pRtWnd->right - pRtWnd->left), 	// ���ڿ��
			(pRtWnd->bottom - pRtWnd->top), 	// ���ڸ߶�
			m_hWnd, 							// �����ھ��
			NULL,								// �˵����
			NULL,
			this);
		if (hPannel)
		{
			DWORD dwExStype = ((DWORD)GetWindowLong(hPannel, GWL_EXSTYLE));
			dwExStype |=WS_EX_TRANSPARENT;
			SetWindowLong(hPannel, GWL_EXSTYLE,dwExStype);
			ModifyWndStyle(hPannel, GWL_STYLE, 0, WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 0);
			m_csPannelMap->Lock();
			map<HWND, HWND>::iterator it = m_PanelMap.find(hPannel);
			if (it != m_PanelMap.end())
				m_PanelMap.erase(it);
			m_PanelMap.insert(pair<HWND, HWND>(hPannel, m_hWnd));
			m_csPannelMap->Unlock();
		}
		return hPannel;
	}
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//afx_msg void OnPaint();
	CPen *m_pSelectedPen/* = NULL*/;
	CPen *m_pUnSelectedPen/* = NULL*/;
	CPen *m_pRestorePen/* = NULL*/;
	afx_msg void OnDestroy();
	virtual BOOL Create(UINT nID, const RECT& rect,int nRow,int nCol,CWnd* pParentWnd,  CCreateContext* pContext = NULL);
	virtual BOOL Create(UINT nID, const RECT& rect,int nCount,CWnd* pParentWnd,  CCreateContext* pContext = NULL);
	// �˴���nStyle��������ȡNormalStyle,���򴴽��޷��ɹ�
	virtual BOOL Create(UINT nID, const RECT& rect,FrameStyle nStyle,CWnd* pParentWnd,  CCreateContext* pContext = NULL );

	void CalcRowCol(int nCount,UINT &nRows,UINT &nCols)
	{
		assert(nCount != 0);
		if (nCount == 0)
			return ;

		float fsqroot = sqrt((float)nCount);
		int nRowCount = floor(fsqroot);
		int nColCount = nRowCount;

		if (nRowCount*nColCount < nCount)
		{
			nColCount++;
			if (nRowCount*nColCount < nCount)
				nRowCount++;
		}

		// ���뱣֤������������
		if (nRowCount > nColCount)
		{
			int nTemp = nRowCount;
			nRowCount = nColCount;
			nColCount = nTemp;
		}
		nRows = nRowCount;
		nCols = nColCount;
	}
	void RefreshSelectedPanel();
	LPRECT GetPanelRect(int nRow, int nCol)
	{
		if ((nRow*m_nCols + nCol) < m_vecPanel.size())		
			return &m_vecPanel[nRow*m_nCols + nCol]->rect;
		else
			return NULL;
	}

	LPRECT GetPanelRect(int nIndex)
	{
		if (nIndex < m_vecPanel.size())
			return &m_vecPanel[nIndex]->rect;
		else
			return NULL;
	}

	HWND GetPanelWnd(int nRow, int nCol)
	{
		if ((nRow*m_nCols + nCol) < m_vecPanel.size())
			return m_vecPanel[nRow*m_nCols + nCol]->hWnd;
		else
			return NULL;
	}

	HWND GetPanelWnd(int nIndex)
	{
		if (nIndex < m_vecPanel.size())
		{
			return m_vecPanel[nIndex]->hWnd;
		}
		else
			return NULL;
	}

	inline bool SetPanelParam(int nIndex, void *pParam)
	{
		if (nIndex < m_vecPanel.size())
		{
			m_vecPanel[nIndex]->pCustumData = pParam;
			return true;
		}
		else
			return false;
	}

	bool SetPanelParam(int nRow, int nCol, void *pParam)
	{
		return SetPanelParam(nRow*m_nCols + nCol, pParam);
	}
	
	inline void *GetPanelParam(int nIndex)
	{
		if (m_vecPanel.size() > 0 && nIndex < m_vecPanel.size())
			return m_vecPanel[nIndex]->pCustumData;
		else
			return NULL;
	}
	void *GetPanelParam(int nRow, int nCol)
	{
		return GetPanelParam(nRow*m_nCols + nCol);
	}
	
	void SetCurPanel(int nIndex)
	{
		if (nIndex < m_vecPanel.size())
		{
			m_pLastSelectRect = m_pCurSelectRect;
			m_pCurSelectRect = &m_vecPanel[nIndex]->rect;
			RefreshSelectedPanel();
		}
	}

	int GetCurPanel()
	{
		if (m_vecPanel.size() > 0 && m_nCurPanel < m_vecPanel.size())
			return m_nCurPanel;
		else
			return -1;
	}
	int GetPanelCount()
	{
		return m_vecPanel.size();
	}
	int GetRows(){ return m_nRows; }
	int GetCols(){ return m_nCols; }
	bool AdjustPanels(int nRow, int nCols,FrameStyle fs = StyleNormal);
	bool AdjustPanels(int nCount,FrameStyle fs = StyleNormal);
	static CVideoFrame *GetCurrentFrame()
	{
		return m_pCurrentFrame;
	};

#define __countof(array) (sizeof(array)/sizeof(array[0]))
#pragma warning (disable:4996)

	static void _TraceMsgA(LPCSTR pFormat, ...)
	{
		va_list args;
		va_start(args, pFormat);
		int nBuff;
		CHAR szBuffer[0x7fff];
		nBuff = _vsnprintf(szBuffer, __countof(szBuffer), pFormat, args);
		//::wvsprintf(szBuffer, pFormat, args);
		//assert(nBuff >=0);
		OutputDebugStringA(szBuffer);
		va_end(args);
	}
	
	static LRESULT CALLBACK PanelWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		//int wmId, wmEvent;
		PAINTSTRUCT ps;
		HDC hdc;
		switch (message)
		{
		case WM_CREATE:
		{
			 CREATESTRUCT *pCs = (CREATESTRUCT *)lParam;
			_TraceMsgA("%08X\tWM_CREATE.\n", hWnd);
			break;
		}
		case WM_SYSKEYDOWN:
		{
		}
		break;
		case WM_KEYDOWN:
		{
			switch (wParam)	// ����ESC�ͻس���
			{
			case VK_ESCAPE:
			case VK_RETURN:
			{
				//printf(_T("Capture Escape or Return Key.\n"));
				return 0;
			}
			break;
			}
		}
		break;
		case WM_PAINT:
		{
			hdc = ::BeginPaint(hWnd, &ps);
			::EndPaint(hWnd, &ps);
			break;
		}
		case WM_SIZE:
		{
			_TraceMsgA("%P\tWM_SIZE.\n", hWnd);
		}
		break;
		case WM_LBUTTONDBLCLK:	// ˫���ָ�����
		{
			_TraceMsgA("%08x\tWM_LBUTTONDBLCLK.\n", hWnd);
			m_csPannelMap->Lock();
			// ���Ҵ��ڶ�Ӧ�ĸ�����
			map<HWND, HWND>::iterator it = m_PanelMap.find(hWnd);
			if (it != m_PanelMap.end() &&
				IsWindow(it->second))
				::PostMessage(it->second, WM_TROGGLEFULLSCREEN, (WPARAM)hWnd, lParam);
			m_csPannelMap->Unlock();
			return 0;
		}
		break;
		case WM_LBUTTONDOWN:
		{
			
			POINT pt;
			pt.x = LOWORD(lParam); 
			pt.y = HIWORD(lParam);
			::ClientToScreen(hWnd,&pt);
			m_csPannelMap->Lock();
			// ���Ҵ��ڶ�Ӧ�ĸ�����
			map<HWND, HWND>::iterator it = m_PanelMap.find(hWnd);
			if (it != m_PanelMap.end())	
			{
				::ScreenToClient(it->second,&pt);
				::PostMessage(it->second, WM_LBUTTONDOWN, wParam, MAKELPARAM(pt.x,pt.y));
			}
			m_csPannelMap->Unlock();

		}
			break;
		case WM_DESTROY:
		{
			_TraceMsgA("%08X\tWM_DESTROY.\n", hWnd);
		}
		break;
		default:
			return ::DefWindowProc(hWnd, message, wParam, lParam);		// ����Ҫ����һ�䣬��Ȼ���ڿ����޷������ɹ�
		}
		return 0l;
	}
	
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnTroggleFullScreen(WPARAM W, LPARAM L);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


