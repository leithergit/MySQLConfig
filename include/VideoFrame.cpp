// VideoFrame.cpp : implementation file
//

#include "stdafx.h"
#include "VideoFrame.h"


// CVideoFrame

IMPLEMENT_DYNAMIC(CVideoFrame, CWnd)

map<HWND, HWND>CVideoFrame::m_PanelMap;
CriticalSectionPtr CVideoFrame::m_csPannelMap = make_shared<CriticalSectionWrap>();
CVideoFrame *CVideoFrame::m_pCurrentFrame = nullptr;

CVideoFrame::CVideoFrame()
{
	m_nCols = m_nRows = 1;
	m_nNewCols = m_nNewRows = 1;
	m_nPannelUsed = 0;		//  ���ô�������
	m_pSelectedPen = NULL;
	m_pUnSelectedPen = NULL;
	m_pRestorePen = NULL;
	m_pCurSelectRect = nullptr;
	m_pLastSelectRect =  nullptr;
	m_nCurPanel = -1;
	m_nFrameStyle = StyleNormal;
}

CVideoFrame::~CVideoFrame()
{
	if (m_pSelectedPen)
		delete m_pSelectedPen;
	if (m_pUnSelectedPen)
		delete m_pUnSelectedPen;
	if (m_pRestorePen)
		delete m_pRestorePen;
}


BEGIN_MESSAGE_MAP(CVideoFrame, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_MESSAGE(WM_TROGGLEFULLSCREEN,OnTroggleFullScreen)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

// CVideoFrame message handlers

BOOL CVideoFrame::Create(UINT nID, const RECT& rect,int nCount,CWnd* pParentWnd,  CCreateContext* pContext)
{
	UINT nRows = 0,nCols = 0;
	CalcRowCol(nCount,nRows,nCols);
	return Create(nID,rect,nRows,nCols,pParentWnd,pContext);
}

BOOL CVideoFrame::Create(UINT nID, const RECT& rect,int nRow,int nCol, CWnd* pParentWnd, CCreateContext* pContext)
{
	// can't use for desktop or pop-up windows (use CreateEx instead)
	ASSERT(pParentWnd != NULL);
	ASSERT(nID != 0);
	ASSERT(nRow != 0 && nCol != 0);
	m_nNewRows = nRow;
	m_nNewCols = nCol;

	LPCTSTR szWndClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH) ::GetStockObject(BLACK_BRUSH), NULL);
	return CreateEx(0, szWndClass, _T("VideoFrame"),
		WS_CHILD | WS_VISIBLE,
		rect.left, rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		pParentWnd->GetSafeHwnd(),
		(HMENU)(UINT_PTR)nID,
		(LPVOID)pContext);
}

BOOL CVideoFrame::Create(UINT nID, const RECT& rect,FrameStyle nStyle,CWnd* pParentWnd,  CCreateContext* pContext)
{
	ASSERT(pParentWnd != NULL);
	ASSERT(nID != 0);
	//ASSERT(nRow != 0 && nCol != 0);
	ASSERT(nStyle != StyleNormal);
	if (nStyle == StyleNormal)
		return FALSE;
	m_nFrameStyle = nStyle;
	switch(nStyle)
	{
	case StyleNormal:
	default:
		break;
	case Style_5P1:
		m_nRows = m_nCols = 3;
		break;
	case Style_7P1:
		m_nRows = m_nCols = 4;
		break;
	case Style_9P1:
		m_nRows = m_nCols = 5;
		break;
	}

	LPCTSTR szWndClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH) ::GetStockObject(BLACK_BRUSH), NULL);
	return CreateEx(0, szWndClass, _T("VideoFrame"),
		WS_CHILD | WS_VISIBLE,
		rect.left, rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		pParentWnd->GetSafeHwnd(),
		(HMENU)(UINT_PTR)nID,
		(LPVOID)pContext);
	
}
BOOL CVideoFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style &= ~WS_BORDER | ~WS_CAPTION | ~WS_SYSMENU | ~WS_THICKFRAME | ~WS_VSCROLL | ~WS_HSCROLL;
	cs.style |= WS_CHILD | WS_VISIBLE;
	
	return CWnd::PreCreateWindow(cs);
}

