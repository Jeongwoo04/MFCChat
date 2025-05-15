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

bool Handle_S_LOGIN_FAIL(PacketSessionRef& session, Protocol::S_LOGIN_FAIL& pkt)
{
	switch (pkt.reason()) {
	case Protocol::Reason::NONE:

		break;
	case Protocol::Reason::INVAILD_NAME:

		break;
	case Protocol::Reason::INVAILD_USER_ID:

		break;
	case Protocol::Reason::SERVER_ERROR:

		break;
	default:

		break;
	}


	return true;
}

bool Handle_S_ENTER(PacketSessionRef& session, Protocol::S_ENTER& pkt)
{
	// TODO : 입장 UI -> 게임 입장
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);
	serverSession->SetName(pkt.name());

	serverSession->_otherPlayers.clear(); // 기존 목록 제거 (중복 방지)

	for (const auto& protoInfo : pkt.players())
	{
		OtherPlayerInfo info;
		info.playerId = protoInfo.player_id();
		info.name = protoInfo.name();
		serverSession->_otherPlayers[info.playerId] = info;
	}

	//WCHAR convertMsg[100] = { 0, };
	//int nLen = MultiByteToWideChar(CP_UTF8, 0, pkt.msg().c_str(), pkt.msg().size() + 1, NULL, NULL);
	//MultiByteToWideChar(CP_UTF8, 0, pkt.msg().c_str(), pkt.msg().size() + 1, convertMsg, nLen);

	//serverSession->_dig->AddEventString(convertMsg);

	return true;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	unordered_map<uint64, OtherPlayerInfo> info = serverSession->_otherPlayers;
	auto it = info.find(pkt.player_id());
	if (it == info.end())
	{
		OtherPlayerInfo newInfo;
		newInfo.playerId = pkt.player_id();
		newInfo.name = pkt.name();
		info[pkt.player_id()] = newInfo;

		string message = "[" + pkt.name() + "] 님이 채팅방에 입장했습니다.";
		wstring wMessage = Convert::UTF8ToWString(message);

		serverSession->_dig->AddEventString(wMessage.c_str());
	}
	else
	{
		std::string message = "[" + pkt.name() + "] " + pkt.message();

		// UTF-8 → UTF-16 변환
		int len = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, nullptr, 0);
		std::wstring wmessage(len, 0);
		MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, &wmessage[0], len);

		// wmessage는 널 포함, 널 종료문자도 길이에 포함
		// 필요시 널 제거 (wstring 내부에 널이 있으면 길이가 꼬일 수 있음)
		if (!wmessage.empty() && wmessage.back() == L'\0')
			wmessage.pop_back();

		serverSession->_dig->AddEventString(wmessage.c_str());

		//WCHAR convertName[50] = { 0, };
		//int nLen = MultiByteToWideChar(CP_UTF8, 0, pkt.name().c_str(), pkt.name().size() + 1, NULL, NULL);
		//MultiByteToWideChar(CP_UTF8, 0, pkt.name().c_str(), pkt.name().size() + 1, convertName, nLen);

		//WCHAR convertMsg[100] = { 0, };
		//nLen = MultiByteToWideChar(CP_UTF8, 0, pkt.message().c_str(), pkt.message().size() + 1, NULL, NULL);
		//MultiByteToWideChar(CP_UTF8, 0, pkt.message().c_str(), pkt.message().size() + 1, convertMsg, nLen);

		//wstring ws_msg = L"[" + static_cast<wstring>(convertName) + L"] : " + convertMsg;
		//ws_msg += L'\0';
		//wstring convertStr = wstring(ws_msg.begin(), ws_msg.end());
		//serverSession->_dig->AddEventString(convertStr.c_str());
	}


	return true;
}

bool Handle_S_LEAVE(PacketSessionRef& session, Protocol::S_LEAVE& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);
	
	unordered_map<uint64, OtherPlayerInfo> info = serverSession->_otherPlayers;
	auto it = info.find(pkt.player_id());
	if (it != info.end())
	{
		string message = u8"[" + it->second.name + u8"님이 채팅방을 나갔습니다.";
		wstring wMessage(message.begin(), message.end());

		serverSession->_dig->AddEventString(wMessage.c_str());
	}
	return true;
}

bool Handle_S_PONG(PacketSessionRef& session, Protocol::S_PONG& pkt)
{
	return true;
}
