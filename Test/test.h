#pragma once
#include "afxwinappex.h"
#include <list>
#include <mutex>

class CTestWnd : public CFrameWnd {
	DECLARE_MESSAGE_MAP()
protected:
	CDC dc;
public:
	CTestWnd() noexcept;
	~CTestWnd();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
};

class App : public CWinAppEx {
protected:

public:
	App() noexcept;
	~App();
	virtual BOOL InitInstance();
};
