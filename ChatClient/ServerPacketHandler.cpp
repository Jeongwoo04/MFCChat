#include "pch.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h"
#include "Convert.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* haeder = reinterpret_cast<PacketHeader*>(buffer);
	return false;
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
	if (pkt.success() == false)
	{
		// TODO : 로그인 실패
		return true;
	}
	if (pkt.players().size() == 0)
	{
		// TODO : 캐릭터 생성
	}
	// TODO : 입장 UI -> 게임 입장
	Protocol::C_ENTER_CHAT enterChatPkt;
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(enterChatPkt);
	session->Send(sendBuffer);

	return true;
}

bool Handle_S_ENTER_CHAT(PacketSessionRef& session, Protocol::S_ENTER_CHAT& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	WCHAR convertMsg[100] = { 0, };
	int nLen = MultiByteToWideChar(CP_UTF8, 0, pkt.msg().c_str(), pkt.msg().size() + 1, NULL, NULL);
	MultiByteToWideChar(CP_UTF8, 0, pkt.msg().c_str(), pkt.msg().size() + 1, convertMsg, nLen);

	serverSession->_dig->AddEventString(convertMsg);


	return true;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	WCHAR convertName[50] = { 0, };
	int nLen = MultiByteToWideChar(CP_UTF8, 0, pkt.name().c_str(), pkt.name().size() + 1, NULL, NULL);
	MultiByteToWideChar(CP_UTF8, 0, pkt.name().c_str(), pkt.name().size() + 1, convertName, nLen);

	WCHAR convertMsg[100] = { 0, };
	nLen = MultiByteToWideChar(CP_UTF8, 0, pkt.msg().c_str(), pkt.msg().size() + 1, NULL, NULL);
	MultiByteToWideChar(CP_UTF8, 0, pkt.msg().c_str(), pkt.msg().size() + 1, convertMsg, nLen);

	wstring ws_msg = L"[" + static_cast<wstring>(convertName) + L"] : " + convertMsg;
	ws_msg += L'\0';
	wstring convertStr = wstring(ws_msg.begin(), ws_msg.end());
	serverSession->_dig->AddEventString(convertStr.c_str());
	return true;
}

bool Handle_S_REQUEST_HISTORY_CHAT(PacketSessionRef& session, Protocol::S_REQUEST_HISTORY_CHAT& pkt)
{
	return true;
}