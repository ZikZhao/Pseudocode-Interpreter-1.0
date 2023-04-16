#pragma once
#include "TagPanel.h"
#include "BreakpointDlg.h"
#include "..\Pseudocode Interpreter\Debug.h"

class CEditor : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CEditor* pObject = nullptr;
protected:
	struct OPERATION {
		CPoint point;
		wchar_t* content;
	};
	CDC m_MemoryDC; // 内存缓冲DC
	CDC m_Free; // 未选中文件时展示
	CDC m_Source; // 渲染文字源
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
	CSize m_CharSize; // 单字符大小
	USHORT m_LineNumberWidth; // 行数信息所需像素宽度
	UINT64 m_FullWidth, m_FullHeight; // 所有文字同时展示计算大小
	CVSlider m_VSlider; // 垂直滚动条
	CHSlider m_HSlider; // 水平滚动条
	std::list<OPERATION> m_Operations; // 操作撤销/还原列表
	CBreakpointDlg* m_Dialog; // 断点对话框
	std::list<BREAKPOINT> m_Breakpoints; // 所有断点
public:
	CEditor();
	virtual ~CEditor();
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
	void LoadFile(CFileTag* tag); // 加载文档
	static void VerticalCallback(double percentage); // 垂直滚动条回调函数
	static void HorizontalCallback(double percentage); // 水平滚动条回调函数
protected:
	void ArrangeText(); // 计算渲染文字源
	void ArrangeSingleLine(); // 计算渲染单行文字
	void ArrangePointer(); // 计算文档指针源
	void ArrangeSelection(); // 计算选区源
	void ArrangeBreakpoints(); // 计算断点源
	void UpdateSlider(); // 更新垂直和水平滚动条
	CPoint TranslatePointer(CPoint point); // 将逻辑坐标转换为字符单位（同时移动当前行指针）
	void Delete(); // 删除字符或选区
};
