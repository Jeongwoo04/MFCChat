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
#include "Convert.h"

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

	Protocol::S_LOGIN loginPkt;
	loginPkt.set_success(true);

	static Atomic<uint64> idGenerator = 1;

	auto player = loginPkt.add_players();

	//memory
	PlayerRef playerRef = MakeShared<Player>();
	playerRef->playerId = idGenerator++;
	playerRef->name = pkt.name();
	playerRef->ownerSession = gameSession; 
	gameSession->_players.push_back(playerRef);

	uint64 index = pkt.playerindex();
	gameSession->_currentPlayer = gameSession->_players[index];

	gameSession->_room = GRoom;

	GRoom->DoAsync(&Room::Enter, gameSession->_currentPlayer);

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(loginPkt);
	gameSession->_currentPlayer->ownerSession->Send(sendBuffer);

	return true;
}

bool Handle_C_ENTER_CHAT(PacketSessionRef& session, Protocol::C_ENTER_CHAT& pkt)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
	// TODO : Validation

	// 답장
	Protocol::S_ENTER_CHAT enterChatPkt;
	enterChatPkt.set_success(true);
	
	WCHAR convertName[100] = { 0 };
	if (!UTF8ToWCHARArray(convertName, pkt.name()))
		return false;

	enterChatPkt.set_msg(u8"[" + gameSession->_currentPlayer->name + u8"] 님이 채팅방에 입장하셨습니다");
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(enterChatPkt);
	
	GRoom->DoAsync(&Room::Broadcast, sendBuffer);

	return true;
}

bool Handle_C_CHAT(PacketSessionRef& session, Protocol::C_CHAT& pkt)
{
	Protocol::S_CHAT chatPkt;
	
	WCHAR convertName[50] = { 0 };
	WCHAR convertMsg[100] = { 0 };

	if (!UTF8ToWCHARArray(convertName, pkt.name()))
		return false;

	if (!UTF8ToWCHARArray(convertMsg, pkt.msg()) || wcslen(convertMsg) == 0)
		return false;

	int32 lenName = static_cast<int32>(wcslen(convertName));
	int32 lenMsg = static_cast<int32>(wcslen(convertMsg));

	// 복사본 사용 -> 캡처 시 메모리 안전
	std::wstring wNameCopy = convertName;
	std::wstring wMsgCopy = convertMsg;

	wcout << L"Send To Room) Name[" << wNameCopy << L"] Msg[" << wMsgCopy << L"]" << endl;
	//// 기존 DB save 블로킹 방식에서 -> GDBJobQueue 등록 DBWorker 비동기 처리.
	GRoom->DoAsync(&Room::DBSave, wNameCopy, wMsgCopy);

	chatPkt.set_name(pkt.name());
	chatPkt.set_msg(pkt.msg());
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(chatPkt);

	GRoom->DoAsync(&Room::Broadcast, sendBuffer);

	return true;
}

bool Handle_C_REQUEST_HISTORY_CHAT(PacketSessionRef& session, Protocol::C_REQUEST_HISTORY_CHAT& pkt)
{
	Protocol::S_REQUEST_HISTORY_CHAT requestHistoryPkt;

	{
		DBConnection* dbConn = GDBConnectionPool->Pop();
		SP::GetAllMsg GetHistory(*dbConn);
		GetHistory.In_Id(0);

		int32 id = 0;
		WCHAR name[100] = { 0 };
		WCHAR msg[200] = { 0 };;

		GetHistory.Out_Id(OUT id);
		GetHistory.Out_Name(OUT name);
		GetHistory.Out_Msg(OUT msg);

		GetHistory.Execute();

		while (GetHistory.Fetch())
		{
			wcout.imbue(locale("ko_KR.UTF-8"));
			wcout << L"Load DB data) Name[" << name << L"] Msg[" << msg << L"]" << endl;
		}
		GDBConnectionPool->Push(dbConn);
	}
	return true;
}
