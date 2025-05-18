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

	static string WideToUTF8(const std::wstring& wstr)
	{
		if (wstr.empty()) return std::string();

		int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
		std::string result(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &result[0], size_needed, nullptr, nullptr);
		return result;
	}

	static bool ConvertUTF8ToWStrCopy(const string& utf8Str, size_t maxLen, wstring& outWStr)
	{
		WCHAR temp[256] = { 0 }; // 충분한 임시 버퍼 (필요시 템플릿 인수로 조정 가능)

		if (!Convert::UTF8ToWCHARArray(temp, utf8Str) || wcslen(temp) == 0)
			return false;

		if (wcslen(temp) > maxLen)
			return false; // 초과 길이 방지

		outWStr = temp;
		return true;
	}

	template <size_t N>
	static bool UTF8ToWCHARArray(WCHAR(&dest)[N], const std::string& src)
	{
		//WCHAR convertName[50] = { 0, };
		//int nLen = MultiByteToWideChar(CP_UTF8, 0, pkt.name().c_str(), pkt.name().size() + 1, NULL, NULL);
		//MultiByteToWideChar(CP_UTF8, 0, pkt.name().c_str(), pkt.name().size() + 1, convertName, nLen);

		//int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, dest, N);
		int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), src.size() + 1, NULL, NULL);
		MultiByteToWideChar(CP_UTF8, 0, src.c_str(), src.size() + 1, dest, len);

		return true;
	}

	static wstring UTF8ToWString(const std::string& str)
	{
		if (str.empty())
			return std::wstring();

		// 변환에 필요한 wchar_t 길이 계산 (널 문자 포함)
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
		if (size_needed == 0)
			return std::wstring();

		// wstring에 크기 맞춰 공간 할당
		std::wstring wstr(size_needed, L'\0');

		// 변환 수행
		int chars_converted = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
		if (chars_converted == 0)
			return std::wstring();

		// MultiByteToWideChar는 널문자까지 포함해 변환하므로,
		// wstring 끝에 자동으로 들어간 널문자 제거
		if (!wstr.empty() && wstr.back() == L'\0')
			wstr.pop_back();

		return wstr;
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

	template <size_t N>
	static void CopyWCHARToArray(WCHAR(&dest)[N], const WCHAR* src)
	{
		// 최대 크기를 초과 X
		wcsncpy_s(dest, src, N - 1);
		dest[N - 1] = L'\0';
	};

	static wstring StringToWString(const std::string& str)
	{
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
		std::wstring wstr(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
		wstr.pop_back(); // remove null terminator
		return wstr;
	}

	static std::string WStringToUTF8(const std::wstring& wstr)
	{
		if (wstr.empty())
			return std::string();

		int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
		if (size_needed <= 0)
			return std::string();

		std::string str(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &str[0], size_needed, NULL, NULL);

		return str;
	}

	static bool IsValidUTF8(const std::string& str)
	{
		const unsigned char* bytes = (const unsigned char*)str.c_str();
		int len = (int)str.size();
		int i = 0;

		while (i < len)
		{
			if (bytes[i] <= 0x7F)
			{
				// ASCII
				i += 1;
			}
			else if ((bytes[i] & 0xE0) == 0xC0)
			{
				// 2-byte sequence
				if (i + 1 >= len) return false;
				if ((bytes[i + 1] & 0xC0) != 0x80) return false;
				i += 2;
			}
			else if ((bytes[i] & 0xF0) == 0xE0)
			{
				// 3-byte sequence
				if (i + 2 >= len) return false;
				if ((bytes[i + 1] & 0xC0) != 0x80) return false;
				if ((bytes[i + 2] & 0xC0) != 0x80) return false;
				i += 3;
			}
			else if ((bytes[i] & 0xF8) == 0xF0)
			{
				// 4-byte sequence
				if (i + 3 >= len) return false;
				if ((bytes[i + 1] & 0xC0) != 0x80) return false;
				if ((bytes[i + 2] & 0xC0) != 0x80) return false;
				if ((bytes[i + 3] & 0xC0) != 0x80) return false;
				i += 4;
			}
			else
			{
				return false;
			}
		}
		return true;
	}
};