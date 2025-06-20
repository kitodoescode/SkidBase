#pragma once

#include "../../Miscellaneous/Includes.h"

#define RegisterFunction(L, Func, Name) lua_pushcclosure(L, Func, Name, 0); \
lua_setglobal(L, Name);

#define RegisterMember(L, Func, Name) lua_pushcclosure(L, Func, Name, 0); \
lua_setfield(L, -2, Name);

inline int isreadonly(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    const LuaTable* table = hvalue(luaA_toobject(L, 1));

    lua_pushboolean(L, static_cast<bool>(table->readonly));
    return 1;
}

inline int setreadonly(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TBOOLEAN);

    bool value = lua_toboolean(L, 2);
    LuaTable* table = hvalue(luaA_toobject(L, 1));

    table->readonly = value;
    return 0;
}

inline int makereadonly(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    LuaTable* table = hvalue(luaA_toobject(L, 1));
    
    table->readonly = true;
    return 0;
}

inline int makewriteable(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);

    LuaTable* table = hvalue(luaA_toobject(L, 1));

    table->readonly = false;
    return 0;
}

inline int getrawmetatable(lua_State* L) {
    luaL_checkany(L, 1);

    if (!lua_getmetatable(L, 1))
        lua_pushnil(L);

    return 1;
}

inline int setrawmetatable(lua_State* L) {
    //luaL_argexpected(L, lua_istable(L, 1) || lua_isuserdata(L,1), 1, OBF("table or userdata"));

    luaC_threadbarrier(L);
    LuaTable* mt = NULL;
    const TValue* obj = luaA_toobject(L, 1);
    switch (ttype(obj))
    {
    case LUA_TTABLE:
        mt = hvalue(obj)->metatable;
        break;
    case LUA_TUSERDATA:
        mt = uvalue(obj)->metatable;
        break;
    default:
        mt = L->global->mt[ttype(obj)];
        break;
    }

    luaL_argexpected(L, mt != NULL, 1, OBF("object with metatable"));
    luaL_argexpected(L, lua_istable(L, 2) || lua_isuserdata(L, 2) || lua_isnil(L, 2), 2, OBF("table, userdata or nil"));

    lua_setmetatable(L, 1);

    // lua_pushvalue(L, 1);
    return 0;
}

extern int hookfunction(lua_State* L);

inline int hookmetamethod(lua_State* L) {
    luaL_checkany(L, 1);
    luaL_checktype(L, 2, LUA_TSTRING);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    if (!luaL_getmetafield(L, 1, lua_tolstring(L, 2, nullptr)))
        luaL_argerrorL(L, 2, OBF("metamethod does not exist"));

    const TValue* t_val = luaA_toobject(L, -1);
    if (t_val->tt != LUA_TFUNCTION)
        luaL_argerrorL(L, 2, OBF("metamethod is not a function"));

    Closure* meta_method_closure = clvalue(t_val);

    lua_rawcheckstack(L, 3);
    luaC_threadbarrier(L);
    lua_pushcclosurek(L, hookfunction, nullptr, 0, nullptr);

    luaC_threadbarrier(L);
    lua_pushvalue(L, -2);

    lua_remove(L, -3);

    luaC_threadbarrier(L);
    lua_pushvalue(L, 3);

    lua_call(L, 2, 1);

    return 1;
}

namespace Library {
    namespace MetaTable {
        inline void RegisterLibrary(lua_State* L) {
            RegisterFunction(L, isreadonly, "isreadonly");
            RegisterFunction(L, setreadonly, "setreadonly");
            RegisterFunction(L, makereadonly, "makereadonly");
            RegisterFunction(L, makereadonly, "make_readonly");
            RegisterFunction(L, makewriteable, "makewriteable");
            RegisterFunction(L, makewriteable, "make_writeable");
            RegisterFunction(L, getrawmetatable, "getrawmetatable");
            RegisterFunction(L, setrawmetatable, "setrawmetatable");
            RegisterFunction(L, hookmetamethod, "hookmetamethod");
        }
    }
}