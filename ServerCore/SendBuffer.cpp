#include "pch.h"
#include "SendBuffer.h"

/*----------------
	SendBuffer
-----------------*/

SendBuffer::SendBuffer(SendBufferChunkRef owner, BYTE* buffer, uint32 allocSize)
	: _owner(owner), _buffer(buffer), _allocSize(allocSize)
{
}

SendBuffer::~SendBuffer()
{
}

void SendBuffer::Close(uint32 writeSize) // 열어서 실제 사용할 공간(writeSize)
{
	ASSERT_CRASH(_allocSize >= writeSize);
	_writeSize = writeSize;
	_owner->Close(writeSize);
}

/*--------------------
	SendBufferChunk
--------------------*/

// TLS 영역에서 실행되어서 싱글스레드 환경이라 생각하고 Lock 없이 진행

SendBufferChunk::SendBufferChunk()
{
}

SendBufferChunk::~SendBufferChunk()
{
}

void SendBufferChunk::Reset()
{
	_open = false;
	_usedSize = 0;
}

SendBufferRef SendBufferChunk::Open(uint32 allocSize) // 덩어리에서 사용중인 곳 부터 꺼내와서 할당하겠다
{
	ASSERT_CRASH(allocSize <= SEND_BUFFER_CHUNK_SIZE);
	ASSERT_CRASH(_open == false);

	if (allocSize > FreeSize())
		return nullptr;

	_open = true;
	// SendBuffer를 뜯어서 SendBufferRef 스마트포인터로 줌
	// 누군가 Chunk 일부를 사용하고있을때 Chunk는 날리면 안됨. -> refCounting 필요
	return ObjectPool<SendBuffer>::MakeShared(shared_from_this(), Buffer(), allocSize);
}

void SendBufferChunk::Close(uint32 writeSize)
{
	ASSERT_CRASH(_open == true); // 사용하려면 open이 먼저 되어있어야함.
	_open = false; // 사용했으니 false로
	_usedSize += writeSize; // 실제 사용한 사이즈만큼 더해서 옮겨줌
}

/*---------------------
	SendBufferManager
----------------------*/

// SendBuffer 덩어리에서 우리가 사용할 만큼을 쪼개서 사용을 하겠다. 뜯어가는 개념
// Data를 이리저리 채운다음 Close -> Close가 실질적으로 사용하는 영역
SendBufferRef SendBufferManager::Open(uint32 size)
{
	// TLS를 활용해서 최대한 Lock을 걸지않고 사용하는 방법
	if (LSendBufferChunk == nullptr) // TLS라 경합X
	{
		LSendBufferChunk = Pop(); // WRITE_LOCK
		LSendBufferChunk->Reset();
	}

	ASSERT_CRASH(LSendBufferChunk->IsOpen() == false); // 이미 꺼내왔는데 또 꺼내면 안돼

	// 다 썼으면 버리고 새거로 교체
	if (LSendBufferChunk->FreeSize() < size)
	{
		LSendBufferChunk = Pop(); // WRITE_LOCK
		LSendBufferChunk->Reset();
	}

	return LSendBufferChunk->Open(size);
}

// Pool에서 꺼내오기
SendBufferChunkRef SendBufferManager::Pop()
{

	{
		WRITE_LOCK;
		if (_sendBufferChunks.empty() == false) // Pool에 여유분이 있다.
		{
			SendBufferChunkRef sendBufferChunk = _sendBufferChunks.back();
			_sendBufferChunks.pop_back();
			return sendBufferChunk;
		}
	}

	return SendBufferChunkRef(xnew<SendBufferChunk>(), PushGlobal); // 여유분이 없어 새로 만들자
	// MemoryPool과 다른점 -> refCount가 0이 될때
	// xdelete가 아닌 PushGlobal에 넣어두고 필요시 다시 꺼내옴 static void (xnew에 멤버함수로 전달해줄수없기 때문에)
}

// Pool에 반납
void SendBufferManager::Push(SendBufferChunkRef buffer)
{
	WRITE_LOCK;
	_sendBufferChunks.push_back(buffer);
}

void SendBufferManager::PushGlobal(SendBufferChunk* buffer)
{
	cout << "PushGlobal SENDBUFFERCHUNK" << endl;
	GSendBufferManager->Push(SendBufferChunkRef(buffer, PushGlobal));
}