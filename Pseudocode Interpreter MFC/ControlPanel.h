#pragma once
#define FIND_BUTTON(id_group, id_button) CControlPanel::pObject->GetGroup(id_group)->GetButton(id_button)

class CControlPanelTag : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CFont m_Font;
	static inline USHORT m_Width;
	static inline USHORT m_Height;
protected:
	bool m_bHover;
	bool m_bSelected;
	CDC m_Source;
	CDC m_Selected;
	CDC m_Hover;
	USHORT m_TagIndex;
	CString m_Text;
public:
	CControlPanelTag(USHORT tag_index, const wchar_t* text);
	virtual ~CControlPanelTag();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	void SetState(bool selected);
};

class CControlPanelButton : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CDC m_HoverBG;
	static inline CFont m_Font;
protected:
	UINT m_ResourceID;
	UINT m_CommandID;
	const wchar_t* m_Text;
	CDC m_Source;
	CDC m_SourceDisabled;
	bool m_bHover;
	bool m_bDisabled;
public:
	CControlPanelButton(UINT commandID, UINT resourceID, const wchar_t* button_text);
	virtual ~CControlPanelButton();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	void SetState(bool state);
};

class CControlPanelGroup : public CWnd
{
	DECLARE_MESSAGE_MAP()
protected:
	USHORT m_TagIndex;
	std::list<CControlPanelButton*> m_Buttons;
public:
	CControlPanelGroup(USHORT tag_index);
	virtual ~CControlPanelGroup();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	CControlPanelButton* GetButton(UINT id);
};

class CControlPanel : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CControlPanel* pObject = nullptr;
protected:
	USHORT m_CurrentTagIndex;
	CControlPanelTag* m_Tags;
	CControlPanelGroup* m_Groups;
public:
	CControlPanel();
	virtual ~CControlPanel();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	void ShiftTag(USHORT tag_index);
	CControlPanelGroup* GetGroup(UINT id);
};