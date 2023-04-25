#include "pch.h"

BEGIN_MESSAGE_MAP(CBreakpointDlg, CDialogEx)
END_MESSAGE_MAP()
CBreakpointDlg::CBreakpointDlg(CWnd* pParent, std::list<BREAKPOINT>* breakpoints)
	: CDialogEx(IDD_BREAKPOINT, pParent)
{
	m_Breakpoints = breakpoints;
	m_BreakpointIndex = -1;
}
CBreakpointDlg::~CBreakpointDlg()
{
}
void CBreakpointDlg::SetCurrentIndex(int index)
{
	m_BreakpointIndex = index;
}
void CBreakpointDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_BreakpointList);
	DDX_Control(pDX, IDC_EDIT1, m_Edit);
	DDX_Control(pDX, IDCANCEL, m_CancelButton);
	DDX_Control(pDX, IDOK, m_OKButton);
	for (std::list<BREAKPOINT>::iterator iter = m_Breakpoints->begin(); iter != m_Breakpoints->end(); iter++) {
		USHORT line_number_digits = iter->line_index == 0 ? 0 : log10(iter->line_index + 1);
		wchar_t* buffer = new wchar_t[4 + line_number_digits];
		StringCchPrintfW(buffer, 4ull + line_number_digits, L"行 %ull", iter->line_index + 1);
		m_BreakpointList.AddString(buffer);
	}
	if (m_BreakpointIndex != -1) {
		m_BreakpointList.SetCurSel(m_BreakpointIndex);
	}
}
