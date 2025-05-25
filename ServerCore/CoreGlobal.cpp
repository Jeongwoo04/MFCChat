#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "Memory.h"
#include "DeadLockProfiler.h"
#include "SocketUtils.h"
#include "SendBuffer.h"
#include "GlobalQueue.h"
#include "Job.h"
#include "DBJob.h"
#include "JobTimer.h"
#include "ConsoleLog.h"
#include "DBConnectionPool.h"

template class JobTimer<Job>;
template class JobTimer<DBJob>;

ThreadManager*		GThreadManager = nullptr;
Memory*				GMemory = nullptr;
SendBufferManager*	GSendBufferManager = nullptr;
GlobalQueue*		GGlobalQueue = nullptr;
GlobalQueue*		GDBJobQueue = nullptr;
JobTimer<Job>*		GJobTimer = nullptr;
JobTimer<DBJob>*	GDBJobTimer = nullptr;

DBConnectionPool*	GDBConnectionPool = nullptr;
DeadLockProfiler*	GDeadLockProfiler = nullptr;
ConsoleLog*			GConsoleLogger = nullptr;

class CoreGlobal
{
public:
	CoreGlobal()
	{
		GThreadManager = new ThreadManager();
		GMemory = new Memory();
		GSendBufferManager = new SendBufferManager();
		GGlobalQueue = new GlobalQueue();
		GDBJobQueue = new GlobalQueue();
		GJobTimer = new JobTimer<Job>();
		GDBJobTimer = new JobTimer<DBJob>();
		GDBConnectionPool = new DBConnectionPool();
		GConsoleLogger = new ConsoleLog();
		GDeadLockProfiler = new DeadLockProfiler();
		SocketUtils::Init();
	}

	~CoreGlobal()
	{
		delete GThreadManager;
		delete GMemory;
		delete GSendBufferManager;
		delete GGlobalQueue;
		delete GDBJobQueue;
		delete GJobTimer;
		delete GDBJobTimer;
		delete GDBConnectionPool;
		delete GConsoleLogger;
		delete GDeadLockProfiler;

		SocketUtils::Clear();
	}

} GCoreGlobal;