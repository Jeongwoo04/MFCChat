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

	if (serverSession == nullptr)
		return false;

	serverSession->Disconnect(L"Login Fail");
	serverSession->_dig->_isConnected = false;

	string message;
	switch (pkt.cause()) {
	case Protocol::Cause::CAUSE_NONE:
		message += "";
		break;
	case Protocol::Cause::CAUSE_INVAILD_NAME:
		message += "[INVALID_NAME] : ";
		break;
	case Protocol::Cause::CAUSE_DB_ERROR:
		message += "[DB_ERROR] : ";
		break;
	case Protocol::Cause::CAUSE_ALREADY_LOGGED_IN:
		message += "[ALREADY_LOGGED_IN] : ";
		break;
	default:
		break;
	}

	message += pkt.message();
	serverSession->_dig->AddEventString(Convert::UTF8ToWStringDynamic(message).c_str());

	return true;
}

bool Handle_S_ENTER(PacketSessionRef& session, Protocol::S_ENTER& pkt)
{
	// TODO : 입장 UI -> 게임 입장
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	if (serverSession == nullptr)
		return false;

	serverSession->SetName(pkt.player().name());
	serverSession->SetPlayerId(pkt.player().player_id());

	return true;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	if (serverSession == nullptr)
		return false;

	Protocol::ChatMessage* chatMsg = pkt.mutable_message();

	int64 serialId = chatMsg->serial_id();
	int64 messageId = chatMsg->message_id();
	string message = chatMsg->message();
	uint64 playerId = chatMsg->player_id();
	string playerName = chatMsg->name();

	if (messageId > GServerSessionManager->GetLastMessageId())
		GServerSessionManager->SetLastMessageId(messageId);

	serverSession->_dig->AddEventString(Convert::UTF8ToWStringDynamic(message).c_str());

	return true;
}

bool Handle_S_CHAT_HISTORY(PacketSessionRef& session, Protocol::S_CHAT_HISTORY& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	if (serverSession == nullptr)
		return false;

	if (pkt.request() == Protocol::REQUEST_RESET)
	{
		serverSession->_dig->chatList.ResetContent(); // 기존 메시지 비우기
	}

	if (pkt.messages_size() > 0)
	{
		int64 firstMessageId = pkt.messages(0).message_id();
		if (firstMessageId < GServerSessionManager->GetOldestMessageId())
			GServerSessionManager->SetOldestMessageId(firstMessageId);
	}

	if (pkt.request() == Protocol::REQUEST_OLDEST)
	{
		// 메시지를 역순으로 순회해서 맨 위에 순서대로 넣기
		auto& messages = *pkt.mutable_messages();

		for (auto it = messages.rbegin(); it != messages.rend(); ++it)
		{
			const auto& chat = *it;

			int64 serialId = chat.serial_id();
			int64 messageId = chat.message_id();
			string message = chat.message();
			uint64 playerId = chat.player_id();
			string playerName = chat.name();

			if (messageId > GServerSessionManager->GetLastMessageId())
				GServerSessionManager->SetLastMessageId(messageId);

			// AddEventString 대신 InsertString(0, ...) 으로 맨 위에 삽입해야 함
			CString wmsg = Convert::UTF8ToWStringDynamic(message).c_str();

			// serverSession->_dig->chatList 가 CListBox라고 가정
			if (serverSession->_dig->chatList.GetSafeHwnd())
				serverSession->_dig->chatList.InsertString(0, wmsg);
		}
	}
	else
	{
		for (const auto& chat : *pkt.mutable_messages())
		{
			int64 serialId = chat.serial_id();
			int64 messageId = chat.message_id();
			string message = chat.message();
			uint64 playerId = chat.player_id();
			string playerName = chat.name();

			if (messageId > GServerSessionManager->GetLastMessageId())
				GServerSessionManager->SetLastMessageId(messageId);

			serverSession->_dig->AddEventString(Convert::UTF8ToWStringDynamic(message).c_str());
		}
	}	

	return true;
}

bool Handle_S_LEAVE(PacketSessionRef& session, Protocol::S_LEAVE& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	if (serverSession == nullptr)
		return false;

	serverSession->Disconnect(L"Leave");

	return true;
}

bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);
	
	if (serverSession == nullptr)
		return false;
	
	auto& players = serverSession->_otherPlayers;

	for (int i = 0; i < pkt.players_size(); i++)
	{
		const Protocol::PlayerInfo& info = pkt.players(i);
		players[info.player_id()] = info;
	}

	return true;
}

bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);
	
	if (serverSession == nullptr)
		return false;
	
	auto& players = serverSession->_otherPlayers;

	players.erase(pkt.player_id());

	return true;
}

bool Handle_S_PING(PacketSessionRef& session, Protocol::S_PING& pkt)
{
	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	if (serverSession == nullptr)
		return false;

	Protocol::C_PONG pongPkt;
	uint64 now = ::GetTickCount64();
	pongPkt.set_timestamp(now); // echo back

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pongPkt);
	serverSession->Send(sendBuffer);

	return true;
}
