#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "GlobalQueue.h"
#include "Protocol.pb.h"
#include "ClientPacketHandler.h"
#include <Convert.h>

void Room::Enter(GameSessionRef gameSession, PlayerRef player)
{
	_players[player->playerId] = player;

	Protocol::S_ENTER enterPkt;
	enterPkt.set_player_id(player->playerId);
	enterPkt.set_name(player->name);

	// 나에게 S_ENTER 전송
	{
		for (auto& [id, p] : _players)
		{
			if (id == player->playerId)
				continue;
			Protocol::PlayerInfo* info = enterPkt.add_players();
			info->set_player_id(p->playerId);
			info->set_name(p->name);
		}

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(enterPkt);
		gameSession->Send(sendBuffer);
	}
	
	// 타인에게 S_SPAWN 전송
	{
		Protocol::S_SPAWN spawnPkt;

		spawnPkt.set_player_id(player->playerId);
		spawnPkt.set_name(player->name);

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(spawnPkt);
		for (auto& [id, p] : _players)
		{
			if (id == player->playerId)
				continue;
			if (auto session = p->ownerSession.lock())
				session->Send(sendBuffer);
		}
	}
	auto it = _lastSentMessageIdPerUser.find(player->playerId);
	if (it != _lastSentMessageIdPerUser.end())
		this->DoDBAsync(&Room::DBLoadRecentMessages, gameSession, _lastSentMessageIdPerUser[player->playerId]);

	// Room에 입장 S_CHAT 알림
	{
		Protocol::S_CHAT chatPkt;

		chatPkt.set_player_id(player->playerId);
		chatPkt.set_name(player->name);
		string message = u8"[" + player->name + u8"] 님이 입장하셨습니다.";
		chatPkt.set_message(message);
		chatPkt.set_timestamp(Convert::GetCurrentEpochMilli());

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(chatPkt);
		wstring wMessage = Convert::UTF8ToWStringDynamic(message);
		int32 retryCount = 0;
		int64 serial = _currentChatSerial++;
		this->DoDBAsync(&Room::DBSaveMessage, gameSession, wMessage, serial, retryCount);
		this->DoAsync(&Room::Broadcast, sendBuffer);
	}
}

void Room::Leave(GameSessionRef gameSession, PlayerRef player)
{
	// 나에게 LEAVE 패킷 전송
	{
		Protocol::S_LEAVE pkt;
		pkt.set_player_id(player->playerId);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);

		gameSession->Send(sendBuffer);
	}
	
	// 타인에게 DESPAWN 패킷 전송
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.set_player_id(player->playerId);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

		BroadcastOthers(player, sendBuffer);
	}

	std::string leaveMsg = u8"[" + player->name + u8"] 님이 채팅방을 나갔습니다.";

	player->ownerSession.reset();

	_players.erase(player->playerId);

	BroadcastSysMessage(leaveMsg);
}

void Room::Broadcast(SendBufferRef sendBuffer)
{
	for (auto it = _players.begin(); it != _players.end(); )
	{
		auto& player = it->second;
		if (!player)
		{
			it = _players.erase(it);
			continue;
		}

		auto session = player->ownerSession.lock();
		if (!session)
		{
			it = _players.erase(it);
			continue;
		}

		session->Send(sendBuffer);
		++it;
	}
}

void Room::BroadcastOthers(PlayerRef player, SendBufferRef sendBuffer)
{
	for (auto& [id, p] : _players)
	{
		if (p->playerId == player->playerId)
			continue;

		if (auto session = p->ownerSession.lock())
			session->Send(sendBuffer);
	}
}

void Room::BroadcastSysMessage(const string& message)
{
	Protocol::S_CHAT sysMsgPkt;
	sysMsgPkt.set_player_id(0);
	sysMsgPkt.set_message(message);

	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(sysMsgPkt);

	Broadcast(sendBuffer);
}

void Room::BroadcastPing()
{
	uint64 now = ::GetTickCount64();

	Protocol::S_PING pingPkt;
	pingPkt.set_timestamp(now);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pingPkt);

	wcout << "Server Send : Broadcast Ping test. Time = " << now << endl;

	Broadcast(sendBuffer);
}

