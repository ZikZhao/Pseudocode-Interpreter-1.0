#include "stdafx.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CBrush* pThemeColorBrush = new CBrush(RGB(254, 74, 99));
CBrush* pGreyBlackBrush = new CBrush(RGB(30, 30, 30));

namespace CALLBACKS {
	DWORD undo(LPVOID lpParameter) {
		return 0;
	}
	DWORD redo(LPVOID lpParameter) {
		return 0;
	}
	DWORD run_normal(LPVOID lpParameter) {
		((CControlPanel::pObject->GetGroups() + 2)->GetButtons() + 0)->SetState(false);
		((CControlPanel::pObject->GetGroups() + 2)->GetButtons() + 1)->SetState(false);
		STARTUPINFO si = CConsole::pObject->InitSubprocess(true);
		PROCESS_INFORMATION pi{};
		ZeroMemory(&pi, sizeof(pi));
		wchar_t* other_opt = new wchar_t[1];
		other_opt[0] = 0;
		size_t command_length = 33 + wcslen(CTagPanel::pObject->GetCurrentTag()->GetPath()) + wcslen(other_opt);
		wchar_t* command = new wchar_t[command_length] {};
		static const wchar_t* format = L"\"Pseudocode Interpreter.exe\" \"%s\" %s";
		StringCchPrintfW(command, command_length, format, CTagPanel::pObject->GetCurrentTag()->GetPath(), other_opt);
		command[command_length - 1] = 0;
		CreateProcessW(0, command, 0, 0, true, CREATE_NO_WINDOW, 0, 0, &si, &pi);
		static const wchar_t* start_message = L"本地伪代码解释器已启动";
		CMainFrame::pObject->UpdateStatus(true, start_message);
		delete[] command;
		// 监听管道
		CConsole::pObject->Join(pi.hProcess);
		// 结束执行
		CConsole::pObject->ExitSubprocess();
		((CControlPanel::pObject->GetGroups() + 2)->GetButtons() + 0)->SetState(true);
		((CControlPanel::pObject->GetGroups() + 2)->GetButtons() + 1)->SetState(true);
		static const wchar_t* end_message = L"代码运行结束";
		CMainFrame::pObject->UpdateStatus(false, end_message);
		return 0;
	}
	DWORD run_debug(LPVOID lpParameter) {
		((CControlPanel::pObject->GetGroups() + 2)->GetButtons() + 0)->SetState(false);
		((CControlPanel::pObject->GetGroups() + 2)->GetButtons() + 1)->SetState(false);
		STARTUPINFO si = CConsole::pObject->InitSubprocess(true);
		PROCESS_INFORMATION pi{};
		ZeroMemory(&pi, sizeof(pi));
		wchar_t* other_opt = new wchar_t[1];
		other_opt[0] = 0;
		size_t command_length = 40 + wcslen(CTagPanel::pObject->GetCurrentTag()->GetPath()) + wcslen(other_opt);
		wchar_t* command = new wchar_t[command_length] {};
		static const wchar_t* format = L"\"Pseudocode Interpreter.exe\" \"%s\" /debug %s";
		StringCchPrintfW(command, command_length, format, CTagPanel::pObject->GetCurrentTag()->GetPath(), other_opt);
		command[command_length - 1] = 0;
		CreateProcessW(0, command, 0, 0, true, CREATE_NO_WINDOW, 0, 0, &si, &pi);
		static const wchar_t* start_message = L"本地伪代码解释器已启动";
		CMainFrame::pObject->UpdateStatus(true, start_message);
		delete[] command;
		// 监听管道
		CConsole::pObject->JoinDebug(pi.hProcess);
		// 结束执行
		CConsole::pObject->ExitSubprocess();
		((CControlPanel::pObject->GetGroups() + 2)->GetButtons() + 0)->SetState(true);
		((CControlPanel::pObject->GetGroups() + 2)->GetButtons() + 1)->SetState(true);
		static const wchar_t* end_message = L"代码运行结束";
		CMainFrame::pObject->UpdateStatus(false, end_message);
		return 0;
	}
}
std::map<DWORD, DWORD(*)(LPVOID)> CALLBACK_MAP {
	{ IDB_UNDO, CALLBACKS::undo },
	{ IDB_REDO, CALLBACKS::redo },
	{ IDB_RUN_NORMAL, CALLBACKS::run_normal },
	{ IDB_RUN_DEBUG, CALLBACKS::run_debug },
};

// CMainFrame
IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)
BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()
CMainFrame::CMainFrame() noexcept
{
	pObject = this;
}
CMainFrame::~CMainFrame()
{
}
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// 创建子组件
	m_ControlPanel.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_TagPanel.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_Editor.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_InfoView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 200), this, NULL);
	m_StatusBar.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);

	// 再创建主窗口
	CFrameWndEx::OnCreate(lpCreateStruct);
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
inline void CMainFrame::OpenFile(const wchar_t* path)
{
	m_TagPanel.NewTag(path);
	m_Editor.LoadFile(m_TagPanel.GetCurrentTag());
}
void CMainFrame::LoadOpenedFiles()
{
	OpenFile(const_cast<wchar_t*>(L"C:\\Users\\Zik\\OneDrive - 6666zik\\Desktop\\code.txt"));
}
inline void CMainFrame::UpdateStatus(bool state, const wchar_t* text)
{
	m_StatusBar.UpdateState(state);
	m_StatusBar.UpdateMessage(text);
}
