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
Client 링커 오류 해결 -> odbc32.lib 추가<br/>
