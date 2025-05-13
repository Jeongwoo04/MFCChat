# Server

기존 DB 저장 Blocking 과정
-> GDBJobQueue 등록 비동기 처리
-> Room / DB Worker Thread

protobuf string UTF-8 한글 깨짐 현상 해결
-> _setmode(_fileno(stdout), _O_U16TEXT); SetConsoleOutputCP(CP_UTF8);
