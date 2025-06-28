#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "GlobalQueue.h"
#include "Protocol.pb.h"
#include "Struct.pb.h"
#include "Enum.pb.h"
#include "ClientPacketHandler.h"
#include <Convert.h>
#include "DBConnectionPool.h"

void Room::Init()
{
	DBConnection* dbConn = GDBConnectionPool->Pop();
	GRoom->DBLoadServerInit(dbConn);
	GDBConnectionPool->Push(dbConn);
}

void Room::Update()
{
	const uint64 now = ::GetTickCount64();

	if (now >= _nextCleanupTime)
	{
		CleanupPlayers();                // 죽은 세션 정리
		_nextCleanupTime = now + 1000;
	}

	if (now >= _nextPingCheckTime)
	{
		CheckPingTimeout();              // 응답 없는 세션 킥
		_nextPingCheckTime = now + 5000;
	}

	if (now >= _nextPingTime)
	{
		BroadcastPing();                 // Ping 전송
		_nextPingTime = now + 5000;
	}
}

void Room::Enter(GameSessionRef gameSession, int64 messageId)
{
	PlayerRef player = gameSession->_currentPlayer;
	if (player == nullptr)
		return;

	_players[player->_info.player_id()] = player;

	// 나에게 정보 전송
	{
		{
			Protocol::S_ENTER enterPkt;
			*enterPkt.mutable_player() = player->_info;

			auto sendBuffer = ClientPacketHandler::MakeSendBuffer(enterPkt);
			gameSession->Send(sendBuffer);
		}

		{
			Protocol::S_SPAWN spawnPkt;

			for (auto& [id, p] : _players)
			{
				if (player != p)
					*spawnPkt.add_players() = p->_info;
			}

			auto sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);
			if (auto session = player->ownerSession.lock())
				session->Send(sendBuffer);
		}
	}
	
	// 타인에게 S_SPAWN 전송
	{
		Protocol::S_SPAWN spawnPkt;

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);
		for (auto& [id, p] : _players)
		{
			if (id != player->_info.player_id())
			{
				auto sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);
				if (auto session = p->ownerSession.lock())
					session->Send(sendBuffer);
			}
		}
	}

	if (messageId == 0)
	{
		if (_chatCache.empty())
		{
			const string& message = u8"[" + player->_info.name() + u8"] 님이 입장하셨습니다.";
			BroadcastChat(message, player, "SYSTEM");
		}
		else
			GRoom->DoAsync(&Room::SendCacheChatFromId, gameSession, _chatCache.front().message_id());
	}		
	else
	{
		messageId = _lastSentMessageIdPerUser[player->_info.player_id()];
		GRoom->DoAsync(&Room::SendCacheChatFromId, gameSession, messageId + 1); // reset -> cache 보내기
	}
}

void Room::Leave(GameSessionRef gameSession, PlayerRef player)
{
	// 나에게 LEAVE 패킷 전송
	{
		Protocol::S_LEAVE pkt;

		std::string leaveMsg = u8"[" + player->_info.name() + u8"] 님이 채팅방을 나갔습니다.";
		BroadcastChat(leaveMsg, player, "SYSTEM");

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);

		gameSession->Send(sendBuffer);
	}
	
	// 타인에게 DESPAWN 패킷 전송
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.set_player_id(player->_info.player_id());
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

		Broadcast(sendBuffer, player->_info.player_id());
	}

	player->ownerSession.reset();

	_players.erase(player->_info.player_id());
}

void Room::Broadcast(SendBufferRef sendBuffer, int64 exceptId)
{
	for (auto& [id, player] : _players)
	{
		if (id == exceptId)
			continue;

		auto session = player->ownerSession.lock();
		if (session)
		{
			session->Send(sendBuffer);
		}
	}
}

