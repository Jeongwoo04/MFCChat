#include "pch.h"
#include "ServerSession.h"
#include "ServerSessionManager.h"

void ServerSession::OnConnected()
{
	GServerSessionManager->Add(static_pointer_cast<ServerSession>(shared_from_this()));
	
	bool isReconnect = GServerSessionManager->GetReconnected();
	if (!isReconnect)
		GServerSessionManager->SetReconnectedTrue();

	_dig->_isConnected = true;
	_dig->_serverSession = static_pointer_cast<ServerSession>(shared_from_this());
	
	// connection
	// 최초 접속
	if (isReconnect == false)
	{
		Protocol::C_LOGIN loginPkt;

		CString str;
		_dig->chatName.GetWindowTextW(str);

		loginPkt.set_name(CW2A(str, CP_UTF8));

		// TODO : webserver 외부 인증
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(loginPkt);
		Send(sendBuffer);
	}
	else // 재접속
	{
		Protocol::C_RECONNECT reconnPkt;

		CString str;
		_dig->chatName.GetWindowTextW(str);

		reconnPkt.set_name(CW2A(str, CP_UTF8));
		reconnPkt.set_last_serial(GServerSessionManager->GetChatSerial());
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(reconnPkt);
		Send(sendBuffer);
	}
}

void ServerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

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
	this->_dig->AddEventString(L"[System] 서버와의 연결이 종료되었습니다.");
}