#include "pch.h"
#define BEGIN_GROUP(id) { UINT id_group = id;
#define END_GROUP() }
#define BUTTON(id, id_bitmap, button_text, x) { CControlPanelButton* button = new CControlPanelButton(id, id_bitmap, button_text); m_Components.push_back(button); button->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(x, 0, x + 68, 88), this, id); CMainFrame::pObject->m_Tip.AddTool(FIND_BUTTON(id_group, id), id); }
#define SWITCH(id, id_bitmap, button_text, x, initial_state) { CControlPanelSwitch* button = new CControlPanelSwitch(id, id_bitmap, button_text, initial_state); m_Components.push_back(button); button->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(x, 0, x + 68, 88), this, id); CMainFrame::pObject->m_Tip.AddTool(FIND_BUTTON(id_group, id), id); }
#define SPLITTER(x) { CControlPanelSplitter* splitter = new CControlPanelSplitter(); m_Components.push_back(splitter); splitter->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(x, 5, x + 1, 83), this, NULL); }

BEGIN_MESSAGE_MAP(CControlPanelTag, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()
CControlPanelTag::CControlPanelTag(USHORT tag_index, const wchar_t* text)
{
	m_TagIndex = tag_index;
	m_Text = CString(text);
	m_bHover = false;
	m_bSelected = false;
}
CControlPanelTag::~CControlPanelTag()
{
}
int CControlPanelTag::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// 创建主源DC
	m_Source.CreateCompatibleDC(&ScreenDC);
	CBitmap* pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, m_Width, m_Height);
	m_Source.SelectObject(pBitmap);
	m_Source.SelectObject(m_Font);
	m_Source.SetBkColor(RGB(30, 30, 30));
	m_Source.SetTextColor(RGB(200, 200, 200));
	CRect rect(0, 0, m_Width, m_Height);
	m_Source.DrawTextW(m_Text, &rect, 0);
	// 创建悬浮源DC
	m_Hover.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, m_Width, m_Height);
	m_Hover.SelectObject(pBitmap);
	m_Hover.SelectObject(m_Font);
	m_Hover.SetBkColor(RGB(30, 30, 30));
	m_Hover.SetTextColor(RGB(255, 255, 255));
	m_Hover.DrawTextW(m_Text, &rect, 0);
	// 创建选中源DC
	m_Selected.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, m_Width, m_Height);
	m_Selected.SelectObject(pBitmap);
	m_Selected.FillRect(&rect, pGreyBlackBrush);
	CDC temp;
	temp.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, m_Width, 3);
	temp.SelectObject(pBitmap);
	temp.SelectObject(pThemeColorBrush);
	CPen pen(PS_SOLID, 0, RGB(254, 74, 99));
	temp.SelectObject(pen);
	temp.RoundRect(0, 0, m_Width, 3, 2, 2);
	m_Selected.TransparentBlt(0, m_Height - 4, m_Width, 3, &temp, 0, 0, m_Width, 3, 0);
	m_Selected.TransparentBlt(0, 0, m_Width, m_Height, &m_Hover, 0, 0, m_Width, m_Height, RGB(30, 30, 30));

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}
BOOL CControlPanelTag::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CControlPanelTag::OnPaint()
{
	CPaintDC dc(this);
	if (m_bSelected) {
		dc.BitBlt(0, 0, m_Width, m_Height, &m_Selected, 0, 0, SRCCOPY);
	}
	else if (m_bHover) {
		dc.BitBlt(0, 0, m_Width, m_Height, &m_Hover, 0, 0, SRCCOPY);
	}
	else {
		dc.BitBlt(0, 0, m_Width, m_Height, &m_Source, 0, 0, SRCCOPY);
	}
}
void CControlPanelTag::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bHover) {
		TRACKMOUSEEVENT tme{};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		TrackMouseEvent(&tme);
		m_bHover = true;
		REDRAW_WINDOW();
	}
}
void CControlPanelTag::OnMouseLeave()
{
	CWnd::OnMouseLeave();
	m_bHover = false;
	REDRAW_WINDOW();
}
void CControlPanelTag::OnLButtonUp(UINT nFlags, CPoint point)
{
	CControlPanel::pObject->ShiftTag(m_TagIndex);
}
void CControlPanelTag::SetState(bool selected)
{
	m_bSelected = selected;
	REDRAW_WINDOW();
}

void CControlPanelComponent::SetState(bool state)
{
}

