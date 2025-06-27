#include "pch.h"
#include "ServerSessionManager.h"
#include "ServerSession.h"

ServerSessionManager* GServerSessionManager = nullptr;
atomic<int64> GServerSessionIdGenerator = 1;

void	ServerSessionManager::Add(ServerSessionRef session)
{
	WRITE_LOCK;
	_sessions.insert(pair<uint64, ServerSessionRef>(session->GetSessionId(), session));
}
void	ServerSessionManager::Remove(ServerSessionRef session)
{
	WRITE_LOCK;
	_sessions.erase(session->GetSessionId());
}

ServerSessionRef ServerSessionManager::Find(int64 sessionId)
{
	READ_LOCK;
	auto it = _sessions.find(sessionId);
	if (it != _sessions.end())
		return it->second;

	return nullptr;
}