#pragma once

class CWatchInput : public CWnd
{
	DECLARE_MESSAGE_MAP()
protected:
	int m_Width; // ���ڿ��
	CDC m_Source; // ��Ⱦ����Դ
	CDC m_Selection; // ѡ��Դ
	bool m_bFocus; // �Ƿ��ü�������
	bool m_bCaret; // �Ƿ���ʾ���
	size_t m_cchBuffer; // ���뻺���ַ�����
	wchar_t* m_Buffer; // ���뻺��
	ULONG m_FullLength; // ����ȫ����ʾ���賤��
	double m_Percentage; // �������
	USHORT m_CharHeight; // ���ָ߶�
	UINT m_Offset; // ��Ⱦ����ƫ������>>>��
	bool m_bDrag; // �Ƿ�������ק
	UINT m_cchDragPointer; // ��ק��ʼʱָ���ַ�λ��
	UINT m_cchPointer; // ָ���ַ�λ��
public:
	CWatchInput();
	~CWatchInput();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	int GetCharHeight(); // ��ȡ�ַ��߶ȣ��Լ�����С���ڸ߶ȣ�
	void ArrangeText(); // ������Ⱦ����Դ
	void ArrangePointer(); // �����ĵ�ָ��Դ
	void ArrangeSelection(); // ����ѡ��Դ
	UINT TranslatePointer(CPoint point); // ���߼�����ת��Ϊ�ַ���λ
	void Delete(); // ɾ���ַ���ѡ��
};

class CWatch : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	struct WATCH {
		wchar_t* expression;
		RPN_EXP* rpn_exp;
		RPN_EXP* target_rpn_exp; // �洢��Ŀ������е�rpn���ʽ
	};
	static inline CWatch* pObject = nullptr;
	IndexedList<WATCH> m_Watches; // ���м�����
	bool m_bPaused; // ���������ͣ�£��½��ı��ʽ���ᱻ��������
protected:
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
	};
	IndexedList<ELEMENT> m_OrderedList; // ���м�����Ľ��
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
	CWatchInput m_WatchInput;
	CVSlider m_Slider;
public:
	CWatch();
	~CWatch();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	void AddWatch(wchar_t* buffer); // ��Ӽ���
	void BuildRemotePages(); // ���ӽ����н���ҳ��
	void CleanRemotePages(); // ������ӽ����д�����ҳ��
	void CleanWatchResult(); // �������н�������ռ�������
	void UpdateWatchResult(WATCH_RESULT* result); // ���¼�������
	static void VerticalCallback(double percentage); // ��ֱ������ص�����
protected:
	void DeleteWatch(USHORT index, USHORT index_leaf); // ɾ������
	void ExpandComplexType(DATA* data, USHORT level); // չ�������ṹ���Ԫ�غ��ֶ�
	void UpdateSlider(); // ���¹�����
	void ArrangeBG(); // ��Ⱦ����Դ
	void ArrangeWatches(); // ��Ⱦջ֡Դ
};
