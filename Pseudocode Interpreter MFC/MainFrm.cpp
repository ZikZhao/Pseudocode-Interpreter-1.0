#include "pch.h"
#define DISPATCH_CASE(id, cls, function) {case id: cls::pObject->function(); break;}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CDC MemoryDC;
CDC ScreenDC;
CBrush* pThemeColorBrush = new CBrush(RGB(254, 74, 99));
CBrush* pGreyBlackBrush = new CBrush(RGB(30, 30, 30));
CPen* pNullPen = new CPen(PS_NULL, 0, RGB(0, 0, 0));
int SCREEN_WIDTH = 0;
int SCREEN_HEIGHT = 0;

// CMainFrame
BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_WM_NCCALCSIZE()
	ON_WM_NCHITTEST()
	ON_WM_NCMOUSEMOVE()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_NCLBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_DROPFILES()
	ON_WM_CLOSE()
	ON_COMMAND_RANGE(0, 65535, CMainFrame::OnDispatchCommand)
END_MESSAGE_MAP()
CMainFrame::CMainFrame() noexcept
{
	m_Width = 0;
	m_Height = 0;
	ZeroMemory(&m_bHover, 3);
	pObject = this;
}
CMainFrame::~CMainFrame()
{
}
BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_MOUSEMOVE)
	{
		m_Tip.RelayEvent(pMsg);
	}
	return CFrameWndEx::PreTranslateMessage(pMsg);
}
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// 为所有组件创建内存DC
	HDC hdc = ::GetDC(NULL);
	ScreenDC.Attach(hdc);
	MemoryDC.CreateCompatibleDC(&ScreenDC);
	
	// 创建图标源
	m_Icon.CreateCompatibleDC(&ScreenDC);
	CBitmap* pBitmap = new CBitmap;
	pBitmap->LoadBitmapW(IDB_PI);
	m_Icon.SelectObject(pBitmap);

	CRect rect(0, 0, 32, 32);
	CBrush brush_grey(RGB(60, 60, 60));
	CBrush brush_red(RGB(230, 20, 20));

	m_Close.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 32, 32);
	m_Close.SelectObject(pBitmap);
	m_CloseHover.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 32, 32);
	m_CloseHover.SelectObject(pBitmap);
	m_Close.FillRect(&rect, pGreyBlackBrush);
	m_CloseHover.FillRect(&rect, &brush_red);
	Gdiplus::Graphics graphic1(m_Close);
	Gdiplus::Graphics graphic2(m_CloseHover);
	Gdiplus::Pen pen1 = Gdiplus::Pen(Gdiplus::Color(214, 214, 214), 2.5);
	graphic1.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	graphic2.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	graphic1.DrawLine(&pen1, Gdiplus::Point(8, 8), Gdiplus::Point(23, 23));
	graphic2.DrawLine(&pen1, Gdiplus::Point(8, 8), Gdiplus::Point(23, 23));
	graphic1.DrawLine(&pen1, Gdiplus::Point(23, 8), Gdiplus::Point(8, 23));
	graphic2.DrawLine(&pen1, Gdiplus::Point(23, 8), Gdiplus::Point(8, 23));

	m_Max.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 32, 32);
	m_Max.SelectObject(pBitmap);
	m_MaxHover.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 32, 32);
	m_MaxHover.SelectObject(pBitmap);
	m_Max.FillRect(&rect, pGreyBlackBrush);
	m_MaxHover.FillRect(&rect, &brush_grey);
	Gdiplus::Graphics graphic3(m_Max);
	Gdiplus::Graphics graphic4(m_MaxHover);
	Gdiplus::Pen pen2 = Gdiplus::Pen(Gdiplus::Color(214, 214, 214), 2.0);
	graphic3.DrawRectangle(&pen2, 9, 9, 14, 14);
	graphic4.DrawRectangle(&pen2, 9, 9, 14, 14);

	m_Restore.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 32, 32);
	m_Restore.SelectObject(pBitmap);
	m_RestoreHover.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 32, 32);
	m_RestoreHover.SelectObject(pBitmap);
	m_Restore.FillRect(&rect, pGreyBlackBrush);
	m_RestoreHover.FillRect(&rect, &brush_grey);
	Gdiplus::Graphics graphic5(m_Restore);
	Gdiplus::Graphics graphic6(m_RestoreHover);
	Gdiplus::Pen pen3 = Gdiplus::Pen(Gdiplus::Color(214, 214, 214), 2.0);
	graphic5.DrawRectangle(&pen3, 12, 8, 12, 12);
	graphic6.DrawRectangle(&pen3, 12, 8, 12, 12);
	graphic5.DrawRectangle(&pen3, 8, 12, 12, 12);
	graphic6.DrawRectangle(&pen3, 8, 12, 12, 12);
	Gdiplus::SolidBrush gdiplus_brush(Gdiplus::Color(30, 30, 30));
	graphic5.FillRectangle(&gdiplus_brush, 9, 13, 10, 10);
	Gdiplus::SolidBrush gdiplus_brush_grey(Gdiplus::Color(60, 60, 60));
	graphic6.FillRectangle(&gdiplus_brush_grey, 9, 13, 10, 10);

	m_Min.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 32, 32);
	m_Min.SelectObject(pBitmap);
	m_MinHover.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 32, 32);
	m_MinHover.SelectObject(pBitmap);
	m_Min.FillRect(&rect, pGreyBlackBrush);
	m_MinHover.FillRect(&rect, &brush_grey);
	Gdiplus::Graphics graphic7(m_Min);
	Gdiplus::Graphics graphic8(m_MinHover);
	Gdiplus::Pen pen4 = Gdiplus::Pen(Gdiplus::Color(214, 214, 214), 2.0);
	graphic7.DrawLine(&pen4, Gdiplus::Point(8, 15), Gdiplus::Point(23, 15));
	graphic8.DrawLine(&pen4, Gdiplus::Point(8, 15), Gdiplus::Point(23, 15));


	// 创建提示文本
	m_Tip.Create(this, TTS_ALWAYSTIP);
	m_Tip.SetTipBkColor(RGB(30, 30, 30));
	m_Tip.SetTipTextColor(RGB(255, 255, 255));
	m_Tip.SetMaxTipWidth(400);
	CRect pRect = new CRect(5, 5, 5, 5);
	m_Tip.SetMargin(pRect);

	// 创建子组件
	m_ControlPanel.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(), this, NULL);
	m_TagPanel.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(), this, NULL);
	m_Editor.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(), this, NULL);
	m_InfoView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 200), this, NULL);
	m_StatusBar.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(), this, NULL);
	m_ControlPanel.REDRAW_WINDOW();

	// 再创建窗口
	if (CFrameWndEx::OnCreate(lpCreateStruct))
		return -1;

	ModifyStyleEx(WS_EX_CLIENTEDGE, NULL, SWP_FRAMECHANGED);
	ModifyStyleEx(WS_EX_DLGMODALFRAME, NULL, SWP_FRAMECHANGED);
	ModifyStyleEx(WS_EX_WINDOWEDGE, NULL, SWP_FRAMECHANGED);
	LoadAccelTable(MAKEINTRESOURCE(IDR_MAINFRAME));
	SetIcon(AfxGetApp()->LoadIconW(MAKEINTRESOURCE(IDI_PI)), true);
	SetIcon(AfxGetApp()->LoadIconW(MAKEINTRESOURCE(IDI_PI)), false);

	return 0;
}
void CMainFrame::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	CFrameWndEx::OnNcCalcSize(bCalcValidRects, lpncsp);
	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);
	if (wp.showCmd == SW_MAXIMIZE) {
		lpncsp->rgrc[0].top -= 23;
	}
	else {
		lpncsp->rgrc[0].top -= 30;
	}
}
LRESULT CMainFrame::OnNcHitTest(CPoint point)
{
	CRect rect;
	GetWindowRect(&rect);
	point.Offset(-rect.left - 8, -rect.top - 8);
	CRect rect_close(m_Width - 42, 10, m_Width - 10, 42);
	CRect rect_max_restore(m_Width - 84, 10, m_Width - 52, 42);
	CRect rect_min(m_Width - 126, 10, m_Width - 94, 42);
	if (rect_close.PtInRect(point)) {
		return HTCLOSE;
	}
	else if (rect_max_restore.PtInRect(point)) {
		return HTMAXBUTTON;
	}
	else if (rect_min.PtInRect(point)) {
		return HTMINBUTTON;
	}
	SHORT value = (point.x <= 2 ? -1 : (point.x >= (m_Width - 3) ? 1 : 0)) +
		(point.y <= 2 ? (- 1 << 4) : (point.y >= (m_Height - 3) ? (1 << 4) : 0));
	switch (value) {
	case 1:
		return HTRIGHT;
	case -1:
		return HTLEFT;
	case 1 << 4:
		return HTBOTTOM;
	case -1 << 4:
		return HTTOP;
	case 1 + (1 << 4):
		return HTBOTTOMRIGHT;
	case -1 + (1 << 4):
		return HTBOTTOMLEFT;
	case 1 + (-1 << 4):
		return HTTOPRIGHT;
	case -1 + (-1 << 4):
		return HTTOPLEFT;
	default:
		if (point.y > 0 and point.y < 50) {
			return HTCAPTION;
		}
		else {
			return NULL;
		}
	}
}
void CMainFrame::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	m_bHover[0] = m_bHover[1] = m_bHover[2] = false;
	switch (nHitTest) {
	case HTMINBUTTON:
		m_bHover[0] = true;
		break;
	case HTMAXBUTTON:
		m_bHover[1] = true;
		break;
	case HTCLOSE:
		m_bHover[2] = true;
		break;
	default:
		CFrameWndEx::OnNcMouseMove(nHitTest, point);
		break;
	}
	CRect rect(m_Width - 126, 0, m_Width, 50);
	InvalidateRect(&rect, FALSE);
}
void CMainFrame::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	if (nHitTest != HTCLOSE and nHitTest != HTMAXBUTTON and nHitTest != HTMINBUTTON) {
		CFrameWndEx::OnNcLButtonDown(nHitTest, point);
	}
}
void CMainFrame::OnNcLButtonUp(UINT nHitTest, CPoint point)
{
	switch (nHitTest) {
	case HTMINBUTTON:
		m_bHover[0] = false;
		ShowWindow(SW_MINIMIZE);
		break;
	case HTMAXBUTTON:
		m_bHover[1] = false;
		WINDOWPLACEMENT wp;
		GetWindowPlacement(&wp);
		if (wp.showCmd == SW_MAXIMIZE) {
			ShowWindow(SW_RESTORE);
		}
		else {
			ShowWindow(SW_MAXIMIZE);
		}
		break;
	case HTCLOSE:
		DestroyWindow();
		return;
	}
	CRect rect(m_Width - 126, 0, m_Width, 50);
	InvalidateRect(&rect, FALSE);
}
BOOL CMainFrame::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CMainFrame::OnPaint()
{
	CPaintDC dc(this);
	CRect rect(0, 0, m_Width, 50);
	MemoryDC.FillRect(&rect, pGreyBlackBrush);
	MemoryDC.BitBlt(10, 10, 32, 32, &m_Icon, 0, 0, SRCCOPY);
	if (m_bHover[2]) {
		MemoryDC.BitBlt(m_Width - 42, 10, 32, 32, &m_CloseHover, 0, 0, SRCCOPY);
	}
	else {
		MemoryDC.BitBlt(m_Width - 42, 10, 32, 32, &m_Close, 0, 0, SRCCOPY);
	}
	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);
	if (wp.showCmd == SW_MAXIMIZE) {
		if (m_bHover[1]) {
			MemoryDC.BitBlt(m_Width - 84, 10, 32, 32, &m_RestoreHover, 0, 0, SRCCOPY);
		}
		else {
			MemoryDC.BitBlt(m_Width - 84, 10, 32, 32, &m_Restore, 0, 0, SRCCOPY);
		}
	}
	else {
		if (m_bHover[1]) {
			MemoryDC.BitBlt(m_Width - 84, 10, 32, 32, &m_MaxHover, 0, 0, SRCCOPY);
		}
		else {
			MemoryDC.BitBlt(m_Width - 84, 10, 32, 32, &m_Max, 0, 0, SRCCOPY);
		}
	}
	if (m_bHover[0]) {
		MemoryDC.BitBlt(m_Width - 126, 10, 32, 32, &m_MinHover, 0, 0, SRCCOPY);
	}
	else {
		MemoryDC.BitBlt(m_Width - 126, 10, 32, 32, &m_Min, 0, 0, SRCCOPY);
	}
	dc.BitBlt(0, 0, m_Width, 50, &MemoryDC, 0, 0, SRCCOPY);
}
void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWndEx::OnSize(nType, cx, cy);
	m_Width = cx;
	m_Height = cy;

	// 创建新的内存位图
	DeleteObject(hBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, cx, cy);
	SelectObject(MemoryDC, hBitmap);

	CRect rect;
	m_InfoView.GetClientRect(&rect);
	int original_height = rect.bottom - rect.top;
	rect = CRect(0, 50, cx, 150);
	m_ControlPanel.MoveWindow(rect);
	rect = CRect(0, 150, 300, cy - 24);
	m_TagPanel.MoveWindow(rect);
	rect = CRect(300, 150, cx, cy - 24 - original_height);
	m_Editor.MoveWindow(rect);
	rect = CRect(300, 150 + cy - 174 - original_height, cx, cy - 24);
	m_InfoView.MoveWindow(rect);
	rect = CRect(0, cy - 24, cx, cy);
	m_StatusBar.MoveWindow(rect);
}
void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize = POINT(800, 500);
}
void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
	UINT count = DragQueryFileW(hDropInfo, (UINT)-1, NULL, NULL);
	for (UINT index = 0; index != count; index++) {
		UINT size = DragQueryFileW(hDropInfo, index, NULL, NULL) + 1;
		wchar_t* path = new wchar_t[size];
		DragQueryFileW(hDropInfo, index, path, size);
		m_TagPanel.OpenFile(path);
	}
}
void CMainFrame::OnClose()
{
	if (CConsole::pObject->m_bRun) {
		int result = AfxMessageBox(L"代码还在运行，是否退出？", MB_ICONWARNING | MB_YESNO);
		if (result == IDNO) {
			return;
		}
		else {
			CConsole::pObject->OnDebugHalt();
		}
	}
	CWnd::OnClose();
}
void CMainFrame::OnDispatchCommand(UINT uID)
{
	switch (uID) {
		DISPATCH_CASE(ID_FILE_NEW, CTagPanel, OnNew)
		DISPATCH_CASE(ID_FILE_OPEN, CTagPanel, OnOpen)
		DISPATCH_CASE(ID_FILE_SAVE, CTagPanel, OnSave)
		DISPATCH_CASE(ID_FILE_SAVEAS, CTagPanel, OnSaveAs)
		DISPATCH_CASE(ID_EDIT_UNDO, CEditor, OnUndo)
		DISPATCH_CASE(ID_EDIT_REDO, CEditor, OnRedo)
		DISPATCH_CASE(ID_EDIT_COPY, CEditor, OnCopy)
		DISPATCH_CASE(ID_EDIT_CUT, CEditor, OnCut)
		DISPATCH_CASE(ID_EDIT_PASTE, CEditor, OnPaste)
		DISPATCH_CASE(ID_EDIT_LEFTARROW, CEditor, OnLeftarrow)
		DISPATCH_CASE(ID_DEBUG_RUN, CConsole, OnDebugRun)
		DISPATCH_CASE(ID_DEBUG_PAUSE, CConsole, OnDebugPause)
		DISPATCH_CASE(ID_DEBUG_HALT, CConsole, OnDebugHalt)
		DISPATCH_CASE(ID_DEBUG_DEBUG, CConsole, OnDebugDebug)
		DISPATCH_CASE(ID_DEBUG_CONTINUE, CConsole, OnDebugContinue)
		DISPATCH_CASE(ID_DEBUG_STEPIN, CConsole, OnDebugStepin)
		DISPATCH_CASE(ID_DEBUG_STEPOVER, CConsole, OnDebugStepover)
		DISPATCH_CASE(ID_DEBUG_STEPOUT, CConsole, OnDebugStepout)
		DISPATCH_CASE(ID_TOOLS_REFERENCE, CMainFrame, OnReference)
	}
}
void CMainFrame::OnReference()
{
	ShellExecuteW(NULL, L"open", L"\"Pseudocode Interpreter Reference\\index.html\"", nullptr, nullptr, SW_SHOWMAXIMIZED);
}
void CMainFrame::UpdateStatus(bool state, wchar_t* text)
{
	m_StatusBar.UpdateState(state);
	m_StatusBar.UpdateMessage(text);
}
