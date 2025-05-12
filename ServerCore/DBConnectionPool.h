#pragma once
#include "DBConnection.h"

/*---------------------
	DBConnectionPool
----------------------*/

class DBConnectionPool
{
public:
	DBConnectionPool();
	~DBConnectionPool();

	// connectionPool 만드는 함수
	bool					Connect(int32 connectionCount, const WCHAR* connectionString);
	void					Clear();

	DBConnection*			Pop();
	void					Push(DBConnection* connection);

private:
	USE_LOCK;
	SQLHENV					_environment = SQL_NULL_HANDLE; // SQL Handle ENV
	Vector<DBConnection*>	_connections;
};

