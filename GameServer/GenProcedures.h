#pragma once
#include "Types.h"
#include <windows.h>
#include "DBBind.h"

namespace SP
{
	
    class InsertMsg : public DBBind<2,0>
    {
    public:
    	InsertMsg(DBConnection& conn) : DBBind(conn, L"{CALL dbo.spInsertMsg(?,?)}") { }
    	template<int32 N> void In_Msg(WCHAR(&v)[N]) { BindParam(0, v); };
    	template<int32 N> void In_Msg(const WCHAR(&v)[N]) { BindParam(0, v); };
    	void In_Msg(WCHAR* v, int32 count) { BindParam(0, v, count); };
    	void In_Msg(const WCHAR* v, int32 count) { BindParam(0, v, count); };
    	template<int32 N> void In_Name(WCHAR(&v)[N]) { BindParam(1, v); };
    	template<int32 N> void In_Name(const WCHAR(&v)[N]) { BindParam(1, v); };
    	void In_Name(WCHAR* v, int32 count) { BindParam(1, v, count); };
    	void In_Name(const WCHAR* v, int32 count) { BindParam(1, v, count); };

    private:
    };

    class GetMsg : public DBBind<1,3>
    {
    public:
    	GetMsg(DBConnection& conn) : DBBind(conn, L"{CALL dbo.spGetMsg(?)}") { }
    	void In_Id(int32& v) { BindParam(0, v); };
    	void In_Id(int32&& v) { _id = std::move(v); BindParam(0, _id); };
    	void Out_Id(OUT int32& v) { BindCol(0, v); };
    	template<int32 N> void Out_Msg(OUT WCHAR(&v)[N]) { BindCol(1, v); };
    	template<int32 N> void Out_Name(OUT WCHAR(&v)[N]) { BindCol(2, v); };

    private:
    	int32 _id = {};
    };

    class GetAllMsg : public DBBind<1,3>
    {
    public:
    	GetAllMsg(DBConnection& conn) : DBBind(conn, L"{CALL dbo.spGetAllMsg(?)}") { }
    	void In_Id(int32& v) { BindParam(0, v); };
    	void In_Id(int32&& v) { _id = std::move(v); BindParam(0, _id); };
    	void Out_Id(OUT int32& v) { BindCol(0, v); };
    	template<int32 N> void Out_Msg(OUT WCHAR(&v)[N]) { BindCol(1, v); };
    	template<int32 N> void Out_Name(OUT WCHAR(&v)[N]) { BindCol(2, v); };

    private:
    	int32 _id = {};
    };


     
};