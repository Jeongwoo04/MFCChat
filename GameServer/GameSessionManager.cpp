#include "pch.h"
#include "GameSessionManager.h"
#include "GameSession.h"
#include "Player.h"

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

// 채팅 프로그램에서 전체 메시지
void GameSessionManager::Broadcast(SendBufferRef sendBuffer) // for 돌면서 동일한 데이터를 보내주겠다. (복사비용 1번)
{
	WRITE_LOCK;
	for (GameSessionRef session : _sessions)
	{
		session->Send(sendBuffer); // -> loop 탈때 _sessions를 건드리는지 조심 !
	}
}

void GameSessionManager::CheckClientAlive(const Set<GameSessionRef>& sessions)
{
    const uint64_t timeoutMs = 15000; // 15초 이상 응답 없으면 끊김 처리
    uint64_t now = GetTickCount64();

    for (auto& session : sessions)
    {
        if (!session->IsTimeOut(now))
        {
            // 끊김 감지: 세션 강제 종료 처리
            session->Disconnect(L"TimeOut");

            // 로그 남기기
            std::cout << "Session disconnected due to timeout. PlayerId: " << session->_currentPlayer->playerId << std::endl;
        }
    }
}
