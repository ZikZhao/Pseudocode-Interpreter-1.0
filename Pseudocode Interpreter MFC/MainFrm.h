#pragma once
#include "ControlPanel.h"
#include "TagPanel.h"
#include "Editor.h"
#include "InfoView.h"
#include "StatusBar.h"
#define REDRAW_WINDOW() RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW)
#define WM_STEP WM_USER + 1 // 单步执行消息

extern CDC MemoryDC;
extern CDC ScreenDC;
extern CBrush* pThemeColorBrush;
extern CBrush* pGreyBlackBrush;
extern CPen* pNullPen;
extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;

class CMainFrame : public CFrameWndEx
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CMainFrame* pObject = nullptr;
	static inline HBITMAP hBitmap = NULL; // 内存DC位图
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
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnClose();
	afx_msg void OnDispatchCommand(UINT uID); // 命令消息分发（因为执行函数需要this指针）
	// 命令
	afx_msg void OnReference();
	void UpdateStatus(bool state, wchar_t* text); // 用于更新状态栏来指示子进程状态
};