BEGIN_MESSAGE_MAP(CControlPanelButton, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()
CControlPanelButton::CControlPanelButton(UINT commandID, UINT resourceID, const wchar_t* button_text)
{
	m_ResourceID = resourceID;
	m_CommandID = commandID;
	m_Text = button_text;
	m_bHover = false;
	m_bDisabled = false;
}
CControlPanelButton::~CControlPanelButton()
{
}
int CControlPanelButton::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CDC* windowDC = GetDC();
	// 创建主源DC
	m_Source.CreateCompatibleDC(windowDC);
	m_Source.SelectObject(m_Font);
	m_Source.SetTextColor(RGB(255, 255, 255));
	m_Source.SetBkColor(RGB(30, 30, 30));
	m_SourceDisabled.CreateCompatibleDC(windowDC);
	static CRect* text_rect = new CRect(4, 64, 64, 84);
	CBitmap* button_bitmap = new CBitmap;
	button_bitmap->CreateCompatibleBitmap(windowDC, 68, 88);
	m_Source.SelectObject(button_bitmap);
	CRect rect(0, 0, 68, 88);
	m_Source.FillRect(&rect, pGreyBlackBrush);
	// 加载按钮位图并绘制
	CDC temp;
	temp.CreateCompatibleDC(windowDC);
	CBitmap icon;
	icon.LoadBitmapW(m_ResourceID);
	temp.SelectObject(&icon);
	m_Source.BitBlt(4, 4, 60, 60, &temp, 0, 0, SRCCOPY);
	m_Source.DrawTextW(m_Text, -1, text_rect, DT_CENTER);
	// 计算灰度图像
	BITMAP bitmap{};
	button_bitmap->GetBitmap(&bitmap);
	BYTE* buffer = new BYTE[bitmap.bmHeight * bitmap.bmWidthBytes];
	button_bitmap->GetBitmapBits(bitmap.bmHeight * bitmap.bmWidthBytes, buffer);
	for (size_t i = 0; i != bitmap.bmHeight; i++) {
		for (size_t j = 0; j != bitmap.bmWidth; j++) {
			size_t offset = (i * bitmap.bmWidth + j) * 4;
			BYTE& alpha = buffer[offset + 3];
			BYTE& red = buffer[offset + 2];
			BYTE& green = buffer[offset + 1];
			BYTE& blue = buffer[offset];
			if (red != 30 or green != 30 or blue != 30) {
				red = green = blue = (BYTE)((red * 0.299 + green * 0.587 + blue * 0.114) * 0.7);
			}
		}
	}
	button_bitmap = new CBitmap;
	button_bitmap->CreateCompatibleBitmap(windowDC, 68, 88);
	button_bitmap->SetBitmapBits(bitmap.bmHeight * bitmap.bmWidthBytes, buffer);
	m_SourceDisabled.SelectObject(button_bitmap);
	delete[] buffer;

	return 0;
}
BOOL CControlPanelButton::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CControlPanelButton::OnPaint()
{
	CPaintDC dc(this);
	static CRect* rect = new CRect(0, 0, 68, 88);
	if (m_bDisabled) {
		dc.BitBlt(0, 0, 68, 88, &m_SourceDisabled, 0, 0, SRCCOPY);
	}
	else {
		if (m_bHover) {
			dc.BitBlt(0, 0, 68, 88, &m_HoverBG, 0, 0, SRCCOPY);
		}
		else {
			dc.FillRect(rect, pGreyBlackBrush);
		}
		dc.TransparentBlt(0, 0, 68, 88, &m_Source, 0, 0, 68, 88, RGB(30, 30, 30));
	}
}
void CControlPanelButton::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bHover) {
		TRACKMOUSEEVENT tme{};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		TrackMouseEvent(&tme);
		m_bHover = true;
		REDRAW_WINDOW();
	}
}
void CControlPanelButton::OnMouseLeave()
{
	m_bHover = false;
	REDRAW_WINDOW();
}
void CControlPanelButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
}
void CControlPanelButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	if (point.x < 68 and point.y < 88 and not m_bDisabled) {
		CMainFrame::pObject->SendMessageW(WM_COMMAND, m_CommandID, NULL);
	}
}
void CControlPanelButton::SetState(bool state)
{
	m_bDisabled = not state;
	Invalidate(FALSE);
}

