#pragma once
#include "TagPanel.h"
#include "ControlPanel.h"
#include "BreakpointDlg.h"

class CEditor : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CEditor* pObject = nullptr;
	static inline std::list<BREAKPOINT> m_Breakpoints; // 所有断点
protected:
	struct OPERATION {
		CPoint start; // 插入/删除起始点
		CPoint end; // 插入/删除结束点
		wchar_t* content; // 插入/删除内容
		size_t length; // 插入/删除长度
		bool insert; // 是否为插入
	};
	CDC m_MemoryDC; // 内存缓冲DC
	CDC m_Free; // 未选中文件时展示
	CDC m_Source; // 渲染文字源
	CDC m_Colour; // 语法高亮色块载体
	CDC m_Pointer; // 文档指针源
	CDC m_Selection; // 选区源
	CDC m_Breakpoint; // 断点源
	bool m_bFocus; //是否获得键盘输入
	CFont m_Font; // 文字字体
	bool m_bDrag; // 是否正在拖拽
	CBrush m_SelectionColor; //选区背景色
	USHORT m_Width, m_Height; // 组件区域大小
	double m_PercentageHorizontal, m_PercentageVertical; // 滚动位置
	CFileTag* m_CurrentTag; // 当前文件对应标签
	CPoint m_PointerPoint; // 文档指针字符位置
	CPoint m_DragPointerPoint; // 选中起点
	CPoint m_cPointer; //文档指针像素位置
	CSize m_CharSize; // 单字符大小（宽度用于绘制换行符）
	USHORT m_LineNumberWidth; // 行数信息所需像素宽度
	UINT64 m_FullWidth, m_FullHeight; // 所有文字同时展示计算大小
	CVSlider m_VSlider; // 垂直滚动条
	CHSlider m_HSlider; // 水平滚动条
	std::list<OPERATION> m_Operations; // 操作撤销/还原列表
	std::list<OPERATION>::iterator m_CurrentOperation; // 当前操作
	CBreakpointDlg* m_Dialog; // 断点对话框
	LONG64 m_CurrentStepLineIndex; // 当前单步执行行数（包括断点）
	CBrush m_BreakpointHitColor; // 断点命中颜色
	bool m_bBackendEnabled; // 当前台任务结束时，允许后台任务恢复执行
	CEvent* m_BackendPaused; // 当所有后台任务被暂停时，设定事件
public:
	// 构造
	CEditor();
	virtual ~CEditor();
	// 消息处理
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnTimer(UINT_PTR nIDEvent); // 用于重启后台任务
	// 命令
	afx_msg void OnUndo();
	afx_msg void OnRedo();
	// 自定义消息
	afx_msg LRESULT OnStep(WPARAM wParam, LPARAM lParam); // 单步执行消息
	// 自定义函数
	void LoadFile(CFileTag* tag); // 加载文档
	static void CALLBACK VerticalCallback(double percentage); // 垂直滚动条回调函数
	static void CALLBACK HorizontalCallback(double percentage); // 水平滚动条回调函数
	static void CALLBACK DeflationCallback(ULONG new_width); // 水平长度削减时回调函数
protected:
	void ArrangeText(); // 计算渲染文字源
	void ArrangeRenderedText(); // 计算渲染文字源（带语法高亮）
	void ArrangePointer(); // 计算文档指针源
	void ArrangeSelection(); // 计算选区源
	void ArrangeBreakpoints(); // 计算断点源
	void UpdateSlider(); // 更新垂直和水平滚动条
	void MovePointer(CPoint pointer); // 更新指针坐标（m_PointerPoint）以及当前行指针
	CPoint TranslatePointer(CPoint point); // 将逻辑坐标转换为字符单位（同时移动当前行指针）
	void Insert(wchar_t* text); // 插入复杂文本
	void Delete(); // 删除字符或选区
	void MoveView(); // 移动垂直与水平进度来显示到当前指针
	void CentralView(LONG64 line_index); // 移动视图到指定行并居中
	ADVANCED_TOKEN LineSplit(wchar_t* line); // 将一行中的缩进和注释分离
	void ParseLine(); // 解析一行语法
	// 以下函数用于后台
	static DWORD BackendTasking(LPVOID); // 后台处理程序（负责调用以下函数）
	void RecalcWidth(); // 重新计算宽度（仅在空闲时进行）
	void ParseAll(); // 解析整个代码文件
	void UpdateCandidates(); // 更新自动填充候选项
};