void Room::BroadcastChat(string message, PlayerRef sender, string name)
{
	int64 serial = GetNextSerialId();

	Protocol::ChatMessage chatMsg;
	chatMsg.set_message_id(-1); // 혹은 메시지 순번
	chatMsg.set_serial_id(serial);            // C_CHAT에서 지정
	if (name == "SYSTEM")
		chatMsg.set_player_id(0);
	else
		chatMsg.set_player_id(sender->_info.player_id());
	chatMsg.set_name(name);
	chatMsg.set_message(message);
	chatMsg.set_timestamp(Convert::GetCurrentEpochMilli());

	_chatCache.push_back(chatMsg);

	Protocol::S_CHAT pkt;
	*pkt.mutable_message() = chatMsg;
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);

	vector<int64> receiverIds;

	for (auto& [id, player] : _players)
	{
		if (auto session = player->ownerSession.lock())
		{
			session->Send(sendBuffer);
			receiverIds.push_back(id);
		}
	}
	int32 retryCount = 0;
	wstring wMessage = Convert::UTF8ToWStringDynamic(message);
	DoDBAsync(&Room::DBSaveMessage, sender, wMessage, serial, retryCount, receiverIds);
}

void Room::SendCacheChatFromId(GameSessionRef gameSession, int64 startMessageId)
{
	if (_chatCache.empty())
		return;

	PlayerRef player = gameSession->_currentPlayer;
	if (player == nullptr)
		return;

	Protocol::S_CHAT_HISTORY pkt;
	if (startMessageId < _chatCache.front().message_id())
		pkt.set_request(Protocol::REQUEST_RESET);
	else
		pkt.set_request(Protocol::REQUEST_NEWEST);

	bool startCopying = false;

	for (auto& msg : _chatCache)
	{
		if (!startCopying && msg.message_id() >= startMessageId)
			startCopying = true;

		if (startCopying)
		{
			Protocol::ChatMessage* cacheList = pkt.add_messages(); // repeated field
			*cacheList = msg;
		}
	}

	if (pkt.messages_size() > 0)
	{
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
		gameSession->Send(sendBuffer);
	}
	{
		const string& message = u8"[" + player->_info.name() + u8"] 님이 입장하셨습니다.";
		BroadcastChat(message, player, "SYSTEM");
	}
}

void Room::SendLoginFail(GameSessionRef gameSession, Protocol::Cause cause, string msg)
{
	Protocol::S_LOGIN_FAIL pkt;
	pkt.set_cause(cause);
	pkt.set_message(msg);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	gameSession->Send(sendBuffer);
}

void Room::DBProcessLogin(DBConnection* dbConn, GameSessionRef gameSession, string name, int64 messageId)
{
	WCHAR wName[50] = { };
	if (name.length() <= 0 || name.length() > 50 || !Convert::UTF8ToWCHARArray(wName, name))
	{
		GRoom->DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::CAUSE_INVAILD_NAME, string("Invalid name encoding"));
		return;
	}

	int64 playerId = -1;

	// name으로 접속 -> playerId가 유일한 키. 중복 이름검사보단 중복 이름 접속 불가로.
	// TODO
	SP::GetPlayerIdByName getPlayer(*dbConn);
	getPlayer.In_Name(wName);
	getPlayer.Out_Player_id(playerId);

	if (getPlayer.Execute() && dbConn->Fetch() && playerId > 0)
	{
		if (_players.find(playerId) != _players.end())
		{
			// 중복 Name 접속
			GRoom->DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::CAUSE_ALREADY_LOGGED_IN, string("Player already logged in"));
			return;
		}
	}
	else
	{
		// 신규 Player 등록
		SP::InsertPlayer insertPlayer(*dbConn);
		insertPlayer.In_Name(wName);
		insertPlayer.Out_Player_id(playerId);
		if (!insertPlayer.Execute())
		{
			GRoom->DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::CAUSE_DB_ERROR, string("Insert player failed"));
			return;
		}
		while (dbConn->MoreResults()) {}
		if (playerId <= 0)
		{
			GRoom->DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::CAUSE_DB_ERROR, string("get playerid failed"));
			return;
		}
	}

	// 로그인 기록 저장
	SP::InsertLogin insertLogin(*dbConn);
	insertLogin.In_Player_id(playerId);
	insertLogin.In_Login_time(Convert::GetCurrentTimestamp());
	if (!insertLogin.Execute())
	{
		GRoom->DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::CAUSE_DB_ERROR, string("Insert login log failed"));
		return;
	}

	PlayerRef player = MakeShared<Player>();
	player->_info.set_player_id(playerId);
	player->_info.set_name(name);
	player->ownerSession = gameSession;

	gameSession->_currentPlayer = player;

	GRoom->DoAsync(&Room::Enter, gameSession, messageId);
}

