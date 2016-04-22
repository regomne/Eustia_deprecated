#pragma once

#include <lua.hpp>

void ExecuteLuaString(lua_State* L, char* script);
char* ExecuteLuaStringWithRet(lua_State* L, char* script);
void InitLuajit(lua_State* L);
