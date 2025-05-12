#include "pch.h"
#include "GameSessionManager.h"
#include "GameSession.h"

//GameSessionManager GSessionManager;

void GameSessionManager::Add(GameSessionRef session)
{
	WRITE_LOCK;
	_sessions.insert(session);
}

void GameSessionManager::Remove(GameSessionRef session)
{
	WRITE_LOCK;
	_sessions.erase(session);
}

// 채팅 프로그램에서 Room 안의 참가자 모두에게 보여지게 뿌림
void GameSessionManager::Broadcast(SendBufferRef sendBuffer) // for 돌면서 동일한 데이터를 보내주겠다. (복사비용 1번)
{
	WRITE_LOCK;
	for (GameSessionRef session : _sessions)
	{
		session->Send(sendBuffer); // -> loop 탈때 _sessions를 건드리는지 조심 !
	}
}