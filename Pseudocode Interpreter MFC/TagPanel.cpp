#include "pch.h"

BEGIN_MESSAGE_MAP(CFileTag, CWnd)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()
CFileTag::CFileTag(wchar_t* path)
{
	m_Width = 0;
	m_Path = path;
	int last = 0;
	for (int index = 0; path[index] != 0; index++) {
		if (path[index] == '\\') {
			last = index;
		}
	}
	m_Directory = new wchar_t[last + 2];
	m_Filename = new wchar_t[wcslen(path) - last + 1];
	memcpy(m_Directory, path, ((size_t)last + 1) * 2);
	memcpy(m_Filename, path + last + 1, (wcslen(path) - last) * 2);
	m_Directory[last + 1] = 0;
	m_Filename[wcslen(path) - last] = 0;
	m_Handle = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (m_Handle) {
		DWORD size_high = 0;
		DWORD size = GetFileSize(m_Handle, &size_high);
		if (size == INVALID_FILE_SIZE) {
			AfxMessageBox(L"文件读取失败", MB_ICONERROR);
		}
		size_t final_size = (size_t)size + (((size_t)size_high << sizeof(DWORD)));
		char* char_buffer = new char[final_size + 1];
		if (not ReadFile(m_Handle, char_buffer, final_size, nullptr, nullptr)) {
			AfxMessageBox(L"文件读取失败", MB_ICONERROR);
		}
		char_buffer[final_size] = 0;
		size_t buffer_size = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, char_buffer, -1, nullptr, 0);
		wchar_t* wchar_buffer = new wchar_t[buffer_size];
		MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, char_buffer, -1, wchar_buffer, buffer_size);
		delete[] char_buffer;
		ULONG64 offset = 0;
		m_Lines.set_construction(true);
		for (ULONG64 index = 0;; index++) {
			if (wchar_buffer[index] == L'\n') {
				wchar_t* this_line = new wchar_t[index - offset];
				memcpy(this_line, wchar_buffer + offset, (index - offset - 1) * 2);
				this_line[index - offset - 1] = 0;
				m_Lines.append(this_line);
				offset = index + 1;
			}
			else if (wchar_buffer[index] == L'\0') {
				wchar_t* this_line = new wchar_t[index - offset + 1];
				memcpy(this_line, wchar_buffer + offset, (index - offset) * 2);
				this_line[index - offset] = 0;
				m_Lines.append(this_line);
				break;
			}
		}
		m_Lines.set_construction(false);
		delete[] wchar_buffer;
		m_CurrentLine = m_Lines.begin();
	}
	else {
		throw L"文件无法访问";
	}
	m_bHover = false;
	m_bHoverClose = false;
	m_bEdited = false;
	m_bSelected = false;
	m_Tokens = new IndexedList<ADVANCED_TOKEN>;
	m_CurrentOperation = m_Operations.end();
}
CFileTag::CFileTag()
{
	m_Width = 0;
	m_Path = new wchar_t[1] {0};
	m_Directory = new wchar_t[1] {0};
	m_Filename = new wchar_t[] {L"unnamed file"};
	m_Handle = NULL;
	m_bHover = false;
	m_bHoverClose = false;
	m_bEdited = false;
	m_bSelected = false;
	m_Lines.append(new wchar_t[1] {0});
	m_CurrentLine = m_Lines.begin();
	m_Tokens = new IndexedList<ADVANCED_TOKEN>;
	m_CurrentOperation = m_Operations.end();
}
CFileTag::~CFileTag()
{
	CloseHandle(m_Handle);
	delete[] m_Path;
	delete[] m_Filename;
	delete[] m_Directory;
}
void CFileTag::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	m_Width = cx;
	;

}
BOOL CFileTag::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}
void CFileTag::OnPaint()
{
	CPaintDC dc(this);
	if (m_bHover or m_bSelected) {
		MemoryDC.BitBlt(0, 0, m_Width, 55, &m_Hover, 0, 0, SRCCOPY);
	}
	else {
		CRect rect(0, 0, 260, 55);
		MemoryDC.FillRect(&rect, &m_Brush);
	}
	if (m_bSelected) {
		MemoryDC.BitBlt(0, 0, 5, 55, &m_Selected, 0, 0, SRCCOPY);
	}
	if (m_bHover) {
		CPen pen;
		if (m_bHoverClose) {
			pen.CreatePen(PS_SOLID, 2, RGB(200, 200, 200));
		}
		else {
			pen.CreatePen(PS_SOLID, 2, RGB(100, 100, 100));
		}
		MemoryDC.SelectObject(pen);
		MemoryDC.MoveTo(CPoint(m_Width - 25, 20));
		MemoryDC.LineTo(CPoint(m_Width - 10, 35));
		MemoryDC.MoveTo(CPoint(m_Width - 10, 20));
		MemoryDC.LineTo(CPoint(m_Width - 25, 35));
	}
	if (m_Path) {
		if (m_bEdited) {
			MemoryDC.TransparentBlt(12, 6, 40, 20, &m_Edited, 0, 0, 40, 20, 0);
		}
		else {
			MemoryDC.TransparentBlt(12, 6, 40, 20, &m_Latest, 0, 0, 40, 20, 0);
		}
	}
	else {
		MemoryDC.TransparentBlt(12, 6, 40, 20, &m_Newed, 0, 0, 40, 20, 0);
	}
	MemoryDC.SetBkMode(TRANSPARENT);
	MemoryDC.SetTextColor(RGB(255, 255, 255));
	CRect rect(54, 0, m_Width, 30);
	MemoryDC.SelectObject(m_Font1);
	MemoryDC.DrawTextW(m_Filename, &rect, DT_END_ELLIPSIS);
	rect.left = 12;
	rect.top = 30;
	rect.bottom = 65;
	if (m_bHover) {
		rect.right -= 30;
	}
	MemoryDC.SelectObject(m_Font2);
	MemoryDC.DrawTextW(m_Directory, &rect, DT_END_ELLIPSIS);
	dc.BitBlt(0, 0, m_Width, 55, &MemoryDC, 0, 0, SRCCOPY);
}
void CFileTag::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
}
void CFileTag::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	if (m_bHoverClose) {
		Close();
	}
	else if (point.x > 0 and point.x < m_Width and point.y > 0 and point.y < 45) {
		CTagPanel::pObject->ShiftTag(this);
	}
}
void CFileTag::OnMouseMove(UINT nFlags, CPoint point)
{
	if (not m_bHover) {
		TRACKMOUSEEVENT tme{};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		TrackMouseEvent(&tme);
		m_bHover = true;
		REDRAW_WINDOW();
	}
	else {
		if (point.x > m_Width - 30 and point.x < m_Width) {
			m_bHoverClose = true;
		}
		else {
			m_bHoverClose = false;
		}
		REDRAW_WINDOW();
	}
}
void CFileTag::OnMouseLeave()
{
	m_bHover = false;
	REDRAW_WINDOW();
}
void CFileTag::SetEdited()
{
	m_bEdited = true;
	REDRAW_WINDOW();
}
inline void CFileTag::SetSelected(bool state) {
	m_bSelected = state;
	REDRAW_WINDOW();
}
const wchar_t* CFileTag::GetPath() const
{
	return m_Path;
}
void CFileTag::Save()
{
	m_bEdited = false;
	REDRAW_WINDOW();
	if (m_Handle) {
		SetFilePointer(m_Handle, 0, 0, FILE_BEGIN);
		for (IndexedList<wchar_t*>::iterator iter = m_Lines.begin();;) {
			int size = WideCharToMultiByte(CP_UTF8, 0, *iter, -1, nullptr, 0, nullptr, nullptr);
			char* buffer = new char[size - 1];
			WideCharToMultiByte(CP_UTF8, 0, *iter, -1, buffer, size - 1, nullptr, nullptr);
			WriteFile(m_Handle, buffer, size - 1, nullptr, nullptr);
			iter++;
			if (iter == m_Lines.end()) {
				break;
			}
			else {
				WriteFile(m_Handle, "\r\n", 2, nullptr, nullptr);
			}
		}
		FlushFileBuffers(m_Handle);
		SetEndOfFile(m_Handle);
	}
	else {
		CMainFrame::pObject->SendMessageW(WM_COMMAND, ID_FILE_SAVEAS, 0);
	}
}
void CFileTag::SaveAs(wchar_t* new_path)
{
	HANDLE handle = CreateFile(new_path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (handle) {
		for (IndexedList<wchar_t*>::iterator iter = m_Lines.begin();;) {
			int size = WideCharToMultiByte(CP_ACP, 0, *iter, -1, nullptr, 0, nullptr, nullptr);
			char* buffer = new char[size - 1];
			WideCharToMultiByte(CP_ACP, 0, *iter, -1, buffer, size - 1, nullptr, nullptr);
			WriteFile(handle, buffer, size - 1, nullptr, nullptr);
			iter++;
			if (iter == m_Lines.end()) {
				break;
			}
			else {
				WriteFile(handle, "\r\n", 2, nullptr, nullptr);
			}
		}
		FlushFileBuffers(m_Handle);
		SetEndOfFile(handle);
	}
	else {
		MessageBoxW(L"文件无法访问", L"异常", MB_OK | MB_ICONERROR);
	}
}
void CFileTag::Close()
{
	if (m_bEdited) {
		wchar_t* buffer = new wchar_t[19 + wcslen(m_Path)];
		StringCchPrintfW(buffer, 19 + wcslen(m_Path), L"正在关闭存在更改的文件：\n%s\n是否保存", m_Path);
		int result = AfxMessageBox(buffer, MB_YESNOCANCEL);
		delete[] buffer;
		if (result == IDCANCEL) {
			return;
		}
		else if (result == IDYES) {
			Save();
		}
	}
	CTagPanel::pObject->DestroyTag(this);
}

BEGIN_MESSAGE_MAP(CTagPanel, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()
CTagPanel::CTagPanel()
{
	m_CurrentIndex = 0;
	pObject = this;
	m_Width = 0;
}
CTagPanel::~CTagPanel()
{
	for (IndexedList<CFileTag*>::iterator iter = m_Tags.begin(); iter != m_Tags.end(); iter++) {
		delete* iter;
	}
}
int CTagPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 准备选中图案
	CFileTag::m_Hover.CreateCompatibleDC(&ScreenDC);
	CPen* pPen = new CPen(PS_SOLID, 1, RGB(61, 61, 61));
	CFileTag::m_Hover.SelectObject(pPen);

	CFileTag::m_Selected.CreateCompatibleDC(&ScreenDC);
	CBitmap* pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 5, 55);
	CFileTag::m_Selected.SelectObject(pBitmap);
	CRect rect(0, 0, 5, 55);
	CFileTag::m_Selected.FillRect(&rect, pThemeColorBrush);

	// 准备状态图案
	CFileTag::m_Latest.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 40, 20);
	CFileTag::m_Latest.SelectObject(pBitmap);
	CFileTag::m_Newed.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 40, 20);
	CFileTag::m_Newed.SelectObject(pBitmap);
	CFileTag::m_Edited.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 40, 20);
	CFileTag::m_Edited.SelectObject(pBitmap);
	CDC temp;
	temp.CreateCompatibleDC(&ScreenDC);
	CBitmap temp_bitmap;
	temp_bitmap.CreateCompatibleBitmap(&ScreenDC, 400, 200);
	temp.SelectObject(&temp_bitmap);
	rect = CRect(0, 0, 400, 200);
	CFont font;
	font.CreateFontW(180, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	temp.SelectObject(&font);
	temp.SetBkMode(TRANSPARENT);
	CBrush brush(RGB(107, 199, 164));
	temp.SetTextColor(RGB(0, 0, 0));
	temp.SelectObject(&brush);
	temp.SelectObject(pNullPen);
	temp.RoundRect(&rect, CPoint(75, 75));
	temp.DrawTextW(L"最新", -1, &rect, DT_CENTER | DT_VCENTER);
	CFileTag::m_Latest.SetStretchBltMode(HALFTONE);
	CFileTag::m_Latest.StretchBlt(0, 0, 40, 20, &temp, 0, 0, 400, 200, SRCCOPY);
	brush.DeleteObject();
	brush.CreateSolidBrush(RGB(190, 183, 255));
	temp.SetTextColor(RGB(0, 0, 0));
	temp.SetBkMode(TRANSPARENT);
	temp.SelectObject(&brush);
	temp.SelectObject(pNullPen);
	temp.RoundRect(&rect, CPoint(75, 75));
	temp.SelectObject(font);
	temp.DrawTextW(L"新建", -1, &rect, DT_CENTER);
	CFileTag::m_Newed.SetStretchBltMode(HALFTONE);
	CFileTag::m_Newed.StretchBlt(0, 0, 40, 20, &temp, 0, 0, 400, 200, SRCCOPY);
	brush.DeleteObject();
	brush.CreateSolidBrush(RGB(202, 205, 56));
	temp.SetTextColor(RGB(0, 0, 0));
	temp.SetBkMode(TRANSPARENT);
	temp.SelectObject(&brush);
	temp.SelectObject(pNullPen);
	temp.RoundRect(&rect, CPoint(75, 75));
	temp.SelectObject(font);
	temp.DrawTextW(L"更新", -1, &rect, DT_CENTER);
	CFileTag::m_Edited.SetStretchBltMode(HALFTONE);
	CFileTag::m_Edited.StretchBlt(0, 0, 40, 20, &temp, 0, 0, 400, 200, SRCCOPY);

	// 准备字体
	CFileTag::m_Font1.CreateFontW(30, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	CFileTag::m_Font2.CreateFontW(20, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");

	// 准备背景
	m_Source.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 300, SCREEN_HEIGHT);
	m_Source.SelectObject(pBitmap);
	m_Source.SetTextColor(RGB(240, 240, 240));
	m_Source.SetBkMode(TRANSPARENT);
	font.DeleteObject();
	font.CreateFontW(30, 0, 0, 0, FW_BOLD, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	m_Source.SelectObject(font);
	rect = CRect(0, 0, 300, SCREEN_HEIGHT);
	CFileTag::m_Brush.CreateSolidBrush(RGB(37, 37, 38));
	m_Source.FillRect(&rect, &CFileTag::m_Brush);
	rect = CRect(20, 23, 25, 49);
	m_Source.FillRect(&rect, pThemeColorBrush);
	rect = CRect(30, 20, 280, 50);
	m_Source.DrawTextW(L"已打开的文件", -1, &rect, DT_SINGLELINE);

	return 0;
}
void CTagPanel::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	m_Width = cx;

	HBITMAP hBitmap = CreateCompatibleBitmap(ScreenDC, cx - 40, 55);
	HGLOBAL hOldBitmap = SelectObject(CFileTag::m_Hover, hBitmap);
	DeleteObject(hOldBitmap);

	CRect rect(0, 0, cx - 40, 55);
	static CBrush brush(RGB(56, 56, 56));
	CFileTag::m_Hover.FillRect(&rect, &brush);
	rect = CRect(20, 75, cx - 20, 130);
	for (USHORT index = 0; index != m_Tags.size(); index++) {
		(*m_Tags[index])->MoveWindow(&rect, TRUE);
		rect.top += 65;
		rect.bottom += 65;
	}
}
BOOL CTagPanel::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}
void CTagPanel::OnPaint()
{
	CPaintDC dc(this);
	dc.BitBlt(0, 0, 300, SCREEN_HEIGHT, &m_Source, 0, 0, SRCCOPY);
}
void CTagPanel::OnNew()
{
	CFileTag* new_tag = new CFileTag;
	m_Tags.append(new_tag);
	USHORT index = m_Tags.size() - 1;
	new_tag->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(20, 75 + index * 65, m_Width - 20, 130 + index * 65), this, NULL);
	ShiftTag(new_tag);
}
void CTagPanel::OnOpen()
{
	OPENFILENAMEW ofn{};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = CMainFrame::pObject->GetSafeHwnd();
	ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0Pseudocode Files (*.pc)\0*.pc\0";
	ofn.lpstrFile = new wchar_t[MAX_PATH] {0};
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_CREATEPROMPT | OFN_HIDEREADONLY;
	GetOpenFileNameW(&ofn);
	if (*ofn.lpstrFile) {
		OpenFile(ofn.lpstrFile);
	}
}
void CTagPanel::OnSave()
{
	size_t length = wcslen((*m_Tags[m_CurrentIndex])->GetPath());
	GetCurrentTag()->Save();
	wchar_t* message = new wchar_t[5 + length];
	StringCchPrintfW(message, 5 + length, L"已保存 %s", (*m_Tags[m_CurrentIndex])->GetPath());
	CCustomStatusBar::pObject->UpdateMessage(message);
}
void CTagPanel::OnSaveAs()
{
	OPENFILENAME ofn{};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = CMainFrame::pObject->GetSafeHwnd();
	ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0Pseudocode Files (*.pc)\0*.pc\0";
	ofn.lpstrFile = new wchar_t[MAX_PATH] {0};
	ofn.nMaxFile = MAX_PATH;
	GetSaveFileNameW(&ofn);
	if (*ofn.lpstrFile) {
		GetCurrentTag()->SaveAs(ofn.lpstrFile);
		size_t length = wcslen((*m_Tags[m_CurrentIndex])->GetPath());
		wchar_t* message = new wchar_t[5 + length];
		StringCchPrintfW(message, 5 + length, L"已保存 %s", ofn.lpstrFile);
		CCustomStatusBar::pObject->UpdateMessage(message);
	}
	delete[] ofn.lpstrFile;
}
void CTagPanel::OpenFile(wchar_t* filename)
{
	CFileTag* new_tag = new CFileTag(filename);
	m_Tags.append(new_tag);
	USHORT index = m_Tags.size() - 1;
	new_tag->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(20, 75 + index * 65, m_Width - 20, 130 + index * 65), this, NULL);
	ShiftTag(new_tag);
}
void CTagPanel::LoadOpenedFiles()
{
	OpenFile(new wchar_t[] { L"C:\\Users\\Zik\\OneDrive - 6666zik\\Desktop\\code.txt" });
	CEditor::pObject->LoadFile(GetCurrentTag());
}
CFileTag* CTagPanel::GetCurrentTag()
{
	return *m_Tags[m_CurrentIndex];
}
void CTagPanel::ShiftTag(CFileTag* tag)
{
	if (CConsole::pObject->m_bRun) {
		AfxMessageBox(L"代码运行期间禁止查看和编辑其他文件", MB_ICONWARNING);
		return;
	}
	USHORT index = 0;
	for (IndexedList<CFileTag*>::iterator iter = m_Tags.begin(); iter != m_Tags.end(); iter++) {
		if (*iter == tag) {
			GetCurrentTag()->SetSelected(false);
			m_CurrentIndex = index;
			tag->SetSelected(true);
			CEditor::pObject->LoadFile(tag);
			break;
		}
		index++;
	}
}
void CTagPanel::DestroyTag(CFileTag* tag)
{
	bool found = false;
	for (USHORT index = 0; index != m_Tags.size(); index++) {
		if (tag == *m_Tags[index]) {
			found = true;
			tag->DestroyWindow();
			if (m_Tags.size() == 1) {
				CEditor::pObject->LoadFile(nullptr);
			}
			else {
				if (index == 0) {
					ShiftTag(*m_Tags[1]);
				}
				else {
					ShiftTag(*m_Tags[index - 1]);
				}
			}
			delete tag;
			m_Tags.pop(index);
			index--;
		}
		else if (found) {
			CRect rect;
			tag->GetWindowRect(&rect);
			rect.top -= 65;
			rect.bottom -= 65;
		}
	}
}
