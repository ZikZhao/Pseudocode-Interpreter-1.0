#pragma once

static class CSlider : public CWnd
{
protected:
	CDC m_Bg;
	CDC m_Source;
	CDC m_Hover;
	USHORT m_Height, m_SliderHeight; // 窗口高度，滚动条长度
	ULONG m_Length; // 削减前长度，仅在削减时使用
	ULONG m_DeflatedLength; // 缩减后长度，通常在再次点击按钮时应用
	double m_Percentage; // 展示范围起点
	double m_Ratio; // 展示区域长度/总长度
	double m_DragPosition; // 拖拽开始时鼠标相对于按钮左上角的比例
	void (*m_pCallback)(double); // 回调函数
	void (*m_pDeflateCallback)(ULONG); // 长度削减生效时的回调函数
	bool m_bHoverSlider; // 悬浮于按钮
	bool m_bDragSlider; // 正在拖拽滚动条
	bool m_bHover; // 悬浮于整个组件
	CPen m_Pen; // 无边框
	CBrush m_SliderColor; // 滚动条颜色
	CBrush m_BgColor; // 滚动条背景颜色
public:
	void SetPercentage(double percentage); // 设置相对位置
	void DeflateLength(ULONG original, ULONG current); // 削减长度（延迟生效）
	void SetCallback(void(*callback)(double)); // 设置回调函数
	void SetDeflateCallback(void(*callback)(ULONG)); // 设置削减长度的回调函数
	void SetRatio(double ratio); // 设置比例
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
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	void SetRatio(double ratio); // 设置比例
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
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	void SetRatio(double ratio); // 设置比例
};