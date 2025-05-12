#include "pch.h"
#include "CoreTLS.h"
// thread_local_storage : thread 마다 있는 고유 저장소

thread_local uint32				LThreadId = 0;
thread_local uint64				LEndTickCount = 0;
thread_local std::stack<int32>	LLockStack;
thread_local SendBufferChunkRef	LSendBufferChunk;
thread_local JobQueue*			LCurrentJobQueue = nullptr;