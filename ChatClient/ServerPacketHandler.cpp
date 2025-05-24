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

	serverSession->SetName(pkt.name());
	for (const auto& protoInfo : pkt.players())
	{
		OtherPlayerInfo newInfo;
		newInfo.playerId = protoInfo.player_id();
		newInfo.name = protoInfo.name();
		serverSession->_otherPlayers[newInfo.playerId] = newInfo;
	}

	return true;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	if (pkt.serial() > serverSession->_serial)
		serverSession->setChatSerial(pkt.serial());
	string message = pkt.message();
	wstring wMessage = Convert::UTF8ToWStringDynamic(message);

	serverSession->_dig->AddEventString(wMessage.c_str());

	return true;
}

bool Handle_S_LEAVE(PacketSessionRef& session, Protocol::S_LEAVE& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	serverSession->Disconnect(L"Leave");

	return true;
}

bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);
	auto& players = serverSession->_otherPlayers;

	OtherPlayerInfo newInfo;
	newInfo.playerId = pkt.player_id();
	newInfo.name = pkt.name();
	players[newInfo.playerId] = newInfo;

	return true;
}

bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);
	auto& players = serverSession->_otherPlayers;

	players.erase(pkt.player_id());

	return true;
}

bool Handle_S_PING(PacketSessionRef& session, Protocol::S_PING& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	Protocol::C_PONG pongPkt;
	uint64 now = ::GetTickCount64();
	pongPkt.set_timestamp(now); // echo back

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pongPkt);
	serverSession->Send(sendBuffer);

	return true;
}
