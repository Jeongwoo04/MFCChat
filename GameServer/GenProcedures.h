#pragma once
#include "Types.h"
#include <windows.h>
#include "DBBind.h"

namespace SP
{
	
    class InsertPlayer : public DBBind<2,0>
    {
    public:
    	InsertPlayer(DBConnection& conn) : DBBind(conn, L"{CALL dbo.spInsertPlayer(?,?)}") { }
          template<int32 N> void In_Name(WCHAR(&v)[N]) { BindParam(0, v); };
          template<int32 N> void In_Name(const WCHAR(&v)[N]) { BindParam(0, v); };
          void In_Name(WCHAR* v, int32 count) { BindParam(0, v, count); };
          void In_Name(const WCHAR* v, int32 count) { BindParam(0, v, count); };
        void Out_Player_id(int64& v) { BindParamOut(1, v); };

    private:
    	int64 _player_id = {};
    };

    class GetPlayerIdByName : public DBBind<1,1>
    {
    public:
    	GetPlayerIdByName(DBConnection& conn) : DBBind(conn, L"{CALL dbo.spGetPlayerIdByName(?)}") { }
          template<int32 N> void In_Name(WCHAR(&v)[N]) { BindParam(0, v); };
          template<int32 N> void In_Name(const WCHAR(&v)[N]) { BindParam(0, v); };
          void In_Name(WCHAR* v, int32 count) { BindParam(0, v, count); };
          void In_Name(const WCHAR* v, int32 count) { BindParam(0, v, count); };
    	void Out_Player_id(OUT int64& v) { BindCol(0, v); };

    private:
    };

    class InsertLogin : public DBBind<2,0>
    {
    public:
    	InsertLogin(DBConnection& conn) : DBBind(conn, L"{CALL dbo.spInsertLogin(?,?)}") { }
          void In_Player_id(int64& v) { BindParam(0, v); };
          void In_Player_id(int64&& v) { _player_id = std::move(v); BindParam(0, _player_id); };
          void In_Login_time(TIMESTAMP_STRUCT& v) { BindParam(1, v); };
          void In_Login_time(TIMESTAMP_STRUCT&& v) { _login_time = std::move(v); BindParam(1, _login_time); };

    private:
    	int64 _player_id = {};
    	TIMESTAMP_STRUCT _login_time = {};
    };

    class InsertChatMessage : public DBBind<4,0>
    {
    public:
    	InsertChatMessage(DBConnection& conn) : DBBind(conn, L"{CALL dbo.spInsertChatMessage(?,?,?,?)}") { }
          void In_Player_id(int64& v) { BindParam(0, v); };
          void In_Player_id(int64&& v) { _player_id = std::move(v); BindParam(0, _player_id); };
          template<int32 N> void In_Message(WCHAR(&v)[N]) { BindParam(1, v); };
          template<int32 N> void In_Message(const WCHAR(&v)[N]) { BindParam(1, v); };
          void In_Message(WCHAR* v, int32 count) { BindParam(1, v, count); };
          void In_Message(const WCHAR* v, int32 count) { BindParam(1, v, count); };
          void In_Serial(int64& v) { BindParam(2, v); };
          void In_Serial(int64&& v) { _serial = std::move(v); BindParam(2, _serial); };
        void Out_Message_id(int64& v) { BindParamOut(3, v); };

    private:
    	int64 _player_id = {};
    	int64 _serial = {};
    	int64 _message_id = {};
    };

    class GetRecentChatMessagesFromId : public DBBind<2,5>
    {
    public:
    	GetRecentChatMessagesFromId(DBConnection& conn) : DBBind(conn, L"{CALL dbo.spGetRecentChatMessagesFromId(?,?)}") { }
          void In_MessageId(int64& v) { BindParam(0, v); };
          void In_MessageId(int64&& v) { _messageId = std::move(v); BindParam(0, _messageId); };
          void In_NeedCount(int64& v) { BindParam(1, v); };
          void In_NeedCount(int64&& v) { _needCount = std::move(v); BindParam(1, _needCount); };
    	void Out_Message_id(OUT int64& v) { BindCol(0, v); };
    	void Out_Player_id(OUT int64& v) { BindCol(1, v); };
    	template<int32 N> void Out_Message(OUT WCHAR(&v)[N]) { BindCol(2, v); };
    	void Out_Timestamp(OUT TIMESTAMP_STRUCT& v) { BindCol(3, v); };
    	void Out_Serial(OUT int64& v) { BindCol(4, v); };

    private:
    	int64 _messageId = {};
    	int64 _needCount = {};
    };


     
};