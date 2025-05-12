#pragma once
#include "NetAddress.h"

/*----------------
	SocketUtils
-----------------*/

class SocketUtils
{
public:
	// 처음엔 nullptr. 실질적으로 runtime에 긁어와야함
	static LPFN_CONNECTEX		ConnectEx; // ConnectEx 함수의 포인터
	static LPFN_DISCONNECTEX	DisconnectEx; // DisconnectEx 함수의 포인터
	static LPFN_ACCEPTEX		AcceptEx; // AcceptEx 함수의 포인터

public:
	static void Init();
	static void Clear();

	static bool BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);
	static SOCKET CreateSocket();

	// setsockopt 자주 쓰는것
	static bool SetLinger(SOCKET socket, uint16 onoff, uint16 linger);
	static bool SetReuseAddress(SOCKET socket, bool flag); // 같은 주소와 포트를 재사용 (재접했을때 누가 차지한 경우?)
	static bool SetRecvBufferSize(SOCKET socket, int32 size); // 커널의 수신 버퍼 크기를 조정할 수 있다.
	static bool SetSendBufferSize(SOCKET socket, int32 size); // 커널의 송신 버퍼 크기를 조정할 수 있다.
	static bool SetTcpNoDelay(SOCKET socket, bool flag); // 네이글 알고리즘
	static bool SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket); // ListenSocket의 특성을 ClientSocket에 그대로 적용

	static bool Bind(SOCKET socket, NetAddress netAddr);
	// 임의의 ip 주소를 바인딩 하겠다. port는 설정해줘야함
	static bool BindAnyAddress(SOCKET socket, uint16 port);
	static bool Listen(SOCKET socket, int32 backlog = SOMAXCONN);
	static void Close(SOCKET& socket);
};

template<typename T>
static inline bool SetSockOpt(SOCKET socket, int32 level, int32 optName, T optVal)
{
	return SOCKET_ERROR != ::setsockopt(socket, level, optName, reinterpret_cast<char*>(&optVal), sizeof(T));
}