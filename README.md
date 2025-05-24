# Server<br/><br/>

기존 DB 저장 Blocking 과정<br/>
-> GDBJobQueue 등록 비동기 처리<br/>
-> Room / DB Worker Thread<br/><br/>

protobuf string UTF-8 한글 깨짐 현상 해결<br/>
-> _setmode(_fileno(stdout), _O_U16TEXT); SetConsoleOutputCP(CP_UTF8);<br/><br/>

Job / DBJob 분리<br/>
-> Job / DBJob 실행 Worker 분리<br/>
-> DoAsync / DoDBAsync 분리<br/>
-> DB Worker안에서 JobQueue를 비울때 DBConnectionPool Pop / Push 한번만 호출<br/>
-> 기존 DoAsync 작업 호출시 매번 꺼내서 사용.<br/><br/>

Client 링커 오류 해결 -> odbc32.lib 추가<br/><br/>

DBConnection Pop -> nullptr 체크 (spin 2회)<br/>

패킷 설계 수정. GameSession / Player / Room 등 수정<br/>

Client->Conncet->Server->Login->DBConnect->Fail or Success->Enter->Broadcast 완료.<br/><br/>

DoDBAsync 등록 후 실행x -> 디버깅 결과 JobQueue::Push 및 각 Job들의 Execute 실행까지 확인.<br/>
-> LCurrentJobQueue 가 기존 JobQueue랑 달리 DBJobQueue에선 첫번째 접근 쓰레드가 처리를 안해줘서 그런가? <br/>
-> DBSave 안에서 sql Execute 예외처리 추가 -> GlobalQueue에서 꺼내올때 문제? <br/>
Job과 DBJob 분리 확실히 하기. _jobCount와 _dbJobCount 따로 사용하기. 공유하게되면 Worker에서 기아 발생<br/><br/>

DB message Save 및 xml parser 수정. Out 추가. spInsertChatMessage 최적화 (완료) <br/>
Message에 serial 부여 -> DBAsync 실패 후 JobQueue에 재등록시 message 순서 보장 <br/>
Server에서 PlayerId와 lastMessageId를 hash로 갖고있기. -> Client에서 가지고 요청하면 유실될 가능성 있음 (완료) <br/>
S_CHAT으로 Broadcast할게 생기면 각 Room에서의 Broadcast 기준.<br/><br/>

# SQL Server OUTPUT + SET과 SELECT 조합<br/>
OUTPUT + SET -> OUT 파라미터에 값을 채움 -> <br/>
Fetch() 대신 SQLMoreResults() 사용을 고려 <br/>
OUTPUT만 받는다면 Fetch()는 오히려 실패할 수 있다. SQL Server는 OUTPUT 값은 결과셋을 다 넘긴 후에야 접근 가능. <br/>
SELECT → Fetch()로 소비 <br/>
OUTPUT → SQLMoreResults() 이후 바인딩된 값 접근 가능 -> SELECT + OUTPUT 조합이면 Fetch -> SQLMoreResults()<br/>
OUTPUT만 있는 경우에도 SQLMoreResults()는 호출 필요 (ODBC는 커서 흐름을 단계별로 봄)<br/><br/>

SELECT -> Fetch로 값을 가져옴<br/>

PING / PONG 처리. Client Ping <-> Server Pong -> Client Pong <-> Server Ping 으로 변경<br/>
Server에서 Ping 보내는 WorkerThread 하나 추가. 확장성 고려. RoomId를 가질경우. RoomManager에서 각 Room 의 BroadcastPing 호출하게끔. 현재는 하나의 룸<br/>
Session 관리 : 기존 Set<GameSessionRef> 에서 unordered_map<sessionId, GameSessionRef> 로 변경 -> SessionId로 관리할 수 있게.<br/>
Enter/Leave 패킷으로 처리 -> Enter/Leave 에서 본인에게 Send + Spawn/Despawn 으로 타인에게 Broadcast 로 나눠서 보내기.<br/>

PING / PONG 처리 중. GameSession 내에서 OnDisconnect 부분에 Session 정리와 Room::Leave를 두어 IocpEvent로 Dispatch가 깨어나 호출하게 될때 처리.<br/>
하지만 Server에서 Client의 연결 끊김을 감지하고 Kick해야하는 상황에서 문제 발생. Disconnect 호출해도 Dispatch가 동작이 안될수도.<br/>
-> Room 안에서 처리? _players 반복문 도중 erase 되는 문제 발견 -> 삭제할 컨테이너 요소 따로 담아두고 -> 반복문 종료시 Leave 동기함수로 바로 호출<br/>
-> 이러면 GameSession은 어디서 지워줘야하나..<br/><br/>

DB Message Insert에서 messageId를 사용. DBSaveMessage 내에서 Execute 실패시. retry 간단 변수 추가. DelayPush, Priority DBJobQueue 등 신기한거 많았음.<br/>

## 1. 최근 채팅 메시지 조회 및 역순 출력 문제 해결

- **문제**  
  최근 30개의 메시지를 `serial` 기준 내림차순으로 `TOP 30` 조회 후 다시 오름차순 정렬하여 클라이언트로 전송했으나,  
  클라이언트에서 시간이 역전된 것처럼 보임.

