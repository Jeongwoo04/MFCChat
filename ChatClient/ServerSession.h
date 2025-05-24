#pragma once

#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Protocol.pb.h"
#include "ServerPacketHandler.h"
#include "ChatClientDlg.h"
#include "resource.h"
#include "ServerSessionManager.h"

class CChatClientDlg;

struct OtherPlayerInfo
{
	uint64 playerId;
	string name;
};

class ServerSession : public PacketSession
{
public:
	ServerSession(CChatClientDlg* dig) : _dig(dig), _sessionId(GServerSessionIdGenerator.fetch_add(1))
	{
		setChatSerial(GServerSessionManager->GetChatSerial());
	}
	~ServerSession()
	{
		GServerSessionManager->SetChatSerial(_serial);
		cout << "~ServerSession" << endl;
	}
	virtual void	OnConnected() override;
	virtual void	OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void	OnSend(int32 len) override;
	virtual void	OnDisconnected() override;
	
public:
	void SetName(const string& name) { _name = name; }
	const string& GetName() const { return _name; }
	uint64 GetSessionId() const { return _sessionId; }
	void setChatSerial(int64 serial) { _serial = serial; }

public:
	CChatClientDlg* _dig;

public:
	unordered_map<uint64, OtherPlayerInfo> _otherPlayers;
	int64 _serial = 0;
	uint64 _sessionId;

private:
	string _name = "";
};

