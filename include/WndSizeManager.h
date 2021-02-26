#pragma once
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC ��������ͱ�׼���
#include <afxext.h>         // MFC ��չ
#include <afxdisp.h>        // MFC �Զ�����

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC �� Internet Explorer 4 �����ؼ���֧��
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC �� Windows �����ؼ���֧��
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <assert.h>

#ifndef RectWidth
#define RectWidth(rt)	(rt.right - rt.left)
#endif

#ifndef RectHeight
#define RectHeight(rt)	(rt.bottom - rt.top)
#endif

#include <vector>
using namespace std;
enum DockType
{
	DockLeft	 = 1,			// ����䶯����
	DockTop		 = 2,			// ����䶯����
	DockRight	 = 4,			// ���ֺ͸����ڵ��ұߵľ���
	DockBottom	 = 8,			// ���ֺ͸����ڵĵײ��ľ���
	DockCenter	 = 16,			// ���ֺ͸������ıߵĵľ���
	DockCtrlLeft = 32,			// ���ֺ�ָ���ؼ���ߵľ���
	DockCtrlTop	 = 64,			// ���ֺ�ָ���ؼ������ľ���
	DockCtrlRight= 128,			// ���ֺ�ָ���ؼ���ߵľ���
	DockCtrlBottom=256,			// ���ֺ�ָ���ؼ��ײ��ľ���
};

enum DockIndex
{
	DILeft = 0,
	DITop  = 1,
	DIRight= 2,
	DIBottom=3
};
struct WndPostionInfo
{
	HWND	hWnd;
	UINT	nID;
	DockType nDockType;			// ͣ������
	UINT	nDockCtrl[4];
	UINT	DockDistance[4];	// ͣ������
	RECT	rect;				// ԭʼ��С
	bool	bAutoMove;			// �Ƿ��Զ��ƶ�
	RECT	rtCurrentPos;
	WndPostionInfo()
	{
		ZeroMemory(this,sizeof(WndPostionInfo));
		bAutoMove = true;
	}
};

//////////////////////////////////////////////////////////////////////////
// �Ӵ���λ���Զ�ͣ��������
// ���ڸ����ڳߴ��λ�÷����仯ʱ���Զ������Ӵ��ڵ�λ����Ϣ
// ����:���۸�
// 2016.04.10
//////////////////////////////////////////////////////////////////////////
class CWndSizeManger
{
private:
	vector<WndPostionInfo> m_vWndPostionInfo;
	CWnd	*m_pParentWnd;
public:
	CWndSizeManger()
	{

	}
	CWndSizeManger(CWnd* ParentWnd)
	{
		m_pParentWnd = ParentWnd;

	}
	~CWndSizeManger(void)
	{
		m_vWndPostionInfo.clear();
	}
	// �����Ӵ��ڵĸ�����
	// �ú�������ҪSaveWndPosition()����ǰ����
	void SetParentWnd(CWnd* ParentWnd)
	{
		m_pParentWnd = ParentWnd;

	}

