#pragma once
#include "JobQueue.h"

using PlayerRef = shared_ptr<class Player>;

class Room : public JobQueue
{
public:
	int64 _currentChatSerial = 1;

public:
	void Update();

	void Enter(GameSessionRef gameSession, PlayerRef player);
	void Leave(GameSessionRef gameSession, PlayerRef player);
	void Broadcast(SendBufferRef sendBuffer);
	void BroadcastOthers(PlayerRef player, SendBufferRef sendBuffer);
	void BroadcastSysMessage(const string& message);
	void BroadcastEnter(GameSessionRef gameSession, PlayerRef player);
	

	void SendLoginFail(GameSessionRef gameSession, Protocol::Cause cause, string msg);

	void DBProcessLogin(DBConnection* dbConn, GameSessionRef gameSession, string name, int64 lastSerial = 0);
	void DBSaveMessage(DBConnection* dbConn, GameSessionRef gameSession, wstring msg, int64 serial, int32 retryCount);
	void DBLoadRecentMessages(DBConnection* dbConn, GameSessionRef session, int lastMessageId);

	void CleanupPlayers();
	void BroadcastPing();
	void CheckPingTimeout();
	void Kick(PlayerRef player);

public:
	USE_LOCK;
	unordered_map<uint64, PlayerRef>	_players;
	unordered_map<uint64, int32>	_lastSentMessageIdPerUser;
	//Vector<GameSessionRef>	_sessions;

private:
	uint64 _nextCleanupTime = 0;
	uint64 _nextPingTime = 0;
	uint64 _nextPingCheckTime = 0;
};
