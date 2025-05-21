#pragma once

#include "GameSessionManager.h"
//#include "DBConnectionPool.h"
#include "Room.h"
#include "Convert.h"

extern GameSessionManager*	GSessionManager;
//extern DBConnectionPool*	GDBConnectionPool;
extern shared_ptr<Room>		GRoom;
inline atomic<uint64>		GSessionIdGenerator = 1;

class ServerGlobal
{
public:
	static void Init();
};