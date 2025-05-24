#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ClientPacketHandler.h"
#include "Player.h"

void GameSession::OnConnected()
{
	GSessionManager->Add(static_pointer_cast<GameSession>(shared_from_this()));
	_lastPongTime = ::GetTickCount64();
}

void GameSession::OnDisconnected()
{
	GSessionManager->Remove(static_pointer_cast<GameSession>(shared_from_this()));
	
	if (_currentPlayer)
	{
		_currentPlayer->ownerSession.reset();
	}

	if (_currentPlayer)
	{
		auto capturedSession = static_pointer_cast<GameSession>(shared_from_this());
		GRoom->DoAsync(&Room::Leave, capturedSession, _currentPlayer);
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