	// �������洰�ڵ���ʼλ�ã������ڴ��ڳ�ʼ�����������Ӵ��ڵ�λ�ö����ٱ䶯ʱ����
	// nDlgItemIDArray		����ID������
	// nIDCount				nDlgItemIDArray�д���ID������
	// nDock				������λ�ñ仯���Ӵ��ڵ�ͣ��λ��
	// bAutoMove			������λ�ñ仯���Ӵ������Զ��ƶ���ͣ��λ��
	bool SaveWndPosition(UINT *nDlgItemIDArray, UINT nIDCount, DockType nDock ,bool bAutoMove = true)
	{
		if (!m_pParentWnd)
			return false;
		RECT rtClientRect;
		m_pParentWnd->GetClientRect(&rtClientRect);
		WndPostionInfo wndPos;
		wndPos.bAutoMove = bAutoMove;
		wndPos.nDockType = nDock;
		CWnd *pTempWnd = NULL;
		for (int i = 0; i < nIDCount; i++)
		{
			wndPos.hWnd = m_pParentWnd->GetDlgItem(nDlgItemIDArray[i])->GetSafeHwnd();
			if (!wndPos.hWnd)
			{
				assert(false);
				continue;
			}
			wndPos.nID = nDlgItemIDArray[i];
			::GetWindowRect(wndPos.hWnd, &wndPos.rect);
			m_pParentWnd->ScreenToClient(&wndPos.rect);
			switch (wndPos.nDockType)
			{
			default:
				{
					if ((wndPos.nDockType & DockLeft) == DockLeft )
						wndPos.DockDistance[DILeft] = wndPos.rect.left - rtClientRect.left;
					if ((wndPos.nDockType & DockTop) == DockTop )
						wndPos.DockDistance[DITop] = wndPos.rect.top - rtClientRect.top;
					if ((wndPos.nDockType & DockRight) == DockRight )
						wndPos.DockDistance[DIRight] = rtClientRect.right - wndPos.rect.right;
					if ((wndPos.nDockType & DockBottom) == DockBottom )
						wndPos.DockDistance[DIBottom] = rtClientRect.bottom - wndPos.rect.bottom;
					if ((wndPos.nDockType & DockCenter) == DockCenter)
					{
						RECT rtNeighbor = {0};
						HWND hWndNeighbor = NULL;
						// �ڿ��Ƽ������
						if ((wndPos.nDockType & DockCtrlLeft) == DockCtrlLeft)
						{
							hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DILeft])->m_hWnd;
							::GetWindowRect(hWndNeighbor, &rtNeighbor);
							m_pParentWnd->ScreenToClient(&rtNeighbor);
							wndPos.DockDistance[DILeft] = rtNeighbor.right- wndPos.rect.left;
						}
						else if ((wndPos.nDockType & DockCtrlTop) == DockCtrlTop)
						{
							hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DITop])->m_hWnd;
							::GetWindowRect(hWndNeighbor, &rtNeighbor);
							m_pParentWnd->ScreenToClient(&rtNeighbor);
							wndPos.DockDistance[DITop] = rtNeighbor.top - wndPos.rect.bottom ;
						}
						else if ((wndPos.nDockType & DockCtrlRight) == DockCtrlRight)
						{
							hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DIRight])->m_hWnd;
							::GetWindowRect(hWndNeighbor, &rtNeighbor);
							m_pParentWnd->ScreenToClient(&rtNeighbor);
							wndPos.DockDistance[DIRight] = wndPos.rect.left - rtNeighbor.right;

						}
						else if ((wndPos.nDockType & DockCtrlBottom) == DockCtrlBottom)
						{
							hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DILeft])->m_hWnd;
							::GetWindowRect(hWndNeighbor, &rtNeighbor);
							m_pParentWnd->ScreenToClient(&rtNeighbor);
							wndPos.DockDistance[DILeft] = wndPos.rect.top - rtNeighbor.bottom;
						}
					}
					
					break;
				}
			case DockTop:		// ����䶯
				{
					wndPos.DockDistance[DITop] = wndPos.rect.top - rtClientRect.top;
					break;
				}
			case DockLeft:		// ����䶯
				{
					wndPos.DockDistance[DILeft] = wndPos.rect.left - rtClientRect.left;
					break;
				}
			case DockBottom:
				{
					wndPos.DockDistance[DIBottom] = rtClientRect.bottom - wndPos.rect.bottom;
					break;
				}
			case DockRight:
				{
					wndPos.DockDistance[DIRight] = rtClientRect.right - wndPos.rect.right;
					break;
				}
			case DockCenter:
				{
					wndPos.DockDistance[DILeft] = wndPos.rect.left - rtClientRect.left;
					wndPos.DockDistance[DITop] = wndPos.rect.top - rtClientRect.top;
					wndPos.DockDistance[DIRight] = rtClientRect.right - wndPos.rect.right;
					wndPos.DockDistance[DIBottom] = rtClientRect.bottom - wndPos.rect.bottom;
				}
				break;
			}

			m_vWndPostionInfo.push_back(wndPos);
		}
		return true;
	}

	// ������λ�ñ䶯, ���ڵ���SaveWndPosition()ʱ�����������Ϊfalse,��ô��ڲ����ƶ���ֻ������rtCurrentPosֵ
	// ��ͨ��GetWndCurrentPostion()����ȡ�øò���ֵ
	void OnSize(UINT nType, int cx, int cy)
	{
		if (!m_pParentWnd)
			return;
		RECT rtClientRect;
		m_pParentWnd->GetClientRect(&rtClientRect);
		for (vector<WndPostionInfo>::iterator it = m_vWndPostionInfo.begin(); it != m_vWndPostionInfo.end(); it++)
		{
			WndPostionInfo &wndPos = (*it);
			RECT rt = wndPos.rect;
			switch (wndPos.nDockType)
			{
			default:
				{
					if (nType == SIZE_MAXIMIZED)
					{
						if ((wndPos.nDockType & DockLeft) == DockLeft)
						{
							rt.left = rtClientRect.left +  wndPos.DockDistance[DILeft];
							if ((wndPos.nDockType & DockRight) != DockRight)
								rt.right = rt.left + RectWidth(wndPos.rect);
						}
						if ((wndPos.nDockType & DockTop) == DockTop)
						{
							rt.top = rtClientRect.top + wndPos.DockDistance[DITop];
							if ((wndPos.nDockType & DockBottom) != DockBottom)
								rt.bottom = rt.top + RectHeight(wndPos.rect);
						}
							
						if ((wndPos.nDockType & DockRight) == DockRight)
						{
							rt.right = rtClientRect.right - wndPos.DockDistance[DIRight];
							if ((wndPos.nDockType & DockLeft) != DockLeft)
								rt.left = rt.right - RectWidth(wndPos.rect);
						}
						if ((wndPos.nDockType & DockBottom) == DockBottom)
						{
							rt.bottom = rtClientRect.bottom - wndPos.DockDistance[DIBottom];
							if ((wndPos.nDockType & DockTop) != DockTop)
								rt.top = rt.bottom - RectHeight(wndPos.rect);
						}
						if ((wndPos.nDockType & DockCenter) == DockCenter)
						{
							RECT rtNeighbor = {0};
							HWND hWndNeighbor = NULL;
							// �ڿ��Ƽ������
							if ((wndPos.nDockType & DockCtrlLeft) == DockCtrlLeft)
							{
								hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DILeft])->m_hWnd;
								::GetWindowRect(hWndNeighbor, &rtNeighbor);
								m_pParentWnd->ScreenToClient(&rtNeighbor);
								wndPos.DockDistance[DILeft] = rtNeighbor.right- wndPos.rect.left;
							}
							if ((wndPos.nDockType & DockCtrlTop) == DockCtrlTop)
							{
								hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DITop])->m_hWnd;
								::GetWindowRect(hWndNeighbor, &rtNeighbor);
								m_pParentWnd->ScreenToClient(&rtNeighbor);
								wndPos.DockDistance[DITop] = rtNeighbor.top - wndPos.rect.bottom ;
							}
							if ((wndPos.nDockType & DockCtrlRight) == DockCtrlRight)
							{
								hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DIRight])->m_hWnd;
								::GetWindowRect(hWndNeighbor, &rtNeighbor);
								m_pParentWnd->ScreenToClient(&rtNeighbor);
								wndPos.DockDistance[DIRight] = wndPos.rect.left - rtNeighbor.right;
							}
							if ((wndPos.nDockType & DockCtrlBottom) == DockCtrlBottom)
							{
								hWndNeighbor = m_pParentWnd->GetDlgItem(wndPos.nDockCtrl[DILeft])->m_hWnd;
								::GetWindowRect(hWndNeighbor, &rtNeighbor);
								m_pParentWnd->ScreenToClient(&rtNeighbor);
								wndPos.DockDistance[DILeft] = wndPos.rect.top - rtNeighbor.bottom;
							}
						}
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					else
					{
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					memcpy(&wndPos.rtCurrentPos,&rt,sizeof(RECT));
					break;
				}
			case DockTop:		// ����䶯
			case DockLeft:		// ����䶯
				{
					memcpy(&wndPos.rtCurrentPos,&rt,sizeof(RECT));
				}
				break;
			case DockBottom:
				{
					if (nType == SIZE_MAXIMIZED)
					{
						rt.bottom = rtClientRect.bottom - wndPos.DockDistance[DIBottom];
						rt.top = rt.bottom - (wndPos.rect.bottom - wndPos.rect.top);
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					else if (nType == SIZE_RESTORED)
					{
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					memcpy(&wndPos.rtCurrentPos,&rt,sizeof(RECT));
				}
				break;
			case DockRight:
				{
					if (nType == SIZE_MAXIMIZED)
					{
						rt.right = rtClientRect.right - wndPos.DockDistance[DIRight];
						rt.left = rt.right - (wndPos.rect.right - wndPos.rect.left);
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					else if (nType == SIZE_RESTORED)
					{
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					memcpy(&wndPos.rtCurrentPos,&rt,sizeof(RECT));
					break;

				}
			case DockCenter:
				{
					if (nType == SIZE_MAXIMIZED)
					{
						rt.left = rtClientRect.left + wndPos.DockDistance[DILeft];
						rt.top = rtClientRect.top + wndPos.DockDistance[DITop];
						rt.right = rtClientRect.right - wndPos.DockDistance[DIRight];
						rt.bottom = rtClientRect.bottom - wndPos.DockDistance[DIBottom];			
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					else if (nType == SIZE_RESTORED)
					{
						if (wndPos.bAutoMove)
							::MoveWindow(wndPos.hWnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, true);
					}
					memcpy(&wndPos.rtCurrentPos,&rt,sizeof(RECT));
					break;
				}
			}
		}
	}

	// ��ȡָ��ID�Ĵ����ڸ����ڳߴ�仯�ĳߴ���Ϣ
	// ����trueʱ��ȡ�ɹ�������ʧ��
	bool GetWndCurrentPostion(UINT nID,RECT &rt)
	{
		for (vector<WndPostionInfo>::iterator it = m_vWndPostionInfo.begin(); it != m_vWndPostionInfo.end(); it++)
			if ((*it).nID == nID)
			{
				memcpy(&rt,&(*it).rtCurrentPos,sizeof(RECT));
				return true;
			}
			return false;
	}
};
