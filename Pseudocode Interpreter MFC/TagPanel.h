#pragma once

class CFileTag : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	struct OPERATION {
		CPoint start; // 插入/删除起始点
		CPoint end; // 插入/删除结束点
		wchar_t* content = nullptr; // 插入/删除内容
		size_t length = 0; // 插入/删除长度
		bool insert; // 是否为插入
	};
	static inline CDC m_Selected;
	static inline CDC m_Hover;
	static inline CDC m_Latest;
	static inline CDC m_Newed;
	static inline CDC m_Edited;
	static inline CFont m_Font1;
	static inline CFont m_Font2;
	static inline CBrush m_Brush;
	IndexedList<wchar_t*> m_Lines;
	IndexedList<wchar_t*>::iterator m_CurrentLine;
	IndexedList<ADVANCED_TOKEN>* m_Tokens;
	std::list<OPERATION> m_Operations; // 操作撤销/还原列表
	std::list<OPERATION>::iterator m_CurrentOperation; // 当前操作
	bool m_bEdited; // 缓冲与物理文档是否有差别
	bool m_bParsed; // 所有代码是否都已解析
protected:
	wchar_t* m_Path;
	wchar_t* m_Directory;
	wchar_t* m_Filename;
	HANDLE m_Handle;
	int m_Width;
	bool m_bHover;
	bool m_bHoverClose;
	bool m_bSelected;
public:
	CFileTag(wchar_t* path); // 物理文档
	CFileTag(); // 内存文档
	~CFileTag();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	void SetEdited();
	void SetSelected(bool state);
	const wchar_t* GetPath() const;
	void Save();
	void SaveAs(wchar_t* new_path);
	void Close();
};

class CTagPanel : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CTagPanel* pObject = nullptr;
protected:
	CDC m_Source;
	int m_Width;
	IndexedList<CFileTag*> m_Tags;
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
	CFileTag* GetCurrentTag();
	void ShiftTag(CFileTag* tag);
	void DestroyTag(CFileTag* tag);
};
