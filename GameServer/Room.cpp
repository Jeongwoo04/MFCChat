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

	{
		WRITE_LOCK;
		_players[player->playerId] = player;
	}

	// S_ENTER 전송
	{
		Protocol::S_ENTER enterPkt;
		enterPkt.set_player_id(player->playerId);
		enterPkt.set_name(player->name);

		for (auto& [id, p] : _players)
		{
			//if (id == player->playerId)
			//	continue;

			Protocol::PlayerInfo* info = enterPkt.add_players();
			info->set_player_id(p->playerId);
			info->set_name(p->name);
		}

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(enterPkt);
		gameSession->Send(sendBuffer);
	}

	// Room 다른 멤버에게 입장 알림
	{
		Protocol::S_CHAT chatPkt;

		chatPkt.set_player_id(player->playerId);
		chatPkt.set_name(player->name);
		string message = u8"[" + player->name + u8"] 님이 입장하셨습니다.";
		chatPkt.set_message(message);
		chatPkt.set_timestamp(Convert::GetCurrentEpochMilli());

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(chatPkt);
		wstring wMessage = Convert::UTF8ToWStringDynamic(message);
		this->DoDBAsync(&Room::DBSaveMessage, gameSession, wMessage);
		this->DoAsync(&Room::Broadcast, gameSession, sendBuffer);
	}
}

void Room::Leave(PlayerRef player)
{
	_players.erase(player->playerId);

	Protocol::S_LEAVE pkt;
	pkt.set_player_id(player->playerId);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);

	this->DoAsync(&Room::BroadcastOthers, player->ownerSession, sendBuffer);
}

void Room::Broadcast(GameSessionRef gameSession, SendBufferRef sendBuffer)
{
	for (auto& player : _players)
	{
		player.second->ownerSession->Send(sendBuffer);
	}
}

// TODO : 사용할 부분이 생기면 수정
void Room::BroadcastOthers(GameSessionRef gameSession, SendBufferRef sendBuffer)
{
	for (auto& player : _players)
	{
		if (player.second == gameSession->_currentPlayer)
			continue;

		player.second->ownerSession->Send(sendBuffer);
	}
}


void Room::DBProcessLogin(DBConnection* dbConn, GameSessionRef gameSession, string name)
{
	WCHAR wName[50] = { };
	if (!Convert::UTF8ToWCHARArray(wName, name))
	{
		DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::INVAILD_NAME, string("Invalid name encoding"));
		return;
	}

	int32 playerId = -1;
	//WCHAR dbName[50] = { };

	SP::GetPlayerIdByName getPlayer(*dbConn);
	getPlayer.In_Name(wName);
	getPlayer.Out_Player_id(playerId);
	if (!getPlayer.Execute())
	{
		DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::DB_ERROR, string("DB query failed"));
		return;
	}

	if (dbConn->Fetch())
	{
		DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::DB_ERROR, string("Already exist name"));
		return;
	}

	// 2. 이름 없으니 신규 등록
	SP::InsertPlayer insertPlayer(*dbConn);
	insertPlayer.In_Name(wName);
	if (!insertPlayer.Execute())
	{
		DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::DB_ERROR, string("DB insert failed"));
		return;
	}

	// 3. 다시 playerId 조회
	SP::GetPlayerIdByName getNewPlayer(*dbConn);
	getNewPlayer.In_Name(wName);
	getNewPlayer.Out_Player_id(playerId);
	if (!getNewPlayer.Execute() || !dbConn->Fetch())
	{
		DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::DB_ERROR, string("Failed to get playerId after insert"));
		return;
	}

	if (playerId == -1)
	{
		DoAsync(&Room::SendLoginFail, gameSession, Protocol::Cause::DB_ERROR, string("No playerId returned"));
		return;
	}

	// 4. 로그인 기록 저장
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

}

//
void Room::DBSaveMessage(DBConnection* dbConn, GameSessionRef gameSession, wstring wMsgCopy)
{
	SP::InsertChatMessage insert(*dbConn);
	insert.In_Player_id(gameSession->_currentPlayer->playerId);
	insert.In_Message(wMsgCopy.c_str(), static_cast<int32>(wMsgCopy.length()));
	insert.In_Timestamp(Convert::GetCurrentTimestamp());
	insert.Execute();

}

PlayerRef Room::FindPlayer(uint64 playerId)
{
	return PlayerRef();
}
