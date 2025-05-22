#pragma once
#include "Job.h"
#include "DBJob.h"
#include "LockQueue.h"
#include "JobTimer.h"

/*--------------
	JobQueue
---------------*/

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
	void DoAsync(CallbackType&& callback)
	{
		Push(ObjectPool<Job>::MakeShared(std::move(callback)));
	}

	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		Push(ObjectPool<Job>::MakeShared(owner, memFunc, std::forward<Args>(args)...));
	}

	template<typename T, typename Ret, typename... Args>
	void DoDBAsync(Ret(T::* memFunc)(DBConnection*, Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());

		using TupleType = std::tuple<std::decay_t<Args>...>;
		TupleType tup = std::make_tuple(std::move(args)...);

		auto callback = [owner, memFunc, tup = std::move(tup)](DBConnection* dbConn) mutable
			{
				std::apply([&](auto&&... unpackedArgs) {
					(owner.get()->*memFunc)(dbConn, std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
					}, tup);
			};

		Push(ObjectPool<DBJob>::MakeShared(std::move(callback)));
	}

	template<typename T, typename Callback>
	void DoTimer(JobTimer<T>* timer, uint64 tickAfter, Callback&& callback)
	{
		shared_ptr<T> job = ObjectPool<T>::MakeShared(std::forward<Callback>(callback));
		timer->Reserve(tickAfter, shared_from_this(), job);
	}

	template<typename TimerJobT, typename OwnerT, typename Ret, typename... Args>
	void DoTimer(JobTimer<TimerJobT>* timer, uint64 tickAfter, Ret(OwnerT::* memFunc)(Args...), Args&&... args)
	{
		shared_ptr<OwnerT> owner = static_pointer_cast<OwnerT>(shared_from_this());
		shared_ptr<TimerJobT> job = ObjectPool<TimerJobT>::MakeShared(owner, memFunc, std::forward<Args>(args)...);
		timer->Reserve(tickAfter, shared_from_this(), job);
	}

	void					ClearJobs() { _jobs.Clear(); }

public:
	void					Push(JobRef job, bool pushOnly = false);
	void					Push(DBJobRef dbJob, bool pushOnly = false);
	void					Execute();
	void					Execute(DBConnection* dbConn);

protected:
	LockQueue<JobRef>		_jobs;
	LockQueue<DBJobRef>		_dbJobs;
	Atomic<int32>			_jobCount = 0;
	Atomic<int32>			_dbJobCount = 0;
};

