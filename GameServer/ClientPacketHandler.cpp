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

	// DB Job ( 내부에서 Room::Enter )

	// Temp
	static Atomic<uint64> idGenerator = 1;
	PlayerRef playerRef = MakeShared<Player>();
	playerRef->playerId = idGenerator++;
	playerRef->name = pkt.name();
	playerRef->ownerSession = gameSession;

	gameSession->_currentPlayer = playerRef;

	gameSession->_room = GRoom;

	GRoom->DoAsync(&Room::Enter, gameSession);

	return true;
}

bool Handle_C_CHAT(PacketSessionRef& session, Protocol::C_CHAT& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	PlayerRef player = gameSession->_currentPlayer;

	WCHAR convertName[50] = { 0 };
	WCHAR convertMsg[100] = { 0 };

	if (!Convert::UTF8ToWCHARArray(convertName, player->name))
		return false;

	if (!Convert::UTF8ToWCHARArray(convertMsg, pkt.message()) || wcslen(convertMsg) == 0)
		return false;

	int32 lenName = static_cast<int32>(wcslen(convertName));
	int32 lenMsg = static_cast<int32>(wcslen(convertMsg));

	// 복사본 사용 -> 캡처 시 메모리 안전
	std::wstring wNameCopy = convertName;
	std::wstring wMsgCopy = convertMsg;

	wcout << L"Send To Room) Name[" << wNameCopy << L"] Msg[" << wMsgCopy << L"]" << endl;
	//// 기존 DB save 블로킹 방식에서 -> GDBJobQueue 등록 DBWorker 비동기 처리.
	GRoom->DoDBAsync(&Room::DBSave, wNameCopy, wMsgCopy);

	Protocol::S_CHAT chatPkt;

	chatPkt.set_name(player->name);
	chatPkt.set_message(pkt.message());
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(chatPkt);

	GRoom->DoAsync(&Room::Broadcast, sendBuffer);

	return true;
}

bool Handle_C_LEAVE(PacketSessionRef& session, Protocol::C_LEAVE& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	PlayerRef player = gameSession->_currentPlayer;

	GRoom->DoAsync(&Room::Leave, player);
	return true;
}

bool Handle_C_PING(PacketSessionRef& session, Protocol::C_PING& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	uint64_t now = ::GetTickCount64();
	gameSession->_lastPingTime.store(now);

	Protocol::S_PONG pongPkt;
	pongPkt.set_timestamp(now);

	SendBufferRef sendBuffer = ClientPacketHandler::MakeSendBuffer(pongPkt);
	session->Send(sendBuffer);


	return true;
}
