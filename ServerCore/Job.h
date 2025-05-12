#pragma once
#include <functional>
#include "DBConnection.h"
#include "../GameServer/GenProcedures.h"

/*---------
	Job
----------*/

using CallbackType = std::function<void()>;

class Job
{
public:
	Job(CallbackType&& callback) : _callback(std::move(callback))
	{
	}

	template<typename T, typename Ret, typename... Args>
	Job(shared_ptr<T> owner, Ret(T::* memFunc)(Args...), Args&&... args)
	{
		_callback = [owner, memFunc, args...]()
			{
				(owner.get()->*memFunc)(args...);
			};
	}

	virtual ~Job() { }

	virtual void Execute()
	{
		_callback();
	}

private:
	CallbackType _callback;
};

//class DBJob : public Job
//{
//public:
//	virtual ~DBJob() override { };
//	virtual void Execute(DBConnection* dbconn) = 0;
//};
//
//class LambdaDBJob : public DBJob
//{
//public:
//	using DBJobFunc = std::function<void(DBConnection*)>;
//
//	LambdaDBJob(DBJobFunc&& func) : _func(std::move(func)) { }
//
//	virtual void Execute(DBConnection* dbconn) override
//	{
//		_func(dbconn);
//	}
//
//private:
//	DBJobFunc _func;
//};