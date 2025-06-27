#pragma once
#include "ChatListBox.h"

class ServerSession;
using ServerSessionRef = shared_ptr<class ServerSession>;

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 7777

// CChatClientDlg 대화 상자
class CChatClientDlg : public CDialogEx
{
	// 생성입니다.
public:
	CChatClientDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATCLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.
	virtual BOOL PreTranslateMessage(MSG* pMsg);


	// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void CChatClientDlg::OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void	OnBnClickedSendBtn();
	void			AddEventString(const WCHAR* ap_string);
	CChatListBox	chatList;
	CListBox		roomList;
	CEdit			chatName;

public:
	afx_msg void OnBnClickedConnectBtn();

public:
	bool				_isConnected = false;
	ServerSessionRef	_serverSession;
	bool				_isRequestingOldChats = false;
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnBnClickedOk();
};