#define BackColor	RGB(0,0,0)
#define SelectColor	RGB(255,255,0)
int CVideoFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	m_pSelectedPen = new CPen(PS_SOLID, 1, SelectColor);
	m_pUnSelectedPen = new CPen(PS_SOLID, _GRID_LINE_WIDTH, BackColor);
	m_pRestorePen = new CPen(PS_SOLID, 1, BackColor);
	
	AdjustPanels(m_nNewRows, m_nNewCols);

	return 0;
}

// ������������
// �����������������Ҫɾ��ԭ�еĴ���
// ����������������ɾ������Ĵ���
bool CVideoFrame::AdjustPanels(int nRows, int nCols,FrameStyle fs)
{
	ASSERT(nRows != 0 && nCols != 0);
	if (!nRows || !nCols)
		return false;
	int nPanelCount = nRows*nCols;
	if (fs == StyleNormal)
	{
		// ��������ͬ����������
		if (fs == m_nFrameStyle && 
			nRows == m_nRows && 
			nCols == m_nCols)
			return true;
	}
		
	m_nFrameStyle = fs;
	switch(m_nFrameStyle)
	{
	case StyleNormal:
	default:
		break;
	case Style_2P1:
		nPanelCount = 3;
		nRows = 2;
		nCols = 2;
		break;
	case Style_5P1:
		nRows = nCols = 3;
		nPanelCount = 6;
		break;
	case Style_7P1:
		nRows = nCols = 4;
		nPanelCount = 8;
		break;
	case Style_9P1:
		nRows = nCols = 5;
		nPanelCount = 10;
		break;
	case Style_11P1:
		nRows = nCols = 6;
		nPanelCount = 12;
		break;
	case Style_13P1:
		nRows = nCols = 7;
		nPanelCount = 14;
		break;
	case Style_15P1:
		nRows = nCols = 8;
		nPanelCount = 16;
		break;
	}
	
	//��������
	if (nPanelCount < m_vecPanel.size())
	{
		int nReduceCount = m_vecPanel.size() - nPanelCount;
		int nReduced = 0;
		// �Ӻ���ǰɾ��
		for (vector<PanelInfoPtr>::reverse_iterator rit = m_vecPanel.rbegin() ;rit != m_vecPanel.rend() && nReduced < nReduceCount;)
		{
			::ShowWindow((*rit)->hWnd,SW_HIDE);
			//::InvalidateRect((*rit)->hWnd, NULL, TRUE);
			m_listRecyclePanel.push_back(*rit);
			rit = vector<PanelInfoPtr>::reverse_iterator(m_vecPanel.erase((++rit).base()));
			nReduced ++;
		}
		// ��������ߴ�
	}
	else if (nPanelCount > m_vecPanel.size())	// ��������
	{
		int nAddCount = nPanelCount - m_vecPanel.size();
		// ���ȴӻ��մ��ڱ���ȡ�ش���
		int nSize = m_listRecyclePanel.size();
		if (nAddCount >= nSize)
		{// ���մ�����������Ҫ������
			for (list<PanelInfoPtr>::reverse_iterator rit = m_listRecyclePanel.rbegin();rit != m_listRecyclePanel.rend();)
			{
				::ShowWindow((*rit)->hWnd,SW_SHOW);
				//::InvalidateRect((*rit)->hWnd, NULL, TRUE);
				m_vecPanel.push_back(*rit);
				rit = list<PanelInfoPtr>::reverse_iterator(m_listRecyclePanel.erase((++rit).base()));
			}
			nAddCount -= nSize;
			for (int i = 0; i < nAddCount; i++)
			{
				m_vecPanel.push_back(PanelInfoPtr(new PanelInfo(0, 0)));
			}
		}
		else
		{
			int nAdded = 0;
			for (list<PanelInfoPtr>::reverse_iterator rit = m_listRecyclePanel.rbegin();
				rit != m_listRecyclePanel.rend() && nAdded < nAddCount;
				nAdded ++)
			{
				::ShowWindow((*rit)->hWnd,SW_SHOW);
				//::InvalidateRect((*rit)->hWnd, NULL, TRUE);
				m_vecPanel.push_back(*rit);
				rit = list<PanelInfoPtr>::reverse_iterator(m_listRecyclePanel.erase((++rit).base()));
			}
		}
	}
	else
		return true;
	
	m_nRows = nRows;
	m_nCols = nCols;
	// ������������б��
	int nIndex = 0;
	if (m_nFrameStyle == StyleNormal)
	{
		for (int nRow = 0; nRow < m_nRows; nRow++)
		{
			for (int nCol = 0; nCol < m_nCols; nCol++)
			{
				m_vecPanel[nIndex]->nCol = nCol;
				m_vecPanel[nIndex]->nRow = nRow;
				nIndex ++;
			}
		}
		// ���¼���ÿһ�����ڵĴ�С
		ResizePanel();
		// ����ÿ�����ڵ�λ��
		nIndex = 0;
		for (int nRow = 0; nRow < m_nRows; nRow++)
		{
			for (int nCol = 0; nCol < m_nCols; nCol++)
			{
				PanelInfoPtr pPanel = m_vecPanel[nIndex ++];
				if (!pPanel->hWnd ||
					!IsWindow(pPanel->hWnd))
				{
					pPanel->hWnd = CreatePanel(nRow, nCol);
					::ShowWindow(pPanel->hWnd, SW_SHOW);
				}
				else
					::MoveWindow(pPanel->hWnd,pPanel->rect.left,pPanel->rect.top,RectWidth(pPanel->rect),RectHeight(pPanel->rect),FALSE);
			}
		}
	}
	else
	{
		// ������������б��
		m_vecPanel[nIndex]->nCol = 0;
		m_vecPanel[nIndex]->nRow = 0;
		if (m_nFrameStyle == Style_2P1)
		{
			m_vecPanel[1]->nCol = 1;
			m_vecPanel[1]->nRow = 0;

			m_vecPanel[2]->nCol = 1;
			m_vecPanel[2]->nRow = 1;

		}
		else
		{
			for (int nRow = 0; nRow < m_nRows; nRow++)
			{
				m_vecPanel[nIndex]->nRow = nRow;
				m_vecPanel[nIndex]->nCol = m_nCols - 1;
				nIndex ++;
			}
			for (int nCol = 0; nCol < m_nCols; nCol++)
			{
				m_vecPanel[nIndex]->nRow = m_nRows - 1;
				m_vecPanel[nIndex]->nCol = nCol;
				nIndex ++;
			}
		}

		// ���¼���ÿһ�����ڵĴ�С
		ResizePanel();
		// ���������Ĵ��ڣ���Ҫ���µ����ߴ�
		PanelInfoPtr pPanel = m_vecPanel[0];
		if (!pPanel->hWnd ||
			!IsWindow(pPanel->hWnd))
		{
			pPanel->hWnd = CreatePanel(0, 0);
			::ShowWindow(pPanel->hWnd, SW_SHOW);
		}
		else
			::MoveWindow(pPanel->hWnd,pPanel->rect.left,pPanel->rect.top,RectWidth(pPanel->rect),RectHeight(pPanel->rect),FALSE);

		nIndex = 1;
		// �ٵ����Ҳ�һ�еĴ���
		int nRows = m_nRows - 1;
		int nCols = m_nCols;
		if (m_nFrameStyle == Style_2P1)
		{
			nRows = m_nRows;
			nCols = 0;
		}
		{
			for (int nRow = 0; nRow < nRows; nRow++)
			{
				PanelInfoPtr pPanel = m_vecPanel[nIndex ];
				if (!pPanel->hWnd ||
					!IsWindow(pPanel->hWnd))
				{
					pPanel->hWnd = CreatePanel(nIndex);
					::ShowWindow(pPanel->hWnd, SW_SHOW);
				}
				else
					::MoveWindow(pPanel->hWnd,pPanel->rect.left,pPanel->rect.top,RectWidth(pPanel->rect),RectHeight(pPanel->rect),FALSE);
				nIndex ++;
			}

			// �����ײ�һ�д���		
			for (int nCol = 0; nCol < nCols; nCol++)
			{
				PanelInfoPtr pPanel = m_vecPanel[nIndex];
				if (!pPanel->hWnd ||
					!IsWindow(pPanel->hWnd))
				{
					pPanel->hWnd = CreatePanel(nIndex );
					::ShowWindow(pPanel->hWnd, SW_SHOW);
				}
				else
					::MoveWindow(pPanel->hWnd,pPanel->rect.left,pPanel->rect.top,RectWidth(pPanel->rect),RectHeight(pPanel->rect),FALSE);
				nIndex ++;
			}
		}

	}
	
	Invalidate();
	return true;
}

