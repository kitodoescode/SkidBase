#pragma once

#include "../../Miscellaneous/Includes.h"
#include "../../Execution/Execution.hpp"
#include "../../RBX/Roblox.hpp"
#include "../../Miscellaneous/Update/Offsets.hpp"

#define RegisterFunction(L, Func, Name) lua_pushcclosure(L, Func, Name, 0); \
lua_setglobal(L, Name);

#define RegisterMember(L, Func, Name) lua_pushcclosure(L, Func, Name, 0); \
lua_setfield(L, -2, Name);

inline int identifyexecutor(lua_State* L) {
    lua_pushstring(L, "SkidBase");
    lua_pushstring(L, "1.0.1");
    return 2;
}

inline int getexecutorname(lua_State* L) {
    lua_pushstring(L, "SkidBase");
    return 1;
}

inline int require(lua_State* L) {
    int type = lua_type(L, 1);
    if (type != LUA_TUSERDATA && type != LUA_TLIGHTUSERDATA) {
        lua_pushnil(L);
        return 1;
    }

    uintptr_t script = *(uintptr_t*)lua_touserdata(L, 1);

    if (!RBX::CheckMemory(script) || !RBX::CheckMemory(*(uintptr_t*)(script + Offsets::Instance::ClassDescriptor))) {
        lua_pushnil(L);
        return 1;
    }

    const char* className = *(const char**)(*(uintptr_t*)(script + Offsets::Instance::ClassDescriptor) + Offsets::Instance::ClassName);

    if (strcmp(className, "ModuleScript") == 0) {
        BYTE oldValue = *(BYTE*)(script + 0x70);
        *(BYTE*)(script + 0x70) = 0;

        lua_getglobal(L, "oldrequire");
        lua_pushvalue(L, 1);
        lua_call(L, 1, 1);

        *(BYTE*)(script + 0x70) = oldValue;

        return 1;
    }

    lua_getglobal(L, "oldrequire");
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);

    return 1;
}

namespace Library {
    namespace Misc {
        inline int getobjects(lua_State* L) {
            luaL_checktype(L, 2, LUA_TSTRING);

            std::string asset = lua_tostring(L, 2);

            lua_getglobal(L, "game");
            lua_getfield(L, -1, "GetService");
            lua_pushvalue(L, -2);
            lua_pushstring(L, "InsertService");
            lua_call(L, 2, 1);

            lua_getfield(L, -1, "LoadLocalAsset");
            lua_pushvalue(L, -2);
            lua_pushstring(L, asset.data());
            lua_call(L, 2, 1);

            lua_newtable(L);
            lua_pushvalue(L, -2);
            lua_rawseti(L, -2, 1);

            return 1;
        }

        inline void RegisterLibrary(lua_State* L) {
            RegisterFunction(L, identifyexecutor, "identifyexecutor");
            RegisterFunction(L, getexecutorname, "getexecutorname");
            //RegisterFunction(L, require, "require");
            //RegisterFunction(L, getobjects, "getobjects");
        }
    }
}