#pragma once

class App : public CWinAppEx
{
	DECLARE_MESSAGE_MAP()
public:
	App() noexcept;
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();
	afx_msg void OnAppAbout();
};

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg() noexcept;

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

extern App theApp;
extern CBrush* pThemeColorBrush;
extern CBrush* pGreyBlackBrush;
extern CPen* pNullPen;
extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;