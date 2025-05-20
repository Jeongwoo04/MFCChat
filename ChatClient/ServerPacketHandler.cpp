#include "pch.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h"
#include "Convert.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* haeder = reinterpret_cast<PacketHeader*>(buffer);
	return true;
}

// 완료
bool Handle_S_LOGIN_FAIL(PacketSessionRef& session, Protocol::S_LOGIN_FAIL& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	serverSession->Disconnect(L"Login Fail");
	serverSession->_dig->_isConnected = false;

	string message;
	switch (pkt.cause()) {
	case Protocol::Cause::NONE:
		message += "";
		break;
	case Protocol::Cause::INVAILD_NAME:
		message += "[INVALID_NAME] : ";
		break;
	case Protocol::Cause::DB_ERROR:
		message += "[DB_ERROR] : ";
		break;
	case Protocol::Cause::ALREADY_LOGGED_IN:
		message += "[ALREADY_LOGGED_IN] : ";
		break;
	default:
		break;
	}

	message += pkt.message();
	wstring wMessage = Convert::UTF8ToWStringDynamic(message);

	serverSession->_dig->AddEventString(wMessage.c_str());

	return true;
}

bool Handle_S_ENTER(PacketSessionRef& session, Protocol::S_ENTER& pkt)
{
	// TODO : 입장 UI -> 게임 입장
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);
	if (serverSession->GetName() == "")
	{
		serverSession->SetName(pkt.name());
		for (const auto& protoInfo : pkt.players())
		{
			OtherPlayerInfo newInfo;
			newInfo.playerId = protoInfo.player_id();
			newInfo.name = protoInfo.name();
			serverSession->_otherPlayers[newInfo.playerId] = newInfo;
		}
	}
	else
	{
		OtherPlayerInfo newInfo;
		newInfo.playerId = pkt.player_id();
		newInfo.name = pkt.name();
		serverSession->_otherPlayers[newInfo.playerId] = newInfo;
	}

	return true;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	string message = pkt.message();
	wstring wMessage = Convert::UTF8ToWStringDynamic(message);

	serverSession->_dig->AddEventString(wMessage.c_str());

	return true;
}

bool Handle_S_LEAVE(PacketSessionRef& session, Protocol::S_LEAVE& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);
	unordered_map<uint64, OtherPlayerInfo> info = serverSession->_otherPlayers;
	
	auto it = info.find(pkt.player_id());
	if (it == info.end())
	{
		serverSession->_dig->_isConnected = false;
		serverSession->Disconnect(L"Leave");
		serverSession->_otherPlayers.clear();
		serverSession->SetName("");

		string message = u8"채팅방을 나갔습니다.";
		wstring wMessage = Convert::UTF8ToWStringDynamic(message);

		serverSession->_dig->AddEventString(wMessage.c_str());
	}
	else
	{
		auto it = info.find(pkt.player_id());
		if (it != info.end())
		{
			string message = u8"[" + it->second.name + u8"] 님이 채팅방을 나갔습니다.";
			wstring wMessage = Convert::UTF8ToWStringDynamic(message);

			serverSession->_dig->AddEventString(wMessage.c_str());
			info.erase(it);
		}
	}
	

	return true;
}

bool Handle_S_PONG(PacketSessionRef& session, Protocol::S_PONG& pkt)
{
	return true;
}
