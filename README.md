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
패킷 설계 수정. GameSession / Player / Room 등 수정<br/><br/><br/><br/>

해결 : DoDBAsync 등록 후 실행x -> 디버깅 결과 JobQueue::Push 및 각 Job들의 Execute 실행까지 확인.<br/>
-> LCurrentJobQueue 가 기존 JobQueue랑 달리 DBJobQueue에선 첫번째 접근 쓰레드가 처리를 안해줘서 그런가? <br/>
-> DBSave 안에서 sql Execute 예외처리 로그 추가. -> 잘 실행됨. -> GlobalQueue에서 꺼내올때 문제가 있나? <br/>
Atomic 인자를 공유해서 쓰지 맙시다. 정말 별짓 다했는데 _jobCount 를 왜 따로 안했지.. 쨋든 해결 <br/><br/>
TODO : Client 강제 종료시 Server Crash. -> DBConnection 을 꺼내온 상태로 Worker가 돌아가다 삭제된 iterator를 참조함.<br/>
TODO : Client <-> Server Ping / Pong 추가
