#include "pch.h"
#include "GameSessionManager.h"
#include "GameSession.h"
#include "Player.h"

//GameSessionManager GSessionManager;

void GameSessionManager::Add(GameSessionRef session)
{
	WRITE_LOCK;
	_sessions.insert(pair<uint64, GameSessionRef>(session->GetSessionId(), session));
}

void GameSessionManager::Remove(GameSessionRef session)
{
	WRITE_LOCK;
	_sessions.erase(session->GetSessionId());
}

GameSessionRef GameSessionManager::Find(uint64 sessionId)
{
	WRITE_LOCK;
	auto it = _sessions.find(sessionId);
	return it->second;
}

// 채팅 프로그램에서 전체 메시지
void GameSessionManager::Broadcast(SendBufferRef sendBuffer) // for 돌면서 동일한 데이터를 보내주겠다. (복사비용 1번)
{
	WRITE_LOCK;
	for (auto session : _sessions)
	{
		session.second->Send(sendBuffer); // -> loop 탈때 _sessions를 건드리는지 조심 !
	}
}

void GameSessionManager::Kick(uint64 sessionId)
{
	WRITE_LOCK;
	auto it = _sessions.find(sessionId);
	if (it != _sessions.end())
	{
		it->second->Disconnect(L"Client Dead");
		_sessions.erase(it);
	}
}

//void GameSessionManager::CheckClientAlive(const Set<GameSessionRef>& sessions)
//{
//    const uint64_t timeoutMs = 15000; // 15초 이상 응답 없으면 끊김 처리
//    uint64_t now = GetTickCount64();
//
//    for (auto& session : sessions)
//    {
//        if (!session->IsTimeOut(now))
//        {
//            // 끊김 감지: 세션 강제 종료 처리
//            session->Disconnect(L"TimeOut");
//
//            // 로그 남기기
//            std::cout << "Session disconnected due to timeout. PlayerId: " << session->_currentPlayer->playerId << std::endl;
//        }
//    }
//}
