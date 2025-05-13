#pragma once

template<typename T> class JobTimer;

extern class ThreadManager*		GThreadManager;
extern class Memory*			GMemory;
extern class SendBufferManager* GSendBufferManager;
extern class GlobalQueue*		GGlobalQueue;
extern class GlobalQueue*		GDBJobQueue;
extern JobTimer<Job>*		GJobTimer;
extern JobTimer<DBJob>*	GDBJobTimer;

extern class DBConnectionPool*	GDBConnectionPool;
extern class DeadLockProfiler*	GDeadLockProfiler;
extern class ConsoleLog*		GConsoleLogger;