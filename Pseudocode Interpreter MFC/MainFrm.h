#pragma once
#include "ControlPanel.h"
#include "TagPanel.h"
#include "Editor.h"
#include "InfoView.h"
#include "StatusBar.h"
#define WM_STEP WM_USER + 1 // 单步执行消息

class CMainFrame : public CFrameWndEx
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CMainFrame* pObject = nullptr;
	static inline CToolTipCtrl m_Tip; // 提示文本
protected:
	CControlPanel m_ControlPanel;
	CTagPanel m_TagPanel;
	CEditor m_Editor;
	CInfoView m_InfoView;
	CCustomStatusBar m_StatusBar;
public:
	CMainFrame() noexcept;
	virtual ~CMainFrame();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnDispatchCommand(UINT uID); // 命令消息分发（因为执行函数需要this指针）
	void OpenFile(const wchar_t* path); // 打开文件
	void LoadOpenedFiles(); // 打开上次未关闭的文件
	void UpdateStatus(bool state, const wchar_t* text); // 用于更新状态栏来指示子进程状态
};
