#include "pch.h"
#define PROCESS_HALTED 2

BEGIN_MESSAGE_MAP(CConsoleOutput, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()
CConsoleOutput::CConsoleOutput()
{
	m_Width = m_Height = 0;
	m_FullLength = 0;
	m_Percentage = 0.0;
	m_bDrag = false;
	m_TotalHeightUnit = 0;

	pObject = this;
}
CConsoleOutput::~CConsoleOutput()
{
	for (IndexedList<LINE>::iterator iter = m_Lines.begin(); iter != m_Lines.end(); iter++) {
		delete[] iter->line;
	}
}
BOOL CConsoleOutput::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.lpszClass = AfxRegisterWndClass(
		CS_HREDRAW | CS_VREDRAW,
		LoadCursor(NULL, IDC_IBEAM),
		(HBRUSH)(COLOR_WINDOW + 1)
	);
	return CWnd::PreCreateWindow(cs);
}
int CConsoleOutput::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 创建渲染文字源
	m_Source.CreateCompatibleDC(&ScreenDC);
	m_Selection.CreateCompatibleDC(&ScreenDC);
	m_Source.SelectObject(CConsole::font);
	// 计算字符大小
	CRect rect(0, 0, 0, 0);
	m_Source.DrawTextW(L" ", 1, &rect, DT_CALCRECT);
	m_CharSize = CSize(rect.right, rect.bottom);
	m_Source.SetTextColor(RGB(255, 255, 255));
	m_Source.SetBkColor(RGB(0, 0, 0));
	// 创建滚动条
	m_Slider.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_Slider.SetRatio(1.0);
	m_Slider.SetCallback(VerticalCallback);
	// 初始化缓存
	ClearBuffer();

	return 0;
}
void CConsoleOutput::OnPaint()
{
	m_Lock.lock(); // 保证m_Source并未用于文本长度估算
	CPaintDC dc(this);
	MemoryDC.BitBlt(0, 0, m_Width, m_Height, &m_Selection, 0, 0, SRCCOPY);
	MemoryDC.TransparentBlt(0, 0, m_Width, m_Height, &m_Source, 0, 0, m_Width, m_Height, 0);
	dc.BitBlt(0, 0, m_Width, m_Height, &MemoryDC, 0, 0, SRCCOPY);
	m_Lock.unlock();
}
BOOL CConsoleOutput::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CConsoleOutput::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	m_Width = cx - 10;
	m_Height = cy;

	HBITMAP hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_Height);
	HGLOBAL hOldBitmap = SelectObject(m_Source, hBitmap);
	DeleteObject(hOldBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_Height);
	hOldBitmap = SelectObject(m_Selection, hBitmap);
	DeleteObject(hOldBitmap);
	RecalcTotalHeight();
	ArrangeText();
	ArrangePointer();
	ArrangeSelection();
	// 更新滚动条
	CRect rect(cx - 10, 0, cx, cy);
	m_Slider.MoveWindow(&rect);
	UpdateSlider();
}
void CConsoleOutput::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDrag) {
		TranslatePointer(point);
		ArrangePointer();
		ArrangeSelection();
		REDRAW_WINDOW();
	}
}
void CConsoleOutput::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	SetFocus();
	TranslatePointer(point);
	m_DragPointerPoint = m_PointerPoint;
	m_bDrag = true;
	ArrangePointer();
	ArrangeSelection();
	REDRAW_WINDOW();
}
void CConsoleOutput::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_bDrag = false;
}
BOOL CConsoleOutput::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	m_Slider.SetPercentage(m_Percentage - (double)zDelta / 120 / m_TotalHeightUnit);
	return TRUE;
}
void CConsoleOutput::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	::CreateCaret(m_hWnd, NULL, 1, m_CharSize.cy);
	ArrangePointer();
}
void CConsoleOutput::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	DestroyCaret();
}
void CConsoleOutput::AppendText(char* buffer, DWORD transferred)
{
	m_Lock.lock();
	size_t size = (size_t)MultiByteToWideChar(CP_ACP, MB_COMPOSITE, buffer, transferred, nullptr, 0);
	wchar_t* wchar_buffer = new wchar_t[size + 1];
	MultiByteToWideChar(CP_ACP, MB_COMPOSITE, buffer, transferred, wchar_buffer, size);
	wchar_buffer[size] = 0;
	CRect rect(0, 0, m_Width, 0);
	LONG64 last_return = -1;
	for (ULONG64 index = 0;; index++) {
		if (wchar_buffer[index] == 10 or wchar_buffer[index] == 0) {
			LINE& last_line = *m_Lines[-1];
			size_t original_length = wcslen(last_line.line);
			wchar_t* combined_line = new wchar_t[original_length + index - last_return];
			memcpy(combined_line, last_line.line, original_length * 2);
			memcpy(combined_line + original_length, wchar_buffer + last_return + 1, (index - last_return - 1) * 2);
			combined_line[original_length + index - last_return - 1] = 0;
			delete[] last_line.line;
			USHORT orginal_height_unit = last_line.height_unit;
			last_line = EncodeLine(combined_line);
			last_line.accumulated_height_unit = m_TotalHeightUnit - orginal_height_unit;
			m_TotalHeightUnit = (size_t)last_line.accumulated_height_unit + last_line.height_unit;
			if (wchar_buffer[index] == 10) {
				// 创建新行
				m_Lines.append(LINE{ new wchar_t[] {0}, 0, 1, m_TotalHeightUnit });
				m_TotalHeightUnit++;
				m_DragPointerPoint = m_PointerPoint = CPoint(0, m_Lines.size() - 1);
			}
			else {
				m_DragPointerPoint = m_PointerPoint = CPoint(original_length + index - last_return - 1, m_Lines.size() - 1);
				break;
			}
			last_return = index;
		}
	}
	m_FullLength = m_TotalHeightUnit * m_CharSize.cy;
	delete[] wchar_buffer;
	m_Lock.unlock();
	UpdateSlider();
	m_Slider.SetPercentage(1.0);
}
void CConsoleOutput::AppendInput(wchar_t* input, DWORD count)
{
	IndexedList<LINE>::iterator last_line = m_Lines[-1];
	size_t original_length = wcslen(last_line->line);
	wchar_t* combined_line = new wchar_t[original_length + count + 1];
	memcpy(combined_line, last_line->line, original_length * 2);
	memcpy(combined_line + original_length, input, (size_t)count * 2);
	combined_line[original_length + count] = 0;
	delete[] last_line->line;
	USHORT original_height_unit = last_line->height_unit;
	*last_line = EncodeLine(combined_line);
	USHORT difference = last_line->height_unit - original_height_unit;
	last_line->height_unit += difference;
	m_Lines.append(LINE{ new wchar_t[] {0}, 0, 1, m_TotalHeightUnit + difference });
	m_TotalHeightUnit += difference + 1;
}
void CConsoleOutput::ClearBuffer()
{
	for (IndexedList<LINE>::iterator iter = m_Lines.begin(); iter != m_Lines.end(); iter++) {
		delete[] iter->line;
	}
	m_Lines.clear();
	m_Lines.append(LINE{ new wchar_t[] {0}, 0, 1, 0 });
	m_CurrentLine = m_Lines[0];
	m_TotalHeightUnit = 1;
	m_PointerPoint = m_DragPointerPoint = CPoint(0, 0);
	ArrangeText();
	ArrangePointer();
	ArrangeSelection();
	UpdateSlider();
	REDRAW_WINDOW();
}
void CConsoleOutput::VerticalCallback(double percentage)
{
	pObject->m_Percentage = percentage;
	pObject->ArrangeText();
	pObject->ArrangePointer();
	pObject->ArrangeSelection();
	pObject->Invalidate(FALSE);
}
inline void CConsoleOutput::SetListState(bool state)
{
	m_Lines.set_construction(state);
}
CConsoleOutput::LINE CConsoleOutput::EncodeLine(wchar_t* line)
{
	LINE line_out{};
	line_out.line = line;
	line_out.length = wcslen(line);
	if (m_Width <= 0) {
		line_out.height_unit = 1;
	}
	else {
		line_out.height_unit = 0;
		ULONG offset = 0;
		while (offset != line_out.length) {
			int number;
			static CSize size;
			GetTextExtentExPointW(m_Source, line + offset, line_out.length - offset, m_Width, &number, NULL, &size);
			offset += number;
			line_out.height_unit += 1;
		}
	}
	return line_out;
}
void CConsoleOutput::RecalcTotalHeight()
{
	if (m_Width <= 0) { return; }
	m_Lock.lock();
	m_TotalHeightUnit = 0;
	ULONG index = 0;
	for (IndexedList<LINE>::iterator this_line = m_Lines.begin(); this_line != m_Lines.end(); this_line++) {
		this_line->height_unit = 0;
		this_line->accumulated_height_unit = m_TotalHeightUnit;
		ULONG offset = 0;
		while (offset != this_line->length) {
			int number;
			static CSize size;
			GetTextExtentExPointW(m_Source, this_line->line + offset, this_line->length - offset, m_Width, &number, NULL, &size);
			offset += number;
			this_line->height_unit++;
			m_TotalHeightUnit++;
		}
		if (this_line->length == 0) {
			this_line->height_unit = 1;
			m_TotalHeightUnit++;
		}
		index++;
	}
	m_Lock.unlock();
}
void CConsoleOutput::ArrangeText()
{
	if (m_Width <= 0) { return; }
	m_Lock.lock();
	// 计算起始行
	m_Source.PatBlt(0, 0, m_Width, m_Height, BLACKNESS);
	LONG cy = m_Percentage * m_TotalHeightUnit * m_CharSize.cy;
	IndexedList<LINE>::iterator this_line = SearchStartLine(cy);
	CRect rect(
		0,
		cy,
		m_Width,
		cy + m_CharSize.cy
	);
	// 计算自动换行
	USHORT width_unit = m_Width / m_CharSize.cx;
	for (; this_line != m_Lines.end(); this_line++) {
		ULONG offset = 0;
		while (offset != this_line->length) {
			int number;
			static CSize size;
			if (not GetTextExtentExPointW(m_Source, this_line->line + offset, this_line->length - offset, m_Width, &number, NULL, &size)) {
				break;
			}
			m_Source.ExtTextOutW(0, rect.top, ETO_CLIPPED, &rect, this_line->line + offset, number, NULL);
			offset += number;
			rect.top = rect.bottom;
			rect.bottom += m_CharSize.cy;
			if (rect.top > m_Height) {
				m_Lock.unlock();
				return;
			}
		}
	}
	m_Lock.unlock();
}
void CConsoleOutput::ArrangePointer()
{
	if (m_Width <= 0) { return; }
	double start_line = m_Percentage * m_TotalHeightUnit;
	double difference = (double)m_CurrentLine->accumulated_height_unit - start_line;
	ULONG offset = 0;
	ULONG remain = m_PointerPoint.x;
	USHORT count = 0;
	while (true) {
		CSize size;
		int number;
		LINE& current_line = *m_Lines[m_PointerPoint.y];
		GetTextExtentExPointW(m_Source, current_line.line + offset, current_line.length - offset, m_Width, &number, NULL, &size);
		if (number >= remain) {
			GetTextExtentPoint32W(m_Source, m_CurrentLine->line + offset, remain, &size);
			CPoint point(size.cx, (difference + count) * m_CharSize.cy);
			SetCaretPos(point);
			ShowCaret();
			return;
		}
		else {
			offset += number;
			remain -= number;
			count++;
		}
	}
}
void CConsoleOutput::ArrangeSelection()
{
	// 单行选区绘制函数
	auto fill_selection = [this](LINE line, UINT start, UINT end, LONG relative) -> void {
		ULONG offset = 0;
		CRect rect(0, relative, 0, 0);
		while (offset != line.length) {
			static CSize size;
			int number;
			GetTextExtentExPointW(this->m_Source, line.line + offset, line.length - offset, m_Width, &number, NULL, &size);
			if (offset <= start and offset + number >= start) {
				// 选区起始行
				this->m_Source.DrawTextW(line.line + offset, start - offset, &rect, DT_CALCRECT);
				LONG start_pixel = rect.right;
				this->m_Source.DrawTextW(line.line + offset, min(number, end - offset), &rect, DT_CALCRECT); // min函数来兼容起始行也是结束行的情况
				if (end == line.length + 1 and number >= end - offset - 1) {
					// end比长度大一表示需要绘制换行符，大小判断用于仅让最后一行生效
					rect.right += this->m_CharSize.cx;
				}
				rect.left = start_pixel;
				this->m_Selection.FillRect(&rect, &CConsole::selectionColor);
			}
			else if (offset < end and offset + number > end) {
				// 选区结束行
				this->m_Source.DrawTextW(line.line + offset, end - offset, &rect, DT_CALCRECT);
				this->m_Selection.FillRect(&rect, &CConsole::selectionColor);
			}
			else if (start < offset and end > offset + number) {
				// 选区中间行（包含start为0，end为长度的情况）
				this->m_Source.DrawTextW(line.line + offset, number, &rect, DT_CALCRECT);
				if (end == line.length + 1 and number >= end - offset - 1) {
					rect.right += this->m_CharSize.cx;
				}
				this->m_Selection.FillRect(&rect, &CConsole::selectionColor);
			}
			// 若都不满足，则该行没有选区
			rect = CRect(0, rect.top + this->m_CharSize.cy, 0, 0);
			offset += number;
		}
	};
	// 分离选区绘制工作
	CRect rect(0, 0, m_Width, m_Height);
	m_Selection.FillRect(&rect, pGreyBlackBrush);
	double start_line = m_Percentage * m_TotalHeightUnit;
	LONG relative = (m_CurrentLine->accumulated_height_unit - start_line) * m_CharSize.cy;
	if (m_PointerPoint.y == m_DragPointerPoint.y) {
		if (m_PointerPoint.x == m_DragPointerPoint.x) {
			return;
		}
		else {
			UINT smaller = min(m_PointerPoint.x, m_DragPointerPoint.x);
			UINT larger = max(m_PointerPoint.x, m_DragPointerPoint.x);
			fill_selection(*m_Lines[m_PointerPoint.y], smaller, larger, relative);
		}
	}
	else if (m_PointerPoint.y > m_DragPointerPoint.y) {
		//正向选区
		IndexedList<LINE>::iterator this_line = m_Lines[m_PointerPoint.y];
		fill_selection(*this_line, 0, m_PointerPoint.x, relative);
		this_line--;
		relative -= this_line->height_unit * m_CharSize.cy;
		for (UINT index = m_PointerPoint.y - 1; index != m_DragPointerPoint.y; index--) {
			fill_selection(*this_line, 0, this_line->length + 1, relative);
			this_line--;
			relative -= this_line->height_unit * m_CharSize.cy;
		}
		fill_selection(*this_line, m_DragPointerPoint.x, this_line->length + 1, relative);
	}
	else {
		// 逆向选区
		IndexedList<LINE>::iterator this_line = m_Lines[m_PointerPoint.y];
		fill_selection(*this_line, m_PointerPoint.x, this_line->length + 1, relative);
		relative += this_line->height_unit * m_CharSize.cy;
		this_line++;
		for (UINT index = m_PointerPoint.y + 1; index != m_DragPointerPoint.y; index++) {
			fill_selection(*this_line, 0, this_line->length + 1, relative);
			relative += this_line->height_unit * m_CharSize.cy;
			this_line++;
		}
		fill_selection(*this_line, 0, m_DragPointerPoint.x, relative);
	}
}
inline void CConsoleOutput::UpdateSlider()
{
	m_Slider.SetRatio((double)m_Height / (m_TotalHeightUnit * m_CharSize.cy));
}
void CConsoleOutput::TranslatePointer(CPoint point)
{
	point.x = max(point.x, 0);
	// 计算行指针
	LONG cy = point.y + m_Percentage * m_TotalHeightUnit * m_CharSize.cy;
	SearchStartLine(cy, &m_PointerPoint.y);
	m_CurrentLine = m_Lines[m_PointerPoint.y];
	// 计算列指针
	USHORT line_count = -cy / m_CharSize.cy;
	size_t total_length = 0;
	ULONG offset = 0;
	CSize size;
	int number;
	while (line_count) {
		GetTextExtentExPointW(m_Source, m_CurrentLine->line + offset, m_CurrentLine->length - offset, m_Width, &number, NULL, &size);
		offset += number;
		line_count--;
	}
	GetTextExtentExPointW(m_Source, m_CurrentLine->line + offset, m_CurrentLine->length - offset, point.x, &number, NULL, &size);
	GetTextExtentPoint32W(m_Source, m_CurrentLine->line + offset, number, &size);
	if (offset + number == m_CurrentLine->length) {
		// 最后一个字符
		m_PointerPoint.x = m_CurrentLine->length;
	}
	else {
		// 判断指针是否离右边更近
		point.x -= size.cx;
		GetTextExtentPoint32W(m_Source, m_CurrentLine->line + offset + number, 1, &size);
		if (point.x > size.cx / 2) {
			m_PointerPoint.x = offset + number + 1;
		}
		else {
			m_PointerPoint.x = offset + number;
		}
	}
}
IndexedList<CConsoleOutput::LINE>::iterator CConsoleOutput::SearchStartLine(LONG& cy, LONG* index)
{
	UINT target_height_unit = max(min(cy / m_CharSize.cy, (LONG)m_Lines[-1]->accumulated_height_unit), 0);
	// 二分查找
	UINT low = 0;
	UINT high = m_Lines.size();
	UINT middle;
	IndexedList<LINE>::iterator element;
	while (low != high) {
		middle = (low + high) / 2;
		element = m_Lines[middle];
		if (element->accumulated_height_unit < target_height_unit) {
			if (element->accumulated_height_unit + element->height_unit > target_height_unit) {
				break;
			}
			else {
				low = middle;
			}
		}
		else if (element->accumulated_height_unit == target_height_unit) {
			break;
		}
		else {
			high = middle;
		}
	}
	cy = element->accumulated_height_unit * m_CharSize.cy - cy;
	if (index) {
		*index = middle;
	}
	return element;
}

