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
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_DROPFILES()
	ON_WM_CLOSE()
	ON_COMMAND_RANGE(0, 65535, CMainFrame::OnDispatchCommand)
END_MESSAGE_MAP()
CMainFrame::CMainFrame() noexcept
{
	pObject = this;
}
CMainFrame::~CMainFrame()
{
}
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	return CFrameWndEx::PreCreateWindow(cs);
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

	// 创建提示文本
	m_Tip.Create(this, TTS_ALWAYSTIP);
	m_Tip.SetTipBkColor(RGB(30, 30, 30));
	m_Tip.SetTipTextColor(RGB(255, 255, 255));
	m_Tip.SetMaxTipWidth(400);
	static CRect rect(5, 5, 5, 5);
	m_Tip.SetMargin(&rect);

	// 创建子组件
	m_ControlPanel.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_TagPanel.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_Editor.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_InfoView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 200), this, NULL);
	m_StatusBar.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_ControlPanel.REDRAW_WINDOW();

	// 再创建窗口
	if (CFrameWndEx::OnCreate(lpCreateStruct))
		return -1;

	ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);
	LoadAccelTable(MAKEINTRESOURCE(IDR_MAINFRAME));
	SetIcon(AfxGetApp()->LoadIconW(MAKEINTRESOURCE(IDI_PI)), true);
	SetIcon(AfxGetApp()->LoadIconW(MAKEINTRESOURCE(IDI_PI)), false);

	return 0;
}
BOOL CMainFrame::OnEraseBkgnd(CDC* pDC)
{
	CRect rect(0, 0, 1920, 1080);
	pDC->FillRect(&rect, pGreyBlackBrush);
	return TRUE;
}
void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWndEx::OnSize(nType, cx, cy);

	// 创建新的内存位图
	DeleteObject(hBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, cx, cy);
	SelectObject(MemoryDC, hBitmap);

	CRect rect;
	m_InfoView.GetClientRect(&rect);
	int original_height = rect.bottom - rect.top;
	rect = CRect(0, 0, cx, 150);
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
	lpMMI->ptMinTrackSize = POINT(600, 300);
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
		DISPATCH_CASE(ID_DEBUG_HALT, CConsole, OnDebugHalt)
		DISPATCH_CASE(ID_DEBUG_DEBUG, CConsole, OnDebugDebug)
		DISPATCH_CASE(ID_DEBUG_CONTINUE, CConsole, OnDebugContinue)
		DISPATCH_CASE(ID_DEBUG_STEPIN, CConsole, OnDebugStepin)
		DISPATCH_CASE(ID_DEBUG_STEPOVER, CConsole, OnDebugStepover)
		DISPATCH_CASE(ID_DEBUG_STEPOUT, CConsole, OnDebugStepout)
	}
}
void CMainFrame::UpdateStatus(bool state, wchar_t* text)
{
	m_StatusBar.UpdateState(state);
	m_StatusBar.UpdateMessage(text);
}
