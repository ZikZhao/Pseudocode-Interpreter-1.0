#include "pch.h"

BEGIN_MESSAGE_MAP(CFileTag, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()
CFileTag::CFileTag(wchar_t* path)
{
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
			throw L"文件读取失败";
		}
		size_t final_size = (size_t)size + (((size_t)size_high << sizeof(DWORD)));
		char* char_buffer = new char[final_size + 1];
		if (not ReadFile(m_Handle, char_buffer, final_size, nullptr, nullptr)) {
			throw L"文件读取失败";
		}
		char_buffer[final_size] = 0;
		size_t buffer_size = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, char_buffer, -1, nullptr, 0);
		wchar_t* wchar_buffer = new wchar_t[buffer_size];
		MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, char_buffer, -1, wchar_buffer, buffer_size);
		delete[] char_buffer;
		size_t offset = 0;
		for (size_t index = 0;; index++) {
			if (wchar_buffer[index] == 10) {
				wchar_t* this_line = new wchar_t[index - offset];
				memcpy(this_line, wchar_buffer + offset, (index - offset - 1) * 2);
				this_line[index - offset - 1] = 0;
				m_Lines.push_back(this_line);
				offset = index + 1;
			}
			else if (wchar_buffer[index] == 0) {
				wchar_t* this_line = new wchar_t[index - offset + 1];
				memcpy(this_line, wchar_buffer + offset, (index - offset) * 2);
				this_line[index - offset] = 0;
				m_Lines.push_back(this_line);
				break;
			}
		}
		delete[] wchar_buffer;
		m_CurrentLine = m_Lines.begin();
	}
	else {
		throw L"文件无法访问";
	}
}
CFileTag::CFileTag()
{
	m_Path = nullptr;
	m_Directory = m_Filename = nullptr;
	m_Handle = NULL;
	m_bHover = false;
	m_bSelected = false;
}
CFileTag::~CFileTag()
{

}
int CFileTag::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CDC* pWindowDC = GetDC();
	m_Source.CreateCompatibleDC(pWindowDC);
	CBitmap* pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, 210, 60);
	m_Source.SelectObject(pBitmap);
	CRect rect(0, 0, 260, 45);
	CBrush brush(RGB(37, 37, 38));
	m_Source.FillRect(&rect, &brush);
	m_Source.SetBkMode(TRANSPARENT);
	m_Source.SetTextColor(RGB(255, 255, 255));

	CFont font;
	font.CreateFontW(30, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	m_Source.SelectObject(font);
	rect = CRect(12, -6, 260, 24);
	m_Source.DrawTextW(m_Filename, &rect, DT_END_ELLIPSIS);
	rect.top = 24;
	rect.bottom = 60;
	font.DeleteObject();
	font.CreateFontW(20, 0, 0, 0, FW_LIGHT, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	m_Source.SelectObject(font);
	m_Source.DrawTextW(m_Directory, &rect, DT_END_ELLIPSIS);
	return 0;
}
BOOL CFileTag::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}
void CFileTag::OnPaint()
{
	CPaintDC dc(this);
	dc.BitBlt(0, 0, 260, 45, &m_Source, 0, 0, SRCCOPY);
	if (m_bSelected) {
		dc.BitBlt(0, 0, 5, 45, &m_Selected, 0, 0, SRCCOPY);
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
		Invalidate(FALSE);
	}
}
void CFileTag::OnMouseLeave()
{
	m_bHover = false;
	Invalidate(FALSE);
}
inline void CFileTag::SetState(bool state) {
	m_bSelected = state;
	Invalidate(false);
}
const wchar_t* CFileTag::GetPath() const
{
	return m_Path;
}
std::list<wchar_t*>* CFileTag::GetLines()
{
	return &m_Lines;
}
std::list<wchar_t*>::iterator& CFileTag::GetCurrentLine()
{
	return m_CurrentLine;
}
void CFileTag::Save()
{
	if (m_Handle) {
		SetFilePointer(m_Handle, 0, 0, FILE_BEGIN);
		for (std::list<wchar_t*>::iterator iter = m_Lines.begin();;) {
			int size = WideCharToMultiByte(CP_ACP, 0, *iter, -1, nullptr, 0, nullptr, nullptr);
			char* buffer = new char[size - 1];
			WideCharToMultiByte(CP_ACP, 0, *iter, -1, buffer, size - 1, nullptr, nullptr);
			WriteFile(m_Handle, buffer, size - 1, nullptr, nullptr);
			iter++;
			if (iter == m_Lines.end()) {
				break;
			}
			else {
				WriteFile(m_Handle, "\r\n", 2, nullptr, nullptr);
			}
		}
		SetEndOfFile(m_Handle);
	}
	else {
		SaveAs();
	}
}
void CFileTag::SaveAs()
{

}

