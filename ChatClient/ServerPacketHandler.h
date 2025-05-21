#pragma once
#include "Protocol.pb.h"

using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum : uint16
{
	PKT_C_LOGIN = 1000,
	PKT_S_LOGIN_FAIL = 1001,
	PKT_S_ENTER = 1002,
	PKT_C_CHAT = 1003,
	PKT_S_CHAT = 1004,
	PKT_C_LEAVE = 1005,
	PKT_S_LEAVE = 1006,
	PKT_S_SPAWN = 1007,
	PKT_S_DESPAWN = 1008,
	PKT_C_PONG = 1009,
	PKT_S_PING = 1010,
};

// Custom Handlers
bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len);
bool Handle_S_LOGIN_FAIL(PacketSessionRef& session, Protocol::S_LOGIN_FAIL& pkt);
bool Handle_S_ENTER(PacketSessionRef& session, Protocol::S_ENTER& pkt);
bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt);
bool Handle_S_LEAVE(PacketSessionRef& session, Protocol::S_LEAVE& pkt);
bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt);
bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt);
bool Handle_S_PING(PacketSessionRef& session, Protocol::S_PING& pkt);

class ServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_S_LOGIN_FAIL] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_LOGIN_FAIL>(Handle_S_LOGIN_FAIL, session, buffer, len); };
		GPacketHandler[PKT_S_ENTER] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ENTER>(Handle_S_ENTER, session, buffer, len); };
		GPacketHandler[PKT_S_CHAT] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_CHAT>(Handle_S_CHAT, session, buffer, len); };
		GPacketHandler[PKT_S_LEAVE] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_LEAVE>(Handle_S_LEAVE, session, buffer, len); };
		GPacketHandler[PKT_S_SPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_SPAWN>(Handle_S_SPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_DESPAWN] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DESPAWN>(Handle_S_DESPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_PING] = [](PacketSessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PING>(Handle_S_PING, session, buffer, len); };
	}

	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::C_LOGIN& pkt) { return MakeSendBuffer(pkt, PKT_C_LOGIN); }
	static SendBufferRef MakeSendBuffer(Protocol::C_CHAT& pkt) { return MakeSendBuffer(pkt, PKT_C_CHAT); }
	static SendBufferRef MakeSendBuffer(Protocol::C_LEAVE& pkt) { return MakeSendBuffer(pkt, PKT_C_LEAVE); }
	static SendBufferRef MakeSendBuffer(Protocol::C_PONG& pkt) { return MakeSendBuffer(pkt, PKT_C_PONG); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static SendBufferRef MakeSendBuffer(T & pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

		SendBufferRef sendBuffer = GSendBufferManager->Open(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;
		ASSERT_CRASH(pkt.SerializeToArray(&header[1], dataSize));
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};