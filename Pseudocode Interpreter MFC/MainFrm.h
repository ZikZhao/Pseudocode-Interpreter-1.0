#pragma once
#include "ControlPanel.h"
#include "TagPanel.h"
#include "Editor.h"
#include "InfoView.h"
#include "StatusBar.h"

class CMainFrame : public CFrameWndEx
{
	DECLARE_DYNCREATE(CMainFrame)
	DECLARE_MESSAGE_MAP()
public:
	static inline CMainFrame* pObject = nullptr;
protected:
	CMainFrame() noexcept;
	virtual ~CMainFrame();
	CControlPanel m_ControlPanel;
	CTagPanel m_TagPanel;
	CEditor m_Editor;
	CInfoView m_InfoView;
	CCustomStatusBar m_StatusBar;
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	void OpenFile(const wchar_t* path); // 打开文件
	void LoadOpenedFiles(); // 打开上次未关闭的文件
	void UpdateStatus(bool state, const wchar_t* text); // 用于更新状态栏来指示子进程状态
};