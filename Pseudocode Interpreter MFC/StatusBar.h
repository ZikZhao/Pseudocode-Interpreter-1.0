#pragma once

class CCustomStatusBar : public CWnd
{
	DECLARE_MESSAGE_MAP()
protected:
	static inline const wchar_t* m_DefaultText = L"就绪"; // 默认消息
	int m_Width, m_Height; // 窗口尺寸
	bool m_bRun; // 子程序是否运行
	const wchar_t* m_Text; // 当前消息
	CFont m_Font; // 字体
public:
	CCustomStatusBar();
	virtual ~CCustomStatusBar();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	void UpdateState(bool bRun); // 更新颜色以指示子程序状态
	void UpdateMessage(const wchar_t* text); // 更新显示文本
};
