#pragma once

#include "JobQueue.h"
#include "Job.h"

template<typename T>
struct JobData
{
	JobData(std::weak_ptr<JobQueue> owner, std::shared_ptr<T> job) : owner(owner), job(job) { }

	weak_ptr<JobQueue> owner;
	shared_ptr<T> job;
};

template<typename T>
struct TimerItem
{
	bool operator<(const TimerItem& other) const
	{
		// 우선순위 큐에서 더 작은 tick이 먼저 나오게 하기 위해 반대 비교
		return executeTick > other.executeTick;
	}

	uint64 executeTick = 0;
	JobData<T>* jobData = nullptr;
};

//struct JobData
//{
//	JobData(weak_ptr<JobQueue> owner, JobRef job) : owner(owner), job(job)
//	{
//
//	}
//
//	weak_ptr<JobQueue>	owner;
//	JobRef				job;
//};
//
//struct TimerItem // 우선순위 큐에 들어갈 Item
//{
//	bool operator<(const TimerItem& other) const
//	{
//		return executeTick > other.executeTick;
//	}
//
//	uint64 executeTick = 0;
//	JobData* jobData = nullptr; // 내부에서만 사용하고 해제. Item이 PriorityQueue에 들어가있어도
//	// 위치가 변화할때마다 복사를 할경우 RefCount가 증감에 영향을 줄 수 있어서 그냥 포인터로
//};
//
//struct DBJobData
//{
//	DBJobData(weak_ptr<JobQueue> owner, DBJobRef job) : owner(owner), job(job) { }
//
//	weak_ptr<JobQueue> owner;
//	DBJobRef job;
//};
//
//struct DBTimerItem
//{
//	bool operator<(const DBTimerItem& other) const
//	{
//		return executeTick > other.executeTick;
//	}
//
//	uint64 executeTick = 0;
//	DBJobData* jobData = nullptr;
//};

/*--------------
	JobTimer
---------------*/

template<typename T>
class JobTimer
{
public:
	void Reserve(uint64 tickAfter, std::weak_ptr<JobQueue> owner, std::shared_ptr<T> job)
	{
		const uint64 executeTick = ::GetTickCount64() + tickAfter;
		JobData<T>* jobData = ObjectPool<JobData<T>>::Pop(owner, job);

		WRITE_LOCK;
		_items.push(TimerItem<T>{ executeTick, jobData });
	}

	void Distribute(uint64 now)
	{
		if (_distributing.exchange(true) == true)
			return;

		Vector<TimerItem<T>> items;

		{
			WRITE_LOCK;

			while (_items.empty() == false)
			{
				const TimerItem<T>& timerItem = _items.top();
				if (now < timerItem.executeTick)
					break;

				items.push_back(timerItem);
				_items.pop();
			}
		}

		for (TimerItem<T>& item : items)
		{
			if (JobQueueRef owner = item.jobData->owner.lock())
				owner->Push(item.jobData->job);

			ObjectPool<JobData<T>>::Push(item.jobData);
		}

		_distributing.store(false);
	}

	void Clear()
	{
		WRITE_LOCK;
		while (_items.empty() == false)
		{
			const TimerItem<T>& timerItem = _items.top();
			ObjectPool<JobData<T>>::Push(timerItem.jobData);
			_items.pop();
		}
	}

private:
	USE_LOCK;
	PriorityQueue<TimerItem<T>> _items;
	Atomic<bool> _distributing = false;
};


//class JobTimer // 전역으로 만들어줄 친구
//{
//public:
//  void			Reserve(uint64 tickAfter, weak_ptr<JobQueue> owner, JobRef job);
//	void			Distribute(uint64 now);
//	void			Clear();
//
//private:
//	USE_LOCK;
//	PriorityQueue<TimerItem>	_items;
//	Atomic<bool>				_distributing = false; // 실행하고 있는지. 다시 배치를 하고있는지
//	// 한번에 한명만 일감 배분을 맡도록
//};
//
//class DBJobTimer
//{
//public:
//	void			Reserve(uint64 tickAfter, weak_ptr<JobQueue> owner, DBJobRef job);
//	void			Distribute(uint64 now);
//	void			Clear();
//
//private:
//	USE_LOCK;
//	PriorityQueue<DBTimerItem> _items;
//	Atomic<bool> _distributing = false;
//};
