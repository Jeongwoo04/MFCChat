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
		enterPkt.set_user_id(player->playerId);
		enterPkt.set_name(player->name);

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

	// Room 다른 멤버에게 입장 알림
	{
		Protocol::S_CHAT chatPkt;

		chatPkt.set_player_id(player->playerId);
		chatPkt.set_name(player->name);

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(chatPkt);

		GRoom->DoAsync(&Room::Broadcast, sendBuffer);
	}

	//static Atomic<uint64> idGenerator = 1; // TODO : DB 긁어오기

	//gameSession->_currentPlayer->playerId = idGenerator++;
	//gameSession->_currentPlayer->name = pkt.name();
	//gameSession->_currentPlayer->ownerSession = gameSession;
	//_players[gameSession->_currentPlayer->playerId] = gameSession->_currentPlayer;
}

void Room::Leave(PlayerRef player)
{
	_players.erase(player->playerId);

	Protocol::S_LEAVE pkt;
	pkt.set_player_id(player->playerId);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
	DoAsync(&Room::BroadcastOthers, player, sendBuffer);
}

void Room::Broadcast(SendBufferRef sendBuffer)
{
	for (auto& player : _players)
	{
		player.second->ownerSession->Send(sendBuffer);
	}
}

// Room 포함 전체에 알림
void Room::BroadcastOthers(PlayerRef owner, SendBufferRef sendBuffer)
{
	for (auto& player : _players)
	{
		if (player.second == owner)
			continue;

		player.second->ownerSession->Send(sendBuffer);
	}
}

void Room::DBSave(DBConnection* dbConn, std::wstring wNameCopy, std::wstring wMsgCopy)
{
	SP::InsertMsg insert(*dbConn);
	insert.In_Msg(wMsgCopy.c_str(), static_cast<int32>(wMsgCopy.length()));
	insert.In_Name(wNameCopy.c_str(), static_cast<int32>(wNameCopy.length()));
	insert.Execute();
}

PlayerRef Room::FindPlayer(uint64 playerId)
{
	return PlayerRef();
}
