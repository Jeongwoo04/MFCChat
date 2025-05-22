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

<br/><br/><br/><br/>
TODO : DB Worker, DBJob 분리. 버그 수정.
TODO : XML Parser에 OUTPUT 파싱 추가. SELECT 와 OUTPUT + SET 조합 jinja tool 자동 생성 코드 템플릿 추가. Binding 함수 세분화.<br/>
TODO : Client 강제 종료시 Server Crash. -> DBConnection 을 꺼내온 상태로 Worker가 돌아가다 삭제된 iterator를 참조함.<br/>
TODO : Client <-> Server Ping / Pong 추가
