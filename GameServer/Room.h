#pragma once
#include "JobQueue.h"

using PlayerRef = shared_ptr<class Player>;

class Room : public JobQueue
{
public:
	void Enter(GameSessionRef gameSession);
	void Leave(PlayerRef player);
	void Broadcast(GameSessionRef gameSession, SendBufferRef sendBuffer);
	void BroadcastOthers(GameSessionRef gameSession, SendBufferRef sendBuffer);

	void SendLoginFail(GameSessionRef gameSession, Protocol::Cause cause, string msg);

	void DBProcessLogin(DBConnection* dbConn, GameSessionRef gameSession, string name);
	void DBSaveMessage(DBConnection* dbConn, GameSessionRef gameSession, wstring msg);

	PlayerRef FindPlayer(uint64 playerId);

public:
	USE_LOCK;
	unordered_map<uint64, PlayerRef>	_players;
	unordered_map<uint64, int32>	_lastSentMessageIdPerUser;
	//Vector<GameSessionRef>	_sessions;
};