- **원인**  
  SQL 쿼리에서 최신 메시지부터 조회(`ORDER BY serial DESC`) 후 다시 오름차순 정렬(`ORDER BY serial ASC`)하여  
  데이터가 예상과 다르게 출력됨.

- **해결책**  
  서버에서 메시지를 클라이언트로 보낼 때 올바른 순서(오름차순)를 유지하도록 조정함.  
  필요 시 클라이언트 또는 서버에서 메시지 버퍼에 넣기 전 순서를 재정렬.

---

## 2. `GameSession` 관련 `shared_ptr` / `weak_ptr` 사용 중 액세스 위반 오류 개선

- **문제**  
  `std::weak_ptr<GameSession>`에서 `lock()` 호출 시 읽기 액세스 위반 오류 발생.  
  특히 클라이언트 재접속 시 간헐적 발생.

- **원인**  
  `weak_ptr.lock()` 호출 전 만료 여부(`expired()`) 미확인.  
  `_players` 내 플레이어가 `nullptr`이거나 세션이 이미 해제된 상태를 체크하지 않음.

- **해결책**  
  - `weak_ptr.lock()` 호출 전에 반드시 `expired()` 체크 및 nullptr 검사 추가.  
  - 만료된 세션이 포함된 플레이어는 `_players`에서 즉시 제거.  
  - 반복자 사용 시 `erase()` 호출 후 안전하게 갱신 처리.

---

## 3. `Room::Broadcast` 함수 개선

- **기존 문제**  
  `_players` 순회 중 `ownerSession.lock()` 실패 시 nullptr 세션 포함으로 예외 발생 가능.

- **개선 코드 예시**

  ```cpp
  void Room::Broadcast(SendBufferRef sendBuffer)
  {
      for (auto it = _players.begin(); it != _players.end(); )
      {
          auto& player = it->second;
          if (!player)
          {
              it = _players.erase(it);
              continue;
          }

          auto session = player->ownerSession.lock();
          if (!session)
          {
              it = _players.erase(it);
              continue;
          }

          session->Send(sendBuffer);
          ++it;
      }
  }
  ```
효과

- nullptr 또는 만료된 세션을 가진 플레이어를 즉시 제거하여  
  안정성이 향상되고 예외 발생 가능성이 크게 감소했습니다.

---

## 4. `GameSession::OnDisconnected()` 변경 사항

### 이슈

- 세션 종료 시 `_currentPlayer`의 `ownerSession`을 해제하지 않아  
  재접속 시 `weak_ptr`이 만료되지 않고 접근 시도하는 문제가 발생했습니다.

### 수정 코드 예시

```cpp
void GameSession::OnDisconnected()
{
    GSessionManager->Remove(static_pointer_cast<GameSession>(shared_from_this()));
    PlayerRef player = this->_currentPlayer;

    if (_currentPlayer)
    {
        _currentPlayer->ownerSession.reset();

        auto capturedSession = static_pointer_cast<GameSession>(shared_from_this());
        GRoom->DoAsync(&Room::Leave, capturedSession, player);
    }

    _currentPlayer = nullptr;
}
```

### 결과

- 플레이어와 세션 연결을 완전히 끊어  
  **안전한 방 이탈 및 재접속**을 보장하게 되었습니다.

---

## 5. 접속-재접속 시 간헐적 Kick 문제 원인 및 대응

### 문제

- 클라이언트가 접속을 끊고 **바로 재접속할 때**,  
  이전 세션이 완전히 종료되지 않아 Ping Timeout 처리 중  
  예외 발생 또는 강제 퇴장(Kick) 처리되는 현상이 있었습니다.

### 원인

- Ping Timeout 검사 시 **만료된 세션에 접근**하여 예외가 발생했습니다.

### 대응

- Ping Timeout 검사 시 `weak_ptr` 만료 여부와 `nullptr` 체크를 강화했습니다.
- `Disconnect()` 시 **완전한 세션 정리**를 보장하도록 수정했습니다.
- 세션 종료 후 **일정 시간 딜레이를 두거나**,  
  **세션 ID 중복 검사 도입**을 검토 중입니다.

---

## 6. 컨테이너 삭제 시 반복자 안전 처리

- `_players` 컨테이너에서 **무효화된 세션 또는 플레이어 제거 시**,  
  **반복자 무효화 방지를 위해 `erase()` 후 반복자 갱신**을 적용했습니다.
  ```cpp
  for (auto it = _players.begin(); it != _players.end(); )
  {
      auto& player = it->second;
      if (!player || player->ownerSession.expired())
      {
          it = _players.erase(it);
          continue;
      }

      auto session = player->ownerSession.lock();
      if (!session)
      {
          it = _players.erase(it);
          continue;
      }

      session->Send(sendBuffer);
      ++it;
  }
  ```
<br/><br/><br/><br/>
TODO : DB Worker, DBJob 분리. 버그 수정.
TODO : XML Parser에 OUTPUT 파싱 추가. SELECT 와 OUTPUT + SET 조합 jinja tool 자동 생성 코드 템플릿 추가. Binding 함수 세분화.<br/>
TODO : Client 강제 종료시 Server Crash. -> DBConnection 을 꺼내온 상태로 Worker가 돌아가다 삭제된 iterator를 참조함.<br/>
TODO : Client <-> Server Ping / Pong 추가