bool CVideoFrame::AdjustPanels(int nCount,FrameStyle fs)
{
	if (fs != StyleNormal && fs == m_nFrameStyle)
		return true;

	CalcRowCol(nCount,m_nNewRows,m_nNewCols);
	_TraceMsgA("%s Rows = %d\tCols = %d.\n", __FUNCTION__, m_nNewRows, m_nNewCols);
	return AdjustPanels(m_nNewRows, m_nNewCols,fs);
}

void CVideoFrame::DrawGrid(CDC *pDc)
{
// 	CRect rtClient;
// 	GetClientRect(&rtClient);
// 	pDc->FillSolidRect(&rtClient,BackColor);
//	CPen* pOldPen = (CPen *)pDc->SelectObject(m_pUnSelectedPen);
// 	for (list<LinePtr>::iterator it = m_listLine.begin();it != m_listLine.end();it ++)
// 	{
// 		pDc->MoveTo((*it)->ptStart);
// 		pDc->LineTo((*it)->ptStop);
// 	}
	/*
	GetClientRect(&rtClient);
	int nWidth = rtClient.Width();
	int nHeight = rtClient.Height();
	int nAvgColWidth = nWidth / m_nCols;
	int nAvgRowHeight = nHeight / m_nRows;

	int nRemainedWidth = nWidth - nAvgColWidth*m_nCols;		// ƽ����������ӯ��
	int nRemainedHeight = nHeight - nAvgRowHeight*m_nRows;	// ƽ������߶���ӯ��

	int nStartX = rtClient.left;
	int nStartY = rtClient.top;

	// ������
	for (int nCol = 0; nCol < m_nCols; nCol++)
	{
		if (nCol > 0 && nRemainedWidth > 0)
		{
			nStartX++;
			nRemainedWidth--;
		}
		if (nCol > 0)
		{
			pDc->MoveTo(nStartX, nStartY);
			pDc->LineTo(nStartX, rtClient.bottom);
		}
		nStartX += nAvgColWidth;
	}
	// ������
	for (int nRow = 0; nRow < m_nRows; nRow++)
	{
		if (nRow > 0 && nRemainedHeight > 0)
		{
			nStartY++;
			nRemainedHeight--;
		}
		if (nRow > 0)
		{
			pDc->MoveTo(rtClient.left, nStartY);
			pDc->LineTo(rtClient.right, nStartY);
		}	
		nStartY += nAvgRowHeight;
	}
	pDc->SelectObject(pOldPen);
	*/
	
//#ifdef _DEBUG
//	_TraceMsgA("Index\tLeft\tRight\tTop\t\tBottom.\n");
//	for (int nRow = 0; nRow < m_nRows; nRow++)	
//	{
//		for (int nCol = 0; nCol < m_nCols; nCol++)
//		{
//			int nIndex = nRow*m_nCols + nCol;
//			RECT rtWnd = m_vecPanel[nIndex]->rect;
//			_TraceMsgA("(%d,%d)\t%d\t\t%d\t\t%d\t\t%d\n",nRow,nCol, pRtWnd->left, pRtWnd->right, pRtWnd->top, pRtWnd->bottom);
//		}
//	}
//#endif
}

