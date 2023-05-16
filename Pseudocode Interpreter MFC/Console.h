#pragma once
#include "..\Pseudocode Interpreter\Debug.h"

struct PIPE {
	HANDLE stdin_read;
	HANDLE stdin_write;
	HANDLE stdout_read;
	HANDLE stdout_write;
	HANDLE stderr_read;
	HANDLE stderr_write;
	HANDLE signal_in_read;
	HANDLE signal_in_write;
	HANDLE signal_out_read;
	HANDLE signal_out_write;
};

class CConsoleOutput : public CWnd
{
	DECLARE_MESSAGE_MAP()
protected:
	struct LINE {
		wchar_t* line; // 行文本
		ULONG length; // 文本总长度
		USHORT height_unit; // 高度单位
		ULONG accumulated_height_unit; // 累计高度单位（不包含该行）
	};
public:
	static inline CConsoleOutput* pObject = nullptr;
	static inline UINT max_lines = 10000; // 最大缓冲行数
protected:
	int m_Width, m_Height; // 窗口宽度/高度
	ULONG m_FullLength; // 展示所有信息所需长度
	CDC m_Source; // 渲染文字源
	CDC m_Selection; // 选区源
	IndexedList<LINE> m_Lines; // 所有缓冲行
	IndexedList<LINE>::iterator m_CurrentLine; // 当前行
	ULONG m_TotalHeightUnit; // 所有行高度合计
	double m_Percentage; // 纵向比例
	CSize m_CharSize; // 文字大小
	CPoint m_PointerPoint; // 指针字符位置
	CPoint m_DragPointerPoint; // 拖拽开始指针位置
	bool m_bDrag; // 是否开始拖拽
	std::mutex m_Lock; // 行缓冲读写锁（以及m_Source源的同步访问）
	CVSlider m_Slider; // 纵向滚动条
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
	void ClearBuffer(); // 清空控制台缓冲行
	static void VerticalCallback(double percentage); // 垂直滚动条回调函数
	void SetListState(bool state); // 设置跳跃式链表是否为构造模式
protected:
	LINE EncodeLine(wchar_t* line); // 计算单行文本所需的高度单位并辨别每一位字符的宽度类型
	void RecalcTotalHeight(); // 计算当前宽度下显示所有文本所需高度单位
	void ArrangeText(); // 计算渲染文字源
	void ArrangePointer(); // 计算文档指针源
	void ArrangeSelection(); // 计算选区源
	void UpdateSlider(); // 更新垂直滚动条
	void TranslatePointer(CPoint point); // 将逻辑坐标转换为字符单位并更新指针位置
	IndexedList<LINE>::iterator SearchStartLine(LONG& cy, LONG* index = nullptr); // 计算
};

class CConsoleInput : public CWnd
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
	CConsoleInput();
	~CConsoleInput();
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

class CConsole : public CWnd
{
	DECLARE_MESSAGE_MAP()
public:
	static inline CConsole* pObject = nullptr;
	static inline CFont font; // 字体
	static inline CBrush selectionColor; //选区背景色
	bool m_bRun; // 子进程正在运行
protected:
	CConsoleOutput m_Output;
	CConsoleInput m_Input;
	PIPE m_Pipes; // 所有管道句柄
	STARTUPINFO m_SI; // 进程启动信息
	PROCESS_INFORMATION m_PI; // 进程信息
	HANDLE m_DebugHandle; // 可读取内存的进程句柄
	bool m_bShow; // 是否显示窗口
	size_t m_BlockSize; // 存储块大小查询的结果
public:
	CConsole();
	virtual ~CConsole();
	// 消息
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	// 命令
	afx_msg void OnDebugRun();
	afx_msg void OnDebugHalt();
	afx_msg void OnDebugDebug();
	afx_msg void OnDebugContinue();
	afx_msg void OnDebugStepin();
	afx_msg void OnDebugStepover();
	afx_msg void OnDebugStepout();
	bool SendInput(wchar_t* input, DWORD count); // 发送输入到子进程（返回值标识是否允许发送输入）
	template<typename Type>
	Type* ReadMemory(Type* address, size_t size = 1); // 读取子进程中地址指向的块
private:
	void InitSubprocess(bool debug_mode); // 准备监听新进程
	void ExitSubprocess(UINT exit_code); // 解释器实例结束时运行
	static DWORD Join(LPVOID lpParamter); // 进入管道监听循环
	static DWORD JoinDebug(LPVOID lpParameter); // 进入管道监听循环（调试模式）
	static void SendSignal(UINT message, WPARAM wParam, LPARAM lParam); // 生产通信信息
	void SignalProc(UINT message, WPARAM wParam, LPARAM lParam); // 子进程信号处理
};

template<typename Type>
inline Type* CConsole::ReadMemory(Type* address, size_t size)
{
	Type* buffer = (Type*)malloc(sizeof(Type) * size);
	ReadProcessMemory(m_DebugHandle, address, buffer, sizeof(Type) * size, NULL);
	return buffer;
}
