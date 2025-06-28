#pragma once
#include "JobQueue.h"
#include "Protocol.pb.h"

using PlayerRef = shared_ptr<class Player>;

class Room : public JobQueue
{
public:
	int64 _currentChatSerial = 1;

public:
	void Init();

	void Update();

	void Enter(GameSessionRef gameSession, int64 messageId);
	void Leave(GameSessionRef gameSession, PlayerRef player);

	void Broadcast(SendBufferRef sendBuffer, int64 exceptId = 0);
	void BroadcastChat(string message, PlayerRef sender, string name);

	void SendCacheChatFromId(GameSessionRef gameSession, int64 startMessageId);
	void SendLoginFail(GameSessionRef gameSession, Protocol::Cause cause, string msg);

	void DBProcessLogin(DBConnection* dbConn, GameSessionRef gameSession, string name, int64 lastSerial = 0);
	void DBSaveMessage(DBConnection* dbConn, PlayerRef sender, wstring msg, int64 serial, int32 retryCount, vector<int64> receiverIds);
	void DBLoadServerInit(DBConnection* dbConn);
	void DBLoadChatFromMessageId(DBConnection* dbConn, GameSessionRef session, int64 messageId, Protocol::RequestHistory request);

	void CleanupPlayers();
	void UpdateCache(int64 serial, int64 messageId);
	void UpdateUserMessageId(vector<int64> receiverIds, int64 messageId);

	void BroadcastPing();
	void CheckPingTimeout();
	void Kick(PlayerRef player);
	
	int64 GetNextSerialId();

	void BindParamsForLoadChatFromMessageId(SP::GetRecentChatMessagesFromId& getMessage, int64 messageId, int64 needCount, int64& outMessageId, int64& outPlayerId, WCHAR (&outMessageBuffer)[200], TIMESTAMP_STRUCT& outTimestamp, int64& outSerial);
	Protocol::ChatMessage FetchChatMessageToProto(int64 messageId, int64 playerId, const WCHAR* messageBuffer, const TIMESTAMP_STRUCT& timestamp, int64 serial);

public:
	USE_LOCK;
	unordered_map<int64, PlayerRef>	_players;
	unordered_map<int64, int64>	_lastSentMessageIdPerUser;
	deque<Protocol::ChatMessage> _chatCache;

	int64 _lastMessageId = 0;
	//Vector<GameSessionRef>	_sessions;

private:
	uint64 _nextCleanupTime = 0;
	uint64 _nextPingTime = 0;
	uint64 _nextPingCheckTime = 0;
};
