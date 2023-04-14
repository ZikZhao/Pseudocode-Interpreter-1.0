#include "stdafx.h"
#include "InfoView.h"

BEGIN_MESSAGE_MAP(CInfoViewTag, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()
CInfoViewTag::CInfoViewTag(wchar_t* text, USHORT index)
{
	m_Width = m_Height = 0;
	m_Text = text;
	m_Index = index;
	m_bHover = false;
	m_bSelected = false;
}
CInfoViewTag::~CInfoViewTag()
{

}
int CInfoViewTag::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CDC* pWindowDC = GetDC();
	CRect rect;
	GetClientRect(&rect);
	m_Width = rect.right - rect.left;
	m_Height = rect.bottom - rect.top;
	// 创建主源
	m_Source.CreateCompatibleDC(pWindowDC);
	CBitmap* pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, m_Width, m_Height);
	m_Source.SelectObject(pBitmap);
	rect = CRect(0, 0, m_Width, m_Height);
	m_Source.FillRect(&rect, pGreyBlackBrush);
	m_Source.SetTextColor(RGB(255, 255, 255));
	m_Source.SetBkMode(TRANSPARENT);
	m_Source.SelectObject(m_Font);
	m_Source.DrawTextW(m_Text, -1, &rect, DT_CENTER | DT_VCENTER);
	// 创建悬浮源
	m_Hover.CreateCompatibleDC(pWindowDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, m_Width, m_Height);
	m_Hover.SelectObject(pBitmap);
	m_Hover.FillRect(&rect, pGreyBlackBrush);
	CBrush brush(RGB(61, 61, 61));
	m_Hover.SelectObject(&brush);
	CPen pen(PS_NULL, 0, (COLORREF)0);
	m_Hover.SelectObject(&pen);
	m_Hover.RoundRect(&rect, CPoint(5, 5));
	m_Hover.SetTextColor(RGB(255, 255, 255));
	m_Hover.SetBkMode(TRANSPARENT);
	m_Hover.SelectObject(m_Font);
	m_Hover.DrawTextW(m_Text, -1, &rect, DT_CENTER | DT_VCENTER);
	// 创建选中源
	m_Selected.CreateCompatibleDC(pWindowDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, m_Width, m_Height);
	m_Selected.SelectObject(pBitmap);
	m_Selected.FillRect(&rect, pGreyBlackBrush);
	m_Selected.SelectObject(pThemeColorBrush);
	m_Selected.SelectObject(&pen);
	m_Selected.RoundRect(&rect, CPoint(5, 5));
	m_Selected.SetTextColor(RGB(255, 255, 255));
	m_Selected.SetBkMode(TRANSPARENT);
	m_Selected.SelectObject(m_Font);
	m_Selected.DrawTextW(m_Text, -1, &rect, DT_CENTER | DT_VCENTER);

	return 0;
}
void CInfoViewTag::OnPaint()
{
	CPaintDC dc(this);
	if (m_bSelected) {
		dc.BitBlt(0, 0, m_Width, m_Height, &m_Selected, 0, 0, SRCCOPY);
	}
	else if (m_bHover) {
		dc.BitBlt(0, 0, m_Width, m_Height, &m_Hover, 0, 0, SRCCOPY);
	}
	else {
		dc.BitBlt(0, 0, m_Width, m_Height, &m_Source, 0, 0, SRCCOPY);
	}
}
BOOL CInfoViewTag::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CInfoViewTag::OnMouseMove(UINT nFlags, CPoint point)
{
	if (not m_bHover) {
		TRACKMOUSEEVENT tme{};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		TrackMouseEvent(&tme);
		m_bHover = true;
		Invalidate(FALSE);
	}
	m_bHover = true;
}
void CInfoViewTag::OnMouseLeave()
{
	m_bHover = false;
	Invalidate(FALSE);
}
void CInfoViewTag::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
}
void CInfoViewTag::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	if (point.x > 0 and point.x <= m_Width and
		point.y > 0 and point.y <= m_Height) {
		CInfoView::pObject->ShiftTag(m_Index);
	}
}
void CInfoViewTag::SetState(bool state)
{
	m_bSelected = state;
	Invalidate(FALSE);
}

BEGIN_MESSAGE_MAP(CInfoView, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()
CInfoView::CInfoView()
{
	m_Width = m_Height = 0;
	m_Tags[0] = new CInfoViewTag(const_cast<wchar_t*>(L"控制台"), 0);
	m_Tags[1] = new CInfoViewTag(const_cast<wchar_t*>(L"变量表"), 1);
	m_Tags[2] = new CInfoViewTag(const_cast<wchar_t*>(L"调用堆栈"), 2);
	m_CurrentIndex = 0;
	pObject = this;
}
CInfoView::~CInfoView()
{

}
int CInfoView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CDC* pWindowDC = GetDC();

	m_Console.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_SplitterPen.CreatePen(PS_SOLID, 1, RGB(254, 74, 99));
	CInfoViewTag::m_Font.CreateFontW(24, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	CDC tempDC;
	tempDC.CreateCompatibleDC(pWindowDC);
	tempDC.SelectObject(&CInfoViewTag::m_Font);
	CRect rect;
	tempDC.DrawTextW(L"一", -1, &rect, DT_CALCRECT);
	m_CharSize = CSize(rect.right, rect.bottom);
	m_Tags[0]->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 1, m_CharSize.cx * 3 + 8, m_CharSize.cy + 4), this, NULL);
	m_Tags[1]->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(m_CharSize.cx * 3 + 16, 1, m_CharSize.cx * 6 + 24, m_CharSize.cy + 4), this, NULL);
	m_Tags[2]->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(m_CharSize.cx * 6 + 32, 1, m_CharSize.cx * 10 + 40, m_CharSize.cy + 4), this, NULL);
	m_Tags[0]->SetState(true);

	return 0;
}
void CInfoView::OnPaint()
{
	CPaintDC dc(this);
	dc.SelectObject(&m_SplitterPen);
	dc.MoveTo(CPoint(0, 0));
	dc.LineTo(CPoint(m_Width, 0));
}
BOOL CInfoView::OnEraseBkgnd(CDC* pDC)
{
	CRect rect(0, 0, m_Width, m_Height);
	pDC->FillRect(&rect, pGreyBlackBrush);
	return TRUE;
}
void CInfoView::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	m_Width = cx;
	m_Height = cy;
	CRect rect(0, m_CharSize.cy + 4, cx, cy);
	m_Console.MoveWindow(rect);
}
void CInfoView::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize = CPoint(0, 100);
}
void CInfoView::ShiftTag(USHORT index)
{
	m_Tags[m_CurrentIndex]->SetState(false);
	switch (m_CurrentIndex) {
	case 0:
		m_Console.ShowWindow(SW_HIDE);
		break;
	}
	m_CurrentIndex = index;
	m_Tags[m_CurrentIndex]->SetState(true);
	switch (m_CurrentIndex) {
	case 0:
		m_Console.ShowWindow(SW_SHOW);
		break;
	}
}
