#pragma once
#include <stack>

extern thread_local uint32				LThreadId;
extern thread_local uint64				LEndTickCount;

extern thread_local std::stack<int32>	LLockStack; // thread 마다 자신만의 LockStack이 필요하다.
extern thread_local SendBufferChunkRef	LSendBufferChunk;
extern thread_local class JobQueue*		LCurrentJobQueue;
extern thread_local class JobQueue*		LCurrentDBJobQueue;