BEGIN_MESSAGE_MAP(CControlPanelSplitter, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()
CControlPanelSplitter::CControlPanelSplitter()
{
}
CControlPanelSplitter::~CControlPanelSplitter()
{
}
void CControlPanelSplitter::OnPaint()
{
	CPaintDC dc(this);
	dc.BitBlt(0, 0, 1, 78, &m_DC, 0, 0, SRCCOPY);
}

BEGIN_MESSAGE_MAP(CControlPanelSwitch, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()
CControlPanelSwitch::CControlPanelSwitch(UINT commandID, UINT resourceID, const wchar_t* button_text, bool initial_state)
{
	m_ResourceID = resourceID;
	m_CommandID = commandID;
	m_Text = button_text;
	m_bHover = false;
	m_bSwitched = initial_state;
}
CControlPanelSwitch::~CControlPanelSwitch()
{
}
int CControlPanelSwitch::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CDC* windowDC = GetDC();
	// 创建主源DC
	m_Source.CreateCompatibleDC(windowDC);
	m_Source.SelectObject(CControlPanelButton::m_Font);
	m_Source.SetTextColor(RGB(255, 255, 255));
	m_Source.SetBkColor(RGB(30, 30, 30));
	static CRect* text_rect = new CRect(4, 64, 64, 84);
	CBitmap* button_bitmap = new CBitmap;
	button_bitmap->CreateCompatibleBitmap(windowDC, 68, 88);
	m_Source.SelectObject(button_bitmap);
	CRect rect(0, 0, 68, 88);
	m_Source.FillRect(&rect, pGreyBlackBrush);
	// 加载按钮位图并绘制
	CDC temp;
	temp.CreateCompatibleDC(windowDC);
	CBitmap icon;
	icon.LoadBitmapW(m_ResourceID);
	temp.SelectObject(&icon);
	m_Source.BitBlt(4, 4, 60, 60, &temp, 0, 0, SRCCOPY);
	m_Source.DrawTextW(m_Text, -1, text_rect, DT_CENTER);

	return 0;
}
BOOL CControlPanelSwitch::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CControlPanelSwitch::OnPaint()
{
	CPaintDC dc(this);
	static CRect* rect = new CRect(0, 0, 68, 88);
	if (m_bHover or m_bSwitched) {
		dc.BitBlt(0, 0, 68, 88, &CControlPanelButton::m_HoverBG, 0, 0, SRCCOPY);
	}
	else {
		dc.FillRect(rect, pGreyBlackBrush);
	}
	dc.TransparentBlt(0, 0, 68, 88, &m_Source, 0, 0, 68, 88, RGB(30, 30, 30));
}
void CControlPanelSwitch::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bHover) {
		TRACKMOUSEEVENT tme{};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		TrackMouseEvent(&tme);
		m_bHover = true;
		REDRAW_WINDOW();
	}
}
void CControlPanelSwitch::OnMouseLeave()
{
	m_bHover = false;
	REDRAW_WINDOW();
}
void CControlPanelSwitch::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
}
void CControlPanelSwitch::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	if (point.x < 68 and point.y < 88) {
		m_bSwitched = not m_bSwitched;
		switch (m_CommandID) {
		case ID_TOOLS_NL:
			CConsole::new_line_after_output = m_bSwitched;
			break;
		case ID_TOOLS_OO:
			CConsole::output_as_object = m_bSwitched;
			break;
		case ID_TOOLS_NNR:
			CConsole::discard_crlf_on_read = m_bSwitched;
			break;
		case ID_TOOLS_NNW:
			CConsole::automatic_new_line_on_write = m_bSwitched;
			break;
		case ID_TOOLS_FI:
			CConsole::flush_file_buffer_after_write = m_bSwitched;
			break;
		}
		Invalidate(FALSE);
	}
}

BEGIN_MESSAGE_MAP(CControlPanelGroup, CWnd)
	ON_WM_CREATE()
