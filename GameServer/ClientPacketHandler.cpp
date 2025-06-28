#include "pch.h"
#include "ClientPacketHandler.h"
#include "GameSession.h"
#include "Player.h"
#include "Room.h"
#include <ctime>
#include "FileUtils.h"
#include "DBConnectionPool.h"
#include "DBBind.h"
#include "XmlParser.h"
#include "DBSynchronizer.h"
#include "GenProcedures.h"
#include "GlobalQueue.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

static bool RequestLogin(GameSessionRef& gameSession, const string& name, int64 messageId)
{
	if (gameSession->_currentPlayer != nullptr)
		return false;

	GRoom->DoDBAsync(&Room::DBProcessLogin, gameSession, name, messageId);
	return true;
}


bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : log
	return false;
}

bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	// TODO : Validation check
	if (gameSession->_currentPlayer != nullptr)
		return false;

	return RequestLogin(gameSession, pkt.name(), 0);
}

bool Handle_C_RECONNECT(PacketSessionRef& session, Protocol::C_RECONNECT& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	// TODO : Validation check
	if (gameSession->_currentPlayer != nullptr)
		return false;

	return RequestLogin(gameSession, pkt.name(), pkt.last_message_id());
}

bool Handle_C_CHAT(PacketSessionRef& session, Protocol::C_CHAT& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	PlayerRef player = gameSession->_currentPlayer;

	if (player == nullptr)
		return false;
		
	const string& message = u8"[" + player->_info.name() + u8"]:" + pkt.message();
	GRoom->DoAsync(&Room::BroadcastChat, message, player, player->_info.name());

	return true;
}

bool Handle_C_LEAVE(PacketSessionRef& session, Protocol::C_LEAVE& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	gameSession->Disconnect(L"Kick");

	return true;
}

bool Handle_C_SCROLL_UP(PacketSessionRef& session, Protocol::C_SCROLL_UP& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	if (gameSession == nullptr)
		return false;

	const uint64 now = ::GetTickCount64();
	const uint64 scrollCooldownMs = 2000; // 2ÃÊ ÄðÅ¸ÀÓ

	if (now - gameSession->_lastScrollUpTick < scrollCooldownMs)
		return true;

	GRoom->DoDBAsync(&Room::DBLoadChatFromMessageId, gameSession, pkt.oldest_message_id(), Protocol::RequestHistory::REQUEST_OLDEST);

	return true;
}

bool Handle_C_PONG(PacketSessionRef& session, Protocol::C_PONG& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	gameSession->_lastPongTime = ::GetTickCount64();

	wcout << ">>> Received C_PONG from " << gameSession->GetSessionId()
		<< " | timestamp: " << pkt.timestamp() << endl;
	return true;
}