BEGIN_MESSAGE_MAP(CConsoleInput, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_CHAR()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()
CConsoleInput::CConsoleInput()
{
	m_bFocus = false;
	m_bCaret = false;
	m_Width = 0;
	m_FullLength = 0;
	m_cchBuffer = 0;
	m_Buffer = new wchar_t[0];
	m_Percentage = 0.0;
	m_CharHeight = 0;
	m_Offset = 0;
	m_cchDragPointer = m_cchPointer = 0;
	m_bDrag = false;
}
CConsoleInput::~CConsoleInput()
{

}
int CConsoleInput::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 创建源
	m_Source.CreateCompatibleDC(&ScreenDC);
	m_Selection.CreateCompatibleDC(&ScreenDC);
	m_Source.SelectObject(CConsole::font);
	// 计算字符大小
	CRect rect;
	m_Source.DrawTextW(L">>>", -1, &rect, DT_CALCRECT);
	m_CharHeight = rect.bottom;
	m_Offset = rect.right + 15;
	m_Source.SetTextColor(RGB(255, 255, 255));
	m_Source.SetBkMode(TRANSPARENT);

	return 0;
}
void CConsoleInput::OnPaint()
{
	CPaintDC dc(this);
	MemoryDC.BitBlt(0, 0, m_Width, m_CharHeight, &m_Selection, 0, 0, SRCCOPY);
	MemoryDC.TransparentBlt(0, 0, m_Width, m_CharHeight, &m_Source, 0, 0, m_Width, m_CharHeight, 0);
	dc.BitBlt(m_Offset, 0, m_Width, m_CharHeight, &MemoryDC, 0, 0, SRCCOPY);
}
BOOL CConsoleInput::OnEraseBkgnd(CDC* pDC)
{
	pDC->SelectObject(CConsole::font);
	pDC->SetTextColor(RGB(255, 255, 255));
	pDC->SetBkMode(TRANSPARENT);
	CRect rect(5, 0, m_Offset, m_CharHeight);
	pDC->DrawTextW(L">>>", -1, &rect, 0);
	return TRUE;
}
void CConsoleInput::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	m_Width = cx;
	
	HBITMAP hBitmap = CreateCompatibleBitmap(ScreenDC, cx - m_Offset, cy);
	HGLOBAL hOldBitmap = SelectObject(m_Source, hBitmap);
	DeleteObject(hOldBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, cx, cy);
	hOldBitmap = SelectObject(m_Selection, hBitmap);
	DeleteObject(hOldBitmap);
	ArrangeText();
	ArrangePointer();
	ArrangeSelection();
}
void CConsoleInput::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar) {
	case L'\r':
	{
		if (CConsole::pObject->SendInput(m_Buffer, m_cchBuffer)) {
			// 成功发送输入
			delete[] m_Buffer;
			m_Buffer = new wchar_t[0];
			m_cchBuffer = 0;
			m_cchPointer = 0;
			m_cchDragPointer = 0;
		}
		break;
	}
	case L'\b':
	{
		Delete();
		break;
	}
	default:
	{
		if (m_cchDragPointer != m_cchPointer) {
			Delete();
		}
		wchar_t* new_buffer = new wchar_t[m_cchBuffer + 1];
		memcpy(new_buffer, m_Buffer, m_cchBuffer * 2);
		new_buffer[m_cchBuffer] = nChar;
		delete[] m_Buffer;
		m_Buffer = new_buffer;
		m_cchBuffer++;
		m_cchPointer++;
		m_cchDragPointer++;
	}
	}
	ArrangeText();
	ArrangePointer();
	ArrangeSelection();
	REDRAW_WINDOW();
}
void CConsoleInput::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDrag) {
		m_cchPointer = TranslatePointer(point);
		ArrangePointer();
		ArrangeSelection();
		REDRAW_WINDOW();
	}
	else {
		if (point.x > m_Offset) {
			SetCursor(LoadCursorW(NULL, IDC_IBEAM));
		}
		else {
			SetCursor(LoadCursorW(NULL, IDC_ARROW));
		}
	}
}
void CConsoleInput::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	SetFocus();
	m_cchDragPointer = m_cchPointer = TranslatePointer(point);
	if (point.x > m_Offset) {
		m_bDrag = true;
	}
	ArrangePointer();
	ArrangeSelection();
	REDRAW_WINDOW();
}
void CConsoleInput::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_bDrag = false;
}
BOOL CConsoleInput::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	return TRUE;
}
void CConsoleInput::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	::CreateCaret(m_hWnd, NULL, 1, m_CharHeight);
	m_bFocus = true;
	ArrangePointer();
}
void CConsoleInput::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	m_bFocus = false;
	m_bCaret = false;
	DestroyCaret();
}
inline int CConsoleInput::GetCharHeight()
{
	return m_CharHeight;
}
void CConsoleInput::ArrangeText()
{
	m_Source.PatBlt(0, 0, m_Width, m_CharHeight, BLACKNESS);
	double offset = -m_Percentage * m_FullLength;
	CRect rect(offset, 0, m_Width, m_CharHeight);
	m_Source.DrawTextW(m_Buffer, m_cchBuffer, &rect, 0);
}
void CConsoleInput::ArrangePointer()
{
	if (not m_bFocus) { return; }
	CSize size;
	GetTextExtentPoint32W(m_Source, m_Buffer, m_cchBuffer, &size);
	CPoint point(size.cx - m_Percentage * m_FullLength + m_Offset, 0);
	SetCaretPos(point);
	if (point.x < m_Offset) {
		if (m_bCaret) {
			m_bCaret = false;
			HideCaret();
		}
	}
	else {
		if (not m_bCaret) {
			m_bCaret = true;
			ShowCaret();
		}
	}
}
void CConsoleInput::ArrangeSelection()
{
	CRect rect(0, 0, m_Width, m_CharHeight);
	m_Selection.FillRect(&rect, pGreyBlackBrush);
	UINT smaller = min(m_cchDragPointer, m_cchPointer);
	UINT larger = max(m_cchDragPointer, m_cchPointer);
	CSize size;
	GetTextExtentPoint32W(m_Source, m_Buffer, smaller, &size);
	rect.left = size.cx;
	GetTextExtentPoint32W(m_Source, m_Buffer, larger, &size);
	rect.right = size.cx;
	m_Selection.FillRect(&rect, &CConsole::selectionColor);
}
UINT CConsoleInput::TranslatePointer(CPoint point)
{
	point.x -= m_Offset;
	point.x = max(point.x, 0);
	CSize size;
	LONG total_length = 0;
	for (UINT index = 0; index != m_cchBuffer; index++) {
		LONG this_length = GetTextExtentPoint32W(m_Source, m_Buffer, index, &size) - total_length;
		if (total_length + this_length > point.x) {
			if (point.x > total_length + this_length / 2) {
				return index + 1;
			}
			else {
				return index;
			}
		}
		else {
			total_length += this_length;
		}
	}
	return m_cchBuffer;
}
void CConsoleInput::Delete()
{
	if (m_cchPointer == m_cchDragPointer) {
		// 删除字符
		if (m_cchPointer == 0) {
			return;
		}
		wchar_t* new_buffer = new wchar_t[m_cchBuffer - 1];
		memcpy(new_buffer, m_Buffer, (size_t)(m_cchPointer - 1) * 2);
		memcpy(new_buffer + m_cchPointer - 1, m_Buffer + m_cchPointer, (size_t)(m_cchBuffer - m_cchPointer) * 2);
		delete[] m_Buffer;
		m_Buffer = new_buffer;
		m_cchBuffer--;
		m_cchDragPointer = --m_cchPointer;
	}
	else {
		// 删除选区
		UINT smaller = min(m_cchPointer, m_cchDragPointer);
		UINT larger = max(m_cchPointer, m_cchDragPointer);
		wchar_t* new_buffer = new wchar_t[m_cchBuffer - (larger - smaller)];
		memcpy(new_buffer, m_Buffer, (size_t)smaller * 2);
		memcpy(new_buffer + smaller, m_Buffer + larger, (m_cchBuffer - larger) * 2);
		delete[] m_Buffer;
		m_Buffer = new_buffer;
		m_cchBuffer -= (size_t)(larger - smaller);
		m_cchDragPointer = m_cchPointer = smaller;
	}
}

