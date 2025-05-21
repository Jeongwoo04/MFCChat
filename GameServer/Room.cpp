#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "GlobalQueue.h"
#include "Protocol.pb.h"
#include "ClientPacketHandler.h"
#include <Convert.h>

void Room::Enter(GameSessionRef gameSession)
{
	PlayerRef player = gameSession->_currentPlayer;
	 
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
			p->ownerSession->Send(sendBuffer);
		}
	}

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
		this->DoDBAsync(&Room::DBSaveMessage, gameSession, wMessage, _currentChatSerial++);
		DoAsync(&Room::Broadcast, sendBuffer);
	}
}

void Room::Leave(PlayerRef player)
{
	// 나에게 LEAVE 패킷 전송
	{
		Protocol::S_LEAVE pkt;
		pkt.set_player_id(player->playerId);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);

		player->ownerSession->Send(sendBuffer);
	}
	
	// 타인에게 DESPAWN 패킷 전송
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.set_player_id(player->playerId);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(despawnPkt);

		BroadcastOthers(player->ownerSession, sendBuffer);
	}

	_players.erase(player->playerId);
}

void Room::Broadcast(SendBufferRef sendBuffer)
{
	for (auto& player : _players)
	{
		player.second->ownerSession->Send(sendBuffer);
	}
}

void Room::BroadcastOthers(GameSessionRef gameSession, SendBufferRef sendBuffer)
{
	for (auto& player : _players)
	{
		if (player.second->playerId == gameSession->_currentPlayer->playerId)
			continue;

		player.second->ownerSession->Send(sendBuffer);
	}
}

void Room::BroadcastPing()
{
	using namespace std::chrono;
	uint64 now = ::GetTickCount64();

	Protocol::S_PING pingPkt;
	pingPkt.set_timestamp(now);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pingPkt);

	Broadcast(sendBuffer);
}

void Room::DBProcessLogin(DBConnection* dbConn, GameSessionRef gameSession, string name)
{
	WCHAR wName[50] = { };
	if (name.length() <= 0 || !Convert::UTF8ToWCHARArray(wName, name))
	{
		DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::INVAILD_NAME, string("Invalid name encoding"));
		return;
	}

	int32 playerId = -1;

	// name으로 접속 -> playerId가 유일한 키. 중복 이름검사보단 중복 이름 접속 불가로.
	// TODO : Account ID / PW -> MFC에선 그만..
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

	// 3. 로그인 기록 저장
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

	DoAsync(&Room::Enter, gameSession);	
}

void Room::SendLoginFail(GameSessionRef gameSession, Protocol::Cause cause, string msg)
{
	Protocol::S_LOGIN_FAIL pkt;
	pkt.set_cause(cause);
	pkt.set_message(msg);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	gameSession->Send(sendBuffer);
}

void Room::CheckPingTimeout()
{
	uint64 now = ::GetTickCount64();
	for (auto& [id, player] : _players)
	{
		auto session = player->ownerSession;

		// 5000ms = 5초 이상 응답 없으면 Disconnect
		if ((now - session->_lastPongTime) >= 5000)
		{
			wcout << L"Kicking session: " << session->GetSessionId() << endl;
			session->Disconnect(L"Kick Client : Client Dead");
		}
	}
}

//
void Room::DBSaveMessage(DBConnection* dbConn, GameSessionRef gameSession, wstring wMsgCopy, int64 serial)
{
	SP::InsertChatMessage insert(*dbConn);
	insert.In_Player_id(gameSession->_currentPlayer->playerId);
	insert.In_Message(wMsgCopy.c_str(), static_cast<int32>(wMsgCopy.length()));
	insert.In_Timestamp(Convert::GetCurrentTimestamp());
	insert.In_Serial(serial);
	int32 messageId = -1;
	insert.Out_Message_id(messageId);
	if (!insert.Execute())
	{
		// TODO : Job 재등록
	}
	while (dbConn->MoreResults()) {}
	if (messageId <= 0)
	{
		// TODO : Job 재등록
	}
}

//PlayerRef Room::FindPlayer(uint64 playerId)
//{
//	return PlayerRef();
//}