BEGIN_MESSAGE_MAP(CTagPanel, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()
CTagPanel::CTagPanel()
{
	m_Ptr = 0;
	m_CurrentIndex = 0;
	pObject = this;
}
CTagPanel::~CTagPanel()
{
}
int CTagPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CDC* pWindowDC = GetDC();
	// 准备悬浮与选中图案
	CFileTag::m_Hover.CreateCompatibleDC(pWindowDC);
	CBitmap* pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, 5, 45);
	CFileTag::m_Hover.SelectObject(pBitmap);
	CRect rect(0, 0, 5, 45);
	CBrush brush(RGB(183, 183, 183));
	CFileTag::m_Hover.FillRect(&rect, &brush);

	CFileTag::m_Selected.CreateCompatibleDC(pWindowDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, 5, 45);
	CFileTag::m_Selected.SelectObject(pBitmap);
	rect = CRect(0, 0, 5, 45);
	CFileTag::m_Selected.FillRect(&rect, pThemeColorBrush);

	// 准备字体
	CFileTag::m_Font1.CreateFontW(30, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	CFileTag::m_Font2.CreateFontW(20, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");

	// 准备背景
	m_Source.CreateCompatibleDC(pWindowDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(pWindowDC, 300, SCREEN_HEIGHT);
	m_Source.SelectObject(pBitmap);
	m_Source.SetTextColor(RGB(240, 240, 240));
	m_Source.SetBkMode(TRANSPARENT);
	CFont font;
	font.CreateFontW(30, 0, 0, 0, FW_BOLD, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	m_Source.SelectObject(font);
	rect = CRect(0, 0, 300, SCREEN_HEIGHT);
	CBrush bgcolor(RGB(37, 37, 38));
	m_Source.FillRect(&rect, &bgcolor);
	rect = CRect(20, 23, 25, 49);
	m_Source.FillRect(&rect, pThemeColorBrush);
	rect = CRect(30, 20, 280, 50);
	m_Source.DrawTextW(L"已打开的文件", -1, &rect, DT_SINGLELINE);

	return 0;
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
	m_Tags[m_Ptr] = new CFileTag;
	m_Tags[m_Ptr]->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(20, 75 + m_Ptr * 75, 280, 120 + m_Ptr * 75), this, NULL);
	ShiftTag(m_Ptr);
	m_Ptr++;
}
void CTagPanel::OnOpen()
{
	OPENFILENAMEW ofn{};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = CMainFrame::pObject->GetSafeHwnd();
	ofn.lpstrFilter = L"Text Files (.txt)\0.txt\0Pseudocode Files (.pc)\0.pc\0";
	ofn.lpstrFile = new wchar_t[MAX_PATH] {0};
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_CREATEPROMPT | OFN_NOREADONLYRETURN;
	GetOpenFileNameW(&ofn);
	if (*ofn.lpstrFile) {
		OpenFile(ofn.lpstrFile);
	}
}
void CTagPanel::OnSave()
{
	size_t length = wcslen(m_Tags[m_CurrentIndex]->GetPath());
	wchar_t* message = new wchar_t[8 + length];
	StringCchPrintfW(message, 8 + length, L"正在保存文件 %s", m_Tags[m_CurrentIndex]->GetPath());
	CCustomStatusBar::pObject->UpdateMessage(message);
	delete[] message;
	m_Tags[m_CurrentIndex]->Save();
	message = new wchar_t[5 + length];
	StringCchPrintfW(message, 5 + length, L"已保存 %s", m_Tags[m_CurrentIndex]->GetPath());
	CCustomStatusBar::pObject->UpdateMessage(message);
	delete[] message;
}
void CTagPanel::OpenFile(wchar_t* filename)
{
	m_Tags[m_Ptr] = new CFileTag(filename);
	m_Tags[m_Ptr]->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(20, 75 + m_Ptr * 75, 280, 120 + m_Ptr * 75), this, NULL);
	ShiftTag(m_Ptr);
	m_Ptr++;
	m_CurrentIndex = m_Ptr - 1;
}
void CTagPanel::LoadOpenedFiles()
{
	OpenFile(const_cast<wchar_t*>(L"C:\\Users\\Zik\\OneDrive - 6666zik\\Desktop\\code.txt"));
	CEditor::pObject->LoadFile(GetCurrentTag());
}
CFileTag* CTagPanel::GetCurrentTag()
{
	return m_Tags[m_CurrentIndex];
}
inline void CTagPanel::ShiftTag(USHORT index) {
	m_Tags[m_CurrentIndex]->SetState(false);
	m_CurrentIndex = index;
	m_Tags[m_CurrentIndex]->SetState(true);
}
