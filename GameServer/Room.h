#pragma once
#include "JobQueue.h"

using PlayerRef = shared_ptr<class Player>;

class Room : public JobQueue
{
public:
	Room() { }
	Room(uint32 roomId) : _roomId(roomId) { }

	void Enter(GameSessionRef gameSession);
	void Leave(PlayerRef player);
	void Broadcast(SendBufferRef sendBuffer);
	void BroadcastOthers(PlayerRef owner, SendBufferRef sendBuffer);

	void DBSave(DBConnection* dbConn, std::wstring name, std::wstring msg);

	PlayerRef FindPlayer(uint64 playerId);

	uint32 GetRoomId() const { return _roomId; }

public:
	USE_LOCK;
	unordered_map<uint64, PlayerRef>	_players;
	uint32 _roomId;
	//Vector<GameSessionRef>	_sessions;
};