#pragma once
#include "JobQueue.h"

using PlayerRef = shared_ptr<class Player>;

class Room : public JobQueue
{
public:
	void Enter(PlayerRef player);
	void Leave(PlayerRef player);
	void Broadcast(SendBufferRef sendBuffer);
	void DBSave(DBConnection* dbConn, std::wstring name, std::wstring msg);
	//void DBLoad(std::wstring wNameCopy, std::wstring wMsgCopy);

public:
	USE_LOCK;
	map<uint64, PlayerRef>	_players;
	//Vector<GameSessionRef>	_sessions;
};