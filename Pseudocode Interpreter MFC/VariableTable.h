#pragma once

class CVariableTable : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CVariableTable* pObject = nullptr;
protected:
	BinaryTree* m_Globals;
	BinaryTree* m_CurrentLocals;
	BinaryTree m_PrevVariables;
	struct ELEMENT {
		wchar_t* name;
		wchar_t* value;
		wchar_t* type;
		USHORT level; // չ�����
		enum STATE { // չ������
			NOT_EXPANDABLE,
			NOT_EXPANDED,
			EXPANDED,
		} state;
		bool modified = false;
	};
	IndexedList<ELEMENT> m_OrderedList;
	int m_Width, m_Height;
	CDC m_BG;
	CDC m_Source;
	CDC m_Folded;
	CDC m_Expanded;
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
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	void RecordPrevious(BinaryTree* last_locals); // ׼��������������
	void LoadGlobal(BinaryTree* globals); // ����ȫ�ֱ�����
	void LoadLocal(BinaryTree* locals); // ���ؾֲ�������
	void LoadReturnValue(DATA* data); // ���ط���ֵ
	static void VerticalCallback(double percentage); // ��ֱ������ص�����
	wchar_t* IdentifyType(DATA* data); // ������������
	wchar_t* RetrieveValue(DATA* data); // ���ر�����ֵ
protected:
	void ExpandComplexType(DATA* data, USHORT level); // չ�������ṹ���Ԫ�غ��ֶ�
	void UpdateSlider(); // ���¹�����
	void ArrangeBG(); // ��Ⱦ����Դ
	void ArrangeTable(); // ��Ⱦջ֡Դ
};
