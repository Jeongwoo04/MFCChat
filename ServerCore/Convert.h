#pragma once

template <size_t N>
bool UTF8ToWCHARArray(WCHAR(&dest)[N], const std::string& src)
{
	//WCHAR convertName[50] = { 0, };
	//int nLen = MultiByteToWideChar(CP_UTF8, 0, pkt.name().c_str(), pkt.name().size() + 1, NULL, NULL);
	//MultiByteToWideChar(CP_UTF8, 0, pkt.name().c_str(), pkt.name().size() + 1, convertName, nLen);

	//int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, dest, N);
	int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), src.size() + 1, NULL, NULL);
	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), src.size() + 1, dest, len);
	
	return true;
}

bool UTF8ToWCHARArray(WCHAR* dest, const std::string& utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, dest, 50);
	return (len > 0);
}

template <size_t N>
void CopyWCHARToArray(WCHAR(&dest)[N], const WCHAR* src)
{
	// 최대 크기를 초과 X
	wcsncpy_s(dest, src, N - 1);
	dest[N - 1] = L'\0';
};

std::wstring StringToWString(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	std::wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
	wstr.pop_back(); // remove null terminator
	return wstr;
}