BEGIN_MESSAGE_MAP(CConsole, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
END_MESSAGE_MAP()
CConsole::CConsole()
{
	m_Pipes = PIPE{};
	m_SI = STARTUPINFO{};
	m_PI = PROCESS_INFORMATION{};
	m_bRun = false;
	m_bShow = false;
	m_DebugHandle = NULL;
	pObject = this;
}
CConsole::~CConsole()
{
}
int CConsole::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 创建字体
	//AddFontResourceW(L"YAHEI CONSOLAS HYBRID.TTF");
	font.CreateFontW(20, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	// 创建选区背景刷
	selectionColor.CreateSolidBrush(RGB(38, 79, 120));
	// 创建组件
	m_Output.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_Input.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	// 更新控制区按钮
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_HALT)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPOVER)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPOUT)->SetState(false);

	return 0;
}
BOOL CConsole::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CConsole::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	CRect rect(2, 2, cx, cy - 4 - m_Input.GetCharHeight());
	m_Output.MoveWindow(&rect);
	rect = CRect(2, cy - 2 - m_Input.GetCharHeight(), cx - 2, cy - 2);
	m_Input.MoveWindow(&rect);
}
void CConsole::OnDebugRun()
{
	// 准备进程启动选项
	InitSubprocess(false);
	ZeroMemory(&m_SI, sizeof(m_SI));
	m_SI.cb = sizeof(m_SI);
	m_SI.dwFlags = STARTF_USESTDHANDLES;
	m_SI.hStdInput = m_Pipes.stdin_read;
	m_SI.hStdOutput = m_Pipes.stdout_write;
	m_SI.hStdError = m_Pipes.stderr_write;
	// 启动子进程
	ZeroMemory(&m_PI, sizeof(m_PI));
	wchar_t* other_opt = new wchar_t[1];
	other_opt[0] = 0;
	size_t command_length = 33 + wcslen(CTagPanel::pObject->GetCurrentTag()->GetPath()) + wcslen(other_opt);
	wchar_t* command = new wchar_t[command_length] {};
	static const wchar_t* format = L"\"Pseudocode Interpreter.exe\" \"%s\" %s";
	StringCchPrintfW(command, command_length, format, CTagPanel::pObject->GetCurrentTag()->GetPath(), other_opt);
	command[command_length - 1] = 0;
	CreateProcessW(0, command, 0, 0, true, CREATE_NO_WINDOW, 0, 0, &m_SI, &m_PI);
	delete[] command;
	// 监听管道
	CreateThread(NULL, NULL, CConsole::Join, nullptr, NULL, NULL);
}
void CConsole::OnDebugHalt()
{
	TerminateProcess(m_PI.hProcess, PROCESS_HALTED);
}
void CConsole::OnDebugDebug()
{
	// 准备进程启动选项
	InitSubprocess(true);
	ZeroMemory(&m_SI, sizeof(m_SI));
	m_SI.cb = sizeof(m_SI);
	m_SI.dwFlags = STARTF_USESTDHANDLES;
	m_SI.hStdInput = m_Pipes.stdin_read;
	m_SI.hStdOutput = m_Pipes.stdout_write;
	m_SI.hStdError = m_Pipes.stderr_write;
	// 启动子进程
	ZeroMemory(&m_PI, sizeof(m_PI));
	wchar_t* other_opt = new wchar_t[1];
	other_opt[0] = 0;
	size_t command_length = 40 + wcslen(CTagPanel::pObject->GetCurrentTag()->GetPath()) + wcslen(other_opt);
	wchar_t* command = new wchar_t[command_length] {};
	static const wchar_t* format = L"\"Pseudocode Interpreter.exe\" \"%s\" /debug %s";
	StringCchPrintfW(command, command_length, format, CTagPanel::pObject->GetCurrentTag()->GetPath(), other_opt);
	command[command_length - 1] = 0;
	CreateProcessW(0, command, 0, 0, true, CREATE_NO_WINDOW, 0, 0, &m_SI, &m_PI);
	CMainFrame::pObject->UpdateStatus(true, new wchar_t[] {L"本地伪代码解释器已启动"});
	delete[] command;
	// 监听管道
	m_DebugHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, m_PI.dwProcessId);
	CreateThread(NULL, NULL, CConsole::JoinDebug, nullptr, NULL, NULL);
}
void CConsole::OnDebugContinue()
{
	CEditor::pObject->SendMessageW(WM_STEP, 0, -1);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_CONTINUE)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	SendSignal(SIGNAL_EXECUTION, EXECUTION_CONTINUE, 0);
}
void CConsole::OnDebugStepin()
{
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_CONTINUE)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	SendSignal(SIGNAL_EXECUTION, EXECUTION_STEPIN, 0);
}
void CConsole::OnDebugStepover()
{
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_CONTINUE)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	SendSignal(SIGNAL_EXECUTION, EXECUTION_STEPOVER, 0);
}
void CConsole::OnDebugStepout()
{
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_CONTINUE)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
	SendSignal(SIGNAL_EXECUTION, EXECUTION_STEPOUT, 0);
}
void CConsole::SetBreakpoint(ULONG64 line_index, bool state)
{
	if (state) {
		SendSignal(SIGNAL_BREAKPOINT, BREAKPOINT_ADD, line_index);
	}
	else {
		SendSignal(SIGNAL_BREAKPOINT, BREAKPOINT_DELETE, line_index);
	}
}
void CConsole::InitSubprocess(bool debug_mode)
{
	// 更新组件状态
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_RUN)->SetState(false);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_HALT)->SetState(true);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_DEBUG)->SetState(false);
	CMainFrame::pObject->UpdateStatus(true, new wchar_t[] {L"本地伪代码解释器已启动"});
	// 创建管道
	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = true;
	sa.lpSecurityDescriptor = 0;
	CreatePipe(&m_Pipes.stdin_read, &m_Pipes.stdin_write, &sa, 0);
	CreatePipe(&m_Pipes.stdout_read, &m_Pipes.stdout_write, &sa, 0);
	CreatePipe(&m_Pipes.stderr_read, &m_Pipes.stderr_write, &sa, 0);
	if (debug_mode) {
		CreatePipe(&m_Pipes.signal_in_read, &m_Pipes.signal_in_write, &sa, 20);
		CreatePipe(&m_Pipes.signal_out_read, &m_Pipes.signal_out_write, &sa, 20);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_DEBUG)->ShowWindow(SW_HIDE);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_CONTINUE)->ShowWindow(SW_SHOW);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_CONTINUE)->SetState(false);
	}
	// 清理控制台
	m_Output.ClearBuffer();
	m_Output.SetListState(true);
	m_bRun = true;
}
void CConsole::ExitSubprocess(UINT exit_code)
{
	// 关闭管道
	CloseHandle(m_Pipes.stdin_read);
	CloseHandle(m_Pipes.stdin_write);
	CloseHandle(m_Pipes.stdout_read);
	CloseHandle(m_Pipes.stdout_write);
	CloseHandle(m_Pipes.stderr_read);
	CloseHandle(m_Pipes.stderr_write);
	if (m_Pipes.signal_in_read) {
		CloseHandle(m_Pipes.signal_in_read);
		CloseHandle(m_Pipes.signal_in_write);
		CloseHandle(m_Pipes.signal_out_read);
		CloseHandle(m_Pipes.signal_out_write);
	}
	m_Pipes = PIPE{};
	// 更新组件状态
	try {
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_RUN)->SetState(true);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_HALT)->SetState(false);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_DEBUG)->ShowWindow(SW_SHOW);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_DEBUG)->SetState(true);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_CONTINUE)->ShowWindow(SW_HIDE);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(false);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPOVER)->SetState(false);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPOUT)->SetState(false);

		switch (exit_code) {
		case 0:
			CMainFrame::pObject->UpdateStatus(false, new wchar_t[] {L"代码运行结束"});
			break;
		case -1:
			CMainFrame::pObject->UpdateStatus(false, new wchar_t[] {L"代码运行异常退出"});
			break;
		case PROCESS_HALTED:
			CMainFrame::pObject->UpdateStatus(false, new wchar_t[] {L"运行已被强制终止"});
			break;
		}
		CEditor::pObject->PostMessageW(WM_STEP, 0, -1);
	}
	catch (int) { /*当窗口被关闭时强制退出*/ }
	m_Output.SetListState(false);
	CCallStack::pObject->LoadCallStack(nullptr);
	CVariableTable::pObject->LoadGlobal(nullptr);
	m_bRun = false;
}
DWORD CConsole::Join(LPVOID lpParameter)
{
	char* buffer = new char[0];
	DWORD size_allocated = 0;
	DWORD state = STILL_ACTIVE;
	DWORD read = 0;
	DWORD remain_stdout = 0;
	DWORD remain_stderr = 0;
	while (true) {
		PeekNamedPipe(pObject->m_Pipes.stdout_read, 0, 0, 0, &remain_stdout, 0);
		PeekNamedPipe(pObject->m_Pipes.stderr_read, 0, 0, 0, &remain_stderr, 0);
		if (remain_stdout or remain_stderr) {
			if (remain_stdout) {
				if (remain_stdout > size_allocated) {
					delete[] buffer;
					buffer = new char[remain_stdout];
					size_allocated = remain_stdout;
				}
				if (not ReadFile(pObject->m_Pipes.stdout_read, buffer, remain_stdout, 0, 0)) {
					throw L"管道读取异常";
				}
				pObject->m_Output.AppendText(buffer, remain_stdout);
			}
			if (remain_stderr) {
				if (remain_stderr > size_allocated) {
					delete[] buffer;
					buffer = new char[remain_stderr];
					size_allocated = remain_stderr;
				}
				if (not ReadFile(pObject->m_Pipes.stderr_read, buffer, remain_stderr, 0, 0)) {
					throw L"管道读取异常";
				}
				pObject->m_Output.AppendText(buffer, remain_stderr);
			}
		}
		else {
			GetExitCodeProcess(pObject->m_PI.hProcess, &state);
			if (state != STILL_ACTIVE) {
				break;
			}
		}
	}
	delete[] buffer;
	// 结束执行
	pObject->ExitSubprocess(state);
	return 0;
}
DWORD CConsole::JoinDebug(LPVOID lpParameter)
{
	// 传递管道句柄
	WriteFile(pObject->m_Pipes.stdin_write, &pObject->m_Pipes.signal_in_read, 8, nullptr, nullptr);
	// 建立连接
	SendSignal(SIGNAL_CONNECTION, CONNECTION_PIPE, (LPARAM)pObject->m_Pipes.signal_out_write);
	// 预设断点
	for (std::list<BREAKPOINT>::iterator iter = CEditor::m_Breakpoints.begin(); iter != CEditor::m_Breakpoints.end(); iter++) {
		SendSignal(SIGNAL_BREAKPOINT, MAKELPARAM(BREAKPOINT_ADD, BREAKPOINT_TYPE_NORMAL), iter->line_index);
	}
	// 启动执行
	SendSignal(SIGNAL_CONNECTION, CONNECTION_START, 0);
	// 准备监听
	char* buffer = new char[20];
	DWORD size_allocated = 20;
	DWORD state = STILL_ACTIVE;
	DWORD read = 0;
	DWORD remain_stdout = 0;
	DWORD remain_stderr = 0;
	DWORD remain_signal = 0;
	while (true) {
		PeekNamedPipe(pObject->m_Pipes.stdout_read, 0, 0, 0, &remain_stdout, 0);
		PeekNamedPipe(pObject->m_Pipes.stderr_read, 0, 0, 0, &remain_stderr, 0);
		PeekNamedPipe(pObject->m_Pipes.signal_out_read, 0, 0, 0, &remain_signal, 0);
		if (remain_stdout or remain_stderr or remain_signal) {
			if (remain_stdout) {
				if (remain_stdout > size_allocated) {
					delete[] buffer;
					buffer = new char[remain_stdout];
					size_allocated = remain_stdout;
				}
				if (not ReadFile(pObject->m_Pipes.stdout_read, buffer, remain_stdout, 0, 0)) {
					throw L"管道读取异常";
				}
				pObject->m_Output.AppendText(buffer, remain_stdout);
			}
			if (remain_stderr) {
				if (remain_stderr > size_allocated) {
					delete[] buffer;
					buffer = new char[remain_stderr];
					size_allocated = remain_stderr;
				}
				if (not ReadFile(pObject->m_Pipes.stderr_read, buffer, remain_stderr, 0, 0)) {
					throw L"管道读取异常";
				}
				pObject->m_Output.AppendText(buffer, remain_stderr);
			}
			if (remain_signal) {
				if (not ReadFile(pObject->m_Pipes.signal_out_read, buffer, 20, nullptr, nullptr)) {
					throw L"管道读取异常";
				}
				UINT message = *(UINT*)buffer;
				WPARAM wParam = *(WPARAM*)(buffer + 4);
				LPARAM lParam = *(LPARAM*)(buffer + 12);
				pObject->SignalProc(message, wParam, lParam);
			}
		}
		else {
			GetExitCodeProcess(pObject->m_PI.hProcess, &state);
			if (state != STILL_ACTIVE) {
				break;
			}
		}
	}
	delete[] buffer;
	// 结束执行
	pObject->ExitSubprocess(state);
	return 0;
}
void CConsole::SendSignal(UINT message, WPARAM wParam, LPARAM lParam)
{
	static char buffer[20];
	memcpy(buffer, &message, 4);
	memcpy(buffer + 4, &wParam, 8);
	memcpy(buffer + 12, &lParam, 8);
	WriteFile(pObject->m_Pipes.signal_in_write, buffer, 20, 0, nullptr);
}
inline bool CConsole::SendInput(wchar_t* input, DWORD count)
{
	if (m_bRun) {
		m_Output.AppendInput(input, count);
		int size = WideCharToMultiByte(CP_ACP, 0, input, count, nullptr, 0, nullptr, nullptr);
		char* buffer = new char[size];
		WideCharToMultiByte(CP_ACP, 0, input, count, buffer, size, nullptr, nullptr);
		WriteFile(m_Pipes.stdin_write, buffer, size, nullptr, nullptr);
		delete[] buffer;
	}
	return m_bRun;
}
void CConsole::SignalProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case SIGNAL_CONNECTION:
		CEditor::pObject->PostMessageW(WM_STEP, 0, -1);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_DEBUG)->ShowWindow(SW_SHOW);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_DEBUG)->SetState(true);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_CONTINUE)->ShowWindow(SW_HIDE);
		break;
	case SIGNAL_BREAKPOINT: case SIGNAL_EXECUTION:
		CEditor::pObject->PostMessageW(WM_STEP, 0, lParam);
		SendSignal(SIGNAL_INFORMATION, INFORMATION_VARIABLE_TABLE_REQUEST, 0);
		SendSignal(SIGNAL_INFORMATION, INFORMATION_CALLING_STACK_REQUEST, 0);
		SendSignal(SIGNAL_INFORMATION, INFORMATION_RETURN_VALUE_REQUEST, 0);
		break;
	case SIGNAL_INFORMATION:
		switch (wParam) {
		case INFORMATION_CALLING_STACK_RESPONSE:
			CCallStack::pObject->LoadCallStack(ReadMemory((CALLSTACK*)lParam));
			break;
		case INFORMATION_VARIABLE_TABLE_RESPONSE:
			CVariableTable::pObject->LoadGlobal(ReadMemory((BinaryTree*)lParam));
			break;
		case INFORMATION_RETURN_VALUE_RESPONSE:
			if (lParam) {
				CVariableTable::pObject->LoadReturnValue(ReadMemory((DATA*)lParam));
			}
			break;
		}
	}
}
