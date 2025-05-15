#include "pch.h"
#include "ThreadManager.h"
#include "JobQueue.h"
#include "GlobalQueue.h"
#include "DBConnectionPool.h"

/*------------------
	ThreadManager
-------------------*/


ThreadManager::ThreadManager()
{
	// Main Thread
	InitTLS();
}

ThreadManager::~ThreadManager()
{
	Join();
}

void ThreadManager::Launch(function<void(void)> callback)
{
	LockGuard guard(_lock);

	_threads.push_back(thread([=]()
		{
			InitTLS();
			callback();
			DestroyTLS();
		}));
}

void ThreadManager::Join()
{
	for (thread& t : _threads)
	{
		if (t.joinable())
			t.join();
	}
	_threads.clear();
}

void ThreadManager::InitTLS()
{
	static Atomic<uint32> SThreadId = 1;
	LThreadId = SThreadId.fetch_add(1);
}

void ThreadManager::DestroyTLS()
{

}

void ThreadManager::DoGlobalQueueWork()
{
	while (true)
	{
		uint64 now = ::GetTickCount64();
		if (now > LEndTickCount)
			break;

		JobQueueRef jobQueue = GGlobalQueue->Pop();
		if (jobQueue == nullptr)
			break;

		jobQueue->Execute();
	}
}

void ThreadManager::DoGDBJobQueueWork()
{
	DBConnection* dbConn = GDBConnectionPool->Pop();

	const int MAX_RETRY = 2;
	for (int i = 0; i < MAX_RETRY; ++i)
	{
		dbConn = GDBConnectionPool->Pop();
		if (dbConn)
			break;
		Sleep(1);
	}
	if (dbConn == nullptr)
		return;

	while (true)
	{
		uint64 now = ::GetTickCount64();
		if (now > LEndTickCount)
			break;

		JobQueueRef jobQueue = GDBJobQueue->Pop();
		if (jobQueue == nullptr)
			break;

		jobQueue->Execute(dbConn);
	}

	GDBConnectionPool->Push(dbConn);
}

void ThreadManager::DistributeReservedJobs()
{
	const uint64 now = ::GetTickCount64();

	GJobTimer->Distribute(now);
}

void ThreadManager::DistributeReservedDBJobs()
{
	const uint64 now = ::GetTickCount64();

	GDBJobTimer->Distribute(now);
}