#include "pch.h"
#include "ServerSessionManager.h"
#include "ServerSession.h"

ServerSessionManager* GServerSessionManager = nullptr;
atomic<uint64_t> GServerSessionIdGenerator = 1;

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

ServerSessionRef ServerSessionManager::Find(uint64 sessionId)
{
	WRITE_LOCK;
	ServerSessionRef session = _sessions[sessionId];
	return session;
}