#include "pch.h"
#include "JobQueue.h"
#include "GlobalQueue.h"
#include "DBConnectionPool.h"

/*--------------
	JobQueue
---------------*/

void JobQueue::Push(JobRef job, bool pushOnly)
{
	const int32 prevCount = _jobCount.fetch_add(1);
	_jobs.Push(job); // WRITE_LOCK

	// 첫번째 Job을 넣은 쓰레드가 실행까지 담당
	if (prevCount == 0)
	{
		// 이미 실행중인 JobQueue가 없으면 실행
		if (LCurrentJobQueue == nullptr && pushOnly == false)
		{
			Execute();
		}
		else
		{
			// 여유 있는 다른 쓰레드가 실행하도록 GlobalQueue에 넘긴다
			GGlobalQueue->Push(shared_from_this());
		}
	}
}

// 1) 일감이 너~무 몰리면?
void JobQueue::Execute()
{
	LCurrentJobQueue = this;

	while (true)
	{
		Vector<JobRef> jobs;
		_jobs.PopAll(OUT jobs);

		const int32 jobCount = static_cast<int32>(jobs.size());
		for (int32 i = 0; i < jobCount; i++)
			jobs[i]->Execute();

		// 남은 일감이 0개라면 종료
		if (_jobCount.fetch_sub(jobCount) == jobCount)
		{
			LCurrentJobQueue = nullptr;
			return;
		}

		const uint64 now = ::GetTickCount64();
		if (now >= LEndTickCount)
		{
			LCurrentJobQueue = nullptr;
			// 여유 있는 다른 쓰레드가 실행하도록 GlobalQueue에 넘긴다
			GGlobalQueue->Push(shared_from_this());
			break;
		}			
	}
}

void JobQueue::Push(DBJobRef dbJob, bool pushOnly)
{
	const int32 prevCount = _dbJobCount.fetch_add(1);
	_dbJobs.Push(dbJob); // WRITE_LOCK

	// 첫번째 Job을 넣은 쓰레드가 실행까지 담당
	if (prevCount == 0 && LCurrentDBJobQueue == nullptr)
	{
		// 이미 실행중인 JobQueue가 없으면 실행
		//if (LCurrentJobQueue == nullptr && pushOnly == false)
		//{
		//	GDBJobQueue->Push(shared_from_this());
		//}
		//else
		//{
			// 여유 있는 다른 쓰레드가 실행하도록 GlobalQueue에 넘긴다
		GDBJobQueue->Push(shared_from_this());
		//}
	}
}

void JobQueue::Execute(DBConnection* dbConn)
{
	if (LCurrentDBJobQueue != nullptr)
		return;

	LCurrentDBJobQueue = this;

	while (true)
	{
		Vector<DBJobRef> jobs;
		_dbJobs.PopAll(OUT jobs);

		const int32 jobCount = static_cast<int32>(jobs.size());
		for (int32 i = 0; i < jobCount; i++)
			jobs[i]->Execute(dbConn); // DBConnection 전달

		if (_dbJobCount.fetch_sub(jobCount) == jobCount)
		{
			LCurrentDBJobQueue = nullptr;
			return;
		}

		const uint64 now = ::GetTickCount64();
		if (now >= LEndTickCount)
		{
			LCurrentDBJobQueue = nullptr;
			// 여유 있는 다른 쓰레드가 실행하도록 GlobalQueue에 넘긴다
			GDBJobQueue->Push(shared_from_this());
			break;
		}
	}
}