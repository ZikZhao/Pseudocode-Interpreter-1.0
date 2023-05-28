#pragma once

class CWatchInput : public CWnd
{
	DECLARE_MESSAGE_MAP()
protected:
	int m_Width; // 窗口宽度
	CDC m_Source; // 渲染文字源
	CDC m_Selection; // 选区源
	bool m_bFocus; // 是否获得键盘输入
	bool m_bCaret; // 是否显示光标
	size_t m_cchBuffer; // 输入缓存字符数量
	wchar_t* m_Buffer; // 输入缓存
	ULONG m_FullLength; // 文字全部显示所需长度
	double m_Percentage; // 横向比例
	USHORT m_CharHeight; // 文字高度
	UINT m_Offset; // 渲染文字偏移量（>>>）
	bool m_bDrag; // 是否正在拖拽
	UINT m_cchDragPointer; // 拖拽开始时指针字符位置
	UINT m_cchPointer; // 指针字符位置
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
	int GetCharHeight(); // 获取字符高度（以计算最小窗口高度）
	void ArrangeText(); // 计算渲染文字源
	void ArrangePointer(); // 计算文档指针源
	void ArrangeSelection(); // 计算选区源
	UINT TranslatePointer(CPoint point); // 将逻辑坐标转换为字符单位
	void Delete(); // 删除字符或选区
};

class CWatch : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	struct WATCH {
		wchar_t* expression;
		RPN_EXP* rpn_exp;
		RPN_EXP* target_rpn_exp; // 存储在目标进程中的rpn表达式
	};
	static inline CWatch* pObject = nullptr;
	IndexedList<WATCH> m_Watches; // 所有监视项
	bool m_bPaused; // 如果程序已停下，新建的表达式将会被立即计算
protected:
	struct ELEMENT {
		wchar_t* name;
		wchar_t* value;
		wchar_t* type;
		USHORT level; // 展开深度
		enum STATE { // 展开设置
			NOT_EXPANDABLE,
			NOT_EXPANDED,
			EXPANDED,
		} state;
	};
	IndexedList<ELEMENT> m_OrderedList; // 所有监视项的结果
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
	void AddWatch(wchar_t* buffer); // 添加监视
	void BuildRemotePages(); // 在子进程中建立页面
	void CleanRemotePages(); // 清除在子进程中创建的页面
	void CleanWatchResult(); // 程序运行结束后清空监视数据
	void UpdateWatchResult(WATCH_RESULT* result); // 更新监视数据
	static void VerticalCallback(double percentage); // 垂直滚动体回调函数
protected:
	void DeleteWatch(USHORT index, USHORT index_leaf); // 删除监视
	void ExpandComplexType(DATA* data, USHORT level); // 展开数组或结构体的元素和字段
	void UpdateSlider(); // 更新滚动条
	void ArrangeBG(); // 渲染背景源
	void ArrangeWatches(); // 渲染栈帧源
};