void Room::DBSaveMessage(DBConnection* dbConn, PlayerRef sender, wstring wMsgCopy, int64 serial, int32 retryCount, vector<int64> receiverIds)
{
	SP::InsertChatMessage insert(*dbConn);
	insert.In_Player_id(sender->_info.player_id());
	insert.In_Message(wMsgCopy.c_str(), static_cast<int32>(wMsgCopy.length()));
	insert.In_Serial(serial);
	int64 messageId = -1;
	insert.Out_Message_id(messageId);

	if (!insert.Execute())
	{
		// TODO : Job 재등록
		if (retryCount < 2)
		{
			DoDBAsync(&Room::DBSaveMessage, sender, wMsgCopy, serial, ++retryCount, receiverIds);
		}
		else
		{
			// 포기하고 로깅
			wcout << L"[DB FATAL] 채팅 저장 실패 : Serial=" << serial;
		}
		return;
	}
	while (dbConn->MoreResults()) {}
	if (messageId <= 0)
	{
		// TODO : 
	}
	else
	{
		GRoom->DoAsync(&Room::UpdateCache, serial, messageId);
		GRoom->DoAsync(&Room::UpdateUserMessageId, receiverIds, messageId);
	}
}

void Room::DBLoadServerInit(DBConnection* dbConn)
{
	if (!_chatCache.empty())
		return;

	const int64 maxCount = 10;
	vector<Protocol::ChatMessage> dbMsgs;

	// IN/OUT 바인딩에 사용할 변수들은 바인딩 함수에서 선언 및 참조 전달
	int64 messageIdParam = 0;
	int64 playerIdParam = 0;
	WCHAR messageBuffer[200] = {};
	TIMESTAMP_STRUCT timestampParam = {};
	int64 serialParam = 0;

	// 서버 캐시 비어있으면 DB에서 최신 maxCount개 조회
	SP::GetRecentChatMessagesFromId getMessage(*dbConn);

	// IN/OUT 파라미터 바인딩
	BindParamsForLoadChatFromMessageId(getMessage, INT64_MAX, maxCount,
		messageIdParam, playerIdParam, messageBuffer, timestampParam, serialParam);

	if (!getMessage.Execute())
	{
		// 실패 처리
		return;
	}

	_chatCache.clear();
	int64 maxSerialId = 0;

	while (getMessage.Fetch())
	{
		Protocol::ChatMessage msg = FetchChatMessageToProto(messageIdParam, playerIdParam, messageBuffer, timestampParam, serialParam);

		if (msg.serial_id() > maxSerialId)
			maxSerialId = msg.serial_id();

		_chatCache.push_back(std::move(msg));  // 바로 Room 캐시에 저장
	}

	_currentChatSerial = maxSerialId + 1;
}

// messageId로부터 메시지 로드.
void Room::DBLoadChatFromMessageId(DBConnection* dbConn, GameSessionRef session, int64 messageId, Protocol::RequestHistory request)
{
	if (_chatCache.empty())
		return;

	PlayerRef player = session->_currentPlayer;
	if (player == nullptr)
		return;

	const int64 maxCount = 10;
	vector<Protocol::ChatMessage> dbMsgs;

	// IN/OUT 바인딩에 사용할 변수들은 바인딩 함수에서 선언 및 참조 전달
	int64 messageIdParam = 0;
	int64 playerIdParam = 0;
	WCHAR messageBuffer[200] = {};
	TIMESTAMP_STRUCT timestampParam = {};
	int64 serialParam = 0;

	SP::GetRecentChatMessagesFromId getMessage(*dbConn);

	// IN/OUT 파라미터 바인딩
	BindParamsForLoadChatFromMessageId(getMessage, messageId, maxCount,
		messageIdParam, playerIdParam, messageBuffer, timestampParam, serialParam);

	if (!getMessage.Execute())
	{
		// 실패 처리
		return;
	}

	while (getMessage.Fetch())
	{
		Protocol::ChatMessage msg = FetchChatMessageToProto(messageIdParam, playerIdParam, messageBuffer, timestampParam, serialParam);
		dbMsgs.push_back(std::move(msg));
	}

	Protocol::S_CHAT_HISTORY pkt;
	pkt.set_request(request);
	
	for (const auto& msg : dbMsgs)
		*pkt.add_messages() = msg;

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	session->Send(sendBuffer);

	if (request == Protocol::REQUEST_OLDEST)
		return;

	if (session)
	{
		const string& message = u8"[" + player->_info.name() + u8"] 님이 입장하셨습니다.";
		GRoom->DoAsync(&Room::BroadcastChat, message, player, string("SYSTEM"));
	}
}

