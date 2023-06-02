#include "pch.h"

BEGIN_MESSAGE_MAP(CVariableTable, CWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()
CVariableTable::CVariableTable()
{
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

	HBITMAP hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_WordSize.cy);
	HGLOBAL hOldBitmap = SelectObject(m_BG, hBitmap);
	DeleteObject(hBitmap);
	hBitmap = CreateCompatibleBitmap(ScreenDC, m_Width, m_Height - m_WordSize.cy);
	hOldBitmap = SelectObject(m_Source, hBitmap);
	DeleteObject(hBitmap);
	CRect rect(m_Width, 0, cx, m_Height);
	m_Slider.MoveWindow(&rect, TRUE);
	m_FirstWidth = m_SecondWidth = m_Width / 3;
	ArrangeBG();
	ArrangeTable();
	Invalidate(FALSE);
}
BOOL CVariableTable::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
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
void CVariableTable::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();
}
void CVariableTable::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	if (point.x >= 0 and point.x < m_Width and
		point.y > m_WordSize.cy and point.y < m_Height and m_OrderedList.size()) {
		double start_line = m_Percentage * m_OrderedList.size();
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
					ArrangeTable();
					Invalidate(FALSE);
					return;
				}
				rect.top = rect.bottom;
				rect.bottom += m_WordSize.cy;
			}
		}
	}
}
void CVariableTable::RecordPrevious(BinaryTree* last_locals)
{
	m_PrevVariables.clear();
	if (m_Globals) {
		BinaryTree::Node* duplicate_root = CConsole::pObject->ReadMemory(m_Globals->root);
		USHORT count = 0;
		BinaryTree::Node* nodes = m_Globals->list_nodes_copy(duplicate_root, CConsole::pObject->m_DebugHandle, &count);
		free(duplicate_root);
		for (USHORT index = 0; index != count; index++) {
			m_PrevVariables.insert(CConsole::pObject->ReadMemory(nodes[index].key, nodes[index].length), nodes[index].value, false);
		}
		free(nodes);
		if (last_locals) {
			duplicate_root = CConsole::pObject->ReadMemory(last_locals->root);
			count = 0;
			nodes = last_locals->list_nodes_copy(duplicate_root, CConsole::pObject->m_DebugHandle, &count);
			free(duplicate_root);
			for (USHORT index = 0; index != count; index++) {
				wchar_t* key = CConsole::pObject->ReadMemory(nodes[index].key, nodes[index].length);
				DATA* duplicate_data = CConsole::pObject->ReadMemory(nodes[index].value);
				BinaryTree::Node* search_result = m_PrevVariables.find(key);
				if (not search_result) {
					m_PrevVariables.insert(key, (DATA*)duplicate_data->value, false);
				}
				else {
					search_result->value = (DATA*)duplicate_data->value;
				}
			}
			free(nodes);
		}
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
		delete m_CurrentLocals;
		m_CurrentLocals = nullptr;
	}
	m_Globals = globals;
	if (not m_Globals) {
		Invalidate(FALSE);
		return;
	}
	USHORT number = 0;
	BinaryTree::Node* duplicate_root = CConsole::pObject->ReadMemory(m_Globals->root);
	BinaryTree::Node* nodes = m_Globals->list_nodes_copy(duplicate_root, CConsole::pObject->m_DebugHandle, &number);
	free(duplicate_root);
	m_OrderedList.set_construction(true);
	for (USHORT index = 0; index != number; index++) {
		DATA* duplicate_data = CConsole::pObject->ReadMemory(nodes[index].value);
		wchar_t* type_name = IdentifyType(duplicate_data);
		wchar_t* key = CConsole::pObject->ReadMemory(nodes[index].key, nodes[index].length);
		wchar_t* value = RetrieveValue(duplicate_data);
		ELEMENT new_element;
		if (duplicate_data->type == 6 or duplicate_data->type == 8) {
			new_element = ELEMENT{ key, value, type_name, 0, ELEMENT::NOT_EXPANDED };
		}
		else {
			new_element = ELEMENT{ key, value, type_name, 0, ELEMENT::NOT_EXPANDABLE };
		}
		BinaryTree::Node* node = m_PrevVariables.find(key);
		if (node) {
			DATA* duplicate_original_data = CConsole::pObject->ReadMemory(node->value);
			if (duplicate_data->value != duplicate_original_data->value) {
				new_element.modified = true;
			}
			free(duplicate_original_data);
		}
		m_OrderedList.append(new_element);
		if (duplicate_data->type == 6 or duplicate_data->type == 8) {
			ExpandComplexType(duplicate_data, 1);
		}
		free(duplicate_data);
	}
	m_OrderedList.set_construction(false);
	// 更新滚动条
	UpdateSlider();
	ArrangeTable();
	Invalidate(FALSE);
}
void CVariableTable::LoadLocal(BinaryTree* locals)
{
	if (m_CurrentLocals) {
		LoadGlobal(m_Globals);
	}
	m_CurrentLocals = locals;
	if (not m_CurrentLocals) { return; }
	USHORT number = 0;
	BinaryTree::Node* duplicate_root = CConsole::pObject->ReadMemory(m_CurrentLocals->root);
	BinaryTree::Node* nodes = m_CurrentLocals->list_nodes_copy(duplicate_root, CConsole::pObject->m_DebugHandle, &number);
	free(duplicate_root);
	UINT insertion_pos = 0;
	for (USHORT index = 0; index != number; index++) {
		DATA* duplicate_data = CConsole::pObject->ReadMemory(nodes[index].value);
		wchar_t* type_name = IdentifyType(duplicate_data);
		wchar_t* key = CConsole::pObject->ReadMemory(nodes[index].key, nodes[index].length);
		wchar_t* value = RetrieveValue(duplicate_data);
		for (; insertion_pos != m_OrderedList.size(); insertion_pos++) {
			int result = CompareStringOrdinal(key, wcslen(key), m_OrderedList[insertion_pos]->name, wcslen(m_OrderedList[insertion_pos]->name), FALSE);
			if (result == CSTR_LESS_THAN or result == CSTR_EQUAL) {
				break;
			}
		}
		ELEMENT new_element;
		if (duplicate_data->type == 6 or duplicate_data->type == 8) {
			new_element = ELEMENT{ key, value, type_name, 0, ELEMENT::NOT_EXPANDED };
		}
		else {
			new_element = ELEMENT{ key, value, type_name, 0, ELEMENT::NOT_EXPANDABLE };
		}
		BinaryTree::Node* node = m_PrevVariables.find(key);
		if (node) {
			DATA* duplicate_current_data = CConsole::pObject->ReadMemory(node->value);
			DATA* duplicate_original_data = CConsole::pObject->ReadMemory(nodes[index].value);
			if (duplicate_current_data->value != duplicate_original_data->value) {
				new_element.modified = true;
			}
			free(duplicate_current_data);
			free(duplicate_original_data);
		}
		m_OrderedList.append(new_element);
		if (duplicate_data->type == 6 or duplicate_data->type == 8) {
			ExpandComplexType(duplicate_data, 1);
		}
		free(duplicate_data);
	}
	// 更新滚动条
	UpdateSlider();
	ArrangeTable();
	Invalidate(FALSE);
}
void CVariableTable::LoadReturnValue(DATA* data)
{
	if (data->type == 65535) {
		free(data);
		return;
	}
	wchar_t* type_name = IdentifyType(data);
	wchar_t* key = new wchar_t[] {L"<函数返回值>"};
	wchar_t* value = RetrieveValue(data);
	if (data->type == 6 or data->type == 8) {
		m_OrderedList.insert(0, ELEMENT{ key, value, type_name, 0, ELEMENT::NOT_EXPANDED });
		ExpandComplexType(data, 1);
	}
	else {
		m_OrderedList.insert(0, ELEMENT{ key, value, type_name, 0, ELEMENT::NOT_EXPANDABLE });
	}
	free(data);
	UpdateSlider();
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
	static const wchar_t* type_names[] = { L"INTEGER", L"REAL", L"CHAR", L"STRING", L"BOOLEAN", L"DATE" };
	wchar_t* type_name;
	switch (data->type) {
	case 6:
		type_name = new wchar_t[] {L"ARRAY"};
		break;
	case 8:
		type_name = new wchar_t[] {L"RECORD"};
		break;
	case 10:
		type_name = new wchar_t[] {L"POINTER"};
		break;
	case 12:
		type_name = new wchar_t[] {L"ENUMERATION"};
		break;
	case 13:
		type_name = new wchar_t[] {L"SUBROUTINE"};
		break;
	case 7: case 9: case 11:
		type_name = new wchar_t[] {L"TYPE"};
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
	DataType::Any* temp = CConsole::pObject->ReadMemory((DataType::Any*)data->value);
	if (not temp->init) {
		free(temp);
		return new wchar_t[] {L"<未初始化>"};
	}
	free(temp);
	switch (data->type) {
	case 0:
	{
		DataType::Integer* object = CConsole::pObject->ReadMemory((DataType::Integer*)data->value);
		message = new wchar_t[5 + GET_DIGITS(object->value)];
		StringCchPrintfW(message, 5 + GET_DIGITS(object->value), L"整数(%d)", object->value);
		free(object);
		break;
	}
	case 1:
	{
		DataType::Real* object = CConsole::pObject->ReadMemory((DataType::Real*)data->value);
		message = new wchar_t[13 + GET_DIGITS(object->value)];
		StringCchPrintfW(message, 13 + GET_DIGITS(object->value), L"浮点数(%.6f)", object->value);
		free(object);
		break;
	}
	case 2:
	{
		DataType::Char* object = CConsole::pObject->ReadMemory((DataType::Char*)data->value);
		message = new wchar_t[6];
		StringCchPrintfW(message, 6, L"字符(%s)", &object->value);
		free(object);
		break;
	}
	case 3:
	{
		DataType::String* object = CConsole::pObject->ReadMemory((DataType::String*)data->value);
		wchar_t* string = CConsole::pObject->ReadMemory(object->string, object->length);
		message = new wchar_t[6 + object->length];
		StringCchPrintfW(message, 6 + object->length, L"字符串(%s)", string);
		free(object);
		free(string);
		break;
	}
	case 4:
	{
		DataType::Boolean* object = CConsole::pObject->ReadMemory((DataType::Boolean*)data->value);
		message = new wchar_t[6 + object->value ? 4 : 5];
		const wchar_t* content = object->value ? L"TRUE" : L"FALSE";
		StringCchPrintfW(message, 6 + object->value ? 4 : 5, L"布尔值(%s)", content);
		free(object);
		break;
	}
	case 5:
	{
		DataType::Date* object = CConsole::pObject->ReadMemory((DataType::Date*)data->value);
		wchar_t* content = object->output();
		message = new wchar_t[8 + wcslen(content)];
		StringCchPrintfW(message, 8 + wcslen(content), L"日期与时间(%s)", content);
		free(object);
		break;
	}
	default:
	{
		message = new wchar_t[1] {0};
		break;
	}
	}
	return message;
}
void CVariableTable::ExpandComplexType(DATA* data, USHORT level)
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
			wchar_t* value = RetrieveValue(duplicate_element);
			wchar_t* type_name = IdentifyType(duplicate_element);
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
			wchar_t* value = RetrieveValue(data);
			wchar_t* type_name = IdentifyType(data);
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
inline void CVariableTable::UpdateSlider()
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
	m_FullHeight = count * m_WordSize.cy;
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
		if (iter->modified) {
			m_Source.SetTextColor(RGB(254, 74, 99));
		}
		else {
			m_Source.SetTextColor(RGB(255, 255, 255));
		}
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
		if (rect1.top > m_Height - m_WordSize.cy) {
			break;
		}
	}
}