void Room::DBProcessLogin(DBConnection* dbConn, GameSessionRef gameSession, string name, int64 lastSerial)
{
	WCHAR wName[50] = { };
	if (name.length() <= 0 || !Convert::UTF8ToWCHARArray(wName, name))
	{
		DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::INVAILD_NAME, string("Invalid name encoding"));
		return;
	}

	int32 playerId = -1;

	// name으로 접속 -> playerId가 유일한 키. 중복 이름검사보단 중복 이름 접속 불가로.
	// TODO : Account ID / PW -> Unity에서
	SP::GetPlayerIdByName getPlayer(*dbConn);
	getPlayer.In_Name(wName);
	getPlayer.Out_Player_id(playerId);

	if (getPlayer.Execute() && dbConn->Fetch() && playerId > 0)
	{
		if (_players.find(playerId) != _players.end())
		{
			// 중복 Name 접속
			DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::ALREADY_LOGGED_IN, string("Player already logged in"));
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
			DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::DB_ERROR, string("Insert player failed"));
			return;
		}
		while (dbConn->MoreResults()) {}
		if (playerId <= 0)
		{
			DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::DB_ERROR, string("get playerid failed"));
			return;
		}
	}

	// 로그인 기록 저장
	SP::InsertLogin insertLogin(*dbConn);
	insertLogin.In_Player_id(playerId);
	insertLogin.In_Login_time(Convert::GetCurrentTimestamp());
	if (!insertLogin.Execute())
	{
		DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::DB_ERROR, string("Insert login log failed"));
		return;
	}

	PlayerRef player = MakeShared<Player>();
	player->playerId = playerId;
	player->name = name;
	player->ownerSession = gameSession;

	gameSession->_currentPlayer = player;

	DoAsync(&Room::Enter, gameSession, player);	
}

void Room::SendLoginFail(GameSessionRef gameSession, Protocol::Cause cause, string msg)
{
	Protocol::S_LOGIN_FAIL pkt;
	pkt.set_cause(cause);
	pkt.set_message(msg);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	gameSession->Send(sendBuffer);
}

// Client 재 연결시 마지막 수신 messageId로부터 메시지 로드.
void Room::DBLoadRecentMessages(DBConnection* dbConn, GameSessionRef session, int lastSerial)
{
	SP::GetRecentChatMessages getMessage(*dbConn);
	getMessage.In_LastSerial(lastSerial);

	int32 messageId = 0;
	getMessage.Out_Message_id(messageId);

	int32 playerId = 0;
	getMessage.Out_Player_id(playerId);

	WCHAR messageBuffer[200] = {};
	getMessage.Out_Message(messageBuffer);

	TIMESTAMP_STRUCT timestamp = {};
	getMessage.Out_Timestamp(timestamp);

	int64 serial = 0;
	getMessage.Out_Serial(serial);

	if (!getMessage.Execute() && dbConn->Fetch())
	{
		return;
	}

	int count = 0;
	const int maxCount = 30;

	while (getMessage.Fetch())
	{
		if (count++ >= maxCount)
			break;

		// 클라이언트에게 전송할 패킷 생성
		Protocol::S_CHAT chatPkt;
		chatPkt.set_message_id(messageId);
		chatPkt.set_player_id(playerId);
		chatPkt.set_timestamp(Convert::GetCurrentEpochMilli());
		chatPkt.set_serial(serial);

		const string& sendMsg = Convert::WStringToUTF8(messageBuffer);
		chatPkt.set_message(sendMsg);

		wcout << "ID[" << playerId << "], NAME[" << _players[playerId] << "], SERIAL[" << serial << "]-" << messageBuffer << endl;

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(chatPkt);
		session->Send(sendBuffer);
	}
}


void Room::CheckPingTimeout()
{
	if (_players.empty())
		return;

	uint64 now = ::GetTickCount64();
	Vector<PlayerRef> timedOutPlayers;

	// 순회 도중 Leave -> _players.erase 위험
	for (auto& [id, player] : _players)
	{
		if (!player)
			continue; // nullptr 보호

		auto session = player->ownerSession.lock();
		if (!session)
			continue;

		// 20초 이상 응답 없으면 Disconnect
		if ((now - session->_lastPongTime) >= 20000)
		{
			wcout << L"Kicking session: " << session->GetSessionId() << endl;
			timedOutPlayers.push_back(player); // 목록에 추가
		}
	}

	// 반복문이 끝난 뒤 안전하게 삭제
	for (PlayerRef p : timedOutPlayers)
	{
		auto session = p->ownerSession.lock();
		session->Disconnect(L"Kick");
	}
}

//
void Room::DBSaveMessage(DBConnection* dbConn, GameSessionRef gameSession, wstring wMsgCopy, int64 serial, int32 retryCount)
{
	SP::InsertChatMessage insert(*dbConn);
	insert.In_Player_id(gameSession->_currentPlayer->playerId);
	insert.In_Message(wMsgCopy.c_str(), static_cast<int32>(wMsgCopy.length()));
	insert.In_Serial(serial);
	int32 messageId = -1;
	insert.Out_Message_id(messageId);
	if (!insert.Execute())
	{
		// TODO : Job 재등록
		if (retryCount < 2)
		{
			DoDBAsync(&Room::DBSaveMessage, gameSession, wMsgCopy, serial, ++retryCount);
		}
		else
		{
			// 포기하고 로깅
			wcout << L"[DB FATAL] 채팅 저장 실패 : Serial=" << serial;
		}
	}
	while (dbConn->MoreResults()) {}
	if (messageId <= 0)
	{
		// TODO : 
	}
	for (auto& [i, p] : _players)
	{
		_lastSentMessageIdPerUser[i] = serial;
	}
}