void Room::BindParamsForLoadChatFromMessageId(
	SP::GetRecentChatMessagesFromId& getMessage,
	int64 messageId, int64 needCount,
	int64& outMessageId, int64& outPlayerId,
	WCHAR (&outMessageBuffer)[200], TIMESTAMP_STRUCT& outTimestamp, int64& outSerial)
{
	getMessage.In_MessageId(messageId);
	getMessage.In_NeedCount(needCount);

	getMessage.Out_Message_id(outMessageId);
	getMessage.Out_Player_id(outPlayerId);
	getMessage.Out_Message(outMessageBuffer);
	getMessage.Out_Timestamp(outTimestamp);
	getMessage.Out_Serial(outSerial);
}

Protocol::ChatMessage Room::FetchChatMessageToProto(int64 messageId, int64 playerId, const WCHAR* messageBuffer, const TIMESTAMP_STRUCT& timestamp, int64 serial)
{
	Protocol::ChatMessage msg;
	msg.set_message_id(messageId);
	msg.set_player_id(playerId);
	msg.set_timestamp(Convert::GetCurrentEpochMilli());
	msg.set_serial_id(serial);
	msg.set_message(Convert::WStringToUTF8(messageBuffer));
	return msg;
}

void Room::CleanupPlayers()
{
	for (auto it = _players.begin(); it != _players.end(); )
	{
		const auto& player = it->second;
		if (!player || player->ownerSession.expired())
		{
			it = _players.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void Room::UpdateCache(int64 serial, int64 messageId)
{
	for (auto& msg : _chatCache)
	{
		if (msg.serial_id() == serial)
		{
			msg.set_message_id(messageId);
			break;
		}
	}
	
	while (_chatCache.size() > 10)
	{
		if (_chatCache.front().message_id() == -1)
			return;
		_chatCache.pop_front();
	}
}

void Room::UpdateUserMessageId(vector<int64> receiverIds, int64 messageId)
{
	for (auto& id : receiverIds)
	{
		int64& last = _lastSentMessageIdPerUser[id];
		if (last < messageId)
			last = messageId;
	}
}

void Room::BroadcastPing()
{
	uint64 now = ::GetTickCount64();

	Protocol::S_PING pingPkt;
	pingPkt.set_timestamp(now);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pingPkt);

	wcout << "Server Send : Broadcast Ping test. Time = " << now << endl;

	GRoom->DoAsync(&Room::Broadcast, sendBuffer, (int64)0);
}

void Room::CheckPingTimeout()
{
	if (_players.empty())
		return;

	uint64 now = ::GetTickCount64();

	// 순회 도중 Leave -> _players.erase 위험
	for (auto& [id, player] : _players)
	{
		if (!player)
			continue; // nullptr 보호

		auto session = player->ownerSession.lock();
		if (!session)
			continue;

		if ((now - session->_lastPongTime) >= 20000)
		{
			wcout << L"[Ping Timeout] Kicking session: " << session->GetSessionId() << endl;

			// 곧바로 Disconnect X, Room Job으로 Kick(Disconnect) 예약
			PlayerRef p = player;
			GRoom->DoAsync(&Room::Kick, p);
		}
	}
}

void Room::Kick(PlayerRef player)
{
	auto session = player->ownerSession.lock();
	if (session)
		session->Disconnect(L"Ping Timeout");
}

int64 Room::GetNextSerialId()
{
	return _currentChatSerial++;
}