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
	m_Width = m_Height = 0;
	m_FullHeight = 0;
	m_FirstWidth = m_SecondWidth = 0;
	m_Percentage = 0.0;
	m_SelectionDepth = 0;
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

	m_Font.CreateFontW(20, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	m_Pen.CreatePen(PS_SOLID, 1, RGB(61, 61, 61));
	m_Brush.CreateSolidBrush(RGB(61, 61, 61));

	m_BG.CreateCompatibleDC(&ScreenDC);
	m_BG.SelectObject(m_Font);
	m_BG.SetTextColor(RGB(255, 255, 255));
	m_BG.SetBkMode(TRANSPARENT);
	m_Source.CreateCompatibleDC(&ScreenDC);
	m_Source.SelectObject(m_Font);
	CRect rect;
	m_Source.DrawTextW(L"栈帧", 2, &rect, DT_CALCRECT);
	m_WordSize = CSize(rect.right + 8, rect.bottom + 4);
	m_Source.SetTextColor(RGB(255, 255, 255));
	m_Source.SetBkMode(TRANSPARENT);
	m_Selection.CreateCompatibleDC(&ScreenDC);

	m_FirstWidth = m_SecondWidth = m_WordSize.cx;
	ArrangeBG();
	m_Slider.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(), this, NULL);
	m_Slider.SetCallback(VerticalCallback);

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
	if (m_CallStack) {
		double start_index = m_Percentage * (m_CallStack->ptr + 1);
		MemoryDC.BitBlt(0, (m_SelectionDepth - start_index + 1) * m_WordSize.cy, m_Width, m_WordSize.cy, &m_Selection, 0, 0, SRCCOPY);
		MemoryDC.TransparentBlt(0, m_WordSize.cy, m_Width, m_Height - m_WordSize.cy, &m_Source, 0, 0, m_Width, m_Height - m_WordSize.cy, 0);
	}
	MemoryDC.BitBlt(0, 0, m_Width, m_WordSize.cy, &m_BG, 0, 0, SRCCOPY);
	dc.BitBlt(0, 0, m_Width, m_Height, &MemoryDC, 0, 0, SRCCOPY);
}
void CCallStack::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	m_Width = cx - 10;
	m_Height = cy;
	HANDLE hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_WordSize.cy);
	HANDLE hOldBitmap = SelectObject(m_BG, hBitmap);
	DeleteObject(hOldBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_Height - m_WordSize.cy);
	hOldBitmap = SelectObject(m_Source, hBitmap);
	DeleteObject(hOldBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_WordSize.cy);
	hOldBitmap = SelectObject(m_Selection, hBitmap);
	DeleteObject(hOldBitmap);
	CRect rect(0, 0, m_Width, m_WordSize.cy);
	m_Selection.FillRect(&rect, &m_Brush);
	rect = CRect(cx - 10, 0, cx, cy);
	m_Slider.MoveWindow(&rect, TRUE);
	ArrangeBG();
	if (m_bShow and m_CallStack) {
		ArrangeFrames();
	}
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
	if (not m_CallStack) { return; }
	// 计算合适的列宽（栈深度）
	CRect rect;
	wchar_t* buffer = new wchar_t[(USHORT)log10(m_CallStack->ptr) + 1];
	for (USHORT index = 0; index != (USHORT)log10(m_CallStack->ptr) + 1; index++) {
		buffer[index] = L'0';
	}
	m_Source.DrawTextW(buffer, (USHORT)log10(m_CallStack->ptr) + 1, &rect, DT_CALCRECT);
	delete[] buffer;
	USHORT m_FirstWidth = max(m_WordSize.cx, rect.right + 8);
	// 计算合适的列宽（行数）
	ULONGLONG largest = 0;
	for (USHORT index = 0; index != m_CallStack->ptr; index++) {
		largest = max(largest, m_CallStack->stack[index].line_number);
	}
	buffer = new wchar_t[(USHORT)log10(largest) + 1];
	for (USHORT index = 0; index != (USHORT)log10(largest) + 1; index++) {
		buffer[index] = L'0';
	}
	m_Source.DrawTextW(buffer, (USHORT)log10(largest) + 1, &rect, DT_CALCRECT);
	delete[] buffer;
	USHORT m_SecondWidth = max(m_WordSize.cx, rect.right + 8);
	// 更新滚动条
	m_FullHeight = m_CallStack->ptr * m_WordSize.cy;
	UpdateSlider();
	ArrangeBG();
	ArrangeFrames();
	Invalidate(FALSE);
}
inline void CCallStack::UpdateSlider()
{
	m_Slider.SetRatio((double)(m_Height - m_WordSize.cy) / m_FullHeight);
}
void CCallStack::VerticalCallback(double percentage)
{
	pObject->m_Percentage = percentage;
	pObject->ArrangeFrames();
	pObject->Invalidate(FALSE);
}
void CCallStack::ArrangeBG()
{
	CRect rect(0, 0, m_Width, m_WordSize.cy);
	m_BG.FillRect(&rect, pGreyBlackBrush);
	m_BG.SelectObject(m_Pen);
	m_BG.MoveTo(CPoint(0, m_WordSize.cy - 1));
	m_BG.LineTo(CPoint(m_Width, m_WordSize.cy - 1));
	m_BG.MoveTo(CPoint(m_FirstWidth, 0));
	m_BG.LineTo(CPoint(m_FirstWidth, m_WordSize.cy));
	m_BG.MoveTo(CPoint(m_FirstWidth + m_SecondWidth, 0));
	m_BG.LineTo(CPoint(m_FirstWidth + m_SecondWidth, m_WordSize.cy));
	rect = CRect(4, 0, m_FirstWidth - 4, m_WordSize.cy);
	m_BG.DrawTextW(L"栈帧", 2, &rect, DT_RIGHT);
	rect = CRect(m_FirstWidth + 4, 0, m_FirstWidth + m_SecondWidth - 4, m_WordSize.cy);
	m_BG.DrawTextW(L"行号", 2, &rect, DT_RIGHT);
	rect = CRect(m_FirstWidth + m_SecondWidth + 4, 0, m_Width - 4, m_WordSize.cy);
	m_BG.DrawTextW(L"名称", 2, &rect, NULL);
}
void CCallStack::ArrangeFrames()
{
	m_Source.PatBlt(0, 0, m_Width, m_Height, BLACKNESS);
	double start_index = m_Percentage * (m_CallStack->ptr + 1);
	int y_offset = -start_index * m_WordSize.cy;
	// 绘制栈帧深度
	CRect rect(4, y_offset, m_FirstWidth + 4, y_offset + m_WordSize.cy);
	for (USHORT index = m_CallStack->ptr - 1;; index--) {
		size_t size = index == 0 ? 2 : log10(index) + 2;
		wchar_t* buffer = new wchar_t[size];
		StringCchPrintfW(buffer, size, L"%d", index);
		m_Source.DrawTextW(buffer, size, &rect, DT_RIGHT);
		delete[] buffer;
		rect.top = rect.bottom;
		rect.bottom += m_WordSize.cy;
		if (index == 0) {
			break;
		}
	}
	// 绘制行数
	rect = CRect(m_FirstWidth + 4, y_offset, m_FirstWidth + m_SecondWidth + 4, y_offset + m_WordSize.cy);
	for (USHORT index = m_CallStack->ptr - 1;; index--) {
		size_t size = m_CallStack->stack[index].line_number == 0 ? 2 : log10(m_CallStack->stack[index].line_number) + 2;
		wchar_t* buffer = new wchar_t[size];
		StringCchPrintfW(buffer, size, L"%d", m_CallStack->stack[index].line_number + 1);
		m_Source.DrawTextW(buffer, size, &rect, DT_RIGHT);
		delete[] buffer;
		rect.top = rect.bottom;
		rect.bottom += m_WordSize.cy;
		if (index == 0) {
			break;
		}
	}
	// 绘制名称
	rect = CRect(m_FirstWidth + m_SecondWidth + 4, y_offset, m_Width - 4, y_offset + m_WordSize.cy);
	for (USHORT index = m_CallStack->ptr - 1;; index--) {
		wchar_t* name = CConsole::pObject->ReadMemory(m_CallStack->stack[index].name, m_CallStack->stack[index].length);
		m_Source.DrawTextW(name, -1, &rect, DT_END_ELLIPSIS);
		delete[] name;
		rect.top = rect.bottom;
		rect.bottom += m_WordSize.cy;
		if (index == 0) {
			break;
		}
	}
}