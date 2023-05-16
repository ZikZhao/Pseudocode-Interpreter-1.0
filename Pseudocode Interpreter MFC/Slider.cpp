#include "pch.h"

inline void CSlider::SetPercentage(double percentage) {
	m_Percentage = max(min(percentage, 1.0 - m_Ratio), 0);
	REDRAW_WINDOW();
	if (m_pCallback) { m_pCallback(m_Percentage); }
}
void CSlider::DeflateLength(ULONG original, ULONG current)
{
	m_Length = original;
	m_DeflatedLength = current;
}
void CSlider::SetCallback(void(*callback)(double))
{
	m_pCallback = callback;
}
void CSlider::SetDeflateCallback(void(*callback)(ULONG))
{
	m_pDeflateCallback = callback;
}
void CSlider::SetRatio(double ratio)
{
	ratio = min(ratio, 1.0);
	if (ratio + m_Percentage > 1.0) {
		// 更改比例防止溢出
		SetPercentage(1.0 - ratio);
	}
	else if (m_Percentage != 0.0) {
		// 更改比例以匹配原本比例
		double original = m_Percentage / m_Ratio;
		SetPercentage(original * ratio);
	}
	m_Ratio = ratio;
}

BEGIN_MESSAGE_MAP(CHSlider, CSlider)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()
CHSlider::CHSlider()
{
	m_bHoverSlider = false;
	m_bDragSlider = false;
	m_bHover = false;
	m_Percentage = 0.0;
	m_pCallback = nullptr;
	m_pDeflateCallback = nullptr;
	m_Length = 0;
	m_DeflatedLength = 0;
	m_Ratio = 1.0;
}
CHSlider::~CHSlider()
{
	m_Pen.DeleteObject();
	m_SliderColor.DeleteObject();
	m_BgColor.DeleteObject();
}
int CHSlider::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 准备工具
	m_Pen.CreatePen(PS_NULL, 0, RGB(0, 0, 0));
	m_SliderColor.CreateSolidBrush(RGB(220, 220, 220));
	m_BgColor.CreateSolidBrush(RGB(46, 46, 46));

	// 准备源
	m_Bg.CreateCompatibleDC(pWindowDC);
	m_Source.CreateCompatibleDC(pWindowDC);
	m_Hover.CreateCompatibleDC(pWindowDC);
	m_Bg.SelectObject(&m_Pen);
	m_Source.SelectObject(&m_Pen);
	m_Hover.SelectObject(&m_Pen);
	m_Bg.SelectObject(&m_BgColor);
	m_Source.SelectObject(&m_SliderColor);
	m_Hover.SelectObject(pThemeColorBrush);

	return 0;
}
BOOL CHSlider::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CHSlider::OnPaint()
{
	CPaintDC dc(this);
	MemoryDC.BitBlt(0, 0, m_Height, 10, &m_Bg, 0, 0, SRCCOPY);
	// 使滑动条最小为一个圆点
	double minimum_ratio = (double)m_SliderHeight / m_Height;
	double adjusted_percentage = min(1.0 - minimum_ratio, m_Percentage);
	if (m_bHoverSlider) {
		MemoryDC.TransparentBlt(m_Height * adjusted_percentage, 0, m_SliderHeight, 10, &m_Hover, 0, 0, m_SliderHeight, 10, 0);
	}
	else {
		MemoryDC.TransparentBlt(m_Height * adjusted_percentage, 0, m_SliderHeight, 10, &m_Source, 0, 0, m_SliderHeight, 10, 0);
	}
	dc.BitBlt(0, 0, m_Height, 10, &MemoryDC, 0, 0, SRCCOPY);
}
void CHSlider::OnSize(UINT nType, int cx, int cy)
{
	m_Height = cx;

	HBITMAP hBitmap = CreateCompatibleBitmap(*pWindowDC, m_Height, 10);
	HGLOBAL hOldBitmap = SelectObject(m_Bg, hBitmap);
	DeleteObject(hOldBitmap);
	CRect rect(0, 0, m_Height, 10);
	m_Bg.FillRect(&rect, pGreyBlackBrush);
	m_Bg.RoundRect(&rect, CPoint(10, 10));
}
void CHSlider::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_DeflatedLength) {
		// 保证位置不变
		double part_length = m_Ratio * m_Length;
		m_pDeflateCallback(m_DeflatedLength);
		SetRatio(part_length / m_DeflatedLength);
		m_DeflatedLength = m_Length = 0;
	}
	if (m_bHoverSlider) {
		m_DragPosition = (double)point.x / m_Height - m_Percentage;
		m_bDragSlider = true;
		SetCapture();
	}
	else {
		double hit_percentage = (double)point.x / m_Height - m_Ratio / 2;
		if (hit_percentage > m_Ratio) {
			SetPercentage(min(m_Percentage + m_Ratio, 1.0 - m_Ratio));
		}
		else {
			SetPercentage(max(m_Percentage - m_Ratio, 0.0));
		}
	}
}
void CHSlider::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDragSlider) {
		ReleaseCapture();
		m_bDragSlider = false;
	}
	bool previous_state = m_bHoverSlider;
	// 计算最小滚动条信息
	double adjusted_ratio = max(m_Ratio, 10.0 / m_Height);
	double adjusted_percentage = min(m_Percentage, 1.0 - adjusted_ratio);
	// 检查是否悬浮于滚动条按钮
	m_bHoverSlider = point.x > adjusted_percentage * m_Height and point.x < (adjusted_percentage + adjusted_ratio) * m_Height and point.y > 0 and point.y < 10;
	if (previous_state != m_bHoverSlider) {
		REDRAW_WINDOW();
	}
}
void CHSlider::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDragSlider) {
		double hit_percentage = (double)point.x / m_Height;
		double raw_percentage = hit_percentage - m_DragPosition;
		SetPercentage(min(1.0 - m_Ratio, max(raw_percentage, 0.0)));
	}
	else {
		bool previous_state = m_bHoverSlider;
		// 计算最小滚动条信息
		double adjusted_ratio = max(m_Ratio, 10.0 / m_Height);
		double adjusted_percentage = min(m_Percentage, 1.0 - adjusted_ratio);
		// 检查是否悬浮于滚动条按钮
		m_bHoverSlider = point.x > adjusted_percentage * m_Height and point.x < (adjusted_percentage + adjusted_ratio) * m_Height;
		if (previous_state != m_bHoverSlider) {
			REDRAW_WINDOW();
		}
		TRACKMOUSEEVENT tme{};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		TrackMouseEvent(&tme);
		m_bHover = true;
	}
}
void CHSlider::OnMouseLeave()
{
	m_bHover = false;
	m_bHoverSlider = false;
	REDRAW_WINDOW();
}
BOOL CHSlider::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	SetCursor(LoadCursorW(NULL, IDC_ARROW));
	return TRUE;
}
void CHSlider::SetRatio(double ratio)
{
	CSlider::SetRatio(ratio);

	m_SliderHeight = (USHORT)max(m_Height * m_Ratio, 10); // 按钮最小是个圆
	HBITMAP hBitmap = CreateCompatibleBitmap(*pWindowDC, m_SliderHeight, 10);
	HGLOBAL hOldBitmap = SelectObject(m_Source, hBitmap);
	DeleteObject(hOldBitmap);
	hBitmap = CreateCompatibleBitmap(*pWindowDC, m_SliderHeight, 10);
	hOldBitmap = SelectObject(m_Hover, hBitmap);
	DeleteObject(hOldBitmap);
	CRect rect(0, 0, m_SliderHeight, 10);
	m_Source.RoundRect(&rect, CPoint(10, 10));
	m_Hover.RoundRect(&rect, CPoint(10, 10));
}

