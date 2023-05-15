#include "pch.h"

BEGIN_MESSAGE_MAP(CCallStack, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()
CCallStack::CCallStack()
{
	m_bShow = false;
	m_CallStack = nullptr;
	m_Width = 0;
	m_Height = 0;
	m_FullHeight = 0;
	m_Percentage = 0.0;
	pObject = this;
}
CCallStack::~CCallStack()
{
	if (m_CallStack) {
		delete m_CallStack;
	}
}
int CCallStack::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_Font.CreateFontW(18, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	m_Pen.CreatePen(PS_SOLID, 1, RGB(61, 61, 61));
	CDC temp;
	temp.CreateCompatibleDC(pWindowDC);
	temp.SelectObject(m_Font);
	GetTextExtentPoint32W(temp, L"名称", 2, &m_WordSize);
	m_WordSize.cy += 2;

	m_Slider.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(), this, NULL);

	return 0;
}
BOOL CCallStack::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CCallStack::OnPaint()
{
	CPaintDC dc(this);
	CRect rect(0, 0, m_Width, m_Height);
	MemoryDC.FillRect(&rect, pGreyBlackBrush);
	MemoryDC.SetTextAlign(TA_LEFT | TA_TOP);
	if (m_CallStack) {
		// 绘制栈帧名称和深度
		double start_index = m_Percentage * (m_CallStack->ptr + 1);
		int y_offset = ((int)start_index - start_index + 1) * m_WordSize.cy;
		if (m_CallStack->ptr != 0) {
			for (USHORT index = m_CallStack->ptr - 1;; index--) {
				MemoryDC.TextOutW(42, y_offset, m_CallStack->stack[index].name, wcslen(m_CallStack->stack[index].name));
				size_t size = m_CallStack->stack[index].line_number == 0 ? 2 : log10(m_CallStack->stack[index].line_number) + 2;
				wchar_t* buffer = new wchar_t[size];
				StringCchPrintfW(buffer, size, L"%d", m_CallStack->stack[index].line_number);
				MemoryDC.TextOutW(42, y_offset, buffer, size);
				delete[] buffer;
				y_offset += m_WordSize.cy;
				if (index == 0) {
					break;
				}
			}
		}
		MemoryDC.TextOutW(50, y_offset, L"<主程序>", 5);
		// 绘制栈帧深度
		MemoryDC.SetTextAlign(TA_RIGHT | TA_TOP);
		y_offset = (start_index - (USHORT)start_index + 1) * m_WordSize.cy;
		if (m_CallStack->ptr != 0) {
			for (USHORT index = m_CallStack->ptr - 1;; index--) {
				size_t size = m_CallStack->ptr == 0 ? 2 : log10(m_CallStack->ptr) + 2;
				wchar_t* buffer = new wchar_t[size];
				StringCchPrintfW(buffer, size, L"%d", m_CallStack->ptr);
				MemoryDC.TextOutW(42, y_offset, buffer, size);
				delete[] buffer;
				y_offset += m_WordSize.cy;
				if (index == 0) {
					break;
				}
			}
		}
		// 主程序栈帧的行号还没做
		size_t size = m_CallStack->ptr == 0 ? 2 : log10(m_CallStack->ptr) + 2;
		wchar_t* buffer = new wchar_t[size];
		StringCchPrintfW(buffer, size, L"%d", m_CallStack->ptr);
		MemoryDC.TextOutW(42, y_offset, buffer, size);
		delete[] buffer;
		size = m_CallStack->stack[0].line_number == 0 ? 2 : log10(m_CallStack->stack[0].line_number) + 2;
		buffer = new wchar_t[size];
		StringCchPrintfW(buffer, size, L"%d", m_CallStack->stack[0].line_number);
		MemoryDC.TextOutW(42, y_offset, buffer, size);
		delete[] buffer;
	}
	rect = CRect(0, 0, m_Width, m_WordSize.cy);
	MemoryDC.FillRect(&rect, pGreyBlackBrush);
	MemoryDC.SelectObject(m_Pen);
	MemoryDC.MoveTo(CPoint(0, m_WordSize.cy));
	MemoryDC.LineTo(CPoint(m_Width, m_WordSize.cy));
	MemoryDC.MoveTo(CPoint(46, 0));
	MemoryDC.LineTo(CPoint(46, m_Height));
	MemoryDC.MoveTo(CPoint(m_Width - 50, 0));
	MemoryDC.LineTo(CPoint(m_Width - 50, m_Height));
	MemoryDC.SelectObject(m_Font);
	MemoryDC.TextOutW(42, 0, L"栈帧", 2);
	MemoryDC.SetTextAlign(TA_LEFT | TA_TOP);
	MemoryDC.TextOutW(50, 0, L"名称", 2);
	MemoryDC.TextOutW(m_Width - 46, 0, L"行号", 2);
	dc.BitBlt(0, 0, m_Width, m_Height, &MemoryDC, 0, 0, SRCCOPY);
}
void CCallStack::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	m_Width = cx - 10;
	m_Height = cy;
	CRect rect(cx - 10, 0, cx, cy);
	m_Slider.MoveWindow(&rect, TRUE);
}
void CCallStack::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CWnd::OnShowWindow(bShow, nStatus);
	m_bShow = bShow;
	if (m_bShow) {
		REDRAW_WINDOW();
	}
}
void CCallStack::LoadCallStack(CALLSTACK* callstack)
{
	if (m_CallStack) {
		delete m_CallStack;
	}
	m_CallStack = callstack;
	Invalidate(FALSE);
}
inline void CCallStack::UpdateSlider()
{
	m_Slider.SetRatio((double)m_Height / m_FullHeight);
}
