#include "pch.h"
#include "../Pseudocode Interpreter/Parser.cpp"
#define LINE_NUMBER_OFFSET 27 // 展示行数所需的额外偏移量
#define SCROLL_UNIT 3 // 每次鼠标滚轮滚动时移动的行数
#define GET_TEXT_WIDTH(string, length) LOWORD(GetTabbedTextExtentW(m_Source, string, length, 1, &tab_position))
#define TIMER_CHAR 1 // 单字符输入计时器

static int tab_position;

class Lock {
private:
	std::recursive_mutex* m_Lock;
public:
	Lock(std::recursive_mutex* lock) {
		m_Lock = lock;
		m_Lock->lock();
	}
	~Lock() {
		m_Lock->unlock();
	}
};

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
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	ON_WM_SETCURSOR()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_STEP, CEditor::OnStep)
END_MESSAGE_MAP()
CEditor::CEditor()
{
	m_bFocus = false;
	m_bCaret = false;
	m_bDrag = false;
	m_CurrentTag = nullptr;
	m_Width = m_Height = 0;
	m_PercentageHorizontal = m_PercentageVertical = 0.0;
	m_FullWidth = m_FullHeight = 0;
	m_LineNumberWidth = 0;
	m_Dialog = new CBreakpointDlg(this, &m_Breakpoints);
	m_bRecord = false;
	m_CurrentStepLineIndex = -1;
	m_bBackendPending = false;
	m_BackendThread = NULL;
	pObject = this;
}
CEditor::~CEditor()
{
}
int CEditor::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 准备未选中文件时背景
	m_Free.CreateCompatibleDC(&ScreenDC);
	CFont* pFont = new CFont;
	pFont->CreateFontW(30, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	m_Free.SelectObject(pFont);
	m_Free.SetTextColor(RGB(210, 210, 210));
	m_Free.SetBkMode(TRANSPARENT);

	// 准备渲染文字源
	m_Source.CreateCompatibleDC(&ScreenDC);
	m_Font.CreateFontW(30, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Consolas");
	m_Source.SelectObject(m_Font);
	m_Source.SetBkColor(0);
	TEXTMETRICW tm;
	GetTextMetricsW(m_Source, &tm);
	m_CharSize = CSize(tm.tmAveCharWidth, tm.tmHeight);
	tab_position = m_CharSize.cx * 4;

	m_Colour.CreateCompatibleDC(&ScreenDC);

	// 准备选区源
	m_Selection.CreateCompatibleDC(&ScreenDC);
	m_SelectionColor.CreateSolidBrush(RGB(38, 79, 120));
	CRect rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	m_Selection.FillRect(&rect, pGreyBlackBrush);

	// 准备断点源
	m_Breakpoint.CreateCompatibleDC(&ScreenDC);
	m_BreakpointHitColor.CreateSolidBrush(RGB(202, 205, 56));
	m_Breakpoint.SelectObject(pNullPen);

	// 创建滚动条
	m_VSlider.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_HSlider.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_VSlider.SetCallback(VerticalCallback);
	m_HSlider.SetCallback(HorizontalCallback);
	m_HSlider.SetDeflateCallback(DeflationCallback);

	return 0;
}
void CEditor::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	Lock lock(&m_BackendLock);
	if (cx == 0) { return; }
	m_Width = cx - 10;
	m_Height = cy - 10;

	HBITMAP hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_Height);
	HGLOBAL hOldBitmap = SelectObject(m_Free, hBitmap);
	DeleteObject(hOldBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_Height);
	hOldBitmap = SelectObject(m_Source, hBitmap);
	DeleteObject(hOldBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_Height);
	hOldBitmap = SelectObject(m_Colour, hBitmap);
	DeleteObject(hOldBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_Height);
	hOldBitmap = SelectObject(m_Selection, hBitmap);
	DeleteObject(hOldBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, 12, m_Height);
	hOldBitmap = SelectObject(m_Breakpoint, hBitmap);
	DeleteObject(hOldBitmap);

	CRect rect(0, 0, m_Width, m_Height);
	m_Free.FillRect(&rect, pGreyBlackBrush);
	m_Free.DrawTextW(L"点击此处或拖拽文件到此窗口来打开文件", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	rect = CRect(m_Width, 0, cx, m_Height);
	m_VSlider.MoveWindow(rect);
	rect = CRect(0, m_Height, m_Width, cy);
	m_HSlider.MoveWindow(rect);
	UpdateSlider();
}
BOOL CEditor::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CEditor::OnPaint()
{
	Lock lock(&m_BackendLock);
	CPaintDC dc(this);
	if (m_CurrentTag) {
		ArrangeText();
		ArrangePointer();
		ArrangeSelection();
		ArrangeBreakpoints();
		MemoryDC.BitBlt(0, 0, m_Width, m_Height, &m_Selection, 0, 0, SRCCOPY);
		MemoryDC.TransparentBlt(0, 0, m_Width, m_Height, &m_Source, 0, 0, m_Width, m_Height, 0);
		MemoryDC.BitBlt(0, 0, m_Width, m_Height, &m_Breakpoint, 0, 0, SRCCOPY);
		dc.BitBlt(0, 0, m_Width, m_Height, &MemoryDC, 0, 0, SRCCOPY);
		CRect rect(m_Width, m_Height, m_Width + 10, m_Height + 10);
		dc.FillRect(&rect, pGreyBlackBrush);
	}
	else {
		dc.BitBlt(0, 0, m_Width, m_Height, &m_Free, 0, 0, SRCCOPY);
	}
}
BOOL CEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	Lock lock(&m_BackendLock);
	double after_scroll_percentage = (m_PercentageVertical * m_FullHeight + (zDelta > 0 ? (-m_CharSize.cy) : m_CharSize.cy) * SCROLL_UNIT) / m_FullHeight;
	double adjusted_percentage = min(max(0.0, after_scroll_percentage), 1.0);
	m_VSlider.SetPercentage(adjusted_percentage);
	return TRUE;
}
void CEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
	SetCapture();
	Lock lock(&m_BackendLock);
	if (m_CurrentTag) {
		if (m_bRecord) {
			KillTimer(TIMER_CHAR);
			EndRecord();
		}
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
}
void CEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	Lock lock(&m_BackendLock);
	if (m_CurrentTag) {
		if (not m_bDrag) {
			if (point.x < 0 or point.x > LINE_NUMBER_OFFSET + m_LineNumberWidth) {
				return;
			}
			double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
			ULONG64 line_index = (ULONG64)((double)point.y / m_CharSize.cy + start_line);
			if (line_index < 0 or line_index >= m_CurrentTag->m_Lines.size()) {
				return;
			}
			for (std::list<BREAKPOINT>::iterator iter = m_Breakpoints.begin(); iter != m_Breakpoints.end(); iter++) {
				if (iter->line_index == line_index) {
					// 删除断点
					m_Breakpoints.erase(iter);
					ArrangeBreakpoints();
					REDRAW_WINDOW();
					return;
				}
			}
			// 添加断点
			m_Breakpoints.push_back(BREAKPOINT{ line_index, 0, 0, 0 });
			ArrangeBreakpoints();
			REDRAW_WINDOW();
		}
		else {
			m_bDrag = false;
		}
	}
	else {
		CTagPanel::pObject->OnOpen();
	}
}
void CEditor::OnRButtonUp(UINT nFlags, CPoint point)
{
	Lock lock(&m_BackendLock);
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
	Lock lock(&m_BackendLock);
	if (m_CurrentTag) {
		if (m_bDrag) {
			m_PointerPoint = TranslatePointer(point);
			MoveView();
			REDRAW_WINDOW();
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
}
void CEditor::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	Lock lock(&m_BackendLock);
	double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
	double difference = m_PointerPoint.y - start_line;
	CPoint point(
		GET_TEXT_WIDTH(*m_CurrentTag->m_CurrentLine, m_PointerPoint.x) + m_LineNumberWidth + LINE_NUMBER_OFFSET,
		difference * m_CharSize.cy
	);
	switch (nChar) {
	case VK_UP:
		point.y -= m_CharSize.cy;
		break;
	case VK_DOWN:
		point.y += m_CharSize.cy;
		break;
	case VK_LEFT:
		point.x = GET_TEXT_WIDTH(*m_CurrentTag->m_CurrentLine, m_PointerPoint.x - 1) + m_LineNumberWidth + LINE_NUMBER_OFFSET;
		break;
	case VK_RIGHT:
		point.x = GET_TEXT_WIDTH(*m_CurrentTag->m_CurrentLine, m_PointerPoint.x + 1) + m_LineNumberWidth + LINE_NUMBER_OFFSET;
		break;
	default:
		return;
	}
	if (GetKeyState(VK_CONTROL) >> 8) {
		m_PointerPoint = TranslatePointer(point);
	}
	else {
		m_DragPointerPoint = m_PointerPoint = TranslatePointer(point);
	}
	ArrangePointer();
	MoveView();
	REDRAW_WINDOW();
}
void CEditor::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	Lock lock(&m_BackendLock);
	m_CurrentTag->SetEdited();
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
		RecordChar(nChar);
		wchar_t* buffer1 = new wchar_t[m_PointerPoint.x + 1];
		wchar_t* buffer2 = new wchar_t[wcslen(*m_CurrentTag->m_CurrentLine) - m_PointerPoint.x + 1];
		memcpy(buffer1, *m_CurrentTag->m_CurrentLine, m_PointerPoint.x * 2);
		buffer1[m_PointerPoint.x] = 0;
		memcpy(buffer2,
			*m_CurrentTag->m_CurrentLine + m_PointerPoint.x,
			(wcslen(*m_CurrentTag->m_CurrentLine) - m_PointerPoint.x + 1) * 2);
		delete[] * m_CurrentTag->m_CurrentLine;
		*m_CurrentTag->m_CurrentLine = buffer1;
		ParseLine();
		m_CurrentTag->m_Lines.insert(m_PointerPoint.y + 1, buffer2);
		m_CurrentTag->m_CurrentLine++;
		m_CurrentTag->m_Tokens->insert(m_PointerPoint.y + 1, ADVANCED_TOKEN{});
		m_PointerPoint.y++;
		m_PointerPoint.x = 0;
		m_DragPointerPoint = m_PointerPoint;
		USHORT digits = (USHORT)log10(m_CurrentTag->m_Lines.size()) + 1;
		m_LineNumberWidth = m_CharSize.cx * digits;
		m_FullHeight += m_CharSize.cy;
		ParseLine();
		UpdateSlider();
		break;
	}
	default:
	{
		if (m_PointerPoint != m_DragPointerPoint) {
			Delete();
		}
		RecordChar(nChar);
		wchar_t* buffer = new wchar_t[wcslen(*m_CurrentTag->m_CurrentLine) + 2];
		memcpy(buffer, *m_CurrentTag->m_CurrentLine, (size_t)m_PointerPoint.x * 2);
		buffer[m_PointerPoint.x] = nChar;
		memcpy(buffer + m_PointerPoint.x + 1,
			*m_CurrentTag->m_CurrentLine + m_PointerPoint.x,
			(wcslen(*m_CurrentTag->m_CurrentLine) - m_PointerPoint.x + 1) * 2);
		delete[] * m_CurrentTag->m_CurrentLine;
		CSize size;
		m_FullWidth = max(m_FullWidth, GET_TEXT_WIDTH(buffer, wcslen(buffer)) + m_CharSize.cx);
		*m_CurrentTag->m_CurrentLine = buffer;
		m_DragPointerPoint.x = ++m_PointerPoint.x;
		ParseLine();
		UpdateSlider();
		break;
	}
	}
	MoveView();
	Invalidate(FALSE);
}
BOOL CEditor::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	return TRUE;
}
void CEditor::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	Lock lock(&m_BackendLock);
	::CreateCaret(m_hWnd, NULL, 1, m_CharSize.cy);
	m_bFocus = true;
	ArrangePointer();
}
void CEditor::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	m_bFocus = false;
	m_bCaret = false;
	DestroyCaret();
}
void CEditor::OnDestroy()
{
	CWnd::OnDestroy();

	m_bBackendPending = true;
	WaitForSingleObject(m_BackendThread, INFINITE);
}
void CEditor::OnUndo()
{
	Lock lock(&m_BackendLock);
	EndRecord();
	if (m_CurrentTag->m_CurrentOperation == m_CurrentTag->m_Operations.begin()) { return; }
	m_CurrentTag->m_CurrentOperation--;
	if (m_CurrentTag->m_CurrentOperation->insert) {
		// 删除插入的内容
		m_DragPointerPoint = m_CurrentTag->m_CurrentOperation->start;
		MovePointer(m_CurrentTag->m_CurrentOperation->end);
		Delete();
		MoveView();
	}
	else {
		// 插入删除的内容
		MovePointer(m_CurrentTag->m_CurrentOperation->end);
		Insert(m_CurrentTag->m_CurrentOperation->content);
		MoveView();
	}
	Invalidate(FALSE);
	if (m_CurrentTag->m_CurrentOperation == m_CurrentTag->m_Operations.begin()) {
		FIND_BUTTON(ID_EDIT, ID_EDIT_UNDO)->SetState(false);
	}
	FIND_BUTTON(ID_EDIT, ID_EDIT_REDO)->SetState(true);
}
void CEditor::OnRedo()
{
	Lock lock(&m_BackendLock);
	EndRecord();
	if (m_CurrentTag->m_CurrentOperation == m_CurrentTag->m_Operations.end()) { return; }
	if (m_CurrentTag->m_CurrentOperation->insert) {
		// 复原插入的内容
		MovePointer(m_CurrentTag->m_CurrentOperation->start);
		Insert(m_CurrentTag->m_CurrentOperation->content);
		MoveView();
	}
	else {
		// 继续删除的内容
		m_DragPointerPoint = m_CurrentTag->m_CurrentOperation->start;
		MovePointer(m_CurrentTag->m_CurrentOperation->end);
		Delete();
		MoveView();
	}
	m_CurrentTag->m_CurrentOperation++;
	if (m_CurrentTag->m_CurrentOperation == m_CurrentTag->m_Operations.end()) {
		FIND_BUTTON(ID_EDIT, ID_EDIT_REDO)->SetState(false);
	}
	FIND_BUTTON(ID_EDIT, ID_EDIT_UNDO)->SetState(true);
	Invalidate(FALSE);
}
void CEditor::OnCopy()
{
	Lock lock(&m_BackendLock);
	if (m_PointerPoint != m_DragPointerPoint) {
		if (!OpenClipboard()) {
			AfxMessageBox(L"无法打开剪切板", MB_ICONERROR);
			return;
		}
		size_t size;
		HANDLE hMem;
		wchar_t* buffer = GetSelectionText(size);
		hMem = GlobalAlloc(GMEM_MOVEABLE, size * 2 + 2);
		if (!hMem) {
			AfxMessageBox(L"内存分配异常", MB_ICONERROR);
			return;
		}
		void* dest = GlobalLock(hMem);
		memcpy(dest, buffer, size * 2 + 2);
		GlobalUnlock(hMem);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
		CloseClipboard();
	}
	else {
		CCustomStatusBar::pObject->UpdateMessage(new wchar_t[] {L"未选中任何数据"});
	}
}
void CEditor::OnCut()
{
	Lock lock(&m_BackendLock);
	if (m_PointerPoint != m_DragPointerPoint) {
		OnCopy();
		Delete();
		Invalidate(FALSE);
	}
}
void CEditor::OnPaste()
{
	Lock lock(&m_BackendLock);
	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		if (!OpenClipboard()) {
			AfxMessageBox(L"无法打开剪切板", MB_ICONERROR);
			return;
		}
		if (m_PointerPoint != m_DragPointerPoint) {
			Delete();
		}
		HANDLE hMem = GetClipboardData(CF_UNICODETEXT);
		wchar_t* buffer = (wchar_t*)GlobalLock(hMem);
		Insert(buffer);
		GlobalUnlock(hMem);
		Invalidate(FALSE);
	}
	else {
		CCustomStatusBar::pObject->UpdateMessage(new wchar_t[] {L"剪切板中没有可用数据"});
	}
}
void CEditor::OnLeftarrow()
{
	Lock lock(&m_BackendLock);
	Insert(L"←");
	Invalidate(FALSE);
}
LRESULT CEditor::OnStep(WPARAM wParam, LPARAM lParam)
{
	Lock lock(&m_BackendLock);
	m_CurrentStepLineIndex = lParam;
	if (lParam != -1) {
		CentralView(lParam);
	}
	Invalidate(FALSE);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_CONTINUE)->SetState(true);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPIN)->SetState(true);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPOVER)->SetState(true);
	FIND_BUTTON(ID_DEBUG, ID_DEBUG_STEPOUT)->SetState(true);
	return 0;
}
void CEditor::LoadFile(CFileTag* tag)
{
	m_bBackendPending = true;
	WaitForSingleObject(m_BackendThread, INFINITE);
	m_CurrentTag = tag;
	SetCursor(LoadCursorW(NULL, IDC_ARROW));
	if (m_CurrentTag) {
		SetCursor(LoadCursorW(NULL, IDC_WAIT));
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
		UpdateSlider();
		m_bBackendPending = false;
		m_BackendThread = CreateThread(NULL, NULL, BackendTasking, nullptr, NULL, NULL);
		SetCursor(LoadCursorW(NULL, IDC_ARROW));
	}
	Invalidate(FALSE);
}
void CEditor::VerticalCallback(double percentage)
{
	pObject->m_PercentageVertical = percentage;
	pObject->Invalidate(FALSE);
}
void CEditor::HorizontalCallback(double percentage)
{
	pObject->m_PercentageHorizontal = percentage;
	pObject->Invalidate(FALSE);
}
void CEditor::DeflationCallback(ULONG new_width)
{
	pObject->m_FullWidth = new_width;
}
void CEditor::EndRecord(HWND, UINT, UINT_PTR, DWORD)
{
	if (pObject->m_bRecord) {
		pObject->KillTimer(TIMER_CHAR);
		pObject->m_CurrentTag->m_Operations.back().end = pObject->m_PointerPoint;
		pObject->m_bRecord = false;
	}
}
void CEditor::ArrangeText()
{
	if (m_CurrentTag->m_Tokens->size()) {
		ArrangeRenderedText();
		return;
	}
	m_Source.PatBlt(0, 0, m_Width, m_Height, BLACKNESS);
	// 计算出现在DC中的第一行行数
	double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
	UINT start_line_index = start_line;
	// 绘制代码
	m_Source.SetTextColor(RGB(255, 255, 255));
	int x_offset = LINE_NUMBER_OFFSET + m_LineNumberWidth - (long)(m_PercentageHorizontal * m_FullWidth);
	int y_offset = (start_line_index - start_line) * m_CharSize.cy;
	IndexedList<wchar_t*>::iterator iter = m_CurrentTag->m_Lines[start_line_index];
	for (; iter != m_CurrentTag->m_Lines.end(); iter++) {
		TabbedTextOutW(m_Source, x_offset, y_offset, *iter, wcslen(*iter), 1, &tab_position, x_offset);
		y_offset += m_CharSize.cy;
		if (y_offset >= m_Height) {
			break;
		}
	}
	// 绘制行号
	m_Source.SetTextColor(RGB(150, 150, 150));
	USHORT digits = (USHORT)log10(m_CurrentTag->m_Lines.size()) + 1;
	wchar_t* buffer = new wchar_t[digits + 1];
	m_Source.PatBlt(0, 0, m_LineNumberWidth + LINE_NUMBER_OFFSET, m_Height, BLACKNESS);
	m_Source.SetTextAlign(TA_RIGHT | TA_TOP);
	CRect rect(LINE_NUMBER_OFFSET - 12,
		(long)((start_line_index - start_line) * m_CharSize.cy),
		LINE_NUMBER_OFFSET - 12 + m_LineNumberWidth,
		(long)((start_line_index - start_line + 1) * m_CharSize.cy));
	size_t total_lines = m_CurrentTag->m_Lines.size();
	for (size_t line_index = start_line_index; line_index != total_lines; line_index++) {
		StringCchPrintfW(buffer, (size_t)digits + 1, L"%d", (int)line_index + 1);
		TextOutW(m_Source, rect.right, rect.top, buffer, wcslen(buffer));
		rect.top = rect.bottom;
		rect.bottom += m_CharSize.cy;
		if (rect.top > m_Height) {
			break;
		}
	}
	delete[] buffer;
}
void CEditor::ArrangeRenderedText()
{
	m_Source.PatBlt(0, 0, m_Width, m_Height, BLACKNESS);
	// 计算出现在DC中的第一行行数
	double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
	UINT start_line_index = start_line;
	// 绘制代码
	static COLORREF colours[] = {
		RGB(255, 255, 255), // 空
		RGB(255, 255, 255), // 标点符号
		RGB(216, 160, 223), // 关键字
		RGB(156, 220, 254), // 变量
		RGB(78, 201, 176), // 类型
		RGB(220, 220, 147), // 子程序
		RGB(255, 255, 255), // 表达式（分解成整数和字符串）
		RGB(181, 206, 168), // 整数
		RGB(214, 157, 133), // 字符串
		RGB(255, 255, 255), // 枚举（分解成整数常量）
		RGB(255, 255, 255), // 参数列表（分解成变量和类型）
		RGB(86, 156, 214), // 参数传递方式
		RGB(77, 166, 74), // 注释
	};
	int x_offset = LINE_NUMBER_OFFSET + m_LineNumberWidth - (long)(m_PercentageHorizontal * m_FullWidth);
	int y_offset = (start_line_index - start_line) * m_CharSize.cy;
	IndexedList<wchar_t*>::iterator this_line = m_CurrentTag->m_Lines[start_line_index];
	IndexedList<ADVANCED_TOKEN>::iterator this_advanced_token = (*m_CurrentTag->m_Tokens)[start_line_index];
	for (; this_line != m_CurrentTag->m_Lines.end();) {
		int current_x_offset = x_offset;
		int char_offset = 0;
		if (this_advanced_token->tokens) {
			// 计算缩进偏移量
			current_x_offset += GET_TEXT_WIDTH(*this_line, this_advanced_token->indentation);
			char_offset += this_advanced_token->indentation;
			// 渲染提取的令牌
			for (TOKEN* this_token = this_advanced_token->tokens; this_token->type != TOKENTYPE::Null; this_token++) {
				m_Source.SetTextColor(colours[this_token->type]);
				LONG width = GET_TEXT_WIDTH(*this_line + char_offset, this_token->length);
				TabbedTextOutW(m_Source, current_x_offset, y_offset, *this_line + char_offset, this_token->length, 1, &tab_position, x_offset);
				current_x_offset += width;
				char_offset += this_token->length;
			}
		}
		else {
			// 无法识别的语法以白色绘制
			m_Source.SetTextColor(RGB(255, 255, 255));
			TabbedTextOutW(m_Source, current_x_offset, y_offset, *this_line, wcslen(*this_line), 1, &tab_position, x_offset);
		}
		// 渲染注释
		m_Source.SetTextColor(colours[12]);
		TabbedTextOutW(m_Source, current_x_offset, y_offset, *this_line + this_advanced_token->comment, wcslen(*this_line) - this_advanced_token->comment, 1, &tab_position, x_offset);

		y_offset += m_CharSize.cy;
		if (y_offset >= m_Height) {
			break;
		}
		this_line++;
		this_advanced_token++;
	}
	// 绘制行号
	m_Source.SetTextColor(RGB(150, 150, 150));
	USHORT digits = (USHORT)log10(m_CurrentTag->m_Lines.size()) + 1;
	wchar_t* buffer = new wchar_t[digits + 1];
	m_Source.PatBlt(0, 0, m_LineNumberWidth + LINE_NUMBER_OFFSET, m_Height, BLACKNESS);
	m_Source.SetTextAlign(TA_RIGHT | TA_TOP);
	CRect rect(LINE_NUMBER_OFFSET - 12,
		(long)((start_line_index - start_line) * m_CharSize.cy),
		LINE_NUMBER_OFFSET - 12 + m_LineNumberWidth,
		(long)((start_line_index - start_line + 1) * m_CharSize.cy));
	size_t total_lines = m_CurrentTag->m_Lines.size();
	for (size_t line_index = start_line_index; line_index != total_lines; line_index++) {
		StringCchPrintfW(buffer, (size_t)digits + 1, L"%d", (int)line_index + 1);
		TextOutW(m_Source, rect.right, rect.top, buffer, wcslen(buffer));
		rect.top = rect.bottom;
		rect.bottom += m_CharSize.cy;
		if (rect.top > m_Height) {
			break;
		}
	}
	delete[] buffer;
}
void CEditor::ArrangePointer()
{
	if (not m_bFocus) { return; }
	double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
	LONG width = GET_TEXT_WIDTH(*m_CurrentTag->m_CurrentLine, m_PointerPoint.x);
	CPoint point;
	point.x = width + m_LineNumberWidth + LINE_NUMBER_OFFSET - m_FullWidth * m_PercentageHorizontal;
	point.y = (m_PointerPoint.y - start_line) * m_CharSize.cy;
	SetCaretPos(point);
	if (point.x < m_LineNumberWidth + LINE_NUMBER_OFFSET) {
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
void CEditor::ArrangeSelection()
{
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
			LONG start = GET_TEXT_WIDTH(line, smaller) + m_LineNumberWidth + LINE_NUMBER_OFFSET + 1;
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
			LONG start = GET_TEXT_WIDTH(*m_CurrentTag->m_CurrentLine, m_PointerPoint.x) + 1;
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
	m_PointerPoint = pointer;
}
CPoint CEditor::TranslatePointer(CPoint point)
{
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
wchar_t* CEditor::GetSelectionText(size_t& size)
{
	wchar_t* buffer;
	if (m_PointerPoint.y == m_DragPointerPoint.y) {
		int smaller = min(m_PointerPoint.x, m_DragPointerPoint.x);
		int larger = max(m_PointerPoint.x, m_DragPointerPoint.x);
		size = (size_t)larger - smaller + 1;
		buffer = new wchar_t[size];
		memcpy(buffer, *m_CurrentTag->m_CurrentLine + smaller, (larger - smaller) * 2);
		buffer[larger - smaller] = 0;
	}
	else if (m_PointerPoint.y > m_DragPointerPoint.y) {
		size = m_PointerPoint.x;
		for (ULONG index = m_PointerPoint.y - 1; index != m_DragPointerPoint.y; index--) {
			size += wcslen(*m_CurrentTag->m_Lines[index]) + 2;
		}
		size += wcslen(*m_CurrentTag->m_Lines[m_DragPointerPoint.y] + m_DragPointerPoint.x) + 2;
		buffer = new wchar_t[size];
		ULONG offset = wcslen(*m_CurrentTag->m_Lines[m_DragPointerPoint.y] + m_DragPointerPoint.x);
		memcpy(
			buffer,
			*m_CurrentTag->m_Lines[m_DragPointerPoint.y] + m_DragPointerPoint.x,
			offset * 2
		);
		buffer[offset] = L'\r';
		buffer[offset + 1] = L'\n';
		offset += 2;
		for (ULONG index = m_DragPointerPoint.y + 1; index != m_PointerPoint.y; index++) {
			size_t part_size = wcslen(*m_CurrentTag->m_Lines[index]);
			memcpy(buffer + offset, *m_CurrentTag->m_Lines[index], part_size * 2);
			buffer[offset + part_size] = L'\r';
			buffer[offset + part_size + 1] = L'\n';
			offset += part_size + 2;
		}
		memcpy(buffer + offset, *m_CurrentTag->m_CurrentLine, m_PointerPoint.x * 2);
	}
	else {
		size = wcslen(*m_CurrentTag->m_CurrentLine + m_PointerPoint.x) + 2;
		for (ULONG index = m_PointerPoint.y + 1; index != m_DragPointerPoint.y; index++) {
			size += wcslen(*m_CurrentTag->m_Lines[index]) + 2;
		}
		size += m_DragPointerPoint.x;
		buffer = new wchar_t[size];
		ULONG offset = wcslen(*m_CurrentTag->m_CurrentLine + m_PointerPoint.x);
		memcpy(
			buffer,
			*m_CurrentTag->m_CurrentLine + m_PointerPoint.x,
			offset * 2
		);
		buffer[offset] = L'\r';
		buffer[offset + 1] = L'\n';
		offset += 2;
		for (ULONG index = m_PointerPoint.y + 1; index != m_DragPointerPoint.y; index++) {
			size_t part_size = wcslen(*m_CurrentTag->m_Lines[index]);
			memcpy(buffer + offset, *m_CurrentTag->m_Lines[index], part_size * 2);
			buffer[offset + part_size] = L'\r';
			buffer[offset + part_size + 1] = L'\n';
			offset += part_size + 2;
		}
		memcpy(buffer + offset, *m_CurrentTag->m_Lines[m_DragPointerPoint.y], m_DragPointerPoint.x * 2);
	}
	return buffer;
}
void CEditor::Insert(const wchar_t* text)
{
	m_CurrentTag->SetEdited();
	m_CurrentTag->m_bParsed = false;
	EndRecord();
	if (m_PointerPoint != m_DragPointerPoint) {
		Delete();
	}
	LONG64 last_return = -1;
	for (ULONG64 index = 0;; index++) {
		if (text[index] == 0 or text[index] == L'\n') {
			wchar_t*& this_line = *m_CurrentTag->m_CurrentLine;
			wchar_t* combined_line = new wchar_t[m_PointerPoint.x + index - last_return];
			memcpy(combined_line, this_line, m_PointerPoint.x * 2);
			memcpy(combined_line + m_PointerPoint.x, text + last_return + 1, (index - last_return - 1) * 2);
			combined_line[m_PointerPoint.x + index - last_return - 1] = 0;
			if (text[index] == 10) {
				// 创建新行
				if (text[index - 1] == L'\r') {
					combined_line[m_PointerPoint.x + index - last_return - 2] = 0;
				}
				wchar_t* new_line = new wchar_t[wcslen(this_line + m_PointerPoint.x) + 1];
				memcpy(new_line, this_line + m_PointerPoint.x, (wcslen(this_line + m_PointerPoint.x) + 1) * 2);
				m_CurrentTag->m_Lines.insert(m_PointerPoint.y + 1, new_line);
				m_CurrentTag->m_Tokens->insert(m_PointerPoint.y + 1, ADVANCED_TOKEN{ 0, nullptr, (UINT)wcslen(*m_CurrentTag->m_CurrentLine) });
				m_PointerPoint = CPoint(0, m_PointerPoint.y + 1);
				m_CurrentTag->m_CurrentLine++;
				delete[] this_line;
				this_line = combined_line;
			}
			else {
				// 合并行
				wchar_t* new_line = new wchar_t[wcslen(this_line) + index - last_return];
				memcpy(new_line, combined_line, wcslen(combined_line) * 2);
				memcpy(new_line + wcslen(combined_line), this_line + m_PointerPoint.x, (wcslen(this_line) - m_PointerPoint.x + 1) * 2);
				m_PointerPoint = CPoint(m_PointerPoint.x + index - last_return - 1, m_PointerPoint.y);
				delete[] this_line;
				delete[] combined_line;
				this_line = new_line;
				break;
			}
			last_return = index;
		}
	}
	m_DragPointerPoint = m_PointerPoint;
	// 重新计算行号长度
	USHORT digits = (USHORT)log10(m_CurrentTag->m_Lines.size()) + 1;
	m_LineNumberWidth = m_CharSize.cx * digits;
	m_FullHeight = m_CurrentTag->m_Lines.size() * m_CharSize.cy;
	UpdateSlider();
}
void CEditor::Delete()
{
	m_CurrentTag->SetEdited();
	m_CurrentTag->m_bParsed = false;
	if (m_PointerPoint == m_DragPointerPoint) {
		// 删除单个字符
		if (m_PointerPoint.x) {
			// 该行还有字符
			wchar_t* current_line = *m_CurrentTag->m_CurrentLine;
			RecordDelete(current_line[m_PointerPoint.x - 1]);
			wchar_t* buffer = new wchar_t[wcslen(current_line)];
			memcpy(buffer, current_line, (size_t)(m_PointerPoint.x - 1) * 2);
			memcpy(buffer + m_PointerPoint.x - 1,
				current_line + m_PointerPoint.x,
				(wcslen(current_line) - m_PointerPoint.x + 1) * 2);
			delete[] current_line;
			*m_CurrentTag->m_CurrentLine = buffer;
			m_PointerPoint.x--;
			ParseLine();
		}
		else {
			// 删除换行符
			if (m_PointerPoint.y) {
				// 如果不是首行
				RecordDelete(L'\n');
				m_CurrentTag->m_CurrentLine--;
				m_CurrentTag->m_Lines.pop(m_PointerPoint.y);
				m_PointerPoint.y--;
				m_PointerPoint.x = wcslen(*m_CurrentTag->m_CurrentLine);
				m_FullHeight -= m_CharSize.cy;
				ParseLine();
			}
		}
		m_DragPointerPoint = m_PointerPoint; // 确保无选区模式
	}
	else {
		// 删除选区
		EndRecord();
		CFileTag::OPERATION new_operation;
		new_operation.insert = false;
		if (m_PointerPoint.y == m_DragPointerPoint.y) {
			// 单行选区
			size_t smaller = min(m_PointerPoint.x, m_DragPointerPoint.x);
			size_t larger = max(m_PointerPoint.x, m_DragPointerPoint.x);
			new_operation.end = CPoint(smaller, m_PointerPoint.y);
			new_operation.start = CPoint(larger, m_PointerPoint.y);
			new_operation.length = larger - smaller;
			new_operation.content = new wchar_t[new_operation.length + 1];
			memcpy(new_operation.content, *m_CurrentTag->m_CurrentLine + smaller, new_operation.length * 2);
			new_operation.content[new_operation.length] = 0;
			wchar_t* buffer = new wchar_t[wcslen(*m_CurrentTag->m_CurrentLine) - (larger - smaller) + 1];
			memcpy(buffer, *m_CurrentTag->m_CurrentLine, smaller * 2);
			memcpy(buffer + smaller, *m_CurrentTag->m_CurrentLine + larger, (wcslen(*m_CurrentTag->m_CurrentLine) - larger + 1) * 2);
			delete[] * (m_CurrentTag->m_CurrentLine);
			*m_CurrentTag->m_CurrentLine = buffer;
			m_PointerPoint.x = smaller;
			m_DragPointerPoint = m_PointerPoint; // 确保无选区模式
			ParseLine();
		}
		else if (m_PointerPoint.y > m_DragPointerPoint.y) {
			// 多行选区（正向）
			new_operation.start = m_PointerPoint;
			new_operation.end = m_DragPointerPoint;
			new_operation.length = m_PointerPoint.x + wcslen(*m_CurrentTag->m_Lines[m_DragPointerPoint.y] + m_DragPointerPoint.x) + 3;
			for (UINT line_index = m_PointerPoint.y - 1; line_index != m_DragPointerPoint.y; line_index--) {
				new_operation.length += wcslen(*m_CurrentTag->m_Lines[m_DragPointerPoint.y + 1]) + 2;
			}
			new_operation.content = new wchar_t[new_operation.length];
			UINT offset = wcslen(*m_CurrentTag->m_Lines[m_DragPointerPoint.y] + m_DragPointerPoint.x);
			memcpy(new_operation.content, *m_CurrentTag->m_Lines[m_DragPointerPoint.y] + m_DragPointerPoint.x, offset * 2);
			new_operation.content[offset] = L'\r';
			new_operation.content[offset + 1] = L'\n';
			offset += 2;
			for (UINT line_index = m_PointerPoint.y - 1; line_index != m_DragPointerPoint.y; line_index--) {
				memcpy(new_operation.content + offset, *m_CurrentTag->m_Lines[m_DragPointerPoint.y + 1], wcslen(*m_CurrentTag->m_Lines[m_DragPointerPoint.y + 1]) * 2);
				offset += wcslen(*m_CurrentTag->m_Lines[m_DragPointerPoint.y + 1]);
				new_operation.content[offset] = L'\r';
				new_operation.content[offset + 1] = L'\n';
				offset += 2;
				m_CurrentTag->m_Lines.pop(m_DragPointerPoint.y + 1);
				delete (*m_CurrentTag->m_Tokens)[m_DragPointerPoint.y + 1]->tokens;
				m_CurrentTag->m_Tokens->pop(m_DragPointerPoint.y + 1);
			}
			m_FullHeight -= (m_PointerPoint.y - m_DragPointerPoint.y) * m_CharSize.cy;
			IndexedList<wchar_t*>::iterator line_before_selection = m_CurrentTag->m_Lines[m_DragPointerPoint.y];
			memcpy(new_operation.content + offset, *m_CurrentTag->m_CurrentLine, m_PointerPoint.x * 2);
			new_operation.content[offset + m_PointerPoint.x] = 0;
			wchar_t* buffer = new wchar_t[m_DragPointerPoint.x + wcslen(*m_CurrentTag->m_CurrentLine + m_PointerPoint.x) + 2];
			memcpy(buffer, *line_before_selection, m_DragPointerPoint.x * 2);
			memcpy(buffer + m_DragPointerPoint.x,
				*m_CurrentTag->m_CurrentLine + m_PointerPoint.x,
				(wcslen(*m_CurrentTag->m_CurrentLine + m_PointerPoint.x) + 1) * 2);
			delete[] * line_before_selection;
			*line_before_selection = buffer;
			m_CurrentTag->m_Lines.pop(m_DragPointerPoint.y + 1);
			m_CurrentTag->m_CurrentLine = line_before_selection;
			delete (*m_CurrentTag->m_Tokens)[m_DragPointerPoint.y + 1]->tokens;
			m_CurrentTag->m_Tokens->pop(m_DragPointerPoint.y + 1);
			m_PointerPoint = m_DragPointerPoint; // 确保无选区模式
			ParseLine();
		}
		else {
			// 多行选区（反向）
			new_operation.start = m_DragPointerPoint;
			new_operation.end = m_PointerPoint;
			new_operation.length = m_DragPointerPoint.x + wcslen(*m_CurrentTag->m_CurrentLine + m_PointerPoint.x) + 3;
			for (UINT line_index = m_PointerPoint.y + 1; line_index != m_DragPointerPoint.y; line_index++) {
				new_operation.length += wcslen(*m_CurrentTag->m_Lines[m_PointerPoint.y + 1]) + 2;
			}
			new_operation.content = new wchar_t[new_operation.length];
			UINT offset = wcslen(*m_CurrentTag->m_CurrentLine + m_PointerPoint.x);
			memcpy(new_operation.content, *m_CurrentTag->m_CurrentLine + m_PointerPoint.x, offset * 2);
			new_operation.content[offset] = L'\r';
			new_operation.content[offset + 1] = L'\n';
			offset += 2;
			for (UINT line_index = m_PointerPoint.y + 1; line_index != m_DragPointerPoint.y; line_index++) {
				memcpy(new_operation.content + offset, *m_CurrentTag->m_Lines[m_PointerPoint.y + 1], wcslen(*m_CurrentTag->m_Lines[m_PointerPoint.y + 1]) * 2);
				offset += wcslen(*m_CurrentTag->m_Lines[m_PointerPoint.y + 1]);
				new_operation.content[offset] = L'\r';
				new_operation.content[offset + 1] = L'\n';
				offset += 2;
				m_CurrentTag->m_Lines.pop(m_PointerPoint.y + 1);
				delete (*m_CurrentTag->m_Tokens)[m_PointerPoint.y + 1]->tokens;
				m_CurrentTag->m_Tokens->pop(m_PointerPoint.y + 1);
			}
			m_FullHeight -= (m_DragPointerPoint.y - m_PointerPoint.y) * m_CharSize.cy;
			IndexedList<wchar_t*>::iterator line_after_selection = m_CurrentTag->m_Lines[m_PointerPoint.y + 1];
			memcpy(new_operation.content + offset, *line_after_selection, m_DragPointerPoint.x * 2);
			new_operation.content[offset + m_DragPointerPoint.x] = 0;
			wchar_t* buffer = new wchar_t[m_PointerPoint.x + wcslen(*line_after_selection) - m_DragPointerPoint.x + 2];
			memcpy(buffer, *m_CurrentTag->m_CurrentLine, (size_t)m_PointerPoint.x * 2);
			memcpy(buffer + m_PointerPoint.x,
				*line_after_selection + m_DragPointerPoint.x,
				(wcslen(*line_after_selection) - m_DragPointerPoint.x + 1) * 2);
			delete[] * (m_CurrentTag->m_CurrentLine);
			*m_CurrentTag->m_CurrentLine = buffer;
			m_CurrentTag->m_Lines.pop(m_PointerPoint.y + 1);
			delete (*m_CurrentTag->m_Tokens)[m_PointerPoint.y + 1]->tokens;
			m_CurrentTag->m_Tokens->pop(m_PointerPoint.y + 1);
			m_DragPointerPoint = m_PointerPoint; // 确保无选区模式
			ParseLine();
		}
		m_CurrentTag->m_Operations.insert(m_CurrentTag->m_CurrentOperation, new_operation);
		FIND_BUTTON(ID_EDIT, ID_EDIT_UNDO)->SetState(true);
	}
	// 重新计算行号长度
	USHORT digits = (USHORT)log10(m_CurrentTag->m_Lines.size()) + 1;
	m_LineNumberWidth = m_CharSize.cx * digits;
	m_FullHeight = m_CurrentTag->m_Lines.size() * m_CharSize.cy;
	UpdateSlider();
}
void CEditor::RecordChar(UINT nChar)
{
	if (m_bRecord) {
		if (m_CurrentTag->m_Operations.back().insert) {
			CFileTag::OPERATION& last = m_CurrentTag->m_Operations.back();
			wchar_t* buffer = new wchar_t[last.length + 2];
			memcpy(buffer, last.content, last.length * 2);
			buffer[last.length] = nChar;
			buffer[last.length + 1] = 0;
			delete[] last.content;
			last.content = buffer;
			last.length++;
			SetTimer(TIMER_CHAR, 1000, EndRecord);
			return;
		}
		else {
			EndRecord();
		}
	}
	CFileTag::OPERATION operation;
	operation.content = new wchar_t[2];
	operation.content[0] = nChar;
	operation.content[1] = 0;
	operation.length = 1;
	operation.start = m_PointerPoint;
	operation.insert = true;
	if (m_CurrentTag->m_CurrentOperation != m_CurrentTag->m_Operations.end()) {
		m_CurrentTag->m_Operations.erase(m_CurrentTag->m_CurrentOperation, m_CurrentTag->m_Operations.end());
	}
	m_CurrentTag->m_Operations.push_back(operation);
	m_CurrentTag->m_CurrentOperation = m_CurrentTag->m_Operations.end();
	FIND_BUTTON(ID_EDIT, ID_EDIT_UNDO)->SetState(true);
	m_bRecord = true;
	SetTimer(TIMER_CHAR, 1000, EndRecord);
}
void CEditor::RecordDelete(UINT nChar)
{
	if (m_bRecord) {
		if (not m_CurrentTag->m_Operations.back().insert) {
			CFileTag::OPERATION& last = m_CurrentTag->m_Operations.back();
			wchar_t* buffer = new wchar_t[last.length + 2];
			memcpy(buffer + 1, last.content, last.length * 2);
			buffer[0] = nChar;
			buffer[last.length + 1] = 0;
			delete[] last.content;
			last.content = buffer;
			last.length++;
			SetTimer(TIMER_CHAR, 1000, EndRecord);
			return;
		}
		else {
			EndRecord();
		}
	}
	CFileTag::OPERATION operation;
	operation.content = new wchar_t[2];
	operation.content[0] = nChar;
	operation.content[1] = 0;
	operation.length = 1;
	operation.start = m_PointerPoint;
	operation.insert = false;
	if (m_CurrentTag->m_CurrentOperation != m_CurrentTag->m_Operations.end()) {
		m_CurrentTag->m_Operations.erase(m_CurrentTag->m_CurrentOperation, m_CurrentTag->m_Operations.end());
	}
	m_CurrentTag->m_Operations.push_back(operation);
	m_CurrentTag->m_CurrentOperation = m_CurrentTag->m_Operations.end();
	FIND_BUTTON(ID_EDIT, ID_EDIT_UNDO)->SetState(true);
	m_bRecord = true;
	SetTimer(TIMER_CHAR, 1000, EndRecord);
}
void CEditor::MoveView()
{
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
ADVANCED_TOKEN CEditor::LineSplit(wchar_t* line)
{
	int end_of_indentation = -1;
	int start_of_comment = -1;
	for (UINT index = 0; line[index] != 0; index++) {
		switch (line[index]) {
		case L' ': case L'\t':
			break;
		case L'/':
		{
			if (line[index + 1] == L'/' and start_of_comment == -1) {
				start_of_comment = index;
			}
			index = wcslen(line) - 1;
			break;
		}
		default:
		{
			if (end_of_indentation == -1) {
				end_of_indentation = index;
			}
			break;
		}
		}
	}
	if (end_of_indentation == -1) {
		end_of_indentation = 0;
	}
	if (start_of_comment == -1) {
		start_of_comment = wcslen(line);
	}
	wchar_t* content = new wchar_t[start_of_comment - end_of_indentation + 1];
	memcpy(content, line + end_of_indentation, (start_of_comment - end_of_indentation) * 2);
	content[start_of_comment - end_of_indentation] = 0;
	strip(content); // 移除末尾空格
	return ADVANCED_TOKEN{ (USHORT)end_of_indentation, reinterpret_cast<TOKEN*>(content), (USHORT)start_of_comment };
}
void CEditor::ExpandToken(wchar_t* line, ADVANCED_TOKEN& token)
{
	auto expand_expression = [=](wchar_t* expr, USHORT& number) -> TOKEN* {
		// 传入number作为expr的长度，接受number作为TOKEN数组长度
		USHORT count = 1; // 前后各加一个
		for (USHORT index = 0; index != number; index++) {
			if (expr[index] >= L'0' and expr[index] <= L'9') {
				for (; expr[index] >= L'0' and expr[index] <= L'9' or expr[index] == L'e'; index++);
				index--;
				count += 2; // 每个相邻元素间一个
			}
			else if (expr[index] == L'\"' or expr[index] == L'\'') {
				size_t skipped = skip_string(expr + index);
				index += skipped;
				count += 2; // 每个相邻元素间一个
			}
			else if (expr[index] >= L'a' and expr[index] <= L'z' or
				expr[index] >= L'A' and expr[index] <= L'Z' or
				expr[index] == L'_') {
				for (; expr[index] >= L'a' and expr[index] <= L'z' or
					expr[index] >= L'A' and expr[index] <= L'Z' or
					expr[index] >= L'0' and expr[index] <= L'9' or
					expr[index] == L'_';
					index++);
				index--;
				count += 2; // 每个相邻元素间一个
			}
		}
		TOKEN* part_token = new TOKEN[count];
		count = 0;
		SHORT last_element = -1;
		for (USHORT index = 0; index != number; index++) {
			if (expr[index] >= 48 and expr[index] <= 57) {
				part_token[count] = TOKEN{ (USHORT)(index - last_element - 1), TOKENTYPE::Punctuation };
				count++;
				USHORT index2 = index;
				for (; expr[index2] >= 48 and expr[index2] <= 57 or expr[index2] == L'e'; index2++);
				part_token[count] = TOKEN{ (USHORT)(index2 - index), TOKENTYPE::Integer };
				count++;
				index = index2 - 1;
				last_element = index;
			}
			else if (expr[index] == L'\"' or expr[index] == L'\'') {
				part_token[count] = TOKEN{ (USHORT)(index - last_element - 1), TOKENTYPE::Punctuation };
				count++;
				size_t skipped = skip_string(expr + index);
				part_token[count] = TOKEN{ (USHORT)(skipped + 1), TOKENTYPE::String };
				index += skipped;
				count++;
				last_element = index;
			}
			else if (expr[index] >= L'a' and expr[index] <= L'z' or
				expr[index] >= L'A' and expr[index] <= L'Z' or
				expr[index] == L'_') {
				part_token[count] = TOKEN{ (USHORT)(index - last_element - 1), TOKENTYPE::Punctuation };
				count++;
				USHORT index2 = index;
				for (; expr[index2] >= L'a' and expr[index2] <= L'z' or
					expr[index2] >= L'A' and expr[index2] <= L'Z' or
					expr[index2] >= L'0' and expr[index2] <= L'9' or
					expr[index2] == L'_';
					index2++);
				// 将TRUE和FALSE渲染为整数
				if ((index2 - index) == 4 and match_keyword(expr + index, L"TRUE") == 0) {
					part_token[count] = TOKEN{ 4, TOKENTYPE::Integer };
				}
				else if ((index2 - index) == 5 and match_keyword(expr + index, L"FALSE") == 0) {
					part_token[count] = TOKEN{ 5, TOKENTYPE::Integer };
				}
				// 将AND，OR和NOT渲染为关键字
				else if ((index2 - index) == 3 and match_keyword(expr + index, L"AND") == 0) {
					part_token[count] = TOKEN{ 3, TOKENTYPE::Keyword };
				}
				else if ((index2 - index) == 2 and match_keyword(expr + index, L"OR") == 0) {
					part_token[count] = TOKEN{ 2, TOKENTYPE::Keyword };
				}
				else if ((index2 - index) == 3 and match_keyword(expr + index, L"NOT") == 0) {
					part_token[count] = TOKEN{ 3, TOKENTYPE::Keyword };
				}
				else {
					part_token[count] = TOKEN{ (USHORT)(index2 - index), TOKENTYPE::Variable };
				}
				count++;
				index = index2 - 1;
				last_element = index;
			}
		}
		part_token[count] = TOKEN{ (USHORT)(number - last_element - 1), TOKENTYPE::Punctuation };
		number = count + 1;
		return part_token;
		};
	auto expand_enumeration = [=](wchar_t* expr, USHORT& number) -> TOKEN* {
		USHORT count = 3; // 前后各一个
		for (USHORT index = 1; index != number; index++) {
			if (expr[index] == L',') {
				count += 2; // 元素之间一个
			}
		}
		TOKEN* part_token = new TOKEN[count];
		part_token[0] = TOKEN{ 1, TOKENTYPE::Punctuation };
		count = 1;
		USHORT last_splitter = 0;
		for (USHORT index = 1; index != number; index++) {
			if (expr[index] == L',' or expr[index] == L')') {
				part_token[count] = TOKEN{ (USHORT)(index - last_splitter - 1), TOKENTYPE::Variable };
				count++;
				part_token[count] = TOKEN{ 1, TOKENTYPE::Punctuation };
				count++;
				last_splitter = index;
			}
		}
		part_token[count - 1].length = number - last_splitter;
		number = count;
		return part_token;
		};
	auto expand_parameters = [=](wchar_t* expr, USHORT& number) -> TOKEN* {
		USHORT count = 1;
		for (USHORT index = 1; index != number; index++) {
			if (expr[index] == L',') {
				count++;
			}
		}
		count = count * 5 + 1; // 首尾各一个，每个参数四个，参数之间一个
		TOKEN* part_token = new TOKEN[count];
		part_token[0] = TOKEN{ 1, TOKENTYPE::Punctuation };
		count = 1;
		USHORT last_colon = 0;
		USHORT last_splitter = 0;
		for (USHORT index = 1; index != number; index++) {
			switch (expr[index]) {
			case L':':
			{
				USHORT space = last_splitter;
				bool variable_scanned = false;
				for (USHORT index2 = index - 1; index2 != 0; index2--) {
					if (expr[index2] == L' ') {
						if (variable_scanned) {
							space = index2;
							break;
						}
					}
					else {
						variable_scanned = true;
					}
				}
				part_token[count] = TOKEN{ (USHORT)(space - last_splitter), TOKENTYPE::PassingMode };
				count++;
				part_token[count] = TOKEN{ (USHORT)(index - space - 1), TOKENTYPE::Variable };
				count++;
				part_token[count] = TOKEN{ 1, TOKENTYPE::Punctuation };
				count++;
				last_colon = index;
				break;
			}
			case L',': case L')':
				part_token[count] = TOKEN{ (USHORT)(index - last_colon - 1), TOKENTYPE::Type };
				count++;
				part_token[count] = TOKEN{ 1, TOKENTYPE::Punctuation };
				count++;
				last_splitter = index;
				break;
			}
		}
		part_token[count - 1].length += number - last_splitter - 1;
		number = count;
		return part_token;
		};
	USHORT offset = token.indentation;
	USHORT index = 0;
	USHORT size = 1;
	for (TOKEN* this_token = token.tokens; this_token->type != TOKENTYPE::Null; this_token++) {
		size++;
	}
	for (TOKEN* this_token = token.tokens; this_token->type != TOKENTYPE::Null; this_token++) {
		if (this_token->type == TOKENTYPE::Expression) {
			USHORT number = this_token->length;
			TOKEN* part_token = expand_expression(line + offset, number);
			TOKEN* new_token = new TOKEN[size + number - 1];
			memcpy(new_token, token.tokens, sizeof(TOKEN) * index);
			memcpy(new_token + index, part_token, sizeof(TOKEN) * number);
			memcpy(new_token + index + number, token.tokens + index + 1, sizeof(TOKEN) * (size - index - 1));
			size += number - 1;
			index += number - 1;
			offset += this_token->length;
			delete[] part_token;
			delete[] token.tokens;
			token.tokens = new_token;
			this_token = token.tokens + index;
		}
		else if (this_token->type == TOKENTYPE::Enumeration) {
			USHORT number = this_token->length;
			TOKEN* part_token = expand_enumeration(line + offset, number);
			TOKEN* new_token = new TOKEN[size + number - 1];
			memcpy(new_token, token.tokens, sizeof(TOKEN) * index);
			memcpy(new_token + index, part_token, sizeof(TOKEN) * number);
			memcpy(new_token + index + number, token.tokens + index + 1, sizeof(TOKEN) * (size - index - 1));
			size += number - 1;
			index += number - 1;
			offset += this_token->length;
			delete[] part_token;
			delete[] token.tokens;
			token.tokens = new_token;
			this_token = token.tokens + index;
		}
		else if (this_token->type == TOKENTYPE::Parameter) {
			USHORT number = this_token->length;
			TOKEN* part_token = expand_parameters(line + offset, number);
			TOKEN* new_token = new TOKEN[size + number - 1];
			memcpy(new_token, token.tokens, sizeof(TOKEN) * index);
			memcpy(new_token + index, part_token, sizeof(TOKEN) * number);
			memcpy(new_token + index + number, token.tokens + index + 1, sizeof(TOKEN) * (size - index - 1));
			size += number - 1;
			index += number - 1;
			offset += this_token->length;
			delete[] part_token;
			delete[] token.tokens;
			token.tokens = new_token;
			this_token = token.tokens + index;
		}
		else {
			offset += this_token->length;
		}
		index++;
	}
}
inline void CEditor::ParseLine()
{
	ADVANCED_TOKEN token = LineSplit(*m_CurrentTag->m_CurrentLine);
	CONSTRUCT* construct = Construct::parse(reinterpret_cast<wchar_t*>(token.tokens));
	delete[] reinterpret_cast<wchar_t*>(token.tokens);
	if (construct->result.tokens) {
		token.tokens = construct->result.tokens;
		ExpandToken(*m_CurrentTag->m_CurrentLine, token);
	}
	else {
		token.tokens = nullptr;
	}
	delete construct;
	delete (*(*m_CurrentTag->m_Tokens)[m_PointerPoint.y]).tokens;
	*(*m_CurrentTag->m_Tokens)[m_PointerPoint.y] = token;
}
DWORD CEditor::BackendTasking(LPVOID)
{
	SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
	while (not pObject->m_bBackendPending) {
		MSG msg;
		if (PeekMessageW(&msg, pObject->m_hWnd, NULL, NULL, PM_NOREMOVE)) {
			Sleep(500);
		}
		else {
			pObject->RecalcWidth();
			pObject->ParseAll();
		}
	}
	return 0;
}
void CEditor::RecalcWidth()
{
	UINT64 temp = 0;
	for (IndexedList<wchar_t*>::iterator this_line = m_CurrentTag->m_Lines.begin(); this_line != m_CurrentTag->m_Lines.end(); this_line++) {
		if (m_BackendLock.try_lock()) {
			temp = max(temp, GET_TEXT_WIDTH(*this_line, wcslen(*this_line)));
			m_BackendLock.unlock();
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
	IndexedList<ADVANCED_TOKEN>* new_token_list = new IndexedList<ADVANCED_TOKEN>;
	new_token_list->set_construction(true);
	for (IndexedList<wchar_t*>::iterator this_line = m_CurrentTag->m_Lines.begin(); this_line != m_CurrentTag->m_Lines.end(); this_line++) {
		if (m_BackendLock.try_lock()) {
			ADVANCED_TOKEN token = LineSplit(*this_line);
			CONSTRUCT* construct = Construct::parse(reinterpret_cast<wchar_t*>(token.tokens));
			delete[] reinterpret_cast<wchar_t*>(token.tokens);
			if (construct->result.tokens) {
				token.tokens = construct->result.tokens;
				ExpandToken(*this_line, token);
			}
			else {
				token.tokens = nullptr;
			}
			delete construct;
			new_token_list->append(token);
			m_BackendLock.unlock();
		}
		else {
			for (IndexedList<ADVANCED_TOKEN>::iterator iter = new_token_list->begin(); iter != new_token_list->end(); iter++) {
				delete[] iter->tokens;
			}
			delete new_token_list;
			return;
		}
	}
	new_token_list->set_construction(false);
	// 即使令牌并未发生改变，重新构造跳跃式链表也会使得访问速度提升
	m_BackendLock.lock();
	if (m_CurrentTag->m_Lines.size() == new_token_list->size()) {
		for (IndexedList<ADVANCED_TOKEN>::iterator iter = m_CurrentTag->m_Tokens->begin(); iter != m_CurrentTag->m_Tokens->end(); iter++) {
			delete[] iter->tokens;
		}
		delete m_CurrentTag->m_Tokens;
		m_CurrentTag->m_Tokens = new_token_list;
		if (not m_CurrentTag->m_bParsed) {
			double start_line = m_PercentageVertical * m_CurrentTag->m_Lines.size();
			int x_offset = LINE_NUMBER_OFFSET + m_LineNumberWidth - (long)(m_PercentageHorizontal * m_FullWidth);
			Invalidate(FALSE);
			m_CurrentTag->m_bParsed = true;
		}
	}
	else {
		for (IndexedList<ADVANCED_TOKEN>::iterator iter = new_token_list->begin(); iter != new_token_list->end(); iter++) {
			delete[] iter->tokens;
		}
		delete new_token_list;
	}
	m_BackendLock.unlock();
}
void CEditor::UpdateCandidates()
{

}
