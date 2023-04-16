#include "stdafx.h"
#include "Editor.h"
#define LINE_NUMBER_OFFSET 27 // 展示行数所需的额外偏移量
#define SCROLL_UNIT 3 // 每次鼠标滚轮滚动时移动的行数

BEGIN_MESSAGE_MAP(CEditor, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CHAR()
	ON_WM_SETCURSOR()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()
CEditor::CEditor()
{
	m_bFocus = false;
	m_bDrag = false;
	m_CurrentTag = nullptr;
	m_Width = m_Height = 0;
	m_PercentageHorizontal = m_PercentageVertical = 0.0;
	m_FullWidth = m_FullHeight = 0;
	m_LineNumberWidth = 0;
	m_VSlider.SetCallback(VerticalCallback);
	m_HSlider.SetCallback(HorizontalCallback);
	m_Dialog = new CBreakpointDlg(this, &m_Breakpoints);
	pObject = this;
}
CEditor::~CEditor()
{
}
int CEditor::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;


	CDC* pWindowDC = GetDC();

	// 准备未选中文件时背景

	// 准备内存缓冲源
	m_MemoryDC.CreateCompatibleDC(pWindowDC);

	// 准备渲染文字源
	m_Source.CreateCompatibleDC(pWindowDC);
	m_Font.CreateFontW(30, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Consolas");
	m_Source.SelectObject(m_Font);
	m_Source.SetBkMode(TRANSPARENT);
	m_LineNumberWidth = 0;
	CRect rect;
	m_Source.DrawTextW(L"0", -1, &rect, DT_CALCRECT);
	m_CharSize = CSize(rect.right, rect.bottom);

	// 准备文档指针源
	m_Pointer.CreateCompatibleDC(pWindowDC);
	CBitmap* pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, 1, m_CharSize.cy);
	m_Pointer.SelectObject(pBitmap);
	rect = CRect(0, 0, 1, m_CharSize.cy);
	CBrush brush(RGB(149, 149, 149));
	m_Pointer.FillRect(&rect, &brush);

	// 准备选区源
	m_Selection.CreateCompatibleDC(pWindowDC);
	m_SelectionColor.CreateSolidBrush(RGB(38, 79, 120));
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, SCREEN_WIDTH, SCREEN_HEIGHT);
	m_Selection.SelectObject(pBitmap);
	rect = CRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	m_Selection.FillRect(&rect, pGreyBlackBrush);

	// 准备断点源
	m_Breakpoint.CreateCompatibleDC(pWindowDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, 15, SCREEN_HEIGHT);
	m_Breakpoint.SelectObject(pBitmap);
	m_Breakpoint.SelectObject(pThemeColorBrush);
	m_Breakpoint.SelectObject(pNullPen);

	// 创建滚动条
	m_VSlider.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_HSlider.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);

	return 0;
}
void CEditor::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	if (cx == 0) { return; }
	m_Width = cx - 10;
	m_Height = cy - 10;
	CDC* pWindowDC = GetDC();

	CBitmap* pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, m_Width, m_Height);
	CBitmap* pOldBitmap = m_MemoryDC.SelectObject(pBitmap);
	pOldBitmap->DeleteObject();
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, m_Width, m_Height);
	pOldBitmap = m_Selection.SelectObject(pBitmap);
	pOldBitmap->DeleteObject();
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, m_Width, m_Height);
	pOldBitmap = m_Source.SelectObject(pBitmap);
	pOldBitmap->DeleteObject();

	ArrangeText();
	ArrangePointer();
	ArrangeSelection();
	CRect rect(cx - 10, 0, cx, cy - 10);
	m_VSlider.MoveWindow(rect);
	rect = CRect(0, cy - 10, cx - 10, cy);
	m_HSlider.MoveWindow(rect);
	ArrangeBreakpoints();
	UpdateSlider();
}
BOOL CEditor::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}
void CEditor::OnPaint()
{
	CPaintDC dc(this);
	m_MemoryDC.BitBlt(0, 0, m_Width, m_Height, &m_Selection, 0, 0, SRCCOPY);
	m_MemoryDC.TransparentBlt(0, 0, m_Width, m_Height, &m_Source, 0, 0, m_Width, m_Height, 0);
	m_MemoryDC.BitBlt(0, 0, m_Width, m_Height, &m_Breakpoint, 0, 0, SRCCOPY);
	if (m_cPointer.x >= (INT64)m_LineNumberWidth + LINE_NUMBER_OFFSET and m_bFocus) {
		m_MemoryDC.BitBlt(m_cPointer.x, m_cPointer.y, 1, m_CharSize.cy, &m_Pointer, 0, 0, SRCCOPY);
	}
	dc.BitBlt(0, 0, m_Width, m_Height, &m_MemoryDC, 0, 0, SRCCOPY);
}
BOOL CEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	double after_scroll_percentage = (m_PercentageVertical * m_FullHeight + (zDelta > 0 ? (-m_CharSize.cy) : m_CharSize.cy) * SCROLL_UNIT) / m_FullHeight;
	double adjusted_percentage = min(max(0.0, after_scroll_percentage), 1.0);
	m_VSlider.SetPercentage(adjusted_percentage);
	return TRUE;
}
void CEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	SetFocus();
	if (point.x > m_LineNumberWidth + LINE_NUMBER_OFFSET) {
		m_bDrag = true;
		m_DragPointerPoint = TranslatePointer(point);
		m_PointerPoint = m_DragPointerPoint;
		CRect rect(0, 0, m_Width, m_Height);
		m_Selection.FillRect(&rect, pGreyBlackBrush);
		ArrangePointer();
		Invalidate(FALSE);
	}
}
void CEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	if (not m_bDrag) {
		double start_line = m_PercentageVertical * m_CurrentTag->GetLines()->size();
		ULONG64 line_index = (ULONG64)((double)point.y / m_CharSize.cy + start_line);
		for (std::list<BREAKPOINT>::iterator iter = m_Breakpoints.begin(); iter != m_Breakpoints.end(); iter++) {
			if (iter->line_index == line_index) {
				// 删除断点
				m_Breakpoints.erase(iter);
				ArrangeBreakpoints();
				Invalidate(FALSE);
				return;
			}
		}
		// 添加断点
		m_Breakpoints.push_back(BREAKPOINT{ line_index, 0, 0, 0 });
		ArrangeBreakpoints();
		Invalidate(FALSE);
	}
	else {
		m_bDrag = false;
	}
}
void CEditor::OnRButtonUp(UINT nFlags, CPoint point)
{
	// 添加或删除断点
	double start_line = m_PercentageVertical * m_CurrentTag->GetLines()->size();
	ULONG64 line_index = (ULONG64)((double)point.y / m_CharSize.cy + m_PercentageVertical);
	std::list<BREAKPOINT>::iterator iter = m_Breakpoints.begin();
	for (UINT index = 0; index != m_Breakpoints.size(); index++) {
		if (iter->line_index == line_index) {
			m_Dialog->SetCurrentIndex(index);
		}
		else {
			iter++;
		}
	}
	if (iter == m_Breakpoints.end()) {
		m_Dialog->SetCurrentIndex(-1);
	}
	m_Dialog->DoModal();
}
void CEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDrag) {
		m_PointerPoint = TranslatePointer(point);
		ArrangePointer();
		ArrangeSelection();
		Invalidate(FALSE);
	}
	else {
		if (point.x > m_LineNumberWidth + LINE_NUMBER_OFFSET) {
			SetCursor(LoadCursorW(NULL, IDC_IBEAM));
		}
		else {
			SetCursor(LoadCursorW(NULL, IDC_ARROW));
		}
	}
}
void CEditor::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar) {
	case L'\b':
	{
		Delete();
		break;
	}
	case L'\r':
	{
		if (m_PointerPoint != m_DragPointerPoint) {
			Delete();
		}
		wchar_t* buffer1 = new wchar_t[m_PointerPoint.x + 1];
		wchar_t* buffer2 = new wchar_t[wcslen(*m_CurrentTag->GetCurrentLine()) - m_PointerPoint.x + 1];
		memcpy(buffer1, *m_CurrentTag->GetCurrentLine(), m_PointerPoint.x * 2);
		buffer1[m_PointerPoint.x] = 0;
		memcpy(buffer2,
			*m_CurrentTag->GetCurrentLine() + m_PointerPoint.x,
			(wcslen(*m_CurrentTag->GetCurrentLine()) - m_PointerPoint.x + 1) * 2);
		delete[] *m_CurrentTag->GetCurrentLine();
		*m_CurrentTag->GetCurrentLine() = buffer1;
		std::list<wchar_t*>::iterator next_line = m_CurrentTag->GetCurrentLine();
		next_line++;
		m_CurrentTag->GetLines()->insert(next_line, buffer2);
		m_CurrentTag->GetCurrentLine()++;
		m_PointerPoint.y++;
		m_PointerPoint.x = 0;
		m_FullHeight += m_CharSize.cy;
		break;
	}
	default:
	{
		if (m_PointerPoint != m_DragPointerPoint) {
			Delete();
		}
		wchar_t* buffer = new wchar_t[wcslen(*m_CurrentTag->GetCurrentLine()) + 2];
		memcpy(buffer, *m_CurrentTag->GetCurrentLine(), (size_t)m_PointerPoint.x * 2);
		buffer[m_PointerPoint.x] = nChar;
		memcpy(buffer + m_PointerPoint.x + 1,
			*m_CurrentTag->GetCurrentLine() + m_PointerPoint.x,
			(wcslen(*m_CurrentTag->GetCurrentLine()) - m_PointerPoint.x + 1) * 2);
		delete[] *m_CurrentTag->GetCurrentLine();
		CRect rect;
		m_Source.DrawTextW(buffer, -1, &rect, DT_CALCRECT);
		rect.right += 15;
		m_FullWidth = max(m_FullWidth, rect.right);
		*m_CurrentTag->GetCurrentLine() = buffer;
		m_PointerPoint.x++;
		break;
	}
	}
	m_DragPointerPoint = m_PointerPoint;
	ArrangeText();
	ArrangePointer();
	Invalidate(FALSE);
}
BOOL CEditor::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	return TRUE;
}
void CEditor::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	m_bFocus = true;
	Invalidate(FALSE);
}
void CEditor::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	m_bFocus = false;
	Invalidate(FALSE);
}
void CEditor::LoadFile(CFileTag* tag)
{
	CDC* pWindowDC = GetDC();

	m_CurrentTag = tag;
	// 计算文字大小
	m_FullHeight = tag->GetLines()->size() * m_CharSize.cy;
	int longest = 0;
	for (std::list<wchar_t*>::iterator iter = tag->GetLines()->begin(); iter != tag->GetLines()->end(); iter++) {
		int size = *((int*)*iter - 4);
		if (size > longest) { longest = size; }
	}
	longest /= 2; // 实际字符长度（包含\0）
	m_FullWidth = (size_t)longest * m_CharSize.cx;
	ArrangeText();
	m_PointerPoint = CPoint(0, 0);
	m_DragPointerPoint = CPoint(0, 0);
	ArrangeSelection();
	ArrangePointer();
	ArrangeBreakpoints();
	UpdateSlider();
	Invalidate(FALSE);
}
void CEditor::VerticalCallback(double percentage)
{
	pObject->m_PercentageVertical = percentage;
	pObject->ArrangeText();
	pObject->ArrangePointer();
	pObject->ArrangeSelection();
	pObject->ArrangeBreakpoints();
	pObject->Invalidate(FALSE);
}
void CEditor::HorizontalCallback(double percentage)
{
	pObject->m_PercentageHorizontal = percentage;
	pObject->ArrangeText();
	pObject->ArrangePointer();
	pObject->ArrangeSelection();
	pObject->Invalidate(FALSE);
}
void CEditor::ArrangeText()
{
	if (not m_CurrentTag) { return; }
	m_Source.PatBlt(0, 0, m_Width, m_Height, BLACKNESS);
	// 计算出现在DC中的第一行行数
	double start_line = m_PercentageVertical * m_CurrentTag->GetLines()->size();
	size_t start_line_index = (size_t)(start_line);
	unsigned short digits = (unsigned short)log10(m_CurrentTag->GetLines()->size()) + 1;
	m_LineNumberWidth = m_CharSize.cx * digits;
	// 绘制代码
	m_Source.SetTextColor(RGB(255, 255, 255));
	std::list<wchar_t*>::iterator iter = m_CurrentTag->GetLines()->begin();
	size_t index = 0;
	for (; index != start_line_index; iter++) {
		index++;
	}
	CRect rect(LINE_NUMBER_OFFSET + m_LineNumberWidth - (long)(m_PercentageHorizontal * m_FullWidth),
		(long)((start_line_index - start_line) * m_CharSize.cy),
		m_Width - 15,
		(long)((start_line_index - start_line + 1) * m_CharSize.cy));
	for (; iter != m_CurrentTag->GetLines()->end(); iter++) {
		m_Source.DrawTextW(*iter, -1, &rect, DT_SINGLELINE | DT_TABSTOP | 4 << 8);
		rect.top = rect.bottom;
		rect.bottom += m_CharSize.cy;
		if (rect.top >= m_Height) { break; }
	}
	// 绘制行号
	m_Source.SetTextColor(RGB(150, 150, 150));
	wchar_t* buffer = new wchar_t[digits + 1];
	m_Source.PatBlt(0, 0, m_LineNumberWidth + LINE_NUMBER_OFFSET, m_Height, BLACKNESS);
	rect = CRect(LINE_NUMBER_OFFSET - 12,
		(long)((start_line_index - start_line) * m_CharSize.cy),
		LINE_NUMBER_OFFSET - 12 + m_LineNumberWidth,
		(long)((start_line_index - start_line + 1) * m_CharSize.cy));
	size_t total_lines = m_CurrentTag->GetLines()->size();
	for (size_t line_index = start_line_index; line_index != total_lines; line_index++) {
		StringCchPrintfW(buffer, (size_t)digits + 1, L"%d", (int)line_index + 1);
		m_Source.DrawTextW(buffer, -1, &rect, DT_SINGLELINE | DT_RIGHT);
		rect.top = rect.bottom;
		rect.bottom += m_CharSize.cy;
		if (rect.top > m_Height) { break; }
	}
	delete[] buffer;
}
void CEditor::ArrangeSingleLine()
{
	double start_line = m_PercentageVertical * m_CurrentTag->GetLines()->size();
	double difference = (double)m_PointerPoint.y - start_line;
	// 绘制代码
	m_Source.PatBlt(0, difference * m_CharSize.cy, m_Width, m_CharSize.cy, BLACKNESS);
	m_Source.SetTextColor(RGB(255, 255, 255));
	CRect rect(20 + m_LineNumberWidth - (long)(m_PercentageHorizontal * m_FullWidth),
		difference * m_CharSize.cy,
		m_Width,
		(difference + 1) * m_CharSize.cy);
	m_Source.DrawTextW(*m_CurrentTag->GetCurrentLine(), -1, &rect, DT_SINGLELINE | DT_TABSTOP | 4 << 8);
	// 绘制行号
	m_Source.PatBlt(0, difference * m_CharSize.cy, LINE_NUMBER_OFFSET + m_LineNumberWidth, m_CharSize.cy, BLACKNESS);
	m_Source.SetTextColor(RGB(150, 150, 150));
	rect.left = 8;
	rect.right = 8 + m_LineNumberWidth;
	unsigned short digits = (unsigned short)log10(m_CurrentTag->GetLines()->size()) + 1;
	wchar_t* buffer = new wchar_t[digits + 1];
	StringCchPrintfW(buffer, (size_t)digits + 1, L"%d", m_PointerPoint.y + 1);
	m_Source.DrawTextW(buffer, -1, &rect, DT_SINGLELINE | DT_RIGHT);
	delete[] buffer;
}
void CEditor::ArrangePointer()
{
	if (not m_CurrentTag) { return; }
	double start_line = m_PercentageVertical * m_CurrentTag->GetLines()->size();
	CRect rect;
	m_Source.DrawTextW(*m_CurrentTag->GetCurrentLine(), m_PointerPoint.x, &rect, DT_CALCRECT);
	m_cPointer.x = rect.right + m_LineNumberWidth + LINE_NUMBER_OFFSET - m_FullWidth * m_PercentageHorizontal;
	m_cPointer.y = (m_PointerPoint.y - start_line) * m_CharSize.cy;
}
void CEditor::ArrangeSelection()
{
	if (not m_CurrentTag) { return; }
	CRect rect(0, 0, m_Width, m_Height);
	m_Selection.FillRect(&rect, pGreyBlackBrush);
	double start_line = m_PercentageVertical * m_CurrentTag->GetLines()->size();
	double difference = (double)m_PointerPoint.y - start_line;
	LONG vertical_offset = difference * m_CharSize.cy;
	LONG horizontal_offset = m_PercentageHorizontal * m_FullWidth;
	rect.right = 0;
	if (m_PointerPoint.y == m_DragPointerPoint.y) {
		if (m_PointerPoint.x == m_DragPointerPoint.x) {
			rect = CRect(0, 0, m_LineNumberWidth + LINE_NUMBER_OFFSET, m_Height);
			m_Selection.FillRect(&rect, pGreyBlackBrush);
			return;
		}
		wchar_t* line = *m_CurrentTag->GetCurrentLine();
		LONG smaller = min(m_PointerPoint.x, m_DragPointerPoint.x);
		LONG larger = max(m_PointerPoint.x, m_DragPointerPoint.x);
		m_Source.DrawTextW(line, smaller, &rect, DT_CALCRECT);
		LONG start = rect.right + m_LineNumberWidth + LINE_NUMBER_OFFSET;
		m_Source.DrawTextW(line, larger, &rect, DT_CALCRECT);
		rect = CRect(start - horizontal_offset, vertical_offset, rect.right + m_LineNumberWidth + LINE_NUMBER_OFFSET, vertical_offset + m_CharSize.cy);
		m_Selection.FillRect(&rect, &m_SelectionColor);
	}
	else {
		if (m_PointerPoint.y > m_DragPointerPoint.y) {
			DrawTextW(m_Source, *m_CurrentTag->GetCurrentLine(), m_PointerPoint.x, &rect, DT_CALCRECT);
			rect = CRect(
				LINE_NUMBER_OFFSET + m_LineNumberWidth - horizontal_offset,
				vertical_offset,
				LINE_NUMBER_OFFSET + m_LineNumberWidth + rect.right - horizontal_offset,
				vertical_offset + m_CharSize.cy
			);
			m_Selection.FillRect(&rect, &m_SelectionColor);
			std::list<wchar_t*>::iterator current_line = m_CurrentTag->GetCurrentLine();
			current_line--;
			for (size_t line_index = m_PointerPoint.y - 1; line_index != m_DragPointerPoint.y; line_index--) {
				rect.top -= this->m_CharSize.cy;
				rect.bottom -= this->m_CharSize.cy;
				if ((*current_line)[0]) {
					m_Source.DrawTextW(*current_line, wcslen(*current_line), &rect, DT_CALCRECT);
				}
				else {
					rect.right = rect.left;
				}
				rect.right += 15;
				m_Selection.FillRect(&rect, &m_SelectionColor);
				current_line--;
			}
			rect.top -= this->m_CharSize.cy;
			rect.bottom -= this->m_CharSize.cy;
			rect.right = rect.left;
			m_Source.DrawTextW(*current_line, m_DragPointerPoint.x, &rect, DT_CALCRECT);
			rect.left = rect.right; // left 此时为选区起点，计算剩余文本长度
			if (*(*current_line + m_DragPointerPoint.x)) {
				m_Source.DrawTextW(*current_line + m_DragPointerPoint.x, -1, &rect, DT_CALCRECT);
			}
			rect.right += 15;
			m_Selection.FillRect(&rect, &m_SelectionColor);
		}
		else {
			m_Source.DrawTextW(*m_CurrentTag->GetCurrentLine(), m_PointerPoint.x, &rect, DT_CALCRECT);
			LONG start = rect.right;
			m_Source.DrawTextW(*m_CurrentTag->GetCurrentLine() + m_PointerPoint.x, -1, &rect, DT_CALCRECT);
			LONG length = rect.right;
			rect = CRect(
				LINE_NUMBER_OFFSET + m_LineNumberWidth + start - horizontal_offset,
				vertical_offset,
				LINE_NUMBER_OFFSET + m_LineNumberWidth + start + length + this->m_CharSize.cx - horizontal_offset,
				vertical_offset + this->m_CharSize.cy
			);
			m_Selection.FillRect(&rect, &m_SelectionColor);
			rect.left = LINE_NUMBER_OFFSET + m_LineNumberWidth - horizontal_offset;
			std::list<wchar_t*>::iterator current_line = m_CurrentTag->GetCurrentLine();
			current_line++;
			for (size_t line_index = m_PointerPoint.y + 1; line_index != m_DragPointerPoint.y; line_index++) {
				rect.top += this->m_CharSize.cy;
				rect.bottom += this->m_CharSize.cy;
				if ((*current_line)[0]) {
					m_Source.DrawTextW(*current_line, wcslen(*current_line), &rect, DT_CALCRECT);
				}
				else {
					rect.right = rect.left;
				}
				rect.right += 15;
				m_Selection.FillRect(&rect, &m_SelectionColor);
				current_line++;
			}
			m_Source.DrawTextW(*current_line, m_DragPointerPoint.x, &rect, DT_CALCRECT);
			rect.top += this->m_CharSize.cy;
			rect.bottom += this->m_CharSize.cy;
			m_Selection.FillRect(&rect, &m_SelectionColor);
		}
	}
	rect = CRect(0, 0, m_LineNumberWidth + LINE_NUMBER_OFFSET, m_Height);
	m_Selection.FillRect(&rect, pGreyBlackBrush);
}
void CEditor::ArrangeBreakpoints()
{
	if (not m_CurrentTag) { return; }
	CRect rect(0, 0, 15, SCREEN_HEIGHT);
	m_Breakpoint.FillRect(&rect, pGreyBlackBrush);
	double start_line = m_PercentageVertical * m_CurrentTag->GetLines()->size();
	double end_line = start_line + (double)m_Height / m_CharSize.cy;
	for (std::list<BREAKPOINT>::iterator iter = m_Breakpoints.begin(); iter != m_Breakpoints.end(); iter++) {
		if (start_line - 1 < iter->line_index and iter->line_index < end_line + 1) {
			m_Breakpoint.MoveTo(CPoint(2, (iter->line_index - start_line) * m_CharSize.cy));
			CPoint* points = new CPoint[]{
				CPoint(2, (iter->line_index - start_line) * m_CharSize.cy - 8),
				CPoint(2, (iter->line_index - start_line) * m_CharSize.cy + 8),
				CPoint(13, (iter->line_index - start_line) * m_CharSize.cy),
			};
			m_Breakpoint.Polygon(points, 3);
			delete[] points;
		}
	}
}
inline void CEditor::UpdateSlider()
{
	m_VSlider.SetRatio((double)(m_Height - 15) / m_FullHeight);
	m_HSlider.SetRatio((double)(m_Width - 15) / m_FullWidth);
}
CPoint CEditor::TranslatePointer(CPoint point)
{
	CPoint point_out;
	double start_line = m_PercentageVertical * m_CurrentTag->GetLines()->size();
	size_t new_pointer_vertical = min(start_line + (double)point.y / m_CharSize.cy, m_CurrentTag->GetLines()->size() - 1);
	long long difference = new_pointer_vertical - m_PointerPoint.y;
	point.x += m_PercentageHorizontal * m_FullWidth - m_LineNumberWidth - LINE_NUMBER_OFFSET;
	// 调整行指针
	if (difference < 0) {
		for (long long index = 0; index != difference; index--) {
			m_CurrentTag->GetCurrentLine()--;
		}
	}
	else if (difference > 0) {
		for (long long index = 0; index != difference; index++) {
			m_CurrentTag->GetCurrentLine()++;
		}
	}
	point_out.y = new_pointer_vertical;
	// 计算列指针
	size_t total_length = 0;
	CRect rect;
	for (UINT index = 0; (*m_CurrentTag->GetCurrentLine())[index] != 0; index++) {
		m_Source.DrawTextW((*m_CurrentTag->GetCurrentLine()) + index, 1, &rect, DT_CALCRECT);
		size_t this_length = rect.right;
		if (total_length + this_length > point.x) {
			if (total_length + this_length / 2 < point.x) {
				point_out.x = index + 1;
			}
			else {
				point_out.x = index;
			}
			return point_out;
		}
		total_length += this_length;
	}
	point_out.x = wcslen(*m_CurrentTag->GetCurrentLine());
	return point_out;
}
void CEditor::Delete()
{
	if (m_PointerPoint == m_DragPointerPoint) {
		// 删除单个字符
		if (m_PointerPoint.x) {
			// 该行还有字符
			wchar_t* current_line = *m_CurrentTag->GetCurrentLine();
			wchar_t* buffer = new wchar_t[wcslen(current_line)];
			memcpy(buffer, current_line, (size_t)(m_PointerPoint.x - 1) * 2);
			memcpy(buffer + m_PointerPoint.x - 1,
				current_line + m_PointerPoint.x,
				(wcslen(current_line) - m_PointerPoint.x + 1) * 2);
			delete[] current_line;
			*m_CurrentTag->GetCurrentLine() = buffer;
			CRect rect;
			m_Source.DrawTextW(buffer, -1, &rect, DT_CALCRECT);
			int longest = 0;
			for (std::list<wchar_t*>::iterator iter = m_CurrentTag->GetLines()->begin(); iter != m_CurrentTag->GetLines()->end(); iter++) {
				int size = *((int*)*iter - 4);
				if (size > longest) { longest = size; }
			}
			longest /= 2; // 实际字符长度（包含\0）
			m_FullWidth = (size_t)longest * m_CharSize.cx;
			m_PointerPoint.x--;
		}
		else {
			// 删除换行符
			if (m_PointerPoint.y) {
				// 如果不是首行
				std::list<wchar_t*>::iterator deleted_line = m_CurrentTag->GetCurrentLine();
				m_CurrentTag->GetCurrentLine()--;
				m_CurrentTag->GetLines()->erase(deleted_line);
				m_PointerPoint.y--;
				m_PointerPoint.x = wcslen(*m_CurrentTag->GetCurrentLine());
				m_FullHeight -= m_CharSize.cy;
			}
		}
		m_DragPointerPoint = m_PointerPoint; // 确保无选区模式
	}
	else {
		// 删除选区
		if (m_PointerPoint.y == m_DragPointerPoint.y) {
			// 单行选区
			size_t smaller = min(m_PointerPoint.x, m_DragPointerPoint.x);
			size_t larger = max(m_PointerPoint.x, m_DragPointerPoint.x);
			wchar_t* buffer = new wchar_t[wcslen(*m_CurrentTag->GetCurrentLine()) - (larger - smaller) + 1];
			memcpy(buffer, *m_CurrentTag->GetCurrentLine(), smaller * 2);
			memcpy(buffer + smaller, *m_CurrentTag->GetCurrentLine() + larger, (wcslen(*m_CurrentTag->GetCurrentLine()) - larger + 1) * 2);
			delete[] *(m_CurrentTag->GetCurrentLine());
			*m_CurrentTag->GetCurrentLine() = buffer;
			m_PointerPoint.x = smaller;
		}
		else if (m_PointerPoint.y > m_DragPointerPoint.y) {
			// 多行选区（正向）
			std::list<wchar_t*>::iterator selection_begin = m_CurrentTag->GetCurrentLine();
			for (size_t line_index = m_PointerPoint.y - 1; line_index != m_DragPointerPoint.y; line_index--) {
				selection_begin--;
			}
			m_FullHeight -= (m_PointerPoint.y - m_DragPointerPoint.y) * m_CharSize.cy;
			std::list<wchar_t*>::iterator line_before_selection = selection_begin;
			line_before_selection--;
			m_CurrentTag->GetLines()->erase(selection_begin, m_CurrentTag->GetCurrentLine());
			wchar_t* buffer = new wchar_t[m_DragPointerPoint.x + wcslen(*m_CurrentTag->GetCurrentLine()) - m_PointerPoint.x + 2];
			memcpy(buffer, *line_before_selection, m_DragPointerPoint.x * 2);
			memcpy(buffer + m_DragPointerPoint.x,
				*m_CurrentTag->GetCurrentLine() + m_PointerPoint.x,
				(wcslen(*m_CurrentTag->GetCurrentLine()) - m_PointerPoint.x + 1) * 2);
			delete[] *line_before_selection;
			*line_before_selection = buffer;
			m_CurrentTag->GetLines()->erase(m_CurrentTag->GetCurrentLine());
			m_CurrentTag->GetCurrentLine() = line_before_selection;
			m_PointerPoint.y = m_DragPointerPoint.y;
			m_PointerPoint.x = m_DragPointerPoint.x;
		}
		else {
			// 多行选区（反向）
			std::list<wchar_t*>::iterator line_after_selection = m_CurrentTag->GetCurrentLine();
			for (size_t line_index = m_PointerPoint.y; line_index != m_DragPointerPoint.y; line_index++) {
				line_after_selection++;
			}
			m_FullHeight -= (m_DragPointerPoint.y - m_PointerPoint.y) * m_CharSize.cy;
			std::list<wchar_t*>::iterator selection_begin = m_CurrentTag->GetCurrentLine();
			selection_begin++;
			m_CurrentTag->GetLines()->erase(selection_begin, line_after_selection);
			wchar_t* buffer = new wchar_t[m_PointerPoint.x + wcslen(*line_after_selection) - m_DragPointerPoint.x + 2];
			memcpy(buffer, *m_CurrentTag->GetCurrentLine(), (size_t)m_PointerPoint.x * 2);
			memcpy(buffer + m_PointerPoint.x,
				*line_after_selection + m_DragPointerPoint.x,
				(wcslen(*line_after_selection) - m_DragPointerPoint.x + 1) * 2);
			delete[] *(m_CurrentTag->GetCurrentLine());
			*m_CurrentTag->GetCurrentLine() = buffer;
			m_CurrentTag->GetLines()->erase(line_after_selection);
		}
		int longest = 0;
		for (std::list<wchar_t*>::iterator iter = m_CurrentTag->GetLines()->begin(); iter != m_CurrentTag->GetLines()->end(); iter++) {
			int size = *((int*)*iter - 4);
			if (size > longest) { longest = size; }
		}
		longest /= 2; // 实际字符长度（包含\0）
		CRect rect(0, 0, m_Width, m_Height);
		m_Selection.FillRect(&rect, pGreyBlackBrush);
		m_DragPointerPoint = m_PointerPoint; // 确保无选区模式
	}
}
