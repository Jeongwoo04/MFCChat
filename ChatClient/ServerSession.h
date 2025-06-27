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

class ServerSession : public PacketSession
{
public:
	ServerSession(CChatClientDlg* dig)
		: _dig(dig), _sessionId(GServerSessionIdGenerator.fetch_add(1))
	{
		GServerSessionManager->GetLastMessageId();
	}
	~ServerSession()
	{
		cout << "~ServerSession" << endl;
	}

	virtual void	OnConnected() override;
	virtual void	OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void	OnSend(int32 len) override;
	virtual void	OnDisconnected() override;
	
	string GetUserNameFromUI() const;
	void SendLoginPacket();
	void SendReconnectPacket();

public:
	void SetName(const string& name) { _name = name; }
	const string& GetName() const { return _name; }
	void SetPlayerId(const uint64& playerId) { _playerId = playerId; }
	const uint64& GetPlayerId() const { return _playerId; }
	uint64 GetSessionId() const { return _sessionId; }

public:
	CChatClientDlg* _dig;

public:
	unordered_map<uint64, Protocol::PlayerInfo> _otherPlayers;
	uint64 _sessionId;

private:
	string _name = "";
	uint64 _playerId;
};

