#include "pch.h"

BEGIN_MESSAGE_MAP(CWatchInput, CWnd)
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
CWatchInput::CWatchInput()
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
CWatchInput::~CWatchInput()
{

}
int CWatchInput::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 创建源
	m_Source.CreateCompatibleDC(&ScreenDC);
	m_Selection.CreateCompatibleDC(&ScreenDC);
	m_Source.SelectObject(CConsole::font);
	// 计算字符大小
	CRect rect;
	m_Source.DrawTextW(L"添加监视：", -1, &rect, DT_CALCRECT);
	m_CharHeight = rect.bottom;
	m_Offset = rect.right + 15;
	m_Source.SetTextColor(RGB(255, 255, 255));
	m_Source.SetBkMode(TRANSPARENT);

	return 0;
}
void CWatchInput::OnPaint()
{
	CPaintDC dc(this);
	MemoryDC.BitBlt(0, 0, m_Width, m_CharHeight, &m_Selection, 0, 0, SRCCOPY);
	MemoryDC.TransparentBlt(0, 0, m_Width - m_Offset, m_CharHeight, &m_Source, 0, 0, m_Width - m_Offset, m_CharHeight, 0);
	dc.BitBlt(m_Offset, 0, m_Width, m_CharHeight, &MemoryDC, 0, 0, SRCCOPY);
}
BOOL CWatchInput::OnEraseBkgnd(CDC* pDC)
{
	pDC->SelectObject(CConsole::font);
	pDC->SetTextColor(RGB(255, 255, 255));
	pDC->SetBkColor(RGB(30, 30, 30));
	CRect rect(5, 0, m_Offset, m_CharHeight);
	pDC->DrawTextW(L"添加监视：", -1, &rect, 0);
	return TRUE;
}
void CWatchInput::OnSize(UINT nType, int cx, int cy)
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
void CWatchInput::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar) {
	case L'\r':
	{
		wchar_t* buffer = new wchar_t[m_cchBuffer + 1];
		memcpy(buffer, m_Buffer, m_cchBuffer * 2);
		buffer[m_cchBuffer] = 0;
		CWatch::pObject->AddWatch(buffer);
		delete[] m_Buffer;
		m_Buffer = new wchar_t[0];
		m_cchBuffer = 0;
		m_cchPointer = 0;
		m_cchDragPointer = 0;
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
void CWatchInput::OnMouseMove(UINT nFlags, CPoint point)
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
void CWatchInput::OnLButtonDown(UINT nFlags, CPoint point)
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
void CWatchInput::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_bDrag = false;
}
BOOL CWatchInput::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	return TRUE;
}
void CWatchInput::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	::CreateCaret(m_hWnd, NULL, 1, m_CharHeight);
	m_bFocus = true;
	ArrangePointer();
}
void CWatchInput::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	m_bFocus = false;
	m_bCaret = false;
	DestroyCaret();
}
inline int CWatchInput::GetCharHeight()
{
	return m_CharHeight;
}
void CWatchInput::ArrangeText()
{
	m_Source.PatBlt(0, 0, m_Width, m_CharHeight, BLACKNESS);
	double offset = -m_Percentage * m_FullLength;
	CRect rect(offset, 0, m_Width, m_CharHeight);
	m_Source.DrawTextW(m_Buffer, m_cchBuffer, &rect, 0);
}
void CWatchInput::ArrangePointer()
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
void CWatchInput::ArrangeSelection()
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
UINT CWatchInput::TranslatePointer(CPoint point)
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
void CWatchInput::Delete()
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