// void CVideoFrame::OnPaint()
// {
// 	CPaintDC dc(this); // device context for painting
// 	
// 	DrawGrid(&dc);
// }

void CVideoFrame::OnDestroy()
{
	CWnd::OnDestroy();

}

void CVideoFrame::ResizePanel(int nWidth,int nHeight)
{
	CRect rtClient;
	m_pLastSelectRect = nullptr;
	m_pCurSelectRect = nullptr;
	if (!nWidth || !nHeight )
	{
		GetClientRect(&rtClient);
		if (!nWidth)
			nWidth = rtClient.Width();
		if (!nHeight)
			nHeight = rtClient.Height();
	}
	int nAvgColWidth = (nWidth - _GRID_LINE_WIDTH*(m_nCols + 1))/ m_nCols;
	int nAvgRowHeight = (nHeight - _GRID_LINE_WIDTH*(m_nRows + 1)) / m_nRows;

	int nRemainedWidth = nWidth - nAvgColWidth*m_nCols  ;	// ƽ����������ӯ��
	int nRemainedHeight = nHeight - nAvgRowHeight*m_nRows;	// ƽ������߶���ӯ��
	

	vector<RECT>vecRect;
	RECT rtSample = {0,0,0,0};
	for (int nRow = 0;nRow < m_nRows;nRow ++)
		for (int nCol = 0;nCol < m_nCols;nCol ++)
		{
			vecRect.push_back(rtSample);
		}
	if (m_nRows == 1 && m_nCols == 1)
	{
		vecRect[0].left =  1;
		vecRect[0].right = nWidth - 1;
		vecRect[0].top =  1;
		vecRect[0].bottom = nHeight - 1;
	}
	else
	{
		// �ȼ����׼�������ÿ�����ӵĳߴ�
		int nStartX = _GRID_LINE_WIDTH;
		int nIndex = 0;
		for (int nCol = 0; nCol < m_nCols; nCol++)
		{
			if (nCol > 0 && nRemainedWidth > 0)
			{
				nStartX++;
				nRemainedWidth--;
			}
			// ����ÿ����������ұ߽�
			for (int nRow = 0; nRow < m_nRows; nRow++)
			{
				if ( nCol > 0)
				{
					vecRect[nRow*m_nCols + nCol - 1].right = nStartX - _GRID_LINE_WIDTH;
					vecRect[nRow*m_nCols + nCol].left = nStartX + _GRID_LINE_WIDTH;
				}
				else
					vecRect[nRow*m_nCols + nCol].left = nStartX ;
			}
			nStartX += nAvgColWidth;
		}
		// ��������һ�е�������
		for (int nRow = 0; nRow < m_nRows; nRow++)
			vecRect[nRow*m_nCols + m_nCols - 1].right = nWidth - _GRID_LINE_WIDTH ;

		int nStartY = _GRID_LINE_WIDTH;
		for (int nRow = 0; nRow < m_nRows; nRow++)
		{
			if (nRow > 0 && nRemainedHeight > 0)
			{
				nStartY++;
				nRemainedHeight--;
			}
			// ����ÿ���ո�����±߽�		
			for (int nCol = 0; nCol < m_nCols; nCol++)
			{
				if (nRow > 0)
				{
					vecRect[nRow*m_nCols + nCol].top = nStartY + _GRID_LINE_WIDTH;
					vecRect[(nRow - 1)*m_nCols + nCol].bottom = nStartY - _GRID_LINE_WIDTH;
				}
				else
					vecRect[nRow*m_nCols + nCol].top = nStartY;
			}
			nStartY += nAvgRowHeight;
		}
	 	// ��������һ�еĵ�����
		for (int nCol = 0; nCol < m_nCols; nCol++)
			vecRect[(m_nRows - 1)*m_nCols + nCol].bottom = nHeight - _GRID_LINE_WIDTH;
	}
	

	if (m_nFrameStyle == StyleNormal)
	{
		for (int i = 0;i < m_vecPanel.size();i ++)
			m_vecPanel[i]->rect = vecRect[i];
// 		// ʹ�ñ���ɫ��Ϊ���������ɫ����˲�����Ҫ�����������
// 		// xionggao.lee @ 2017.02.08 
// 		// ��������������
// 		for (int nCol = 0;nCol < m_nCols - 1;nCol ++)
// 		{
// 			LinePtr pLine = LinePtr(new _Line);
// 			pLine->ptStart.x = vecRect[nCol].right + 2;
// 			pLine->ptStart.y = vecRect[nCol].top - 2;
// 
// 			pLine->ptStop.x = vecRect[(m_nRows - 1)*m_nCols + nCol].right + 2;
// 			pLine->ptStop.y = vecRect[(m_nRows - 1)*m_nCols + nCol].bottom + 2;
// 			m_listLine.push_back(pLine);
// 		}
// 		// �������������
// 		for (int nRow = 0;nRow < m_nRows - 1;nRow ++)
// 		{
// 			LinePtr pLine = LinePtr(new _Line);
// 			pLine->ptStart.x = vecRect[nRow*m_nCols].left - 2;
// 			pLine->ptStart.y = vecRect[nRow*m_nCols].bottom + 2;
// 
// 			pLine->ptStop.x = vecRect[(nRow + 1)*m_nCols - 1].right + 2;
// 			pLine->ptStop.y = vecRect[(nRow + 1)*m_nCols - 1].bottom + 2;
// 			m_listLine.push_back(pLine);
// 		}
	}
	else
	{
		if (m_nFrameStyle == Style_2P1)
		{
			m_vecPanel[0]->rect.left	 = vecRect[0].left;
			m_vecPanel[0]->rect.top		 = vecRect[0].top;
			m_vecPanel[0]->rect.right	 = vecRect[0].right;
			m_vecPanel[0]->rect.bottom	 = vecRect[2].bottom;

			m_vecPanel[1]->rect = vecRect[1];
			m_vecPanel[2]->rect = vecRect[3];

// 			// ʹ�ñ���ɫ��Ϊ���������ɫ����˲�����Ҫ�����������
// 			// xionggao.lee @ 2017.02.08 
// 			LinePtr pLine = LinePtr(new _Line);
// 			pLine->ptStart.x = vecRect[0].right + 2;
// 			pLine->ptStart.y = vecRect[0].top - 2;
// 
// 			pLine->ptStop.x = vecRect[2].right + 2;
// 			pLine->ptStop.y = vecRect[2].bottom + 2;
// 			m_listLine.push_back(pLine);
// 
// 			pLine = LinePtr(new _Line);
// 			pLine->ptStart.x = vecRect[0].right + 2;
// 			pLine->ptStart.y = vecRect[0].bottom + 2;
// 
// 			pLine->ptStop.x = vecRect[1].right + 2;
// 			pLine->ptStop.y = vecRect[1].bottom + 2;
// 			m_listLine.push_back(pLine);
		}
		else
		{
			m_vecPanel[0]->rect.left	 = vecRect[0].left;
			m_vecPanel[0]->rect.top		 = vecRect[0].top;
			int nRightBottom = (m_nRows-1)*m_nCols  - 2;
			m_vecPanel[0]->rect.right	 = vecRect[nRightBottom].right;
			m_vecPanel[0]->rect.bottom	 = vecRect[nRightBottom].bottom;
			int nIndex = 1;
			for (int nRow = 0;nRow < m_nRows - 1;nRow ++)
				m_vecPanel[nIndex ++]->rect = vecRect[(nRow + 1)*m_nCols - 1];

			for (int nCol = 0;nCol < m_nCols;nCol ++)
				m_vecPanel[nIndex ++]->rect = vecRect[(m_nRows-1)*m_nCols + nCol];

// 			// ʹ�ñ���ɫ��Ϊ���������ɫ����˲�����Ҫ�����������
// 			// xionggao.lee @ 2017.02.08 
//			// ���㳤������
// 			LinePtr pLine = LinePtr(new _Line);
// 			pLine->ptStart.x = vecRect[m_nCols - 2].right + 2;
// 			pLine->ptStart.y = vecRect[m_nCols - 2].top - 2;
// 
// 			pLine->ptStop.x = vecRect[m_nRows*m_nCols - 2].right + 2;
// 			pLine->ptStop.y = vecRect[m_nRows*m_nCols - 2].bottom + 2;
// 			m_listLine.push_back(pLine);
// 
// 			// ���㳤������
// 			pLine = LinePtr(new _Line);
// 			pLine->ptStart.x = vecRect[(m_nRows - 2)*m_nCols].left - 2;
// 			pLine->ptStart.y = vecRect[(m_nRows - 2)*m_nCols].bottom + 2;
// 
// 			pLine->ptStop.x = vecRect[(m_nRows - 1)*m_nCols - 1].right + 2;
// 			pLine->ptStop.y = vecRect[(m_nRows - 1)*m_nCols - 1].bottom + 2;
// 			m_listLine.push_back(pLine);
// 
// 			// ���������������
// 			for (int nCol = 0;nCol < m_nCols - 2;nCol ++)
// 			{
// 				LinePtr pLine = LinePtr(new _Line);
// 				pLine->ptStart.x = vecRect[(m_nRows -2)*nCol].right + 2;
// 				pLine->ptStart.y = vecRect[(m_nRows -2)*nCol].bottom + 2;
// 
// 				pLine->ptStop.x = vecRect[(m_nRows - 1)*m_nCols + nCol].right + 2;
// 				pLine->ptStop.y = vecRect[(m_nRows - 1)*m_nCols + nCol].bottom + 2;
// 				m_listLine.push_back(pLine);
// 			}
// 
// 			// ����̺���������
// 			for (int nRow = 0;nRow < m_nRows - 2;nRow ++)
// 			{
// 				LinePtr pLine = LinePtr(new _Line);
// 				pLine->ptStart.x = vecRect[(nRow + 1)*m_nCols - 2].left - 2;
// 				pLine->ptStart.y = vecRect[(nRow + 1)*m_nCols - 2].bottom + 2;
// 
// 				pLine->ptStop.x = vecRect[(nRow + 1)*m_nCols - 1].right + 2;
// 				pLine->ptStop.y = vecRect[(nRow + 1)*m_nCols - 1].bottom + 2;
// 				m_listLine.push_back(pLine);
// 			}
		}
	}
	
	//���о�������һ������
// 	for (vector<PanelInfoPtr>::iterator it = m_vecPanel.begin();it != m_vecPanel.end();it ++)
// 	{
// 		LPRECT pRect = &(*it)->rect;
// 		pRect->left += 1;
// 		pRect->top += 1;
// 		pRect->right -= 1;
// 		pRect->bottom -= 1;
// 	}

//#ifdef _DEBUG
//	_TraceMsgA("Index\tLeft\tRight\tTop\t\tBottom.\n");
//	for (int nRow = 0; nRow < m_nRows; nRow++)	
//	{
//		for (int nCol = 0; nCol < m_nCols; nCol++)
//		{
//			int nIndex = nRow*m_nCols + nCol;
//			RECT rtWnd = m_vecPanel[nIndex]->rect;
//			_TraceMsgA("(%d,%d)\t%d\t\t%d\t\t%d\t\t%d\n",nRow,nCol, pRtWnd->left, pRtWnd->right, pRtWnd->top, pRtWnd->bottom);
//		}
//	}
//#endif
}

