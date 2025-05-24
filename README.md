# Server<br/><br/>

### 1. DB 저장 Blocking → 비동기 처리 전환
- 기존: 메시지 저장 시 Main Thread에서 직접 Blocking DB 호출
- 변경: `GDBJobQueue` 등록 후, Room과 분리된 DB Worker Thread에서 비동기로 처리
- 효과: 메인 로직과 DB I/O 분리로 병렬 처리 및 지연 최소화

---

### 2. Protobuf UTF-8 한글 깨짐 현상 해결
- 출력 설정 추가:
  ```cpp
  _setmode(_fileno(stdout), _O_U16TEXT);
  SetConsoleOutputCP(CP_UTF8);
  ```
효과: 콘솔에서 UTF-8 기반 한글 정상 출력 가능

3. Job / DBJob 구조 분리
Job / DBJob 실행 스레드 분리

DoAsync() / DoDBAsync() 함수 구분

DBWorker 내에서는 Connection Pool Pop/Push 최소화

각각 JobCount/DBJobCount 분리 관리 → 기아(starvation) 방지

4. 클라이언트 링커 오류 해결
ODBC 링커 오류: odbc32.lib 추가하여 해결

5. DBConnection Pop 시 nullptr 보호
2회 재시도 로직 추가 (spin 2회)

실패 시 로그 또는 예외 처리

6. 패킷 구조 및 플레이어 흐름 정비
GameSession, Player, Room 구조 재정비

Client → Connect → Server → Login → DBConnect → Enter → Broadcast 흐름 확립

7. DBJobQueue 실행 안되는 문제 분석
DoDBAsync 등록 이후 실행되지 않던 이슈:

LCurrentJobQueue와 DBJobQueue 간 구분

첫 접근 쓰레드에서 처리 누락 가능성

Execute() 내부 예외 처리 강화

해결:

JobQueue 실행 흐름 전수 검사

DBSave() 내 예외 처리 및 GlobalQueue 추적 강화

8. DB 메시지 저장 최적화 및 안정성 강화
spInsertChatMessage 저장 절차 개선

xml 파서에서 <out="true"> 파라미터 적용 처리 완료

메시지에 serial 부여 → 실패 시 재전송 가능

PlayerId + lastMessageId 조합으로 메시지 순서 보장

9. SQL Server OUTPUT + SELECT 혼합 처리
OUTPUT + SELECT 조합 시 Fetch → SQLMoreResults 호출 필요

SELECT → Fetch()
OUTPUT → SQLMoreResults() 이후 OUT 파라미터 접근 가능

10. Ping / Pong 구조 개선
기존: Client Ping → Server Pong

변경: Server Ping → Client Pong 응답 구조로 재설계

RoomManager 기반 Ping Broadcast 확장 고려

Session 관리: unordered_map<sessionId, GameSessionRef> 구조로 변경

입장/퇴장 시:

본인: Send

타인: Broadcast(Spawn/Despawn)

11. OnDisconnected 처리 개선
GameSession::OnDisconnected 시:

ownerSession.reset()

Room::Leave() 호출 등록

Ping Timeout 등으로 인한 Kick 시:

_players 반복 중 erase() 문제 → 제거 대상 별도 저장 후 처리

Room 내부에서 Leave() 호출로 마무리

12. DB 실패 시 Retry 흐름 설계
DBSaveMessage 내 Execute() 실패 시:

retry 변수 추가

추후 DelayPush, Priority DBJobQueue 적용 가능성 탐색

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
