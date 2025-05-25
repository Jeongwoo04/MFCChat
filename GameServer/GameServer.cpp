#include "pch.h"
#include "ServerGlobal.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "BufferWriter.h"
#include "ClientPacketHandler.h"
#include "Protocol.pb.h"
#include "Room.h"
#include "DBConnectionPool.h"
#include "DBBind.h"
#include "XmlParser.h"
#include "DBSynchronizer.h"
#include "GenProcedures.h"
#include "GlobalQueue.h"

#include <fcntl.h>
#include <io.h>
#include <windows.h>

enum
{
	WORKER_TICK = 64,
	ROOM_UPDATE_TICK = 50
};

void DoWorkerJob(ServerServiceRef& service)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// 네트워크 입출력 처리 -> 인게임 로직까지 (패킷 핸들러에 의해)
		service->GetIocpCore()->Dispatch(10);

		// 예약된 일감 처리
		ThreadManager::DistributeReservedJobs();

		// 글로벌 큐
		ThreadManager::DoGlobalQueueWork();
	}
}

void DoDBWorkerJob()
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		ThreadManager::DistributeReservedDBJobs();

		ThreadManager::DoGDBJobQueueWork();
	}
}

void DoRoomUpdateJob()
{
	while (true)
	{
		this_thread::sleep_for(chrono::milliseconds(ROOM_UPDATE_TICK));

		GRoom->DoAsync(&Room::Update);
	}
}

void InitConsole()
{
	_setmode(_fileno(stdout), _O_U16TEXT);
	SetConsoleOutputCP(CP_UTF8);
}

int main()
{
	ServerGlobal::Init();
	InitConsole();


	ASSERT_CRASH(GDBConnectionPool->Connect(4, L"Driver={SQL Server Native Client 11.0};Server=(localdb)\\MSSQLLocalDB;Database=ServerDb;Trusted_Connection=Yes;charset='UTF8'"));
	DBConnection* dbConn = GDBConnectionPool->Pop();
	DBSynchronizer dbSync(*dbConn);
	dbSync.Synchronize(L"GameDB.xml");
	GDBConnectionPool->Push(dbConn);

	ClientPacketHandler::Init();

	ServerServiceRef service = MakeShared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		MakeShared<IocpCore>(),
		MakeShared<GameSession>, // TODO : SessionManager 등
		10);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([&service]()
			{
				DoWorkerJob(service);
			});
	}
	for (int32 i = 0; i < 4; i++)
	{
		GThreadManager->Launch([]()
			{
				DoDBWorkerJob();
			});
	}

	GThreadManager->Launch([]()
		{
			DoRoomUpdateJob();
		});

	GThreadManager->Join();
}