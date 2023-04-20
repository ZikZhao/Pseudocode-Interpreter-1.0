#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int SCREEN_WIDTH = 0;
int SCREEN_HEIGHT = 0;


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

	SetRegistryKey(_T("PseudocodeInterpreter"));
	LoadStdProfileSettings(4);  // 加载标准 INI 文件选项(包括 MRU)

	InitContextMenuManager();
	InitKeyboardManager();
	InitTooltipManager();

	m_pMainWnd = new CMainFrame;
	m_pMainWnd->CreateEx(NULL, NULL, L"Pseudocode Interpreter", WS_OVERLAPPEDWINDOW, CRect(0, 0, 0, 0), nullptr, NULL);
	m_pMainWnd->ShowWindow(m_nCmdShow);
	m_pMainWnd->UpdateWindow();
	m_pMainWnd->DragAcceptFiles();
	((CMainFrame*)m_pMainWnd)->LoadOpenedFiles();
	return TRUE;
}
int App::ExitInstance()
{
	//TODO: 处理可能已添加的附加资源
	AfxOleTerm(FALSE);

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