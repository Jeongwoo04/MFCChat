#pragma once
#include <sqlext.h>

class Convert
{
public:
	static int64 GetCurrentEpochMilli()
	{
		using namespace std::chrono;
		auto now = system_clock::now();
		auto epoch = now.time_since_epoch();
		return duration_cast<milliseconds>(epoch).count();
	}

	static TIMESTAMP_STRUCT GetCurrentTimestamp()
	{
		using namespace std::chrono;

		system_clock::time_point now = system_clock::now();
		std::time_t now_c = system_clock::to_time_t(now);
		std::tm utc_tm;
		gmtime_s(&utc_tm, &now_c); // UTC

		TIMESTAMP_STRUCT ts;
		ts.year = utc_tm.tm_year + 1900;
		ts.month = utc_tm.tm_mon + 1;
		ts.day = utc_tm.tm_mday;
		ts.hour = utc_tm.tm_hour;
		ts.minute = utc_tm.tm_min;
		ts.second = utc_tm.tm_sec;
		ts.fraction = 0; // <-- 안전하게 처리

		return ts;
	}

	static wstring UTF8ToWStringDynamic(const std::string& utf8Str)
	{
		if (utf8Str.empty())
			return std::wstring();

		int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>(utf8Str.size()), NULL, 0);
		if (len <= 0)
			return std::wstring();

		std::wstring wstr(len, 0); // 정확한 길이만큼 wstring 공간 확보
		MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>(utf8Str.size()), &wstr[0], len);

		return wstr;
	}

	static bool UTF8ToWCHARArray(WCHAR* dest, const std::string& utf8)
	{
		int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, dest, 50);
		return (len > 0);
	}

	static string WStringToUTF8(const std::wstring& wstr)
	{
		if (wstr.empty()) return {};

		int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (len <= 0) return {};

		std::string result(len, 0); // 널 문자 포함 크기 확보
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], len, nullptr, nullptr);

		// 널 문자 제거 (선택 사항)
		if (!result.empty() && result.back() == '\0')
			result.pop_back();

		return result;
	}
};