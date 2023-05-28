#include "pch.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(App, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &App::OnAppAbout)
	// 基于文件的标准文档命令
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
END_MESSAGE_MAP()
App::App() noexcept
{
	SetAppID(_T("PseudocodeInterpreter.IDE.1.0"));
	SCREEN_WIDTH = GetSystemMetrics(SM_CXFULLSCREEN);
	SCREEN_HEIGHT = GetSystemMetrics(SM_CYFULLSCREEN);
	m_GdiplusToken = 0;
}
BOOL App::InitInstance()
{
	CWinAppEx::InitInstance();

	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	EnableTaskbarInteraction(FALSE);

	SetRegistryKey(_T("Pseudocode Interpreter"));

	InitTooltipManager();

	Gdiplus::GdiplusStartupInput si{};
	Gdiplus::GdiplusStartup(&m_GdiplusToken, &si, NULL);

	m_pMainWnd = new CMainFrame;
	m_pMainWnd->CreateEx(NULL, NULL, L"Pseudocode Interpreter", WS_OVERLAPPEDWINDOW, CRect(), nullptr, NULL);
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	m_pMainWnd->DragAcceptFiles();
	return TRUE;
}
int App::ExitInstance()
{
	AfxOleTerm(FALSE);
	Gdiplus::GdiplusShutdown(m_GdiplusToken);

	return CWinAppEx::ExitInstance();
}
void App::PreLoadState()
{

}
void App::LoadCustomState()
{
}
void App::SaveCustomState()
{
}
void App::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()
CAboutDlg::CAboutDlg() noexcept : CDialogEx(IDD_ABOUTBOX)
{
}
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

App theApp;