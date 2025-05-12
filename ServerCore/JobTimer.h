#pragma once

struct JobData
{
	JobData(weak_ptr<JobQueue> owner, JobRef job) : owner(owner), job(job)
	{

	}

	weak_ptr<JobQueue>	owner;
	JobRef				job;
};

struct TimerItem // 우선순위 큐에 들어갈 Item
{
	bool operator<(const TimerItem& other) const
	{
		return executeTick > other.executeTick;
	}

	uint64 executeTick = 0;
	JobData* jobData = nullptr; // 내부에서만 사용하고 해제. Item이 PriorityQueue에 들어가있어도
	// 위치가 변화할때마다 복사를 할경우 RefCount가 증감에 영향을 줄 수 있어서 그냥 포인터로
};

/*--------------
	JobTimer
---------------*/

class JobTimer // 전역으로 만들어줄 친구
{
public:
	void			Reserve(uint64 tickAfter, weak_ptr<JobQueue> owner, JobRef job);
	void			Distribute(uint64 now);
	void			Clear();

private:
	USE_LOCK;
	PriorityQueue<TimerItem>	_items;
	Atomic<bool>				_distributing = false; // 실행하고 있는지. 다시 배치를 하고있는지
	// 한번에 한명만 일감 배분을 맡도록
};

