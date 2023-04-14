#pragma once
#include "Console.h"

class CInfoViewTag : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CFont m_Font; // 字体
protected:
	int m_Width, m_Height; // 窗口大小
	CDC m_Source; // 渲染文字源
	CDC m_Hover; // 悬浮源
	CDC m_Selected; // 选中源
	bool m_bSelected; // 是否选择
	bool m_bHover; // 是否悬浮
	wchar_t* m_Text; // 按钮文本
	USHORT m_Index; // 按钮下标
public:
	CInfoViewTag() = delete;
	CInfoViewTag(wchar_t* text, USHORT index);
	~CInfoViewTag();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	void SetState(bool state);
};

class CInfoView : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CInfoView* pObject = nullptr;
protected:
	int m_Width, m_Height; // 窗口宽度/高度
	CSize m_CharSize; // 标签文字大小
	CPen m_SplitterPen; // 绘制分割线用的画笔
	USHORT m_CurrentIndex; // 当前标签下标
	CInfoViewTag* m_Tags[3];
	CConsole m_Console;
public:
	CInfoView();
	~CInfoView();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	void ShiftTag(USHORT index);
};
