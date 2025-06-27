#include "pch.h"
#include "ChatListBox.h"
#include "ChatClientDlg.h"
#include "Protocol.pb.h"
#include "ServerSession.h"
#include "ServerSessionManager.h"
#include "ServerPacketHandler.h"

IMPLEMENT_DYNAMIC(CChatListBox, CListBox)

CChatListBox::CChatListBox()
{
}

CChatListBox::~CChatListBox()
{
}

BEGIN_MESSAGE_MAP(CChatListBox, CListBox)
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

void CChatListBox::SetOwnerDlg(CChatClientDlg* dlg)
{
	_ownerDlg = dlg;
}

void CChatListBox::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CListBox::OnVScroll(nSBCode, nPos, pScrollBar);

	if (_ownerDlg == nullptr || GetCount() == 0)
		return;

	CRect rc;
	GetClientRect(&rc);
	int itemHeight = GetItemHeight(0);
	int visibleCount = itemHeight > 0 ? rc.Height() / itemHeight : 0;
	int topIndex = GetTopIndex();
	int itemCount = GetCount();

	// 스크롤이 생기지 않았거나, 최상단이 아닌 경우
	if (itemCount <= visibleCount || topIndex != 0)
		return;

	// 쿨타임 적용
	static ULONGLONG lastScrollTime = 0;
	ULONGLONG now = GetTickCount64();
	const ULONGLONG cooldown = 2000;

	if (now - lastScrollTime < cooldown)
		return;

	lastScrollTime = now;

	// 최상단 도달 시 scrollUp 패킷 전송
	Protocol::C_SCROLL_UP pkt;
	pkt.set_oldest_message_id(GServerSessionManager->GetOldestMessageId());
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	_ownerDlg->_serverSession->Send(sendBuffer);
}