END_MESSAGE_MAP()
CControlPanelGroup::CControlPanelGroup(USHORT tag_index)
{
	m_TagIndex = tag_index;
}
CControlPanelGroup::~CControlPanelGroup()
{
}
int CControlPanelGroup::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// 相邻控件间宽度为12px
	switch (m_TagIndex) {
	case 0:
		BEGIN_GROUP(ID_FILE)
			BUTTON(ID_FILE_NEW, IDB_FILE_NEW, L"新建", 0)
			BUTTON(ID_FILE_OPEN, IDB_FILE_OPEN, L"打开", 80)
			BUTTON(ID_FILE_SAVE, IDB_FILE_SAVE, L"保存", 160)
			BUTTON(ID_FILE_SAVEAS, IDB_FILE_SAVEAS, L"另存为", 240)
		END_GROUP()
		break;
	case 1:
		BEGIN_GROUP(ID_EDIT)
			BUTTON(ID_EDIT_UNDO, IDB_EDIT_UNDO, L"撤销", 0)
			BUTTON(ID_EDIT_REDO, IDB_EDIT_REDO, L"重做", 80)
			SPLITTER(160)
			BUTTON(ID_EDIT_COPY, IDB_EDIT_COPY, L"复制", 172)
			BUTTON(ID_EDIT_CUT, IDB_EDIT_CUT, L"剪切", 252)
			BUTTON(ID_EDIT_PASTE, IDB_EDIT_PASTE, L"粘贴", 332)
			SPLITTER(412)
			BUTTON(ID_EDIT_LEFTARROW, IDB_EDIT_LEFTARROW, L"赋值符", 424)
		END_GROUP()
		FIND_BUTTON(ID_EDIT, ID_EDIT_UNDO)->SetState(false);
		FIND_BUTTON(ID_EDIT, ID_EDIT_REDO)->SetState(false);
		break;
	case 2:
		BEGIN_GROUP(ID_DEBUG)
			BUTTON(ID_DEBUG_RUN, IDB_DEBUG_RUN, L"运行", 0)
			BUTTON(ID_DEBUG_PAUSE, IDB_DEBUG_PAUSE, L"暂停", 0)
			BUTTON(ID_DEBUG_HALT, IDB_DEBUG_HALT, L"终止", 80)
			SPLITTER(160)
			BUTTON(ID_DEBUG_DEBUG, IDB_DEBUG_DEBUG, L"调试", 172)
			BUTTON(ID_DEBUG_CONTINUE, IDB_DEBUG_CONTINUE, L"继续", 172)
			BUTTON(ID_DEBUG_STEPIN, IDB_DEBUG_STEPIN, L"步入", 252)
			BUTTON(ID_DEBUG_STEPOVER, IDB_DEBUG_STEPOVER, L"步过", 332)
			BUTTON(ID_DEBUG_STEPOUT, IDB_DEBUG_STEPOUT, L"步出", 412)
		END_GROUP()
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_RUN)->SetState(false);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_DEBUG)->SetState(false);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_PAUSE)->ShowWindow(SW_HIDE);
		FIND_BUTTON(ID_DEBUG, ID_DEBUG_CONTINUE)->ShowWindow(SW_HIDE);
		break;
	case 3:
		BEGIN_GROUP(ID_TOOLS)
			SWITCH(ID_TOOLS_NL, IDB_TOOLS_NL, L"输出换行", 0, true)
			SWITCH(ID_TOOLS_OO, IDB_TOOLS_OO, L"输出对象", 80, true)
			SWITCH(ID_TOOLS_NNR, IDB_TOOLS_NNR, L"读取换行", 160, true)
			SWITCH(ID_TOOLS_NNW, IDB_TOOLS_NNW, L"写入换行", 240, true)
			SWITCH(ID_TOOLS_FI, IDB_TOOLS_FI, L"写入刷新", 320, false)
			SPLITTER(400)
			BUTTON(ID_TOOLS_REFERENCE, IDB_TOOLS_REFERENCE, L"参考", 412)
		END_GROUP()
		break;
	}

	return 0;
}
CControlPanelButton* CControlPanelGroup::GetButton(UINT id)
{
	for (std::list<CControlPanelComponent*>::iterator iter = m_Components.begin(); iter != m_Components.end(); iter++) {
		UINT nID = GetWindowLongW((*iter)->m_hWnd, GWL_ID);
		if (nID == id) {
			return (CControlPanelButton*)*iter;
		}
		else if (nID == 0) {
			throw -1;
		}
	}
	return nullptr;
}

