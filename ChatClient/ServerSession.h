#pragma once

#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Protocol.pb.h"
#include "ServerPacketHandler.h"
#include "ChatClientDlg.h"
#include "resource.h"

using ServerSessionRef = shared_ptr<class ServerSession>;
class CChatClientDlg;

struct OtherPlayerInfo
{
	uint64 playerId;
	std::string name;
};

class ServerSession : public PacketSession
{
public:
	ServerSession(CChatClientDlg* dig) : _dig(dig) { }
	~ServerSession()
	{
		cout << "~ServerSession" << endl;
	}
	virtual void OnConnected() override
	{
		_dig->_isConnected = true;
		_dig->_serverSession = static_pointer_cast<ServerSession>(shared_from_this());

		// connection
		Protocol::C_LOGIN loginPkt;

		CString str;
		_dig->chatName.GetWindowTextW(str);
		
		//{
		//	wstring wstr(str);

		//	int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		//	string utf8Str(size - 1, 0);
		//	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8Str[0], size, nullptr, nullptr);
		//	loginPkt.set_name(utf8Str);
		//}
		loginPkt.set_name(CW2A(str, CP_UTF8));
		
		// TODO : webserver 외부 인증
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(loginPkt);
		Send(sendBuffer);
	}

	virtual void OnRecvPacket(BYTE* buffer, int32 len) override
	{
		PacketSessionRef session = GetPacketSessionRef();
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		ServerPacketHandler::HandlePacket(session, buffer, len);
	}

	virtual void	OnSend(int32 len) override
	{
		//cout << "OnSend Len = " << len << endl;
	}

	virtual void	OnDisconnected() override
	{
		//cout << "OnDisconnected" << endl;
	}

public:
	void SetName(const string& name) { _name = name; }
	const string& GetName() const { return _name; }
	void SetLastPongTime(uint64 lastRecvTick) { _lastRecvTick = lastRecvTick; }

public:
	CChatClientDlg* _dig;

public:
	unordered_map<uint64, OtherPlayerInfo> _otherPlayers;
	uint64_t _lastRecvTick;
	

private:
	string _name = "";
};

