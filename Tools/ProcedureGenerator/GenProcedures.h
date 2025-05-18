#pragma once
#include "Types.h"
#include <windows.h>
#include "DBBind.h"

namespace SP
{
	
    class  : public DBBind<2,0>
    {
    public:
    	(DBConnection& conn) : DBBind(conn, L"{CALL dbo.sp(?,?)}") { }
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

    class  : public DBBind<1,0>
    {
    public:
    	(DBConnection& conn) : DBBind(conn, L"{CALL dbo.sp(?)}") { }
    	void In_Id(int32& v) { BindParam(0, v); };
    	void In_Id(int32&& v) { _id = std::move(v); BindParam(0, _id); };

    private:
    	int32 _id = {};
    };

    class  : public DBBind<1,0>
    {
    public:
    	(DBConnection& conn) : DBBind(conn, L"{CALL dbo.sp(?)}") { }
    	void In_Id(int32& v) { BindParam(0, v); };
    	void In_Id(int32&& v) { _id = std::move(v); BindParam(0, _id); };

    private:
    	int32 _id = {};
    };


     
};