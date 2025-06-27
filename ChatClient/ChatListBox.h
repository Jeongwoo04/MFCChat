#pragma once

class CChatClientDlg;

class CChatListBox : public CListBox
{
	DECLARE_DYNAMIC(CChatListBox)

public:
	CChatListBox();
	virtual ~CChatListBox();

	void SetOwnerDlg(CChatClientDlg* dlg);

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

private:
	CChatClientDlg* _ownerDlg = nullptr;
};