﻿#pragma once
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
	CDC m_Free; // 未选中文件时展示
	CDC m_Source; // 渲染文字源
	CDC m_Colour; // 语法高亮色块载体
	CDC m_Selection; // 选区源
	CDC m_Breakpoint; // 断点源
	bool m_bFocus; // 是否获得键盘输入
	bool m_bCaret; // 光标是否显示
	CFont m_Font; // 文字字体
	bool m_bDrag; // 是否正在拖拽
	CBrush m_SelectionColor; //选区背景色
	USHORT m_Width, m_Height; // 组件区域大小
	double m_PercentageHorizontal, m_PercentageVertical; // 滚动位置
	CFileTag* m_CurrentTag; // 当前文件对应标签
	CPoint m_PointerPoint; // 文档指针字符位置
	CPoint m_DragPointerPoint; // 选中起点
	CSize m_CharSize; // 单字符大小（宽度用于绘制换行符）
	USHORT m_LineNumberWidth; // 行数信息所需像素宽度
	UINT64 m_FullWidth, m_FullHeight; // 所有文字同时展示计算大小
	CVSlider m_VSlider; // 垂直滚动条
	CHSlider m_HSlider; // 水平滚动条
	bool m_bRecord; // 正在记录连续的单字符输入/删除
	CBreakpointDlg* m_Dialog; // 断点对话框
	LONG64 m_CurrentStepLineIndex; // 当前单步执行行数（包括断点）
	bool m_bBackendPending; // 等待后台任务结束
	HANDLE m_BackendThread; // 后台线程句柄
	std::recursive_mutex m_BackendLock; // 后台任务锁（用于短暂暂停后台任务以避免访问冲突）
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
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnDestroy();
	// 命令
	afx_msg void OnUndo();
	afx_msg void OnRedo();
	afx_msg void OnCopy();
	afx_msg void OnCut();
	afx_msg void OnPaste();
	afx_msg void OnLeftarrow();
	// 自定义消息
	afx_msg LRESULT OnStep(WPARAM wParam, LPARAM lParam); // 单步执行消息
	// 自定义函数
	void LoadFile(CFileTag* tag); // 加载文档
	static void CALLBACK VerticalCallback(double percentage); // 垂直滚动条回调函数
	static void CALLBACK HorizontalCallback(double percentage); // 水平滚动条回调函数
	static void CALLBACK DeflationCallback(ULONG new_width); // 水平长度削减时回调函数
	static void EndRecord(HWND = 0, UINT = 0, UINT_PTR = 0, DWORD = 0); // 单字符输入/删除超时处理函数
protected:
	void ArrangeText(); // 计算渲染文字源
	void ArrangeRenderedText(); // 计算渲染文字源（带语法高亮）
	void ArrangePointer(); // 计算文档指针源
	void ArrangeSelection(); // 计算选区源
	void ArrangeBreakpoints(); // 计算断点源
	void UpdateSlider(); // 更新垂直和水平滚动条
	void MovePointer(CPoint pointer); // 更新指针坐标（m_PointerPoint）以及当前行指针
	CPoint TranslatePointer(CPoint point); // 将逻辑坐标转换为字符单位（同时移动当前行指针）
	wchar_t* GetSelectionText(size_t& size); // 获取当前选区文本
	void Insert(const wchar_t* text); // 插入复杂文本
	void Delete(); // 删除字符或选区
	void RecordChar(UINT nChar); // 记录超时内的所有单字符输入
	void RecordDelete(UINT nChar); // 记录超时内的所有单字符删除
	void MoveView(); // 移动垂直与水平进度来显示到当前指针
	void CentralView(LONG64 line_index); // 移动视图到指定行并居中
	ADVANCED_TOKEN LineSplit(wchar_t* line); // 将一行中的缩进和注释分离
	void ExpandToken(wchar_t* line, ADVANCED_TOKEN& token); // 将表达式或是枚举类型令牌展开以进行渲染
	void ParseLine(); // 解析一行语法
	// 以下函数用于后台
	static DWORD BackendTasking(LPVOID); // 后台处理程序（负责调用以下函数）
	void RecalcWidth(); // 重新计算宽度（仅在空闲时进行）
	void ParseAll(); // 解析整个代码文件
	void UpdateCandidates(); // 更新自动填充候选项
};
