#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ClientPacketHandler.h"

void GameSession::OnConnected()
{
	GSessionManager->Add(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnDisconnected()
{
	//_disconnected.store(true);

	GSessionManager->Remove(static_pointer_cast<GameSession>(shared_from_this()));

	if (_currentPlayer)
	{
		if (auto room = _room.lock()) // _room 을 weak_ptr로 들고있기때문에 lock을 이용해 share_ptr로 변환
			room->DoAsync(&Room::Leave, _currentPlayer);
	}

	_currentPlayer = nullptr;
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	// TODO : packetId 대역 체크
	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
}