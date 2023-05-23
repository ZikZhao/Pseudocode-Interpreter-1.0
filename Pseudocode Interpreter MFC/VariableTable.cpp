#include "pch.h"

BEGIN_MESSAGE_MAP(CVariableTable, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
END_MESSAGE_MAP()
CVariableTable::CVariableTable()
{
	m_bShow = false;
	m_Globals = nullptr;
	m_CurrentLocals = nullptr;
	m_Width = m_Height = 0;
	m_FirstWidth = m_SecondWidth = 0;
	m_FullHeight = 0;
	m_Percentage = 0.0;
	pObject = this;
}
CVariableTable::~CVariableTable()
{
}
int CVariableTable::OnCreate(LPCREATESTRUCT lpCreateStruct)
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

	m_FirstWidth = m_WordSize.cx;
	ArrangeBG();
	m_Slider.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(), this, NULL);
	m_Slider.SetCallback(VerticalCallback);

	return 0;
}
BOOL CVariableTable::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}
void CVariableTable::OnPaint()
{
	CPaintDC dc(this);
	CRect rect(0, 0, m_Width, m_Height);
	MemoryDC.FillRect(&rect, pGreyBlackBrush);
	if (m_Globals) {
		MemoryDC.BitBlt(0, m_WordSize.cy, m_Width, m_Height - m_WordSize.cy, &m_Source, 0, 0, SRCCOPY);
	}
	MemoryDC.BitBlt(0, 0, m_Width, m_WordSize.cy, &m_BG, 0, 0, SRCCOPY);
	dc.BitBlt(0, 0, m_Width, m_Height, &MemoryDC, 0, 0, SRCCOPY);
}
void CVariableTable::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	m_Width = cx - 10;
	m_Height = cy;

	HBITMAP hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_Height);
	HGLOBAL hOldBitmap = SelectObject(m_BG, hBitmap);
	DeleteObject(hBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_Height);
	hOldBitmap = SelectObject(m_Source, hBitmap);
	DeleteObject(hBitmap);
	CRect rect(m_Width, 0, cx, m_Height);
	m_Slider.MoveWindow(&rect, TRUE);
	m_FirstWidth = m_SecondWidth = m_Width / 3;
	ArrangeBG();
	if (m_bShow and m_Globals) {
		ArrangeTable();
	}
}
void CVariableTable::LoadGlobal(BinaryTree* globals)
{
	for (IndexedList<ELEMENT>::iterator iter = m_OrderedList.begin(); iter != m_OrderedList.end(); iter++) {
		delete iter->name;
		delete iter->value;
		delete iter->type;
	}
	m_OrderedList.clear();
	if (m_CurrentLocals) {
		m_CurrentLocals = nullptr;
	}
	m_Globals = globals;
	if (not m_Globals) { return; }
	USHORT number = 0;
	BinaryTree::Node* duplicate_root = CConsole::pObject->ReadMemory(m_Globals->root);
	BinaryTree::Node* nodes = m_Globals->list_nodes_copy(duplicate_root, CConsole::pObject->m_DebugHandle, &number);
	free(duplicate_root);
	for (USHORT index = 0; index != number; index++) {
		DATA* duplicate_data = CConsole::pObject->ReadMemory(nodes[index].value);
		wchar_t* type_name = IdentifyType(duplicate_data);
		wchar_t* key = CConsole::pObject->ReadMemory(nodes[index].key, nodes[index].length);
		wchar_t* value = RetrieveValue(duplicate_data);
		free(duplicate_data);
		m_OrderedList.append(ELEMENT{ key, value, type_name });
	}
	// 更新滚动条
	m_FullHeight = m_OrderedList.size() * m_WordSize.cy;
	UpdateSlider();
	ArrangeBG();
	ArrangeTable();
	Invalidate(FALSE);
}
void CVariableTable::VerticalCallback(double percentage)
{
	pObject->m_Percentage = percentage;
	pObject->ArrangeTable();
	pObject->Invalidate(FALSE);
}
wchar_t* CVariableTable::IdentifyType(DATA* data)
{
	static const wchar_t* type_names[] = { L"整数", L"浮点数", L"字符", L"字符串", L"布尔值", L"日期与时间" };
	wchar_t* type_name;
	switch (data->type) {
	case 6:
		type_name = new wchar_t[] {L"数组"};
		break;
	case 8:
		type_name = new wchar_t[] {L"结构体"};
		break;
	case 10:
		type_name = new wchar_t[] {L"指针"};
		break;
	case 12:
		type_name = new wchar_t[] {L"枚举"};
		break;
	case 13:
		type_name = new wchar_t[] {L"子程序"};
		break;
	case 7: case 9: case 11:
		type_name = new wchar_t[] {L"类型"};
		break;
	default:
		type_name = new wchar_t[wcslen(type_names[data->type]) + 1];
		memcpy(type_name, type_names[data->type], (wcslen(type_names[data->type]) + 1) * 2);
		break;
	}
	return type_name;
}
wchar_t* CVariableTable::RetrieveValue(DATA* data)
{
	wchar_t* message = nullptr;
	switch (data->type) {
	case 0:
	{
		DataType::Integer* object = CConsole::pObject->ReadMemory((DataType::Integer*)data->value);
		message = new wchar_t[6 + object->value == 0 ? 0 : log10(object->value)];
		StringCchPrintfW(message, 6 + object->value == 0 ? 0 : log10(object->value), L"整数(%d)", object->value);
		break;
	}
	case 1:
	{
		DataType::Real * object = CConsole::pObject->ReadMemory((DataType::Real*)data->value);
		message = new wchar_t[14 + object->value < 1.0 ? 0 : log10(object->value)];
		StringCchPrintfW(message, 14 + object->value < 1.0 ? 0 : log10(object->value), L"浮点数(%f)", object->value);
		break;
	}
	case 2:
	{
		DataType::Char* object = CConsole::pObject->ReadMemory((DataType::Char*)data->value);
		break;
	}
	case 3:
	{
		DataType::String* object = CConsole::pObject->ReadMemory((DataType::String*)data->value);
		break;
	}
	case 4:
	{
		DataType::Boolean* object = CConsole::pObject->ReadMemory((DataType::Boolean*)data->value);
		break;
	}
	case 5:
	{
		DataType::Date* object = CConsole::pObject->ReadMemory((DataType::Date*)data->value);
		break;
	}
	case 6:
	{
		DataType::Array* object = CConsole::pObject->ReadMemory((DataType::Array*)data->value);
		break;
	}
	case 8:
	{
		message = new wchar_t[] {L"记录"};
		break;
	}
	case 10:
	{
		message = new wchar_t[] {L"指针"};
		break;
	}
	case 12:
	{
		message = new wchar_t[] {L"枚举"};
		break;
	}
	case 13:
		message = new wchar_t[] {L"子过程"};
		break;
	default:
	{
		message = new wchar_t[] {L"类型"};
		break;
	}
	}
	return message;
}
inline void CVariableTable::UpdateSlider()
{
	m_Slider.SetRatio((double)(m_Height - m_WordSize.cy) / m_FullHeight);
}
void CVariableTable::ArrangeBG()
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
	m_BG.DrawTextW(L"标识符", 3, &rect, NULL);
	rect = CRect(m_FirstWidth + 4, 0, m_FirstWidth + m_SecondWidth - 4, m_WordSize.cy);
	m_BG.DrawTextW(L"值", 1, &rect, NULL);
	rect = CRect(m_FirstWidth + m_SecondWidth + 4, 0, m_Width - 4, m_WordSize.cy);
	m_BG.DrawTextW(L"类型", 2, &rect, NULL);
}
void CVariableTable::ArrangeTable()
{
	if (not m_Globals) { return; }
	CRect rect(0, 0, m_Width, m_Height);
	m_Source.FillRect(&rect, pGreyBlackBrush);
	double start_index = m_Percentage * (m_OrderedList.size() + 1);
	int y_offset = -start_index * m_WordSize.cy;
	CRect rect1(4, y_offset, m_FirstWidth, y_offset + m_WordSize.cy);
	CRect rect2(m_FirstWidth + 4, y_offset, m_FirstWidth + m_SecondWidth + 4, y_offset + m_WordSize.cy);
	CRect rect3(m_FirstWidth + m_SecondWidth + 4, y_offset, m_Width, y_offset + m_WordSize.cy);
	for (IndexedList<ELEMENT>::iterator iter = m_OrderedList.begin(); iter != m_OrderedList.end(); iter++) {
		// 绘制变量名
		m_Source.DrawTextW(iter->name, -1, &rect1, NULL);
		// 绘制值
		m_Source.DrawTextW(iter->value, -1, &rect2, NULL);
		// 绘制类型名
		m_Source.DrawTextW(iter->type, -1, &rect3, NULL);
		rect1.top = rect2.top = rect3.top = rect1.bottom;
		rect1.bottom += m_WordSize.cy;
		rect2.bottom = rect3.bottom = rect1.bottom;
	}
}
