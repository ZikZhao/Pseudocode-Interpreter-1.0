#pragma once

static class CSlider : public CWnd
{
protected:
	CDC m_MemoryDC;
	CDC m_Bg;
	CDC m_Source;
	CDC m_Hover;
	USHORT m_Height, m_SliderHeight; // 窗口高度，滚动条长度
	double m_Percentage; // 展示范围起点
	double m_Ratio; // 展示区域长度/总长度
	double m_DragPosition; // 拖拽开始时鼠标相对于按钮左上角的比例
	void (*m_pCallback)(double); // 回调函数
	bool m_bHoverSlider; // 悬浮于按钮
	bool m_bDragSlider; // 正在拖拽滚动条
	bool m_bHover; // 悬浮于整个组件
	CPen m_Pen;
	CBrush m_SliderColor;
	CBrush m_BgColor;
public:
	void SetPercentage(double percentage);
};

class CHSlider : public CSlider {
	DECLARE_MESSAGE_MAP()
public:
	CHSlider();
	~CHSlider();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	void SetCallback(void(*callback)(double));
	void SetRatio(double ratio);
};

class CVSlider : public CSlider {
	DECLARE_MESSAGE_MAP()
public:
	CVSlider();
	~CVSlider();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	void SetCallback(void(*callback)(double));
	void SetRatio(double ratio);
};