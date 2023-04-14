#include "test.h"

CBrush* pGreyBlackBrush = new CBrush(RGB(30, 30, 30));
namespace CConsole {
	CFont font;
	CBrush selectionColor;
}

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
	m_bFocus = false;
	m_Width = m_Height = 0;
	m_FullLength = 0;
	m_Percentage = 0.0;
	m_bDrag = false;
	m_AccumulatedHeightUnit = 0;
	m_TotalHeightUnit = 0;

	pObject = this;
}
CConsoleOutput::~CConsoleOutput()
{
	for (std::list<LINE>::iterator iter = m_Lines.begin(); iter != m_Lines.end(); iter++) {
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

	CDC* pWindowDC = GetDC();
	// 创建缓存源
	m_Temp.CreateCompatibleDC(pWindowDC);
	// 创建渲染文字源
	m_Source.CreateCompatibleDC(pWindowDC);
	m_Selection.CreateCompatibleDC(pWindowDC);
	m_Pointer.CreateCompatibleDC(pWindowDC);
	m_Source.SelectObject(CConsole::font);
	// 计算字符大小
	CRect rect(0, 0, 0, 0);
	m_Source.DrawTextW(L" ", 1, &rect, DT_CALCRECT);
	m_CharSize = CSize(rect.right, rect.bottom);
	m_Source.SetTextColor(RGB(255, 255, 255));
	m_Source.SetBkColor(RGB(0, 0, 0));
	// 绘制光标
	CBitmap* pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, 1, m_CharSize.cy);
	m_Pointer.SelectObject(pBitmap);
	rect = CRect(0, 0, 1, m_CharSize.cy);
	CBrush brush(RGB(149, 149, 149));
	m_Pointer.FillRect(&rect, &brush);
	// 创建滚动条
	/*m_Slider.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, NULL);
	m_Slider.SetRatio(1.0);
	m_Slider.SetCallback(VerticalCallback);*/
	// 初始化缓存
	ClearBuffer();

	return 0;
}
void CConsoleOutput::OnPaint()
{
	CPaintDC dc(this);
	m_Temp.BitBlt(0, 0, m_Width, m_Height, &m_Selection, 0, 0, SRCCOPY);
	m_Temp.TransparentBlt(0, 0, m_Width, m_Height, &m_Source, 0, 0, m_Width, m_Height, 0);
	if (m_bFocus) {
		m_Temp.BitBlt(m_cPointer.x, m_cPointer.y, 1, m_CharSize.cy, &m_Pointer, 0, 0, SRCCOPY);
	}
	dc.BitBlt(0, 0, m_Width, m_Height, &m_Temp, 0, 0, SRCCOPY);
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

	CDC* pWindowDC = GetDC();

	CBitmap* pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, m_Width, m_Height);
	CBitmap* pOldBitmap = m_Temp.SelectObject(pBitmap);
	if (pOldBitmap) {
		pOldBitmap->DeleteObject();
	}
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, m_Width, m_Height);
	pOldBitmap = m_Source.SelectObject(pBitmap);
	if (pOldBitmap) {
		pOldBitmap->DeleteObject();
	}
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, m_Width, m_Height);
	pOldBitmap = m_Selection.SelectObject(pBitmap);
	if (pOldBitmap) {
		pOldBitmap->DeleteObject();
	}
	RecalcTotalHeight();
	ArrangeText();
	ArrangePointer();
	ArrangeSelection();
	// 更新滚动条
	CRect rect(cx - 10, 0, cx, cy);
	//m_Slider.MoveWindow(&rect);
	UpdateSlider();
}
void CConsoleOutput::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDrag) {
		TranslatePointer(point);
		ArrangePointer();
		ArrangeSelection();
		Invalidate(FALSE);
	}
}
void CConsoleOutput::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
	SetFocus();
	m_DragPointerPoint = TranslatePointer(point);
	m_bDrag = true;
	ArrangePointer();
	ArrangeSelection();
	Invalidate(FALSE);
}
void CConsoleOutput::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_bDrag = false;
}
BOOL CConsoleOutput::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	//m_Slider.SetPercentage(m_Percentage + (double)zDelta / 120 / m_TotalHeightUnit);
	return TRUE;
}
void CConsoleOutput::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	m_bFocus = true;
	Invalidate(FALSE);
}
void CConsoleOutput::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	m_bFocus = false;
	Invalidate(FALSE);
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
			LINE& last_line = m_Lines.back();
			size_t original_length = wcslen(last_line.line);
			wchar_t* combined_line = new wchar_t[original_length + index - last_return];
			memcpy(combined_line, last_line.line, original_length * 2);
			memcpy(combined_line + original_length, wchar_buffer + last_return + 1, (index - last_return - 1) * 2);
			combined_line[original_length + index - last_return - 1] = 0;
			delete[] last_line.line;
			USHORT orginal_height_unit = last_line.height_unit;
			last_line = EncodeLine(combined_line);
			m_TotalHeightUnit += (size_t)last_line.height_unit - orginal_height_unit;
			if (wchar_buffer[index] == 10) {
				// 创建新行
				m_Lines.push_back(LINE{ new wchar_t[] {0}, 0, 1, 0 });
				m_TotalHeightUnit++;
				MovePointer(CPoint(0, m_Lines.size() - 1));
			}
			else {
				MovePointer(CPoint(original_length + index - last_return - 1, m_Lines.size() - 1));
				break;
			}
			last_return = index;
		}
	}
	m_FullLength = m_TotalHeightUnit * m_CharSize.cy;
	delete[] wchar_buffer;
	m_Lock.unlock();
	UpdateSlider();
	//m_Slider.SetPercentage(1.0);
	Invalidate(FALSE);
}
void CConsoleOutput::AppendInput(wchar_t* input, DWORD count)
{
	std::list<LINE>::iterator last_line = m_Lines.end();
	last_line--;
	size_t original_length = wcslen(last_line->line);
	wchar_t* combined_line = new wchar_t[original_length + count + 1];
	memcpy(combined_line, last_line->line, original_length * 2);
	memcpy(combined_line + original_length, input, (size_t)count * 2);
	combined_line[original_length + count] = 0;
	delete[] last_line->line;
	last_line->line = combined_line;
	CRect rect(0, 0, m_Width, 0);
	m_Source.DrawTextW(combined_line, -1, &rect, DT_CALCRECT);
	USHORT difference = rect.bottom / m_CharSize.cy - last_line->height_unit;
	last_line->height_unit += difference;
	m_Lines.push_back(LINE{ new wchar_t[] {0}, 0, 1, 0 });
	m_TotalHeightUnit += difference + 1;
}
void CConsoleOutput::VerticalCallback(double percentage)
{
	pObject->m_Percentage = percentage;
	pObject->ArrangeText();
	pObject->ArrangePointer();
	pObject->ArrangeSelection();
	pObject->Invalidate(FALSE);
}
CConsoleOutput::LINE CConsoleOutput::EncodeLine(wchar_t* line)
{
	LINE line_out{};
	line_out.line = line;
	line_out.length = wcslen(line);
	if (m_Width <= 0) {
		line_out.height_unit = 1;
		line_out.offset = 0;
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
		line_out.offset = offset;
	}
	return line_out;
}
void CConsoleOutput::RecalcTotalHeight()
{
	if (m_Width <= 0) { return; }
	m_TotalHeightUnit = 0;
	m_AccumulatedHeightUnit = 0;
	ULONG index = 0;
	for (std::list<LINE>::iterator this_line = m_Lines.begin(); this_line != m_Lines.end(); this_line++) {
		this_line->height_unit = 0;
		ULONG offset = 0;
		while (offset != this_line->length) {
			this_line->offset = offset;
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
		if (index < m_PointerPoint.y) {
			m_AccumulatedHeightUnit += this_line->height_unit;
		}
		index++;
	}
}
void CConsoleOutput::ArrangeText()
{
	if (m_Width <= 0) { return; }
	m_Lock.lock();
	// 计算起始行
	double start_height_unit = m_Percentage * m_TotalHeightUnit;
	m_Source.PatBlt(0, 0, m_Width, m_Height, BLACKNESS);
	std::list<LINE>::iterator this_line = m_Lines.begin();
	ULONG accumulated_height_unit = 0;
	for (; accumulated_height_unit <= start_height_unit;) {
		accumulated_height_unit += this_line->height_unit;
		this_line++;
	}
	this_line--;
	accumulated_height_unit -= this_line->height_unit;
	CRect rect(
		0,
		((double)accumulated_height_unit - start_height_unit) * m_CharSize.cy,
		m_Width,
		((double)accumulated_height_unit - start_height_unit + 1) * m_CharSize.cy
	);
	// 计算自动换行
	USHORT width_unit = m_Width / m_CharSize.cx;
	for (; this_line != m_Lines.end(); this_line++) {
		ULONG offset = 0;
		while (offset != this_line->length) {
			int number;
			static CSize size;
			GetTextExtentExPointW(m_Source, this_line->line + offset, this_line->length - offset, m_Width, &number, NULL, &size);
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
	double difference = (double)m_AccumulatedHeightUnit - start_line;
	ULONG offset = 0;
	ULONG remain = m_PointerPoint.x;
	USHORT count = 0;
	while (true) {
		static CSize size;
		int number;
		GetTextExtentExPointW(m_Source, m_CurrentLine->line + offset, m_CurrentLine->length - offset, m_Width, &number, NULL, &size);
		if (number >= remain) {
			CRect rect;
			m_Source.DrawTextW(m_CurrentLine->line + offset, remain, &rect, DT_CALCRECT);
			m_cPointer = CPoint(rect.right, (difference + count) * m_CharSize.cy);
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
	LONG relative = (m_AccumulatedHeightUnit - start_line) * m_CharSize.cy;
	if (m_PointerPoint.y == m_DragPointerPoint.y) {
		if (m_PointerPoint.x == m_DragPointerPoint.x) {
			return;
		}
		else {
			UINT smaller = min(m_PointerPoint.x, m_DragPointerPoint.x);
			UINT larger = max(m_PointerPoint.x, m_DragPointerPoint.x);
			fill_selection(*m_CurrentLine, smaller, larger, relative);
		}
	}
	else if (m_PointerPoint.y > m_DragPointerPoint.y) {
		//正向选区
		std::list<LINE>::iterator this_line = m_CurrentLine;
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
		std::list<LINE>::iterator this_line = m_CurrentLine;
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
	//m_Slider.SetRatio((double)m_Height / (m_TotalHeightUnit * m_CharSize.cy));
}
CPoint CConsoleOutput::TranslatePointer(CPoint point)
{
	point.x = max(point.x, 0);
	double start_line = m_Percentage * m_TotalHeightUnit;
	// 计算行指针
	ULONG target_height_unit = min(start_line + (double)point.y / m_CharSize.cy, m_TotalHeightUnit - 1);
	while (target_height_unit > m_AccumulatedHeightUnit) {
		m_AccumulatedHeightUnit += m_CurrentLine->height_unit;
		m_CurrentLine++;
		m_PointerPoint.y++;
	}
	while (target_height_unit < m_AccumulatedHeightUnit) {
		m_CurrentLine--;
		m_PointerPoint.y--;
		m_AccumulatedHeightUnit -= m_CurrentLine->height_unit;
	}
	// 计算列指针
	USHORT line_count = point.y / m_CharSize.cy - m_AccumulatedHeightUnit;
	size_t total_length = 0;
	ULONG offset = 0;
	static CSize size;
	int number;
	while (line_count) {
		GetTextExtentExPointW(m_Source, m_CurrentLine->line + offset, m_CurrentLine->length - offset, m_Width, &number, NULL, &size);
		offset += number;
		line_count--;
	}
	GetTextExtentExPointW(m_Source, m_CurrentLine->line + offset, m_CurrentLine->length - offset, point.x, &number, NULL, &size);
	CRect rect;
	m_Source.DrawTextW(m_CurrentLine->line + offset, number, &rect, DT_CALCRECT);
	if (offset + number == m_CurrentLine->length) {
		// 最后一个字符
		m_PointerPoint.x = m_CurrentLine->length;
	}
	else {
		// 判断指针是否离右边更近
		point.x -= rect.right;
		GetTextExtentExPointW(m_Source, m_CurrentLine->line + offset + number, 1, 65535, NULL, NULL, &size);
		if (point.x > size.cx / 2) {
			m_PointerPoint.x = offset + number + 1;
		}
		else {
			m_PointerPoint.x = offset + number;
		}
	}
	return m_PointerPoint;
}
void CConsoleOutput::MovePointer(CPoint point)
{
	LONG difference = point.y - m_PointerPoint.y;
	// 调整行指针
	if (difference < 0) {
		for (long long index = 0; index != difference; index--) {
			m_CurrentLine--;
			m_AccumulatedHeightUnit -= m_CurrentLine->height_unit;
		}
	}
	else if (difference > 0) {
		for (long long index = 0; index != difference; index++) {
			m_CurrentLine++;
			m_AccumulatedHeightUnit += m_CurrentLine->height_unit;
		}
	}
	m_DragPointerPoint = m_PointerPoint = point;
}
void CConsoleOutput::ClearBuffer()
{
	for (std::list<LINE>::iterator iter = m_Lines.begin(); iter != m_Lines.end(); iter++) {
		delete[] iter->line;
	}
	m_Lines.clear();
	m_Lines.push_back(LINE{ new wchar_t[] {0}, 1, 1, 0 });
	m_CurrentLine = m_Lines.begin();
	m_TotalHeightUnit = 1;
	m_AccumulatedHeightUnit = 0;
	m_PointerPoint = m_DragPointerPoint = CPoint(0, 0);
	ArrangeText();
	ArrangePointer();
	ArrangeSelection();
	UpdateSlider();
	Invalidate(FALSE);
}

BEGIN_MESSAGE_MAP(CTestWnd, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()
CTestWnd::CTestWnd() noexcept
{
}
CTestWnd::~CTestWnd()
{
}
int CTestWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_Component.Create(0, 0, WS_CHILD | WS_VISIBLE, CRect(), this, NULL);
	m_Component.AppendText(const_cast<char*>("整数(1)整数(2)整数(2)整数(2)整数(2)\n"), 46);
	m_Component.AppendText(const_cast<char*>("整数(2)整数(2)整数(2)整数(2)整数(2)\n"), 46);
	m_Component.AppendText(const_cast<char*>("整数(3)整数(2)整数(2)整数(2)整数(2)\n"), 46);
	m_Component.AppendText(const_cast<char*>("整数(4)整数(2)整数(2)整数(2)整数(2)\n"), 46);
	m_Component.AppendText(const_cast<char*>("整数(5)整数(2)整数(2)整数(2)整数(2)\n"), 46);

	return 0;
}
void CTestWnd::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);
	CRect rect(0, 0, cx, cy);
	m_Component.MoveWindow(&rect);
}

App::App() noexcept
{
	AddFontResourceW(L"YAHEI CONSOLAS HYBRID.TTF");
	CConsole::font.CreateFontW(24, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"YAHEI CONSOLAS HYBRID");
	CConsole::selectionColor.CreateSolidBrush(RGB(38, 79, 120));
}
App::~App()
{
}
BOOL App::InitInstance()
{
	CWinAppEx::InitInstance();
	m_pMainWnd = (CFrameWnd*)new CTestWnd;
	m_pMainWnd->CreateEx(NULL, NULL, L"Test", WS_OVERLAPPEDWINDOW, CRect(0, 0, 400, 400), NULL, NULL);
	m_pMainWnd->ShowWindow(SW_NORMAL);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

App theApp;