BEGIN_MESSAGE_MAP(CControlPanel, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()
CControlPanel::CControlPanel()
{
	// 创建静态字体对象
	CControlPanelTag::m_Font.CreateFontW(30, 0, 0, 0, FW_NORMAL, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");
	CControlPanelButton::m_Font.CreateFontW(18, 0, 0, 0, FW_LIGHT, false, false,
		false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		PROOF_QUALITY, DEFAULT_PITCH | FF_DONTCARE << 2, L"Microsoft Yahei UI");

	// 创建标签栏
	m_Tags = new CControlPanelTag[]{
		CControlPanelTag(0, L"文件"),
		CControlPanelTag(1, L"编辑"),
		CControlPanelTag(2, L"调试"),
		CControlPanelTag(3, L"工具"),
	};

	// 创建组
	m_CurrentTagIndex = 0;
	m_Groups = new CControlPanelGroup[]{
		CControlPanelGroup(0),
		CControlPanelGroup(1),
		CControlPanelGroup(2),
		CControlPanelGroup(3),
		CControlPanelGroup(4),
	};

	pObject = this;
}
CControlPanel::~CControlPanel()
{
}
int CControlPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	;
	// 创建悬浮于按钮之上时的高亮背景
	CControlPanelButton::m_HoverBG.CreateCompatibleDC(&ScreenDC);
	CBitmap* pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 68, 88);
	CControlPanelButton::m_HoverBG.SelectObject(pBitmap);
	CBrush brush(RGB(61, 61, 61));
	CControlPanelButton::m_HoverBG.SelectObject(&brush);
	CPen pen(PS_SOLID, 1, RGB(112, 112, 112));
	CControlPanelButton::m_HoverBG.SelectObject(&pen);
	CControlPanelButton::m_HoverBG.Rectangle(0, 0, 68, 88);

	// 计算Tag组件字体大小
	CDC temp;
	temp.CreateCompatibleDC(&ScreenDC);
	temp.SelectObject(CControlPanelTag::m_Font);
	CSize size;
	GetTextExtentPoint32W(temp, L"你好", 2, &size);
	CControlPanelTag::m_Width = size.cx;
	CControlPanelTag::m_Height = size.cy;

	// 计算分割条
	CControlPanelSplitter::m_DC.CreateCompatibleDC(&ScreenDC);
	pBitmap = new CBitmap;
	pBitmap->CreateCompatibleBitmap(&ScreenDC, 1, 78);
	CControlPanelSplitter::m_DC.SelectObject(pBitmap);
	CRect rect(0, 0, 1, 78);
	CBrush brush2(RGB(56, 56, 56));
	CControlPanelSplitter::m_DC.FillRect(&rect, &brush2);

	// 创建当前组
	m_Tags[0].Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(60, 10, 60 + size.cx, 10 + size.cy), GetOwner(), NULL);
	m_Tags[1].Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(120, 10, 120 + size.cx, 10 + size.cy), GetOwner(), NULL);
	m_Tags[2].Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(180, 10, 180 + size.cx, 10 + size.cy), GetOwner(), NULL);
	m_Tags[3].Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(240, 10, 240 + size.cx, 10 + size.cy), GetOwner(), NULL);
	m_Groups[0].Create(NULL, NULL, WS_CHILD, CRect(10, 0, 1910, 100), this, ID_FILE);
	m_Groups[1].Create(NULL, NULL, WS_CHILD, CRect(10, 0, 1910, 100), this, ID_EDIT);
	m_Groups[2].Create(NULL, NULL, WS_CHILD, CRect(10, 0, 1910, 100), this, ID_DEBUG);
	m_Groups[3].Create(NULL, NULL, WS_CHILD, CRect(10, 0, 1910, 100), this, ID_TOOLS);
	ShiftTag(0);

	return 0;
}
BOOL CControlPanel::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CControlPanel::OnPaint()
{
	CPaintDC dc(this);
	static CRect* rect = new CRect(0, 0, SCREEN_WIDTH, 100);
	dc.FillRect(rect, pGreyBlackBrush);
}
void CControlPanel::ShiftTag(USHORT tag_index) {
	m_Tags[m_CurrentTagIndex].SetState(false);
	m_Groups[m_CurrentTagIndex].ShowWindow(SW_HIDE);
	m_CurrentTagIndex = tag_index;
	m_Tags[m_CurrentTagIndex].SetState(true);
	m_Groups[m_CurrentTagIndex].ShowWindow(SW_NORMAL);
}
CControlPanelGroup* CControlPanel::GetGroup(UINT id)
{
	switch (id) {
	case ID_FILE:
		return m_Groups + 0;
	case ID_EDIT:
		return m_Groups + 1;
	case ID_DEBUG:
		return m_Groups + 2;
	case ID_TOOLS:
		return m_Groups + 3;
	case ID_SETTINGS:
		return m_Groups + 4;
	}
}
