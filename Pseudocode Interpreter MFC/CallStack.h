#pragma once

class CCallStack : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CCallStack* pObject = nullptr;
protected:
	bool m_bShow;
	int m_Width, m_Height;
	CSize m_WordSize;
	int m_FullHeight;
	double m_Percentage;
	CALLSTACK* m_CallStack;
	CPen m_Pen;
	CFont m_Font;
	CVSlider m_Slider;
	ULONGLONG m_CII; // 主程序栈
public:
	CCallStack();
	virtual ~CCallStack();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void LoadCallStack(CALLSTACK* callstack);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	void UpdateSlider();
};
