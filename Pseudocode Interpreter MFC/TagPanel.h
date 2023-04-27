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
	wchar_t* m_Path;
	wchar_t* m_Directory;
	wchar_t* m_Filename;
	HANDLE m_Handle;
	std::list<wchar_t*> m_Lines;
	std::list<wchar_t*>::iterator m_CurrentLine;
	bool m_bHover;
	bool m_bSelected;
public:
	CFileTag(wchar_t* path); // 物理文档
	CFileTag(); // 内存文档
	~CFileTag();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	void SetState(bool state);
	const wchar_t* GetPath() const;
	std::list<wchar_t*>* GetLines();
	std::list<wchar_t*>::iterator& GetCurrentLine();
	void Save();
	void SaveAs();
};

class CTagPanel : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CTagPanel* pObject = nullptr;
protected:
	CDC m_Source;
	CFileTag* m_Tags[10] {};
	USHORT m_Ptr;
	USHORT m_CurrentIndex;
public:
	CTagPanel();
	virtual ~CTagPanel();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	// 命令
	afx_msg void OnNew();
	afx_msg void OnOpen();
	afx_msg void OnSave();
	// 自定义函数
	void OpenFile(wchar_t* filename); // 打开文件
	void LoadOpenedFiles(); // 打开上次未关闭的文件
	CFileTag* GetCurrentTag();
protected:
	void ShiftTag(USHORT index);
};
