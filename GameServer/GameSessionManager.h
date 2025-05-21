#pragma once

class GameSession;

using GameSessionRef = shared_ptr<GameSession>;

class GameSessionManager
{
public:
	void	Add(GameSessionRef session);
	void	Remove(GameSessionRef session);
	GameSessionRef Find(uint64 sessionId);

	// 전체 메시지
	void	Broadcast(SendBufferRef sendBuffer);

	void	Kick(uint64 sessionId);

private:
	USE_LOCK;
	unordered_map<uint64, GameSessionRef>	_sessions;
};

//extern GameSessionManager GSessionManager;
