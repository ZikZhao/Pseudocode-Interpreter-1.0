#include "pch.h"
#include "../Pseudocode Interpreter/SyntaxCheck.h"
#define LINE_NUMBER_OFFSET 27 // 展示行数所需的额外偏移量
#define SCROLL_UNIT 3 // 每次鼠标滚轮滚动时移动的行数
#define PAUSE_BACKEND() { m_bBackendEnabled = false; WaitForSingleObject(*m_BackendPaused, INFINITE); SetTimer(NULL, 100, nullptr); } // 停止后台任务以避免访问冲突
#define GET_TEXT_WIDTH(string, length) LOWORD(GetTabbedTextExtentW(m_Source, string, length, 1, &tab_position))

static int tab_position;

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
	ON_WM_TIMER()
	ON_MESSAGE(WM_STEP, CEditor::OnStep)
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
	m_Dialog = new CBreakpointDlg(this, &m_Breakpoints);
	m_bBackendEnabled = false;
	m_BackendPaused = new CEvent(FALSE, TRUE);
	m_BackendPaused->SetEvent();
	m_CurrentStepLineIndex = -1;
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
	TEXTMETRICW tm;
	GetTextMetricsW(m_Source, &tm);
	m_CharSize = CSize(tm.tmAveCharWidth, tm.tmHeight);
	tab_position = m_CharSize.cx * 4;

	// 准备文档指针源
	m_Pointer.CreateCompatibleDC(pWindowDC);
	CBitmap* pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, 1, m_CharSize.cy);
	m_Pointer.SelectObject(pBitmap);
	CRect rect(0, 0, 1, m_CharSize.cy);
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
	m_BreakpointHitColor.CreateSolidBrush(RGB(202, 205, 56));
	m_Breakpoint.SelectObject(pNullPen);

	// 创建滚动条
	m_VSlider.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_HSlider.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_VSlider.SetCallback(VerticalCallback);
	m_HSlider.SetCallback(HorizontalCallback);
	m_HSlider.SetDeflateCallback(DeflationCallback);

	// 启动后台任务
	CreateThread(NULL, NULL, BackendTasking, nullptr, NULL, NULL);
	SetTimer(NULL, 1000, nullptr);

	return 0;
}
void CEditor::OnSize(UINT nType, int cx, int cy)
{
	PAUSE_BACKEND()
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
	return TRUE;
}
void CEditor::OnPaint()
{
	CPaintDC dc(this);
	m_MemoryDC.BitBlt(0, 0, m_Width, m_Height, &m_Selection, 0, 0, SRCCOPY);
	m_MemoryDC.TransparentBlt(0, 0, m_Width, m_Height, &m_Source, 0, 0, m_Width, m_Height, 0);
	m_MemoryDC.BitBlt(0, 0, m_Width, m_Height, &m_Breakpoint, 0, 0, SRCCOPY);
	if (m_cPointer.x >= (LONG)m_LineNumberWidth + LINE_NUMBER_OFFSET and m_bFocus) {
		m_MemoryDC.BitBlt(m_cPointer.x, m_cPointer.y, 1, m_CharSize.cy, &m_Pointer, 0, 0, SRCCOPY);
	}
	dc.BitBlt(0, 0, m_Width, m_Height, &m_MemoryDC, 0, 0, SRCCOPY);
	CRect rect(m_Width, m_Height, m_Width + 10, m_Height + 10);
	dc.FillRect(&rect, pGreyBlackBrush);
}
BOOL CEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	PAUSE_BACKEND()
	double after_scroll_percentage = (m_PercentageVertical * m_FullHeight + (zDelta > 0 ? (-m_CharSize.cy) : m_CharSize.cy) * SCROLL_UNIT) / m_FullHeight;
	double adjusted_percentage = min(max(0.0, after_scroll_percentage), 1.0);
	m_VSlider.SetPercentage(adjusted_percentage);
	return TRUE;
}
void CEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	PAUSE_BACKEND()
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
	PAUSE_BACKEND()
	ReleaseCapture();
	if (not m_bDrag) {
		double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
		ULONG64 line_index = (ULONG64)((double)point.y / m_CharSize.cy + start_line);
		if (line_index < 0 or line_index >= m_CurrentTag->m_Lines.size());
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
	PAUSE_BACKEND()
	// 添加或删除断点
	double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
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
	PAUSE_BACKEND()
	if (m_bDrag) {
		m_PointerPoint = TranslatePointer(point);
		MoveView();
		ArrangeText();
		ArrangePointer();
		ArrangeSelection();
		RedrawWindow(NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE | RDW_INTERNALPAINT);
		if (point.x < m_LineNumberWidth + LINE_NUMBER_OFFSET or point.x > m_Width + 10
			or point.y < 0 or point.y > m_Height + 10) {
			MSG msg;
			if (not PeekMessageW(&msg, m_hWnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_NOREMOVE)) {
				PostMessageW(WM_MOUSEMOVE, nFlags, MAKELPARAM(point.x, point.y));
			}
		}
	}
	else {
		if (point.x > m_LineNumberWidth + LINE_NUMBER_OFFSET or m_bDrag) {
			SetCursor(LoadCursorW(NULL, IDC_IBEAM));
		}
		else {
			SetCursor(LoadCursorW(NULL, IDC_ARROW));
		}
	}
}
void CEditor::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	PAUSE_BACKEND()
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
		wchar_t* buffer2 = new wchar_t[wcslen(*m_CurrentTag->m_CurrentLine) - m_PointerPoint.x + 1];
		memcpy(buffer1, *m_CurrentTag->m_CurrentLine, m_PointerPoint.x * 2);
		buffer1[m_PointerPoint.x] = 0;
		memcpy(buffer2,
			*m_CurrentTag->m_CurrentLine + m_PointerPoint.x,
			(wcslen(*m_CurrentTag->m_CurrentLine) - m_PointerPoint.x + 1) * 2);
		delete[] *m_CurrentTag->m_CurrentLine;
		*m_CurrentTag->m_CurrentLine = buffer1;
		m_CurrentTag->m_Lines.insert(m_PointerPoint.y + 1, buffer2);
		m_CurrentTag->m_CurrentLine++;
		m_PointerPoint.y++;
		m_PointerPoint.x = 0;
		m_DragPointerPoint = m_PointerPoint;
		USHORT digits = (USHORT)log10(m_CurrentTag->m_Lines.size()) + 1;
		m_LineNumberWidth = m_CharSize.cx * digits;
		m_FullHeight += m_CharSize.cy;
		UpdateSlider();
		break;
	}
	default:
	{
		if (m_PointerPoint != m_DragPointerPoint) {
			Delete();
		}
		wchar_t* buffer = new wchar_t[wcslen(*m_CurrentTag->m_CurrentLine) + 2];
		memcpy(buffer, *m_CurrentTag->m_CurrentLine, (size_t)m_PointerPoint.x * 2);
		buffer[m_PointerPoint.x] = nChar;
		memcpy(buffer + m_PointerPoint.x + 1,
			*m_CurrentTag->m_CurrentLine + m_PointerPoint.x,
			(wcslen(*m_CurrentTag->m_CurrentLine) - m_PointerPoint.x + 1) * 2);
		delete[] *m_CurrentTag->m_CurrentLine;
		CSize size;
		GetTextExtentPoint32W(m_Source, buffer, wcslen(buffer), &size);
		m_FullWidth = max(m_FullWidth, size.cx + m_CharSize.cx);
		UpdateSlider();
		*m_CurrentTag->m_CurrentLine = buffer;
		m_PointerPoint.x++;
		break;
	}
	}
	m_DragPointerPoint = m_PointerPoint;
	MoveView();
	ArrangeText();
	ArrangePointer();
	ArrangeSelection();
	Invalidate(FALSE);
}
BOOL CEditor::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	return TRUE;
}
void CEditor::OnSetFocus(CWnd* pOldWnd)
{
	PAUSE_BACKEND()
	CWnd::OnSetFocus(pOldWnd);
	m_bFocus = true;
	Invalidate(FALSE);
}
void CEditor::OnKillFocus(CWnd* pNewWnd)
{
	PAUSE_BACKEND()
	CWnd::OnKillFocus(pNewWnd);
	m_bFocus = false;
	Invalidate(FALSE);
}
void CEditor::OnTimer(UINT_PTR nIDEvent)
{
	m_bBackendEnabled = true;
	m_BackendPaused->ResetEvent();
}
void CEditor::OnUndo()
{
	PAUSE_BACKEND()
	m_CurrentOperation--;
	if (m_CurrentOperation == m_Operations.begin()) {
		m_CurrentOperation++;
	}
	else {
		if (m_CurrentOperation->content) {
			// 删除插入的内容
			m_DragPointerPoint = m_CurrentOperation->start;
			MovePointer(m_CurrentOperation->end);
			Delete();
		}
		else {
			// 插入删除的内容
			MovePointer(m_CurrentOperation->end);
			Insert(m_CurrentOperation->content);
		}
		FIND_BUTTON(ID_EDIT, ID_EDIT_UNDO)->SetState(false);
	}
}
void CEditor::OnRedo()
{
	PAUSE_BACKEND()
	if (m_CurrentOperation != m_Operations.end()) {
		if (m_CurrentOperation->content) {
			// 复原插入的内容
			MovePointer(m_CurrentOperation->start);
			Insert(m_CurrentOperation->content);
			m_DragPointerPoint = m_PointerPoint;
		}
		else {
			// 继续删除的内容
			m_DragPointerPoint = m_CurrentOperation->start;
			MovePointer(m_CurrentOperation->end);
			Delete();
		}
		m_CurrentOperation++;
		if (m_CurrentOperation == m_Operations.end()) {
			FIND_BUTTON(ID_EDIT, ID_EDIT_REDO)->SetState(false);
		}
	}
}
LRESULT CEditor::OnStep(WPARAM wParam, LPARAM lParam)
{
	PAUSE_BACKEND()
	m_CurrentStepLineIndex = lParam;
	CentralView(lParam);
	ArrangeText();
	ArrangePointer();
	ArrangeSelection();
	ArrangeBreakpoints();
	Invalidate(FALSE);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_CONTINUE)->SetState(true);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(true);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPOVER)->SetState(true);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPOUT)->SetState(true);
	return 0;
}
void CEditor::LoadFile(CFileTag* tag)
{
	PAUSE_BACKEND()
	CDC* pWindowDC = GetDC();

	m_CurrentTag = tag;
	// 计算文字大小
	m_FullHeight = tag->m_Lines.size() * m_CharSize.cy;
	m_FullWidth = 0;
	CSize size;
	for (IndexedList<wchar_t*>::iterator iter = tag->m_Lines.begin(); iter != tag->m_Lines.end(); iter++) {
		GetTextExtentPointW(m_Source, *iter, wcslen(*iter), &size);
		m_FullWidth = max(m_FullWidth, size.cx);
	}
	m_FullWidth += m_CharSize.cx;
	USHORT digits = (USHORT)log10(tag->m_Lines.size()) + 1;
	m_LineNumberWidth = m_CharSize.cx * digits;
	m_PointerPoint = CPoint(0, 0);
	m_DragPointerPoint = CPoint(0, 0);
	ArrangeText();
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
void CEditor::DeflationCallback(ULONG new_width)
{
	pObject->m_FullWidth = new_width;
}
void CEditor::ArrangeText()
{
	if (not m_CurrentTag) { return; }
	PAUSE_BACKEND()
	m_Source.PatBlt(0, 0, m_Width, m_Height, BLACKNESS);
	// 计算出现在DC中的第一行行数
	double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
	size_t start_line_index = (size_t)(start_line);
	// 绘制代码
	m_Source.SetTextColor(RGB(255, 255, 255));
	IndexedList<wchar_t*>::iterator iter = m_CurrentTag->m_Lines.begin();
	size_t index = 0;
	for (; index != start_line_index; iter++) {
		index++;
	}
	CRect rect(LINE_NUMBER_OFFSET + m_LineNumberWidth - (long)(m_PercentageHorizontal * m_FullWidth),
		(long)((start_line_index - start_line) * m_CharSize.cy),
		m_Width,
		(long)((start_line_index - start_line + 1) * m_CharSize.cy));
	for (; iter != m_CurrentTag->m_Lines.end(); iter++) {
		m_Source.DrawTextW(*iter, -1, &rect, DT_EXPANDTABS | DT_TABSTOP | 4 << 8);
		rect.top = rect.bottom;
		rect.bottom += m_CharSize.cy;
		if (rect.top >= m_Height) {
			break;
		}
	}
	// 绘制行号
	m_Source.SetTextColor(RGB(150, 150, 150));
	USHORT digits = (USHORT)log10(m_CurrentTag->m_Lines.size()) + 1;
	wchar_t* buffer = new wchar_t[digits + 1];
	m_Source.PatBlt(0, 0, m_LineNumberWidth + LINE_NUMBER_OFFSET, m_Height, BLACKNESS);
	rect = CRect(LINE_NUMBER_OFFSET - 12,
		(long)((start_line_index - start_line) * m_CharSize.cy),
		LINE_NUMBER_OFFSET - 12 + m_LineNumberWidth,
		(long)((start_line_index - start_line + 1) * m_CharSize.cy));
	size_t total_lines = m_CurrentTag->m_Lines.size();
	for (size_t line_index = start_line_index; line_index != total_lines; line_index++) {
		StringCchPrintfW(buffer, (size_t)digits + 1, L"%d", (int)line_index + 1);
		m_Source.DrawTextW(buffer, -1, &rect, DT_RIGHT);
		rect.top = rect.bottom;
		rect.bottom += m_CharSize.cy;
		if (rect.top > m_Height) { break; }
	}
	delete[] buffer;
}
void CEditor::ArrangePointer()
{
	if (not m_CurrentTag) { return; }
	PAUSE_BACKEND()
	double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
	LONG width = GET_TEXT_WIDTH(*m_CurrentTag->m_CurrentLine, m_PointerPoint.x);
	m_cPointer.x = width + m_LineNumberWidth + LINE_NUMBER_OFFSET - m_FullWidth * m_PercentageHorizontal;
	m_cPointer.y = (m_PointerPoint.y - start_line) * m_CharSize.cy;
}
void CEditor::ArrangeSelection()
{
	if (not m_CurrentTag) { return; }
	PAUSE_BACKEND()
	CRect rect(0, 0, m_Width, m_Height);
	m_Selection.FillRect(&rect, pGreyBlackBrush);
	double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
	double difference = (double)m_PointerPoint.y - start_line;
	LONG vertical_offset = difference * m_CharSize.cy;
	LONG horizontal_offset = m_PercentageHorizontal * m_FullWidth;
	if (m_PointerPoint.y == m_DragPointerPoint.y) {
		if (m_PointerPoint.x != m_DragPointerPoint.x) {
			wchar_t* line = *m_CurrentTag->m_CurrentLine;
			LONG smaller = min(m_PointerPoint.x, m_DragPointerPoint.x);
			LONG larger = max(m_PointerPoint.x, m_DragPointerPoint.x);
			LONG start = GET_TEXT_WIDTH(line, smaller) + m_LineNumberWidth + LINE_NUMBER_OFFSET;
			LONG end = GET_TEXT_WIDTH(line, larger) + m_LineNumberWidth + LINE_NUMBER_OFFSET;
			rect = CRect(
				start - horizontal_offset,
				vertical_offset,
				end - horizontal_offset,
				vertical_offset + m_CharSize.cy
			);
			m_Selection.FillRect(&rect, &m_SelectionColor);
		}
	}
	else {
		if (m_PointerPoint.y > m_DragPointerPoint.y) {
			LONG end = GET_TEXT_WIDTH(*m_CurrentTag->m_CurrentLine, m_PointerPoint.x);
			rect = CRect(
				LINE_NUMBER_OFFSET + m_LineNumberWidth - horizontal_offset,
				vertical_offset,
				LINE_NUMBER_OFFSET + m_LineNumberWidth + end - horizontal_offset,
				vertical_offset + m_CharSize.cy
			);
			m_Selection.FillRect(&rect, &m_SelectionColor);
			IndexedList<wchar_t*>::iterator current_line = m_CurrentTag->m_CurrentLine;
			current_line--;
			rect.left = LINE_NUMBER_OFFSET + m_LineNumberWidth - horizontal_offset;
			for (size_t line_index = m_PointerPoint.y - 1; line_index != m_DragPointerPoint.y; line_index--) {
				rect.top -= this->m_CharSize.cy;
				rect.bottom -= this->m_CharSize.cy;
				rect.right = rect.left;
				if ((*current_line)[0]) {
					rect.right += GET_TEXT_WIDTH(*current_line, wcslen(*current_line));
				}
				rect.right += m_CharSize.cx;
				m_Selection.FillRect(&rect, &m_SelectionColor);
				current_line--;
			}
			rect.top -= this->m_CharSize.cy;
			rect.bottom -= this->m_CharSize.cy;
			rect.right = rect.left;
			rect.left += GET_TEXT_WIDTH(*current_line, m_DragPointerPoint.x);
			rect.right += GET_TEXT_WIDTH(*current_line, wcslen(*current_line)) + m_CharSize.cx;
			m_Selection.FillRect(&rect, &m_SelectionColor);
		}
		else {
			LONG start = GET_TEXT_WIDTH(*m_CurrentTag->m_CurrentLine, m_PointerPoint.x);
			LONG end = GET_TEXT_WIDTH(*m_CurrentTag->m_CurrentLine, wcslen(*m_CurrentTag->m_CurrentLine));
			rect = CRect(
				LINE_NUMBER_OFFSET + m_LineNumberWidth + start - horizontal_offset,
				vertical_offset,
				LINE_NUMBER_OFFSET + m_LineNumberWidth + end + this->m_CharSize.cx - horizontal_offset,
				vertical_offset + this->m_CharSize.cy
			);
			m_Selection.FillRect(&rect, &m_SelectionColor);
			rect.left = LINE_NUMBER_OFFSET + m_LineNumberWidth - horizontal_offset;
			IndexedList<wchar_t*>::iterator current_line = m_CurrentTag->m_CurrentLine;
			current_line++;
			for (size_t line_index = m_PointerPoint.y + 1; line_index != m_DragPointerPoint.y; line_index++) {
				rect.top += this->m_CharSize.cy;
				rect.bottom += this->m_CharSize.cy;
				rect.right = rect.left;
				rect.right += GET_TEXT_WIDTH(*current_line, wcslen(*current_line)) + m_CharSize.cx;
				m_Selection.FillRect(&rect, &m_SelectionColor);
				current_line++;
			}
			rect.right = rect.left + GET_TEXT_WIDTH(*current_line, m_DragPointerPoint.x);
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
	PAUSE_BACKEND()
	CRect rect(0, 0, 15, SCREEN_HEIGHT);
	m_Breakpoint.FillRect(&rect, pGreyBlackBrush);
	double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
	double end_line = start_line + (double)m_Height / m_CharSize.cy;
	m_Breakpoint.SelectObject(pThemeColorBrush);
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
	m_Breakpoint.SelectObject(&m_BreakpointHitColor);
	if (m_CurrentStepLineIndex != -1) {
		if (start_line - 1 < m_CurrentStepLineIndex and m_CurrentStepLineIndex < end_line + 1) {
			m_Breakpoint.MoveTo(CPoint(2, (m_CurrentStepLineIndex - start_line) * m_CharSize.cy));
			CPoint* points = new CPoint[]{
				CPoint(2, (m_CurrentStepLineIndex - start_line) * m_CharSize.cy - 8),
				CPoint(2, (m_CurrentStepLineIndex - start_line) * m_CharSize.cy + 8),
				CPoint(13, (m_CurrentStepLineIndex - start_line) * m_CharSize.cy),
			};
			m_Breakpoint.Polygon(points, 3);
			delete[] points;
		}
	}
}
inline void CEditor::UpdateSlider()
{
	m_VSlider.SetRatio((double)(m_Height) / m_FullHeight);
	m_HSlider.SetRatio((double)(m_Width - m_LineNumberWidth - LINE_NUMBER_OFFSET) / m_FullWidth);
}
void CEditor::MovePointer(CPoint pointer)
{
	LONG difference = pointer.y - m_PointerPoint.y;
	if (difference < 0) {
		for (LONG index = 0; index != difference; index--) {
			m_CurrentTag->m_CurrentLine--;
		}
	}
	else if (difference > 0) {
		for (LONG index = 0; index != difference; index++) {
			m_CurrentTag->m_CurrentLine++;
		}
	}
	m_PointerPoint.x = pointer.x;
}
CPoint CEditor::TranslatePointer(CPoint point)
{
	PAUSE_BACKEND()
	CPoint point_out;
	double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
	size_t new_pointer_vertical = max(min(start_line + (double)point.y / m_CharSize.cy, m_CurrentTag->m_Lines.size() - 1), 0);
	point.x += m_PercentageHorizontal * m_FullWidth - m_LineNumberWidth - LINE_NUMBER_OFFSET;
	// 调整行指针
	MovePointer(CPoint(0, new_pointer_vertical));
	point_out.y = new_pointer_vertical;
	// 计算列指针
	if (point.x < 0) {
		point_out.x = 0;
		return point_out;
	}
	size_t total_length = 0;
	for (UINT index = 0; (*m_CurrentTag->m_CurrentLine)[index] != 0; index++) {
		size_t this_length = GET_TEXT_WIDTH(*m_CurrentTag->m_CurrentLine, index + 1) - total_length;
		if (total_length + this_length > point.x) {
			if (total_length + this_length / 2 < point.x) {
				point_out.x = index + 1;
				return point_out;
			}
			else {
				point_out.x = index;
				return point_out;
			}
		}
		total_length += this_length;
	}
	point_out.x = wcslen(*m_CurrentTag->m_CurrentLine);
	return point_out;
}
void CEditor::Insert(wchar_t* text)
{
	PAUSE_BACKEND()
	LONG64 last_return = -1;
	for (ULONG64 index = 0;; index++) {
		if (text[index] == 0 or text[index] == 10) {
			wchar_t*& this_line = *m_CurrentTag->m_CurrentLine;
			size_t original_length = wcslen(this_line);
			wchar_t* combined_line = new wchar_t[original_length + index - last_return];
			memcpy(combined_line, this_line, original_length * 2);
			memcpy(combined_line + original_length, this_line + last_return + 1, (index - last_return - 1) * 2);
			combined_line[original_length + index - last_return - 1] = 0;
			delete[] this_line;
			if (this_line[index] == 10) {
				// 创建新行
				m_CurrentTag->m_Lines.insert(m_PointerPoint.y, new wchar_t[] {0});
				m_PointerPoint = CPoint(0, m_PointerPoint.y + 1);
				m_CurrentTag->m_CurrentLine++;
			}
			else {
				m_PointerPoint = CPoint(original_length + index - last_return - 1, m_PointerPoint.y);
				break;
			}
			last_return = index;
		}
	}
	m_DragPointerPoint = m_PointerPoint;
}
void CEditor::Delete()
{
	PAUSE_BACKEND()
	if (m_PointerPoint == m_DragPointerPoint) {
		// 删除单个字符
		if (m_PointerPoint.x) {
			// 该行还有字符
			wchar_t* current_line = *m_CurrentTag->m_CurrentLine;
			wchar_t* buffer = new wchar_t[wcslen(current_line)];
			memcpy(buffer, current_line, (size_t)(m_PointerPoint.x - 1) * 2);
			memcpy(buffer + m_PointerPoint.x - 1,
				current_line + m_PointerPoint.x,
				(wcslen(current_line) - m_PointerPoint.x + 1) * 2);
			delete[] current_line;
			*m_CurrentTag->m_CurrentLine = buffer;
			m_PointerPoint.x--;
		}
		else {
			// 删除换行符
			if (m_PointerPoint.y) {
				// 如果不是首行
				m_CurrentTag->m_CurrentLine--;
				m_CurrentTag->m_Lines.pop(m_PointerPoint.y);
				m_PointerPoint.y--;
				m_PointerPoint.x = wcslen(*m_CurrentTag->m_CurrentLine);
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
			wchar_t* buffer = new wchar_t[wcslen(*m_CurrentTag->m_CurrentLine) - (larger - smaller) + 1];
			memcpy(buffer, *m_CurrentTag->m_CurrentLine, smaller * 2);
			memcpy(buffer + smaller, *m_CurrentTag->m_CurrentLine + larger, (wcslen(*m_CurrentTag->m_CurrentLine) - larger + 1) * 2);
			delete[] *(m_CurrentTag->m_CurrentLine);
			*m_CurrentTag->m_CurrentLine = buffer;
			m_PointerPoint.x = smaller;
		}
		else if (m_PointerPoint.y > m_DragPointerPoint.y) {
			// 多行选区（正向）
			for (UINT line_index = m_PointerPoint.y - 1; line_index != m_DragPointerPoint.y; line_index--) {
				m_CurrentTag->m_Lines.pop(m_DragPointerPoint.y + 1);
			}
			m_FullHeight -= (m_PointerPoint.y - m_DragPointerPoint.y) * m_CharSize.cy;
			IndexedList<wchar_t*>::iterator line_before_selection = m_CurrentTag->m_Lines[m_DragPointerPoint.y];
			wchar_t* buffer = new wchar_t[m_DragPointerPoint.x + wcslen(*m_CurrentTag->m_CurrentLine) - m_PointerPoint.x + 2];
			memcpy(buffer, *line_before_selection, m_DragPointerPoint.x * 2);
			memcpy(buffer + m_DragPointerPoint.x,
				*m_CurrentTag->m_CurrentLine + m_PointerPoint.x,
				(wcslen(*m_CurrentTag->m_CurrentLine) - m_PointerPoint.x + 1) * 2);
			delete[] *line_before_selection;
			*line_before_selection = buffer;
			m_CurrentTag->m_Lines.pop(m_DragPointerPoint.y + 1);
			m_CurrentTag->m_CurrentLine = line_before_selection;
			m_PointerPoint = m_DragPointerPoint; // 确保无选区模式
		}
		else {
			// 多行选区（反向）
			for (size_t line_index = m_PointerPoint.y + 1; line_index != m_DragPointerPoint.y; line_index++) {
				m_CurrentTag->m_Lines.pop(m_PointerPoint.y + 1);
			}
			m_FullHeight -= (m_DragPointerPoint.y - m_PointerPoint.y) * m_CharSize.cy;
			IndexedList<wchar_t*>::iterator line_after_selection = m_CurrentTag->m_Lines[m_PointerPoint.y + 1];
			wchar_t* buffer = new wchar_t[m_PointerPoint.x + wcslen(*line_after_selection) - m_DragPointerPoint.x + 2];
			memcpy(buffer, *m_CurrentTag->m_CurrentLine, (size_t)m_PointerPoint.x * 2);
			memcpy(buffer + m_PointerPoint.x,
				*line_after_selection + m_DragPointerPoint.x,
				(wcslen(*line_after_selection) - m_DragPointerPoint.x + 1) * 2);
			delete[] *(m_CurrentTag->m_CurrentLine);
			*m_CurrentTag->m_CurrentLine = buffer;
			m_CurrentTag->m_Lines.pop(m_PointerPoint.y + 1);
			m_DragPointerPoint = m_PointerPoint; // 确保无选区模式
		}
	}
}
void CEditor::MoveView()
{
	PAUSE_BACKEND()
	double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
	double end_line = start_line + (double)m_Height / m_CharSize.cy - 1;
	if (m_PointerPoint.y < start_line) {
		m_VSlider.SetPercentage((double)m_PointerPoint.y * m_CharSize.cy / m_FullHeight);
	}
	else if (m_PointerPoint.y > end_line) {
		m_VSlider.SetPercentage(((double)m_PointerPoint.y * m_CharSize.cy - m_Height + m_CharSize.cy) / m_FullHeight);
	}
	double start_x = m_PercentageHorizontal * m_FullWidth;
	double end_x = start_x + m_Width - m_LineNumberWidth - LINE_NUMBER_OFFSET - m_CharSize.cx;
	LONG width = GET_TEXT_WIDTH(*m_CurrentTag->m_CurrentLine, m_PointerPoint.x);
	if (width < start_x) {
		m_HSlider.SetPercentage((double)width / m_FullWidth);
	}
	else if (width > end_x) {
		m_HSlider.SetPercentage(((double)width - m_Width + m_LineNumberWidth + LINE_NUMBER_OFFSET + m_CharSize.cx) / m_FullWidth);
	}
}
inline void CEditor::CentralView(LONG64 line_index)
{
	double half_height = (double)m_Height / 2;
	m_VSlider.SetPercentage((double)line_index / m_CurrentTag->m_Lines.size() - half_height / m_FullHeight);
	m_HSlider.SetPercentage(0);
}
void CEditor::ParseLine()
{
	CONSTRUCT construct = Construct::parse(*m_CurrentTag->m_CurrentLine);
	if (construct.result) {

	}
}
DWORD CEditor::BackendTasking(LPVOID)
{
	SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
	HWND hWnd = pObject->GetSafeHwnd();
	while (IsWindow(hWnd)) {
		if (not pObject->m_bBackendEnabled) {
			pObject->m_BackendPaused->SetEvent();
		}
		else {
			pObject->RecalcWidth();
		}
	}
	return 0;
}
void CEditor::RecalcWidth()
{
	if (not m_CurrentTag) { return; }
	UINT64 temp = 0;
	CSize size;
	for (IndexedList<wchar_t*>::iterator this_line = m_CurrentTag->m_Lines.begin(); this_line != m_CurrentTag->m_Lines.end(); this_line++) {
		if (m_bBackendEnabled) {
			GetTextExtentPoint32W(m_Source, *this_line, wcslen(*this_line), &size);
			temp = max(temp, size.cx);
		}
		else {
			return;
		}
	}
	if (m_FullWidth != temp + m_CharSize.cx) {
		m_HSlider.DeflateLength(m_FullWidth, temp + m_CharSize.cx);
	}
}
void CEditor::ParseAll()
{
	if (not m_CurrentTag->m_Tokens.size()) {
		for (IndexedList<wchar_t*>::iterator this_line = m_CurrentTag->m_Lines.begin(); this_line != m_CurrentTag->m_Lines.end(); this_line++) {
			CONSTRUCT construct = Construct::parse(*this_line);
			if (construct.result) {
				if (construct.result->error_message) {
					m_CurrentTag->m_Tokens.append(new TOKEN[]{ 0 });
				}
				else {
					m_CurrentTag->m_Tokens.append(construct.result->tokens);
				}
			}
			else {
				m_CurrentTag->m_Tokens.append(new TOKEN[]{ 0 });
			}
		}
	}
}
void CEditor::UpdateCandidates()
{

}