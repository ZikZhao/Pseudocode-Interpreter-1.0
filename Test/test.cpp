#include "test.h"

CBrush* pGreyBlackBrush = new CBrush(RGB(30, 30, 30));
namespace CConsole {
	CFont font;
	CBrush selectionColor;
}

BEGIN_MESSAGE_MAP(CTestWnd, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_NCCALCSIZE()
END_MESSAGE_MAP()
CTestWnd::CTestWnd() noexcept
{
}
CTestWnd::~CTestWnd()
{
}
int CTestWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}
void CTestWnd::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);
	CRect rect(0, 0, cx, cy);
	//m_Component.MoveWindow(&rect);
}

App::App() noexcept
{
	AddFontResourceW(L"YAHEI CONSOLAS HYBRID.TTF");
	CConsole::font.CreateFontW(24, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"YAHEI CONSOLAS HYBRID");
	CConsole::selectionColor.CreateSolidBrush(RGB(38, 79, 120));
}
App::~App()
{
}
BOOL App::InitInstance()
{
	CWinAppEx::InitInstance();
	m_pMainWnd = (CFrameWnd*)new CTestWnd;
	m_pMainWnd->CreateEx(NULL, NULL, L"Test", WS_OVERLAPPEDWINDOW, CRect(0, 0, 400, 400), NULL, NULL);
	m_pMainWnd->ShowWindow(SW_NORMAL);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

App theApp;

void CTestWnd::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	
	CFrameWnd::OnNcCalcSize(bCalcValidRects, lpncsp);
	lpncsp->rgrc[0].left -= 2;
	lpncsp->rgrc[0].right += 2;
	lpncsp->rgrc[0].bottom += 2;
}
