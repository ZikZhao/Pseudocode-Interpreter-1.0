#pragma once

class CFileTag : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CDC m_Memory;
	static inline CDC m_Selected;
	static inline CDC m_Hover;
	static inline CFont m_Font1;
	static inline CFont m_Font2;
	static inline CBrush m_Brush;
	IndexedList<wchar_t*> m_Lines;
	IndexedList<wchar_t*>::iterator m_CurrentLine;
	IndexedList<ADVANCED_TOKEN>* m_Tokens;
protected:
	CDC m_Source;
	wchar_t* m_Path;
	wchar_t* m_Directory;
	wchar_t* m_Filename;
	HANDLE m_Handle;
	int m_Width;
	bool m_bHover;
	bool m_bSelected;
public:
	CFileTag(wchar_t* path); // 物理文档
	CFileTag(); // 内存文档
	~CFileTag();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	void SetState(bool state);
	const wchar_t* GetPath() const;
	void Save();
	void SaveAs(wchar_t* new_path);
};

class CTagPanel : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CTagPanel* pObject = nullptr;
protected:
	CDC m_Source;
	int m_Width;
	CFileTag* m_Tags[10] {};
	USHORT m_Ptr;
	USHORT m_CurrentIndex;
public:
	CTagPanel();
	virtual ~CTagPanel();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	// 命令
	afx_msg void OnNew();
	afx_msg void OnOpen();
	afx_msg void OnSave();
	afx_msg void OnSaveAs();
	// 自定义函数
	void OpenFile(wchar_t* filename); // 打开文件
	void LoadOpenedFiles(); // 打开上次未关闭的文件
	CFileTag* GetCurrentTag();
	void ShiftTag(USHORT index);
};
