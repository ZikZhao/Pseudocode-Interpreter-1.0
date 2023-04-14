#include "stdafx.h"
#include "StatusBar.h"
#define LASTING_TIME 5000


BEGIN_MESSAGE_MAP(CCustomStatusBar, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
END_MESSAGE_MAP()
CCustomStatusBar::CCustomStatusBar()
{
	m_Width = m_Height = 0;
	m_bRun = false;
	m_Text = m_DefaultText;
}
CCustomStatusBar::~CCustomStatusBar()
{
}
int CCustomStatusBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_Font.CreateFontW(18, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");

	return 0;
}
BOOL CCustomStatusBar::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CCustomStatusBar::OnPaint()
{
	CPaintDC dc(this);
	dc.SelectObject(&m_Font);
	CRect rect(0, 0, m_Width, m_Height);
	if (m_bRun) {
		dc.FillRect(&rect, pThemeColorBrush);
	}
	else {
		dc.FillRect(&rect, pGreyBlackBrush);
	}
	rect = CRect(5, 4, m_Width, 24);
	dc.SetTextColor(RGB(255, 255, 255));
	dc.SetBkMode(TRANSPARENT);
	dc.DrawTextW(m_Text, -1, &rect, DT_END_ELLIPSIS);
}
void CCustomStatusBar::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	m_Width = cx;
	m_Height = cy;
	Invalidate(FALSE);
}
void CCustomStatusBar::OnTimer(UINT_PTR nIDEvent)
{
	m_Text = m_DefaultText;
	Invalidate(FALSE);
}
void CCustomStatusBar::UpdateState(bool bRun)
{
	m_bRun = bRun;
	Invalidate(FALSE);
}
void CCustomStatusBar::UpdateMessage(const wchar_t* text)
{
	m_Text = text;
	SetTimer(NULL, LASTING_TIME, nullptr);
	Invalidate(FALSE);
}
