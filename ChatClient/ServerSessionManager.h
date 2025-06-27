#pragma once

class ServerSession;
using ServerSessionRef = shared_ptr<ServerSession>;

class ServerSessionManager;
extern ServerSessionManager* GServerSessionManager;
extern atomic<int64> GServerSessionIdGenerator;

class ServerSessionManager
{
public:
	void	Add(ServerSessionRef session);
	void	Remove(ServerSessionRef session);

	ServerSessionRef Find(int64 sessionId);

	int64 GetLastMessageId() { return _lastRecvMessageId; }
	bool GetReconnected() { return _isReconnected; }
	void SetLastMessageId(int64 messageId) { _lastRecvMessageId = messageId; }
	void SetReconnectedTrue() { _isReconnected = true; }

	int64 GetOldestMessageId() { return _oldestMessageId; }
	void SetOldestMessageId(int64 messageId) { _oldestMessageId = messageId; }

private:
	USE_LOCK;
	unordered_map<uint64, ServerSessionRef>	_sessions;
	int64 _lastRecvMessageId;
	int64 _oldestMessageId = INT64_MAX;
	bool _isReconnected = false;
};