#pragma once
#include <list>

class CFileTag : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CDC m_Selected;
	static inline CDC m_Hover;
	static inline CFont m_Font1;
	static inline CFont m_Font2;
protected:
	CDC m_Source;
	const wchar_t* m_Path;
	wchar_t* m_Directory;
	wchar_t* m_Filename;
	HANDLE m_Handle;
	std::list<wchar_t*> m_Lines;
	std::list<wchar_t*>::iterator m_CurrentLine;
	bool m_bHover;
	bool m_bSelected;
public:
	CFileTag(const wchar_t* path);
	~CFileTag();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	void SetState(bool state);
	const wchar_t* GetPath();
	std::list<wchar_t*>* GetLines();
	std::list<wchar_t*>::iterator& GetCurrentLine();
};

class CTagPanel : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CTagPanel* pObject = nullptr;
protected:
	CDC m_Source;
	CFileTag* m_OpenedFiles[10] {};
	USHORT m_Ptr;
	USHORT m_CurrentIndex;
public:
	CTagPanel();
	virtual ~CTagPanel();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	void NewTag(const wchar_t* filename);
	void ShiftTag(USHORT index);
	CFileTag* GetCurrentTag();
};
