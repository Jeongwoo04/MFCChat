# Server<br/><br/>

### 1. DB 저장 Blocking → 비동기 처리 전환
- **기존**: 메시지 저장 시 Main Thread에서 Blocking DB 호출
- **변경**: `GDBJobQueue` 등록 → Room과 분리된 DB Worker Thread에서 비동기 처리
- **효과**: 게임 로직과 DB I/O 분리로 병목 해소, 병렬 처리 가능

---

### 2. Job / DBJob 구조 분리
- `Job` / `DBJob` 실행 스레드 분리
- `DoAsync()` / `DoDBAsync()` 함수 구분
- DBWorker에서는 Connection Pool `Pop/Push`를 **한 번만 호출**
- `JobCount` / `DBJobCount`를 별도로 관리하여 **기아(starvation)** 방지

---

### 3. 클라이언트 링커 오류 해결
- ODBC 관련 링커 오류 발생 → `odbc32.lib` 추가로 해결

---

### 4. DBConnection Pop 시 nullptr 보호
- Connection이 `nullptr`일 경우 **2회 재시도 (spin)** 로직 적용
- 최종 실패 시 **로그 기록 또는 예외 처리**로 대응

---

### 5. 패킷 구조 및 플레이어 흐름 정비
- `GameSession`, `Player`, `Room` 구조 개선
- 흐름 정리:
Client → Connect → Server → Login → DBConnect
→ (Success/Fail) → Enter → Broadcast

---

### 6. DBJobQueue 실행 안 되는 문제 분석 및 해결
- `DoDBAsync()` 등록 후 실행되지 않던 이슈:
- `LCurrentJobQueue`와 `DBJobQueue`의 **구조 차이**
- 첫 접근 스레드에서 `Execute()` 호출 누락 가능성
- 내부 예외 처리 부족 → 예외 발생 시 중단
- **대응**:
- `JobQueue` 실행 흐름 전체 점검
- `DBSave()` 내부 예외 처리 및 `GlobalQueue` 로직 디버깅

---

### 7. DB 메시지 저장 안정성 강화
- `spInsertChatMessage` 최적화
- XML 파서에서 `<out="true">` 파라미터 반영 완료
- 메시지에 **serial 번호** 부여 → 실패 시 재전송 보장
- `PlayerId + lastMessageId` 조합으로 **메시지 순서 보장**

---

### 8. SQL Server OUTPUT + SELECT 혼합 처리
- `SELECT` → `Fetch()`로 결과 소비
- `OUTPUT` → `SQLMoreResults()` 이후 OUT 파라미터 접근
- `OUTPUT`만 있는 경우에도 **`SQLMoreResults()` 호출 필요**
- ODBC는 결과셋을 끝까지 넘긴 후 OUT 바인딩을 반환

---

### 9. Ping / Pong 구조 개선
- **변경 전**: Client → Ping / Server → Pong
- **변경 후**: Server → Ping / Client → Pong
- `RoomManager`에서 각 Room에 Ping Broadcast 가능하도록 확장성 고려
- `Session` 관리 방식 개선:
- `Set<GameSessionRef>` → `unordered_map<sessionId, GameSessionRef>` 로 변경
- 입장/퇴장 처리 방식:
- **본인**: `Send()`
- **타인**: `Broadcast(Spawn/Despawn)`

---

### 10. OnDisconnected 처리 개선
- `GameSession::OnDisconnected()`에서:
- `ownerSession.reset()`
- `Room::Leave()` 등록
- Ping Timeout 또는 강제 Kick 시 문제 발생:
- `_players` 반복 중 `erase()` 문제 → 제거 대상 따로 저장
- 반복 종료 후 Leave 처리 → **반복자 무효화 방지**

---

### 11. DB 실패 시 Retry 흐름 설계
- `DBSaveMessage` 내 `Execute()` 실패 시:
- `retry` 변수로 재시도 제어
- 향후 기능 확장 고려:
- `DelayPush`, `Priority DBJobQueue` 등 우선순위 기반 재등록 구조 검토

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
