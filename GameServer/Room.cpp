#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"

void Room::Enter(PlayerRef player)
{
	_players[player->playerId] = player;
}

void Room::Leave(PlayerRef player)
{
	_players.erase(player->playerId);
}

void Room::Broadcast(SendBufferRef sendBuffer)
{
	//for (auto& session : _sessions)
	//{
	//	session->Send(sendBuffer);
	//}
	for (auto& player : _players)
	{
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