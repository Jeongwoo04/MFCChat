#pragma once

class ServerSession;
using ServerSessionRef = shared_ptr<ServerSession>;

class ServerSessionManager;
extern ServerSessionManager* GServerSessionManager;
extern atomic<uint64_t> GServerSessionIdGenerator;

class ServerSessionManager
{
public:
	void	Add(ServerSessionRef session);
	void	Remove(ServerSessionRef session);

	ServerSessionRef Find(uint64 sessionId);

	uint64 GetChatSerial() { return _lastRecvChatSerial; }
	bool GetReconnected() { return _isReconnected; }
	void SetChatSerial(uint64 serial) { _lastRecvChatSerial = serial; }
	void SetReconnectedTrue() { _isReconnected = true; }

private:
	USE_LOCK;
	unordered_map<uint64, ServerSessionRef>	_sessions;
	int64 _lastRecvChatSerial;
	bool _isReconnected = false;
};