BEGIN_MESSAGE_MAP(CWatch, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()
CWatch::CWatch()
{
	m_Width = m_Height = 0;
	m_FirstWidth = m_SecondWidth = 0;
	m_FullHeight = 0;
	m_Percentage = 0.0;
	m_bPaused = false;
	pObject = this;
}
CWatch::~CWatch()
{
}
int CWatch::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_Font.CreateFontW(20, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	m_Pen.CreatePen(PS_SOLID, 1, RGB(61, 61, 61));

	m_BG.CreateCompatibleDC(&ScreenDC);
	m_BG.SelectObject(m_Font);
	m_BG.SelectObject(m_Pen);
	m_BG.SetTextColor(RGB(255, 255, 255));
	m_BG.SetBkMode(TRANSPARENT);
	m_Source.CreateCompatibleDC(&ScreenDC);
	m_Source.SelectObject(m_Font);
	CRect rect;
	m_Source.DrawTextW(L"全局", 2, &rect, DT_CALCRECT);
	m_WordSize = CSize(rect.right + 8, rect.bottom + 4);
	m_Source.SetTextColor(RGB(255, 255, 255));
	m_Source.SetBkMode(TRANSPARENT);

	m_Folded.CreateCompatibleDC(&ScreenDC);
	CBitmap* pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 20, 20);
	m_Folded.SelectObject(pBitmap);
	rect = CRect(0, 0, 20, 20);
	m_Folded.FillRect(&rect, pGreyBlackBrush);
	Gdiplus::Point* points = new Gdiplus::Point[]{ {5, 4}, {5, 16}, {15, 10} };
	Gdiplus::SolidBrush brush(Gdiplus::Color(214, 214, 214));
	Gdiplus::Graphics graphic(m_Folded);
	graphic.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphic.FillPolygon(&brush, points, 3);
	delete[] points;

	m_Expanded.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 20, 20);
	m_Expanded.SelectObject(pBitmap);
	m_Expanded.FillRect(&rect, pGreyBlackBrush);
	points = new Gdiplus::Point[]{ {16, 5}, {4, 5}, {10, 15} };
	Gdiplus::Graphics graphic2(m_Expanded);
	graphic2.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphic2.FillPolygon(&brush, points, 3);
	delete[] points;

	m_FirstWidth = m_WordSize.cx;
	ArrangeBG();
	m_WatchInput.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(), this, NULL);
	m_Slider.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(), this, NULL);
	m_Slider.SetCallback(VerticalCallback);

	return 0;
}
BOOL CWatch::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CWatch::OnPaint()
{
	CPaintDC dc(this);
	CRect rect(0, 0, m_Width, m_Height);
	MemoryDC.FillRect(&rect, pGreyBlackBrush);
	ArrangeWatches();
	MemoryDC.BitBlt(0, m_WordSize.cy, m_Width, m_Height - m_WordSize.cy, &m_Source, 0, 0, SRCCOPY);
	MemoryDC.BitBlt(0, 0, m_Width, m_WordSize.cy, &m_BG, 0, 0, SRCCOPY);
	dc.BitBlt(0, 0, m_Width, m_Height, &MemoryDC, 0, 0, SRCCOPY);
}
void CWatch::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	m_Width = cx - 10;
	m_Height = cy - m_WordSize.cy;

	HBITMAP hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_WordSize.cy);
	HGLOBAL hOldBitmap = SelectObject(m_BG, hBitmap);
	DeleteObject(hBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_Height - m_WordSize.cy);
	hOldBitmap = SelectObject(m_Source, hBitmap);
	DeleteObject(hBitmap);
	CRect rect(0, m_Height, m_Width, m_Height + m_WordSize.cy);
	m_WatchInput.MoveWindow(&rect, TRUE);
	rect = CRect(m_Width, 0, cx, m_Height);
	m_Slider.MoveWindow(&rect, TRUE);
	m_FirstWidth = m_SecondWidth = m_Width / 3;
	ArrangeBG();
	ArrangeWatches();
	Invalidate(FALSE);
}
BOOL CWatch::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	double delta = (double)m_WordSize.cy / m_FullHeight;
	if (zDelta > 0) {
		m_Slider.SetPercentage(m_Percentage - delta);
	}
	else {
		m_Slider.SetPercentage(m_Percentage + delta);
	}
	return TRUE;
}
void CWatch::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
}
void CWatch::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	if (point.x >= 0 and point.x < m_Width and
		point.y > m_WordSize.cy and point.y < m_Height and m_Watches.size()) {
		double start_line = m_Percentage * m_Watches.size();
		USHORT level = 0;
		int count = 0;
		CRect rect;
		bool started = false;
		for (IndexedList<ELEMENT>::iterator iter = m_OrderedList.begin(); iter != m_OrderedList.end(); iter++) {
			if (iter->level > level) {
				continue;
			}
			level = iter->level;
			if (iter->state == ELEMENT::EXPANDED) {
				level++;
			}
			if (count == (int)start_line) {
				rect = CRect(0, (double)(count - start_line + 1) * m_WordSize.cy, m_Width, 0);
				rect.bottom = rect.top + m_WordSize.cy;
				started = true;
			}
			count++;
			if (started) {
				if (rect.PtInRect(point)) {
					if (iter->state == ELEMENT::NOT_EXPANDED) {
						iter->state = ELEMENT::EXPANDED;
					}
					else if (iter->state == ELEMENT::EXPANDED) {
						iter->state = ELEMENT::NOT_EXPANDED;
					}
					UpdateSlider();
					ArrangeWatches();
					Invalidate(FALSE);
					return;
				}
				rect.top = rect.bottom;
				rect.bottom += m_WordSize.cy;
			}
		}
	}
}
void CWatch::OnRButtonUp(UINT nFlags, CPoint point)
{
	point.y -= m_WordSize.cy;
	double start_line = m_Percentage * (m_FullHeight / m_WordSize.cy);
	USHORT level = 0;
	USHORT count = 0;
	USHORT count_base = 0;
	CRect rect;
	bool started = false;
	for (IndexedList<ELEMENT>::iterator iter = m_OrderedList.begin(); iter != m_OrderedList.end(); iter++) {
		if (level < iter->level) {
			continue;
		}
		level = iter->level;
		if (count == (USHORT)start_line) {
			rect = CRect(0, (count - start_line) * m_WordSize.cy, m_Width, 0);
			rect.bottom = rect.top + m_WordSize.cy;
			started = true;
		}
		if (started) {
			if (rect.PtInRect(point)) {
				if (iter->level == 0) {
					DeleteWatch(count_base, count);
				}
				return;
			}
			rect.top = rect.bottom;
			rect.bottom += m_WordSize.cy;
		}
		count++;
		if (level == 0) {
			count_base++;
		}
		if (iter->state == ELEMENT::EXPANDED) {
			level++;
		}
	}
	if (m_OrderedList.size() == 0) {
		double difference = (double)point.y / m_WordSize.cy;
		double target_line = difference + start_line;
		m_Watches.pop((USHORT)target_line);
		Invalidate(FALSE);
	}
}
void CWatch::AddWatch(wchar_t* buffer)
{
	WATCH watch{ buffer, nullptr, nullptr };
	if (Element::expression(buffer, &watch.rpn_exp)) {
		if (m_bPaused) {
			size_t count = 0;
			for (USHORT index = 0; index != watch.rpn_exp->number_of_element; index++) {
				count += wcslen(watch.rpn_exp->rpn + count) + 1;
			}
			wchar_t* rpn_string = (wchar_t*)VirtualAllocEx(CConsole::pObject->GetProcessHandle(), NULL, count * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			WriteProcessMemory(CConsole::pObject->m_DebugHandle, rpn_string, watch.rpn_exp->rpn, count * 2, nullptr);
			wchar_t* original_rpn_string = watch.rpn_exp->rpn;
			watch.rpn_exp->rpn = rpn_string;
			watch.target_rpn_exp = (RPN_EXP*)VirtualAllocEx(CConsole::pObject->GetProcessHandle(), NULL, sizeof(RPN_EXP), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			WriteProcessMemory(CConsole::pObject->m_DebugHandle, watch.target_rpn_exp, watch.rpn_exp, sizeof(RPN_EXP), nullptr);
			watch.rpn_exp->rpn = original_rpn_string;
			CConsole::pObject->SendSignal(SIGNAL_WATCH, WATCH_EVALUATION, (LPARAM)watch.target_rpn_exp);
		}
	}
	m_Watches.append(watch);
	Invalidate(FALSE);
}
void CWatch::BuildRemotePages()
{
	for (IndexedList<WATCH>::iterator iter = m_Watches.begin(); iter != m_Watches.end(); iter++) {
		if (iter->rpn_exp) {
			wchar_t* rpn_string = (wchar_t*)VirtualAllocEx(CConsole::pObject->GetProcessHandle(), NULL, _msize(iter->rpn_exp->rpn), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			WriteProcessMemory(CConsole::pObject->m_DebugHandle, rpn_string, iter->rpn_exp->rpn, _msize(iter->rpn_exp->rpn), nullptr);
			wchar_t* original_rpn_string = iter->rpn_exp->rpn;
			iter->rpn_exp->rpn = rpn_string;
			iter->target_rpn_exp = (RPN_EXP*)VirtualAllocEx(CConsole::pObject->GetProcessHandle(), NULL, sizeof(RPN_EXP), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			WriteProcessMemory(CConsole::pObject->m_DebugHandle, iter->target_rpn_exp, iter->rpn_exp, sizeof(RPN_EXP), nullptr);
			iter->rpn_exp->rpn = original_rpn_string;
		}
	}
}
void CWatch::CleanRemotePages()
{
	for (IndexedList<WATCH>::iterator iter = m_Watches.begin(); iter != m_Watches.end(); iter++) {
		if (iter->target_rpn_exp) {
			RPN_EXP* duplicate_rpn_exp = CConsole::pObject->ReadMemory(iter->target_rpn_exp);
			VirtualFreeEx(CConsole::pObject->GetProcessHandle(), duplicate_rpn_exp->rpn, 0, MEM_RELEASE);
			VirtualFreeEx(CConsole::pObject->GetProcessHandle(), iter->rpn_exp, 0, MEM_RELEASE);
			free(duplicate_rpn_exp);
		}
	}
}
void CWatch::CleanWatchResult()
{
	for (IndexedList<ELEMENT>::iterator iter = m_OrderedList.begin(); iter != m_OrderedList.end(); iter++) {
		if (iter->name) {
			delete[] iter->name;
		}
		if (iter->value) {
			delete[] iter->value;
		}
		if (iter->type) {
			delete[] iter->type;
		}
	}
	m_OrderedList.clear();
	Invalidate(FALSE);
}
void CWatch::UpdateWatchResult(WATCH_RESULT* result)
{
	WATCH_RESULT* duplicate_result = CConsole::pObject->ReadMemory(result);
	for (IndexedList<WATCH>::iterator iter = m_Watches.begin(); iter != m_Watches.end(); iter++) {
		if (iter->target_rpn_exp == duplicate_result->rpn_exp) {
			DATA* duplicate_data = CConsole::pObject->ReadMemory(duplicate_result->data);
			wchar_t* duplicate_expression = new wchar_t[wcslen(iter->expression) + 1];
			memcpy(duplicate_expression, iter->expression, (wcslen(iter->expression) + 1) * 2);
			if (duplicate_data->type != 65535) {
				wchar_t* type_name = CVariableTable::pObject->IdentifyType(duplicate_data);
				wchar_t* value = CVariableTable::pObject->RetrieveValue(duplicate_data);
				if (duplicate_data->type == 6 or duplicate_data->type == 8) {
					m_OrderedList.append(ELEMENT{ duplicate_expression, value, type_name, 0, ELEMENT::NOT_EXPANDED });
					ExpandComplexType(duplicate_data, 1);
				}
				else {
					m_OrderedList.append(ELEMENT{ duplicate_expression, value, type_name, 0, ELEMENT::NOT_EXPANDABLE });
				}
			}
			else {
				wchar_t* type_name = new wchar_t[1] {0};
				MEMORY_BASIC_INFORMATION mbi;
				VirtualQueryEx(CConsole::pObject->GetProcessHandle(), duplicate_data->value, &mbi, sizeof(mbi));
				wchar_t* value = CConsole::pObject->ReadMemory((wchar_t*)duplicate_data->value, mbi.RegionSize / 2);
				m_OrderedList.append(ELEMENT{ duplicate_expression, value, type_name });
			}
			free(duplicate_data);
			Invalidate(FALSE);
			break;
		}
	}
	free(duplicate_result);
	VirtualFreeEx(CConsole::pObject->GetProcessHandle(), result, 0, MEM_RELEASE);
}
void CWatch::VerticalCallback(double percentage)
{
	pObject->m_Percentage = percentage;
	pObject->ArrangeWatches();
	pObject->Invalidate(FALSE);
}
void CWatch::DeleteWatch(USHORT index, USHORT index_leaf)
{
	USHORT count = 0;
	IndexedList<WATCH>::iterator iter = m_Watches[index];
	delete[] iter->expression;
	if (iter->rpn_exp) {
		delete iter->rpn_exp;
	}
	if (iter->target_rpn_exp) {
		RPN_EXP* duplicate_rpn_exp = CConsole::pObject->ReadMemory(iter->target_rpn_exp);
		VirtualFreeEx(CConsole::pObject->GetProcessHandle(), duplicate_rpn_exp->rpn, 0, MEM_RELEASE);
		VirtualFreeEx(CConsole::pObject->GetProcessHandle(), iter->rpn_exp, 0, MEM_RELEASE);
		free(duplicate_rpn_exp);
	}
	m_Watches.pop(index);
	USHORT level = m_OrderedList[index_leaf]->level;
	m_OrderedList.pop(index_leaf);
	if (index_leaf != m_OrderedList.size()) {
		for (IndexedList<ELEMENT>::iterator iter = m_OrderedList[index_leaf + 1]; iter != m_OrderedList.end(); iter++) {
			if (iter->level == level) {
				break;
			}
			m_OrderedList.pop(index_leaf);
		}
	}
	Invalidate(FALSE);
}
void CWatch::ExpandComplexType(DATA* data, USHORT level)
{
	if (data->type == 6) {
		// 展开数组
		DataType::Array* object = CConsole::pObject->ReadMemory((DataType::Array*)data->value);
		DATA** memory = CConsole::pObject->ReadMemory(object->memory, object->total_size);
		size_t key_length = 1;
		USHORT* start_indexes = CConsole::pObject->ReadMemory(object->start_indexes, object->dimension);
		USHORT* sizes = CConsole::pObject->ReadMemory(object->size, object->dimension);
		for (USHORT index = 0; index != object->dimension; index++) {
			int largest = start_indexes[index] + sizes[index] - 1;
			key_length += 2 + GET_DIGITS(largest);
		}
		for (ULONG64 index = 0; index != object->total_size; index++) {
			DATA* duplicate_element = CConsole::pObject->ReadMemory(memory[index]);
			wchar_t* key = new wchar_t[key_length] {0};
			ULONG64 offset = index;
			for (USHORT index2 = object->dimension; index2 != 0; index2--) {
				USHORT this_index = offset % sizes[index2 - 1];
				offset -= this_index;
				this_index += start_indexes[index2 - 1];
				offset /= sizes[index2 - 1];
				wchar_t* temp = new wchar_t[GET_DIGITS(this_index) + 3];
				StringCchPrintfW(temp, GET_DIGITS(this_index) + 3, L"[%d]", this_index);
				StringCchCatW(key, key_length, temp);
				delete[] temp;
			}
			wchar_t* value = CVariableTable::pObject->RetrieveValue(duplicate_element);
			wchar_t* type_name = CVariableTable::pObject->IdentifyType(duplicate_element);
			if (duplicate_element->type == 6 or duplicate_element->type == 8) {
				m_OrderedList.append(ELEMENT{ key, value, type_name, level, ELEMENT::NOT_EXPANDED });
				ExpandComplexType(duplicate_element, level + 1);
			}
			else {
				m_OrderedList.append(ELEMENT{ key, value, type_name, level, ELEMENT::NOT_EXPANDABLE });
			}
			free(duplicate_element);
		}
		free(memory);
		free(start_indexes);
		free(object);
	}
	else {
		// 展开结构体
		DataType::Record* object = CConsole::pObject->ReadMemory((DataType::Record*)data->value);
		DataType::RecordType* type = CConsole::pObject->ReadMemory(object->source);
		DATA** datas = CConsole::pObject->ReadMemory(object->values, type->number_of_fields);
		wchar_t** fields = CConsole::pObject->ReadMemory(type->fields, type->number_of_fields);
		size_t* lengths = CConsole::pObject->ReadMemory(type->lengths, type->number_of_fields);
		for (USHORT index = 0; index != type->number_of_fields; index++) {
			wchar_t* key = CConsole::pObject->ReadMemory(fields[index], lengths[index]);
			DATA* data = CConsole::pObject->ReadMemory(datas[index]);
			wchar_t* value = CVariableTable::pObject->RetrieveValue(data);
			wchar_t* type_name = CVariableTable::pObject->IdentifyType(data);
			if (data->type == 6 or data->type == 8) {
				m_OrderedList.append(ELEMENT{ key, value, type_name, level, ELEMENT::NOT_EXPANDED });
				ExpandComplexType(data, level + 1);
			}
			else {
				m_OrderedList.append(ELEMENT{ key, value, type_name, level, ELEMENT::NOT_EXPANDABLE });
			}
			free(data);
		}
		free(fields);
		free(type);
		free(object);
	}
}
inline void CWatch::UpdateSlider()
{
	UINT count = 0;
	USHORT level = 0;
	for (IndexedList<ELEMENT>::iterator iter = m_OrderedList.begin(); iter != m_OrderedList.end(); iter++) {
		if (iter->level > level) {
			continue;
		}
		level = iter->level;
		if (iter->state == ELEMENT::EXPANDED) {
			level++;
		}
		count++;
	}
	if (m_OrderedList.size() == 0) {
		count = m_Watches.size();
	}
	m_FullHeight = count * m_WordSize.cy;
	m_Slider.SetRatio((double)(m_Height - m_WordSize.cy) / m_FullHeight);
}
void CWatch::ArrangeBG()
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
	m_BG.DrawTextW(L"表达式", 3, &rect, NULL);
	rect = CRect(m_FirstWidth + 4, 0, m_FirstWidth + m_SecondWidth - 4, m_WordSize.cy);
	m_BG.DrawTextW(L"值", 1, &rect, NULL);
	rect = CRect(m_FirstWidth + m_SecondWidth + 4, 0, m_Width - 4, m_WordSize.cy);
	m_BG.DrawTextW(L"类型", 2, &rect, NULL);
}
void CWatch::ArrangeWatches()
{
	CRect rect(0, 0, m_Width, m_Height);
	m_Source.FillRect(&rect, pGreyBlackBrush);
	double start_index = m_Percentage * (m_Watches.size() + 1);
	int y_offset = -start_index * m_WordSize.cy;
	CRect rect1(20, y_offset, m_FirstWidth, y_offset + m_WordSize.cy);
	CRect rect2(m_FirstWidth + 4, y_offset, m_FirstWidth + m_SecondWidth + 4, y_offset + m_WordSize.cy);
	CRect rect3(m_FirstWidth + m_SecondWidth + 4, y_offset, m_Width, y_offset + m_WordSize.cy);
	USHORT level = 0;
	for (IndexedList<ELEMENT>::iterator iter = m_OrderedList.begin(); iter != m_OrderedList.end(); iter++) {
		// 绘制变量名
		if (iter->level > level) {
			continue;
		}
		rect1.left += 20 * iter->level;
		m_Source.DrawTextW(iter->name, -1, &rect1, DT_END_ELLIPSIS);
		level = iter->level;
		if (iter->state == ELEMENT::NOT_EXPANDED) {
			m_Source.BitBlt(20 * iter->level, rect1.top, 20, 20, &m_Folded, 0, 0, SRCCOPY);
		}
		else if (iter->state == ELEMENT::EXPANDED) {
			level++;
			m_Source.BitBlt(20 * iter->level, rect1.top, 20, 20, &m_Expanded, 0, 0, SRCCOPY);
		}
		// 绘制值
		m_Source.DrawTextW(iter->value, -1, &rect2, DT_END_ELLIPSIS);
		// 绘制类型名
		m_Source.DrawTextW(iter->type, -1, &rect3, DT_END_ELLIPSIS);
		rect1.top = rect2.top = rect3.top = rect1.bottom;
		rect1.bottom += m_WordSize.cy;
		rect2.bottom = rect3.bottom = rect1.bottom;
		rect1.left = 20;
	}
	// 未启动子进程时
	if (m_OrderedList.size() == 0) {
		for (IndexedList<WATCH>::iterator iter = m_Watches.begin(); iter != m_Watches.end(); iter++) {
			// 绘制变量名
			m_Source.DrawTextW(iter->expression, -1, &rect1, DT_END_ELLIPSIS);
			rect1.top = rect1.bottom;
			rect1.bottom += m_WordSize.cy;
		}
	}
}
