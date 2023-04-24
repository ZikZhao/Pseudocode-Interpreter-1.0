#pragma once

class CBreakpointDlg : public CDialogEx
{
	DECLARE_MESSAGE_MAP()
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BREAKPOINT };
#endif
protected:
	// components
	CComboBox m_BreakpointList;
	CEdit m_Edit;
	CButton m_CancelButton;
	CButton m_OKButton;
	// attributes
	std::list<BREAKPOINT>* m_Breakpoints;
	int m_BreakpointIndex;
public:
	CBreakpointDlg(CWnd* pParent, std::list<BREAKPOINT>* breakpoints);
	virtual ~CBreakpointDlg();
	void SetCurrentIndex(int index);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
};
