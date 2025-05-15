#pragma once

class GameSession;

using GameSessionRef = shared_ptr<GameSession>;

class GameSessionManager
{
public:
	void	Add(GameSessionRef session);
	void	Remove(GameSessionRef session);

	// 전체 메시지
	void	Broadcast(SendBufferRef sendBuffer);

	void	CheckClientAlive(const Set<GameSessionRef>& sessions);

private:
	USE_LOCK;

	Set<GameSessionRef>	_sessions;
};

//extern GameSessionManager GSessionManager;
