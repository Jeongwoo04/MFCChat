#pragma once

#include "GameSessionManager.h"
#include "DBConnectionPool.h"
#include "Room.h"

extern GameSessionManager*	GSessionManager;
extern DBConnectionPool*	GDBConnectionPool;
extern shared_ptr<Room>		GRoom;

class ServerGlobal
{
public:
	static void Init();
};