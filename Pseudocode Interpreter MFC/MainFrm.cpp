#include "stdafx.h"
#define DISPATCH_CASE(id, cls, function) {case id: cls::pObject->function(); break;}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CBrush* pThemeColorBrush = new CBrush(RGB(254, 74, 99));
CBrush* pGreyBlackBrush = new CBrush(RGB(30, 30, 30));
CPen* pNullPen = new CPen(PS_NULL, 0, RGB(0, 0, 0));

// CMainFrame
BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_NCCALCSIZE()
	ON_COMMAND_RANGE(32768, 65535, CMainFrame::OnDispatchCommand)
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
	// 创建提示文本
	m_Tip.Create(this, TTS_ALWAYSTIP);
	m_Tip.SetTipBkColor(RGB(30, 30, 30));
	m_Tip.SetTipTextColor(RGB(255, 255, 255));
	static CRect rect(5, 5, 5, 5);
	m_Tip.SetMargin(&rect);

	// 创建子组件
	m_ControlPanel.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_TagPanel.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_Editor.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_InfoView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 200), this, NULL);
	m_StatusBar.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);

	// 再创建窗口
	if (CFrameWndEx::OnCreate(lpCreateStruct))
		return -1;

	ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);
	LoadAccelTable(MAKEINTRESOURCE(IDR_MAINFRAME));

	return 0;
}
BOOL CMainFrame::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWndEx::OnSize(nType, cx, cy);
	CRect rect;
	m_InfoView.GetClientRect(&rect);
	int original_height = rect.bottom - rect.top;
	rect = CRect(0, 0, cx, 150);
	m_ControlPanel.MoveWindow(rect, FALSE);
	rect = CRect(0, 150, 300, cy - 24);
	m_TagPanel.MoveWindow(rect, FALSE);
	rect = CRect(300, 150, cx, cy - 24 - original_height);
	m_Editor.MoveWindow(rect);
	rect = CRect(300, 150 + cy - 174 - original_height, cx, cy - 24);
	m_InfoView.MoveWindow(rect, FALSE);
	rect = CRect(0, cy - 24, cx, cy);
	m_StatusBar.MoveWindow(rect, FALSE);
}
void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize = POINT(300, 300);
}
void CMainFrame::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	CFrameWndEx::OnNcCalcSize(bCalcValidRects, lpncsp);
	lpncsp->rgrc[0].left -= 2;
	lpncsp->rgrc[0].right += 2;
	lpncsp->rgrc[0].bottom += 2;
}
void CMainFrame::OnDispatchCommand(UINT uID)
{
	switch (uID) {
		DISPATCH_CASE(ID_EDIT_UNDO, CEditor, OnUndo)
		DISPATCH_CASE(ID_EDIT_REDO, CEditor, OnRedo)
		DISPATCH_CASE(ID_DEBUG_RUN, CConsole, OnDebugRun)
		DISPATCH_CASE(ID_DEBUG_DEBUG, CConsole, OnDebugDebug)
		DISPATCH_CASE(ID_DEBUG_HALT, CConsole, OnDebugHalt)
	}
}
inline void CMainFrame::OpenFile(const wchar_t* path)
{
	m_TagPanel.NewTag(path);
	m_Editor.LoadFile(m_TagPanel.GetCurrentTag());
}
void CMainFrame::LoadOpenedFiles()
{
	OpenFile(const_cast<wchar_t*>(L"C:\\Users\\Zik\\OneDrive - 6666zik\\Desktop\\code.txt"));
}
void CMainFrame::UpdateStatus(bool state, const wchar_t* text)
{
	m_StatusBar.UpdateState(state);
	m_StatusBar.UpdateMessage(text);
}
