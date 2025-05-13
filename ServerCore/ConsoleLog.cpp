#include "pch.h"
#include "ConsoleLog.h"

ConsoleLog::ConsoleLog()
{
	_stdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
	_stdErr = ::GetStdHandle(STD_ERROR_HANDLE);
}

ConsoleLog::~ConsoleLog()
{
}

void ConsoleLog::WriteStdOut(Color color, const WCHAR* format, ...)
{
	if (format == nullptr)
		return;

	SetConsoleOutputCP(CP_UTF8); // or CP_ACP for default codepage

	SetColor(true, color);

	va_list ap;
	va_start(ap, format);

	// vwprintf 대신 wcout 사용 (문자열 버퍼 구성)
	WCHAR buffer[512] = { 0 };
	vswprintf(buffer, sizeof(buffer) / sizeof(WCHAR), format, ap);
	va_end(ap);

	//std::wcout.imbue(std::locale("kor"));  // 한국어 로케일 설정
	//std::wcout << buffer << std::endl;

	// 버퍼 비우기
	fflush(stdout);

	// 색상 복원
	SetColor(true, Color::WHITE);
}

void ConsoleLog::WriteStdErr(Color color, const WCHAR* format, ...)
{
	WCHAR buffer[BUFFER_SIZE];

	if (format == nullptr)
		return;

	SetColor(false, color);

	va_list ap;
	va_start(ap, format);
	::vswprintf_s(buffer, BUFFER_SIZE, format, ap);
	va_end(ap);

	::fwprintf_s(stderr, buffer);
	fflush(stderr);

	SetColor(false, Color::WHITE);
}

void ConsoleLog::SetColor(bool stdOut, Color color)
{
	static WORD SColors[]
	{
		0,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
		FOREGROUND_RED | FOREGROUND_INTENSITY,
		FOREGROUND_GREEN | FOREGROUND_INTENSITY,
		FOREGROUND_BLUE | FOREGROUND_INTENSITY,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
	};

	::SetConsoleTextAttribute(stdOut ? _stdOut : _stdErr, SColors[static_cast<int32>(color)]);
}