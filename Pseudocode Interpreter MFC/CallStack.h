#pragma once

class CCallStack : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CCallStack* pObject = nullptr;
protected:
	CDC m_BG;
	CDC m_Source;
	CDC m_Selection;
	int m_Width, m_Height;
	CSize m_WordSize;
	ULONG m_FullHeight;
	USHORT m_FirstWidth, m_SecondWidth;
	double m_Percentage;
	CALLSTACK* m_CallStack;
	CPen m_Pen;
	CFont m_Font;
	CBrush m_Brush;
	CVSlider m_Slider;
	USHORT m_SelectionDepth;
public:
	CCallStack();
	virtual ~CCallStack();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	void LoadCallStack(CALLSTACK* callstack); // 加载调用堆栈
	static void VerticalCallback(double percentage); // 垂直滚动体回调函数
	BinaryTree* GetLastLocals(); // 返回最高层局部变量
protected:
	void UpdateSlider(); // 更新滚动条
	void ArrangeBG(); // 渲染背景源
	void ArrangeFrames(); // 渲染栈帧源
};
