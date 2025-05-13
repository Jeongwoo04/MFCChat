#pragma once
#include <functional>
#include "DBConnection.h"
#include "../GameServer/GenProcedures.h"

class DBJob
{
public:
	using CallbackType = std::function<void(DBConnection*)>;

	DBJob(CallbackType&& callback) : _callback(std::move(callback)) {}

	virtual ~DBJob() {}

	virtual void Execute(DBConnection* dbConn)
	{
		_callback(dbConn);
	}

private:
	CallbackType _callback;
};