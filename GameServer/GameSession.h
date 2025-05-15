#pragma once

#include "Session.h"

class GameSession : public PacketSession // sealed로 인해 OnRecv 사용불가
{
public:
	GameSession() { }
	~GameSession()
	{
		//cout << "~GameSession" << endl;
	}

	virtual void	OnConnected() override;
	virtual void	OnDisconnected() override;
	virtual void	OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void	OnSend(int32 len) override;

public:
	PlayerRef				_currentPlayer; // 현재 어떤 Player로 접속을 하고 있는지
	weak_ptr<class Room>	_room; // room은 현재 없을수도 있으니 weak_ptr로 (자원을 할당 받아도 참조 카운트 영향X )
	//ENTER_GAME에서 currentPlayer로 사용 및 현재 room도 사용
	//이런 포인터를 들고있는게 별로면 id를 가지고 빠르게 dictionary / hash-table에서 가져와도 됨.

	enum
	{
		TIMEOUT_SECONDS = 15000,
	};

public:
	bool IsTimeOut(uint64_t now)
	{
		return (::GetTickCount64() - _lastPingTime.load() >= TIMEOUT_SECONDS);
	}

public:
	bool IsDisconnected() const { return _disconnected.load(); }
	atomic<uint64_t> _lastPingTime = 0;

private:
	atomic<bool> _disconnected = false;
};