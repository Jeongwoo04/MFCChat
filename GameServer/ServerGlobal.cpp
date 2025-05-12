#include "pch.h"
#include "ServerGlobal.h"

GameSessionManager*		GSessionManager = nullptr;
DBConnectionPool*		GDBConnectionPool = nullptr;
shared_ptr<Room>		GRoom = nullptr;

void ServerGlobal::Init()
{
	GSessionManager		= new GameSessionManager();
	GDBConnectionPool	= new DBConnectionPool();
	GRoom				= MakeShared<Room>();
}