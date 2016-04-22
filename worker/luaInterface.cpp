#include "worker.h"
#ifdef ENABLE_LUA
#include "luaInterface.h"
#include "dialog.h"
#include "asm.h"
#include <vector>

using namespace std;

#pragma comment(lib, "lua51.lib") //luajit

#define THROW_EXCEPTION(str) lua_pushstring(L, str);lua_error(L);return 0;
#define CHECK_ARGS_COUNT(cnt,rc)\
{\
    if(rc!=(cnt))\
    {\
        THROW_EXCEPTION("need " #cnt " arguments!");\
    }\
}
#define CHECK_ARGS_MINCOUNT(cnt,rc)\
{\
    if(rc<(cnt))\
    {\
        THROW_EXCEPTION("need at least " #cnt " arguments!");\
    }\
}

static const wchar_t* ToWString(const char* s)
{
    static wchar_t ws[1000];
    wcscpy(ws, L"convert faild");
    if(s) MultiByteToWideChar(CP_UTF8, 0, s, -1, ws, 1000);
    return ws;
}

static int Print(lua_State* L)
{
    bool first = true;
    int cnt = lua_gettop(L);
    for (int i = 0;i < cnt;i++)
    {
        auto str = lua_tostring(L, i + 1);
        if (!str)
            break;

        if (first)
        {
            first = false;
        }
        else
        {
            OutputWriter::OutputInfo(L" ");
        }
        OutputWriter::OutputInfo(L"%s", ToWString(str));
    }
    OutputWriter::OutputInfo(L"\r\n");
    return 0;
}

template<typename T>
static int ReadUint(lua_State* L)
{
    int cnt = lua_gettop(L);
    CHECK_ARGS_MINCOUNT(1, cnt);
    lua_Integer addr = lua_tointeger(L, 1);
    __try
    {
        lua_pushinteger(L, *(T*)(DWORD)addr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        THROW_EXCEPTION("mem access error!");
    }
    return 1;
}

static int CallFunction(lua_State* L)
{
    int cnt = lua_gettop(L);
    CHECK_ARGS_MINCOUNT(3, cnt);
    auto funcAddr = lua_tointeger(L, 1);
    auto callType = (FunctionCallType)lua_tointeger(L, 2);
    if (!lua_istable(L, 3))
    {
        THROW_EXCEPTION("arg 3 must be a register table!");
    }
    Registers regs;
    DWORD regFlags = 0;
    vector<DWORD> arguments;

    for (int i = 0;i < 8;i++)
    {
        lua_pushinteger(L, i + 1); //reg index from 1
        lua_gettable(L, 3);
        if (!lua_isnil(L, -1))
        {
            regFlags |= (1 << i);
            *((DWORD*)&regs + (i + 1)) = lua_tointeger(L, -1);
        }
        lua_pop(L, 1);
    }

    if (cnt >= 4)
    {
        if (!lua_istable(L, 4))
        {
            THROW_EXCEPTION("arg 4 must be a arguments table!");
        }

        for (int i = 0;i < 20;i++) //max 20 arguments
        {
            lua_pushinteger(L, i + 1);
            lua_gettable(L, 4);
            if (!lua_isnil(L, -1))
            {
                arguments.push_back((DWORD)lua_tointeger(L, -1));
            }
            else
            {
                lua_pop(L, 1);
                break;
            }
            lua_pop(L, 1);
        }
    }
    ReturnValues callRetVal = { 0 };
    auto callRslt = CallFunction(funcAddr, callType, arguments, regFlags, &regs, &callRetVal);
    if (!callRslt)
    {
        THROW_EXCEPTION("exception occured.");
    }
    else
    {
        lua_pushinteger(L, callRetVal.eax);
        lua_pushinteger(L, callRetVal.edx);
        lua_pushnumber(L, callRetVal.st0);
        lua_pushinteger(L, callRetVal.xmm0);
    }
    return 4;
}

void InitLuajit(lua_State* L)
{
    struct FuncPair
    {
        char* funcName;
        lua_CFunction func;
    } funcList[] =
    {
        { "print", Print },
        { "u8", ReadUint<BYTE> },
        { "u16", ReadUint<WORD> },
        { "u32", ReadUint<DWORD> },
        { "_CallFunction", CallFunction },
    };
    
    for (int i = 0;i < sizeof(funcList) / sizeof(funcList[0]);i++)
    {
        lua_register(L, funcList[i].funcName, funcList[i].func);
    }
}

//script is a utf8 string which ends with zero
char* ExecuteLuaStringWithRet(lua_State* L, char* script)
{
    int lastTop = lua_gettop(L);
    char* retStr = nullptr;

    do 
    {
        int err = luaL_loadstring(L, script);
        if (err)
        {
            OutputWriter::OutputInfo(L"parse error, code:%d, msg:%s\r\n", err, ToWString(lua_tostring(L, -1)));
            break;
        }
        err = lua_pcall(L, 0, -1, 0);
        if (err)
        {
            OutputWriter::OutputInfo(L"execute error: code:%d, msg:%s\r\n", err, ToWString(lua_tostring(L, -1)));
            break;
        }
        int cnt = lua_gettop(L);
        if (cnt > lastTop)
        {
            const char* ret = lua_tostring(L, -1);
            if (!ret)
                break;
            int len = strlen(ret);
            retStr = new char[len + 1];
            if (retStr)
            {
                memcpy(retStr, ret, len + 1);
            }
        }
    } while (false);
    lua_settop(L, lastTop);
    return retStr;
}

void ExecuteLuaString(lua_State* L, char* script)
{
    auto ret = ExecuteLuaStringWithRet(L, script);
    if (ret)
    {
        OutputWriter::OutputInfo("%s\r\n", ret);
        delete[] ret;
    }
}


#endif