#pragma once

class CVariableTable : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CVariableTable* pObject = nullptr;
protected:
	BinaryTree* m_Globals;
	BinaryTree* m_CurrentLocals;
	struct ELEMENT {
		wchar_t* name;
		wchar_t* value;
		wchar_t* type;
	};
	IndexedList<ELEMENT> m_OrderedList;
	int m_Width, m_Height;
	bool m_bShow;
	CDC m_BG;
	CDC m_Source;
	CFont m_Font;
	CPen m_Pen;
	ULONG m_FullHeight;
	USHORT m_FirstWidth, m_SecondWidth;
	double m_Percentage;
	CSize m_WordSize;
	CVSlider m_Slider;
public:
	CVariableTable();
	~CVariableTable();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void LoadGlobal(BinaryTree* globals);
	void LoadLocal(BinaryTree* locals);
	static void VerticalCallback(double percentage); // 垂直滚动体回调函数
protected:
	wchar_t* IdentifyType(DATA* data); // 返回数据类型
	wchar_t* RetrieveValue(DATA* data); // 返回变量的值
	void UpdateSlider(); // 更新滚动条
	void ArrangeBG(); // 渲染背景源
	void ArrangeTable(); // 渲染栈帧源
};
