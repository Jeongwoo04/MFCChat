#include "pch.h"
#include "ServerSession.h"
#include "ServerSessionManager.h"

void ServerSession::OnConnected()
{
	GServerSessionManager->Add(static_pointer_cast<ServerSession>(shared_from_this()));
	
	bool isReconnect = GServerSessionManager->GetReconnected();
	if (!isReconnect)
		GServerSessionManager->SetReconnectedTrue();

	if (_dig)
	{
		_dig->_isConnected = true;
		_dig->_serverSession = static_pointer_cast<ServerSession>(shared_from_this());
	}

	// connection
	// 최초 접속
	if (isReconnect)
		SendReconnectPacket();
	else
		SendLoginPacket();
}

void ServerSession::SendLoginPacket()
{
	Protocol::C_LOGIN loginPkt;
	loginPkt.set_name(GetUserNameFromUI());

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(loginPkt);
	Send(sendBuffer);
}

void ServerSession::SendReconnectPacket()
{
	Protocol::C_RECONNECT reconnPkt;
	reconnPkt.set_name(GetUserNameFromUI());
	reconnPkt.set_last_message_id(GServerSessionManager->GetLastMessageId());

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(reconnPkt);
	Send(sendBuffer);
}

void ServerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();

	ServerPacketHandler::HandlePacket(session, buffer, len);
}

void	ServerSession::OnSend(int32 len)
{
	//cout << "OnSend Len = " << len << endl;
}

void	ServerSession::OnDisconnected()
{
	GServerSessionManager->Remove(static_pointer_cast<ServerSession>(shared_from_this()));
	if (_dig->_isConnected == false)
		return;

	//cout << "OnDisconnected" << endl;

	this->_otherPlayers.clear();
	this->SetName("");
	this->_dig->_isConnected = false;

	if (!_dig || !_dig->IsWindowVisible())
		return;
	if (_dig->IsWindowVisible())
		_dig->AddEventString(L"[System] 서버와의 연결이 종료되었습니다.");
}

string ServerSession::GetUserNameFromUI() const
{
	if (_dig == nullptr)
		return "";

	CString str;
	_dig->chatName.GetWindowTextW(str);
	
	CW2A utf8(str, CP_UTF8);
	return std::string(utf8);
}