void CVideoFrame::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	ResizePanel(cx, cy);
	for (vector<PanelInfoPtr>::iterator it = m_vecPanel.begin(); it != m_vecPanel.end(); it++)
		(*it)->UpdateWindow();
	RefreshSelectedPanel();
}

LRESULT CVideoFrame::OnTroggleFullScreen(WPARAM W, LPARAM L)
{
	HWND hRoot = ::GetAncestor(m_hWnd, GA_ROOT);
	if (hRoot)
	{
		::PostMessage(hRoot, WM_TROGGLEFULLSCREEN, W, L);
	}
	return 0;
}

void CVideoFrame::RefreshSelectedPanel()
{
	if (m_nCurPanel == -1)
		return ;
	CClientDC  dc(this);
	RECT rtTemp;
	CPen* pOldPen = nullptr;
	CBrush *pBrush= CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH));
	CBrush *pOldBrush=dc.SelectObject(pBrush);

	if (m_pLastSelectRect)
	{
		::CopyRect(&rtTemp,m_pLastSelectRect);
		rtTemp.left -=1;
		rtTemp.top -= 1;
		rtTemp.right += 1;
		rtTemp.bottom += 1;
		pOldPen = (CPen *)dc.SelectObject(m_pRestorePen);	

		dc.Rectangle(&rtTemp);
	}
	if (m_nCurPanel < m_vecPanel.size())
		m_pCurSelectRect = &m_vecPanel[m_nCurPanel]->rect;
	if (m_pCurSelectRect)
	{
		::CopyRect(&rtTemp,m_pCurSelectRect);
		rtTemp.left -=1;
		rtTemp.top -= 1;
		rtTemp.right += 1;
		rtTemp.bottom += 1;
		if (!pOldPen)
			pOldPen = (CPen*)dc.SelectObject(m_pSelectedPen);
		else
			dc.SelectObject(m_pSelectedPen);
		dc.Rectangle(&rtTemp);
	}
	dc.SelectObject(pOldBrush);
	dc.SelectObject(pOldPen);
}
void CVideoFrame::OnLButtonDown(UINT nFlags, CPoint point)
{
	for (int nIndex = 0;nIndex < m_vecPanel.size();nIndex ++)
	{
		if (::PtInRect(&m_vecPanel[nIndex]->rect,point))
		{
			m_nCurPanel = nIndex;
			if (!m_pCurSelectRect)
			{
				m_pLastSelectRect = nullptr;
			}
			else if (m_pCurSelectRect != &m_vecPanel[nIndex]->rect)//����һ�����л�����ǰ����
			{
				m_pLastSelectRect = m_pCurSelectRect;
			}
			m_pCurSelectRect = &m_vecPanel[nIndex]->rect;

			RefreshSelectedPanel();
			m_pCurrentFrame = this;
		}
	}
	CWnd::OnLButtonDown(nFlags, point);
}