BEGIN_MESSAGE_MAP(CVSlider, CSlider)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()
CVSlider::CVSlider()
{
	m_bHoverSlider = false;
	m_bDragSlider = false;
	m_bHover = false;
	m_Percentage = 0.0;
	m_pCallback = nullptr;
	m_pDeflateCallback = nullptr;
	m_Length = 0;
	m_DeflatedLength = 0;
	m_Ratio = 1.0;
}
CVSlider::~CVSlider()
{
	m_Pen.DeleteObject();
	m_SliderColor.DeleteObject();
	m_BgColor.DeleteObject();
}
int CVSlider::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 准备工具
	m_Pen.CreatePen(PS_NULL, 0, RGB(0, 0, 0));
	m_SliderColor.CreateSolidBrush(RGB(220, 220, 220));
	m_BgColor.CreateSolidBrush(RGB(46, 46, 46));

	// 准备源
	m_Bg.CreateCompatibleDC(pWindowDC);
	m_Source.CreateCompatibleDC(pWindowDC);
	m_Hover.CreateCompatibleDC(pWindowDC);
	m_Bg.SelectObject(&m_Pen);
	m_Source.SelectObject(&m_Pen);
	m_Hover.SelectObject(&m_Pen);
	m_Bg.SelectObject(&m_BgColor);
	m_Source.SelectObject(&m_SliderColor);
	m_Hover.SelectObject(pThemeColorBrush);

	return 0;
}
BOOL CVSlider::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CVSlider::OnPaint()
{
	CPaintDC dc(this);
	MemoryDC.BitBlt(0, 0, 10, m_Height, &m_Bg, 0, 0, SRCCOPY);
	// 使滑动条最小为一个圆点
	double minimum_ratio = (double)m_SliderHeight / m_Height;
	double adjusted_percentage = min(1.0 - minimum_ratio, m_Percentage);
	if (m_bHoverSlider) {
		MemoryDC.TransparentBlt(0, m_Height * adjusted_percentage, 10, m_SliderHeight, &m_Hover, 0, 0, 10, m_SliderHeight, 0);
	}
	else {
		MemoryDC.TransparentBlt(0, m_Height * adjusted_percentage, 10, m_SliderHeight, &m_Source, 0, 0, 10, m_SliderHeight, 0);
	}
	dc.BitBlt(0, 0, 10, m_Height, &MemoryDC, 0, 0, SRCCOPY);
}
void CVSlider::OnSize(UINT nType, int cx, int cy)
{
	m_Height = cy;

	HBITMAP hBitmap = CreateCompatibleBitmap(*pWindowDC, 10, m_Height);
	HGLOBAL hOldBitmap = SelectObject(m_Bg, hBitmap);
	DeleteObject(hOldBitmap);
	CRect rect(0, 0, 10, m_Height);
	m_Bg.FillRect(&rect, pGreyBlackBrush);
	m_Bg.RoundRect(&rect, CPoint(10, 10));
	SetRatio(m_Ratio);
}
void CVSlider::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_bHoverSlider) {
		m_DragPosition = (double)point.y / m_Height - m_Percentage;
		m_bDragSlider = true;
		SetCapture();
	}
	else {
		double hit_percentage = (double)point.y / m_Height - m_Ratio / 2;
		if (hit_percentage > m_Ratio) {
			SetPercentage(min(m_Percentage + m_Ratio, 1.0 - m_Ratio));
		}
		else {
			SetPercentage(max(m_Percentage - m_Ratio, 0.0));
		}
	}
}
void CVSlider::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDragSlider) {
		ReleaseCapture();
		m_bDragSlider = false;
	}
	bool previous_state = m_bHoverSlider;
	// 计算最小滚动条信息
	double adjusted_ratio = max(m_Ratio, 10.0 / m_Height);
	double adjusted_percentage = min(m_Percentage, 1.0 - adjusted_ratio);
	// 检查是否悬浮于滚动条按钮
	m_bHoverSlider = point.x > 0 and point.x < 10 and point.y > adjusted_percentage * m_Height and point.y < (adjusted_percentage + adjusted_ratio) * m_Height;
	if (previous_state != m_bHoverSlider) {
		REDRAW_WINDOW();
	}
}
void CVSlider::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDragSlider) {
		double hit_percentage = (double)point.y / m_Height;
		double raw_percentage = hit_percentage - m_DragPosition;
		SetPercentage(min(1.0 - m_Ratio, max(raw_percentage, 0.0)));
	}
	else {
		bool previous_state = m_bHoverSlider;
		// 计算最小滚动条信息
		double adjusted_ratio = max(m_Ratio, 10.0 / m_Height);
		double adjusted_percentage = min(m_Percentage, 1.0 - adjusted_ratio);
		// 检查是否悬浮于滚动条按钮
		m_bHoverSlider = point.y > adjusted_percentage * m_Height and point.y < (adjusted_percentage + adjusted_ratio) * m_Height;
		if (previous_state != m_bHoverSlider) {
			REDRAW_WINDOW();
		}
		TRACKMOUSEEVENT tme{};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		TrackMouseEvent(&tme);
		m_bHover = true;
	}
}
void CVSlider::OnMouseLeave()
{
	m_bHover = false;
	m_bHoverSlider = false;
	REDRAW_WINDOW();
}
BOOL CVSlider::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	SetCursor(LoadCursorW(NULL, IDC_ARROW));
	return TRUE;
}
void CVSlider::SetRatio(double ratio)
{
	CSlider::SetRatio(ratio);
	
	m_SliderHeight = (USHORT)max(m_Height * m_Ratio, 10); // 按钮最小是个圆
	HBITMAP hBitmap = CreateCompatibleBitmap(*pWindowDC, 10, m_SliderHeight);
	HGLOBAL hOldBitmap = SelectObject(m_Source, hBitmap);
	DeleteObject(hOldBitmap);
	hBitmap = CreateCompatibleBitmap(*pWindowDC, 10, m_SliderHeight);
	hOldBitmap = SelectObject(m_Hover, hBitmap);
	DeleteObject(hOldBitmap);
	CRect rect(0, 0, 10, m_SliderHeight);
	m_Source.RoundRect(&rect, CPoint(10, 10));
	m_Hover.RoundRect(&rect, CPoint(10, 10));
	REDRAW_WINDOW();
}
