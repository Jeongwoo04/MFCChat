#pragma once

class Convert
{
public:
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
};