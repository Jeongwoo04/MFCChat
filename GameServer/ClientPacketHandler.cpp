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

	string name = pkt.name();

	GRoom->DoDBAsync(&Room::DBProcessLogin, gameSession, name);

	return true;
}

bool Handle_C_CHAT(PacketSessionRef& session, Protocol::C_CHAT& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	PlayerRef player = gameSession->_currentPlayer;

	wstring wNameCopy = Convert::UTF8ToWStringDynamic(player->name);
	wstring wMessageCopy = Convert::UTF8ToWStringDynamic(pkt.message());

	int32 retryCount = 0;
	GRoom->DoDBAsync(&Room::DBSaveMessage, gameSession, wMessageCopy, GRoom->_currentChatSerial++, retryCount);
	wcout << L"Send To Room) ID[" << player->playerId << "] " << "Name[" << wNameCopy << L"] Msg[" << wMessageCopy << L"]" << endl;

	Protocol::S_CHAT chatPkt;

	chatPkt.set_message_id(GRoom->_currentChatSerial);
	chatPkt.set_player_id(player->playerId);
	chatPkt.set_name(player->name);
	const string& sendMsg = u8"[" + player->name + u8"]:" + pkt.message();
	chatPkt.set_message(sendMsg);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(chatPkt);

	GRoom->DoAsync(&Room::Broadcast, sendBuffer);

	return true;
}

bool Handle_C_LEAVE(PacketSessionRef& session, Protocol::C_LEAVE& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	GRoom->DoAsync(&Room::Leave, gameSession->_currentPlayer);
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
