#pragma once
#include "afxwinappex.h"
#include <list>
#include <mutex>

class CConsoleOutput : public CWnd
{
	DECLARE_MESSAGE_MAP()
protected:
	struct LINE {
		wchar_t* line; // 行文本
		ULONG length; // 文本总长度
		int height_unit; // 高度单位
		ULONG offset; // 最后一行起始字符下标
	};
public:
	static inline CConsoleOutput* pObject = nullptr;
	static inline UINT max_lines = 10000; // 最大缓冲行数
protected:
	int m_Width, m_Height; // 窗口宽度/高度
	ULONG m_FullLength; // 展示所有信息所需长度
	CDC m_Temp; // 缓存源
	CDC m_Source; // 渲染文字源
	CDC m_Selection; // 选区源
	CDC m_Pointer; // 指针源
	bool m_bFocus; //是否获得键盘输入
	std::list<LINE> m_Lines; // 所有缓冲行
	std::list<LINE>::iterator m_CurrentLine; // 指针指向的当前行
	ULONG m_TotalHeightUnit; // 所有行高度合计
	ULONG m_AccumulatedHeightUnit; // 到当前行的累计高度单位（不包含）
	double m_Percentage; // 纵向比例
	CSize m_CharSize; // 文字大小
	CPoint m_PointerPoint; // 指针字符位置
	CPoint m_DragPointerPoint; // 拖拽开始指针位置
	CPoint m_cPointer; // 指针像素位置
	bool m_bDrag; // 是否开始拖拽
	std::mutex m_Lock; // 行缓冲读写锁
public:
	CConsoleOutput();
	~CConsoleOutput();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	void AppendText(char* buffer, DWORD transferred); // 接收子进程输出
	void AppendInput(wchar_t* input, DWORD count); // 记录输入
	static void VerticalCallback(double percentage); // 垂直滚动条回调函数
protected:
	LINE EncodeLine(wchar_t* line); // 计算单行文本所需的高度单位并辨别每一位字符的宽度类型
	void RecalcTotalHeight(); // 计算当前宽度下显示所有文本所需高度单位
	void ArrangeText(); // 计算渲染文字源
	void ArrangePointer(); // 计算文档指针源
	void ArrangeSelection(); // 计算选区源
	void UpdateSlider(); // 更新垂直滚动条
	CPoint TranslatePointer(CPoint point); // 将逻辑坐标转换为字符单位（包含MovePointer的功能）
	void MovePointer(CPoint point); // 移动指针（以及当前行指针）
	void ClearBuffer(); // 清空控制台缓冲行
};

class CTestWnd : public CFrameWnd {
	DECLARE_MESSAGE_MAP()
protected:
	CConsoleOutput m_Component;
public:
	CTestWnd() noexcept;
	~CTestWnd();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

class App : public CWinAppEx {
protected:

public:
	App() noexcept;
	~App();
	virtual BOOL InitInstance();
};