void CVideoFrame::OnRButtonDown(UINT nFlags, CPoint point)
{
	for (int nIndex = 0;nIndex < m_vecPanel.size();nIndex ++)
	{
		if (::PtInRect(&m_vecPanel[nIndex]->rect,point))
		{
			m_nCurPanel = nIndex;
			if (!m_pCurSelectRect)
			{
				m_pLastSelectRect = nullptr;
			}
			else if (m_pCurSelectRect != &m_vecPanel[nIndex]->rect)//����һ�����л�����ǰ����
			{
				m_pLastSelectRect = m_pCurSelectRect;
			}
			m_pCurSelectRect = &m_vecPanel[nIndex]->rect;

			RefreshSelectedPanel();
			m_pCurrentFrame = this;
			HWND hParentWnd = GetParent()->GetSafeHwnd();
			if (!hParentWnd)
				SendMessage(WM_FRAME_RBUTTONDOWN,(WPARAM)MAKEWPARAM(point.x,point.y),(LPARAM)m_nCurPanel);
		}
	}

	CWnd::OnRButtonDown(nFlags, point);
}

HBRUSH CVideoFrame::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
 	HBRUSH hbr =  CreateSolidBrush(BackColor); //��������ˢ;  
 	pDC->SetBkMode(TRANSPARENT);  
	return hbr;  
}

BOOL CVideoFrame::OnEraseBkgnd(CDC* pDC)
{
	CBrush back(BackColor);  
	CBrush* pold=pDC->SelectObject(&back);  
	CRect rect;  
	pDC->GetClipBox (&rect);  
	pDC->PatBlt (rect.left,rect.top,rect.Width(),rect.Height(),PATCOPY);  
	pDC->SelectObject(pold);  
	RefreshSelectedPanel();
	return TRUE;   
}
