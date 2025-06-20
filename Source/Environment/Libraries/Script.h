#pragma once

#include "../../Miscellaneous/Includes.h"
#include "../../RBX/Roblox.hpp"

#include <lstate.h>
#include <lmem.h>
#include <lualib.h>
#include <lapi.h>

#include <crypt/sha3_384.h>
#include <crypt/sha384.h>
#include <crypt/sha256.h>
#include <crypt/base64.h>

struct lua_Page {
    lua_Page* listnext;
};

#define RegisterFunction(L, Func, Name) lua_pushcclosure(L, Func, Name, 0); \
lua_setglobal(L, Name);

#define RegisterMember(L, Func, Name) lua_pushcclosure(L, Func, Name, 0); \
lua_setfield(L, -2, Name);

inline int getscriptbytecode(lua_State* L) {
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

    bool decrypt = true;
    std::string bytecode = "";

    if (strcmp(className, OBF("LocalScript")) == 0) {
        uintptr_t bytecodePointer = *(uintptr_t*)(script + Offsets::Scripts::LocalScriptEmbedded);
        bytecode = *(std::string*)(bytecodePointer + 0x10);

        if (bytecode.size() <= 8) {
            bytecode = "";
            decrypt = false;

            lua_pushstring(L, OBF("Invalid bytecode size"));
        }
    }
    else if (strcmp(className, OBF("ModuleScript")) == 0) {
        uintptr_t bytecodePointer = *(uintptr_t*)(script + Offsets::Scripts::ModuleScriptEmbedded);
        bytecode = *(std::string*)(bytecodePointer + 0x10);

        if (bytecode.size() <= 8) {
            bytecode = "";
            decrypt = false;

            lua_pushstring(L, OBF("Invalid bytecode size"));
        }
    }
    else if (strcmp(className, OBF("Script")) == 0) {
        bytecode = "";
        decrypt = false;

        lua_pushstring(L, OBF("Scripts cannot be decompressed"));
    }
    else {
        bytecode = "";
        decrypt = false;

        lua_pushstring(L, OBF("Invalid Script"));
    }

    if (decrypt) {
        const auto decompressedBytecode = RBX::DecompressBytecode(bytecode);
        if (decompressedBytecode.empty()) {
            lua_pushstring(L, OBF("Invalid bytecode format"));
            return 1;
        }

        lua_pushlstring(L, decompressedBytecode.data(), decompressedBytecode.size());
    }

    return 1;
}

inline int getscriptclosure(lua_State* L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);

    const auto scriptPtr = *(uintptr_t*)lua_topointer(L, 1);
    std::string scriptCode = RBX::RequestBytecode(scriptPtr);

    lua_State* L2 = lua_newthread(L);
    luaL_sandboxthread(L2);

    uintptr_t Userdata = (uintptr_t)L2->userdata;
    if (!Userdata) {
        RBX::Print(3, "Invalid userdata in thread");
        lua_gc(L, LUA_GCRESTART, 0);
        return 0;
    }

    uintptr_t identityPtr = Userdata + Offsets::ExtraSpace::Identity;
    uintptr_t capabilitiesPtr = Userdata + Offsets::ExtraSpace::Capabilities;

    if (identityPtr && capabilitiesPtr) {
        *(uintptr_t*)identityPtr = 8;
        *(int64_t*)capabilitiesPtr = ~0ULL;
    }
    else {
        RBX::Print(3, "Invalid memory addresses for identity or capabilities");
        lua_gc(L, LUA_GCRESTART, 0);
        return 0;
    }

    lua_pushvalue(L, 1);
    lua_xmove(L, L2, 1);
    lua_setglobal(L2, "script");

    int result = RBX::LuaVM__Load(L2, &scriptCode, "", 0);

    if (result == LUA_OK) {
        Closure* cl = clvalue(luaA_toobject(L2, -1));

        if (cl) {
            Proto* p = cl->l.p;
            if (p) {
                Execution->SetProtoCapabilities(p);
            }
        }

        lua_pop(L2, lua_gettop(L2));
        lua_pop(L, lua_gettop(L));

        setclvalue(L, L->top, cl);
        incr_top(L);

        return 1;
    }

    lua_pushnil(L);
    return 1;
};

inline int getgc(lua_State* L) {
    bool IncludeTables = lua_gettop(L) ? luaL_optboolean(L, 1, 0) : false;

    lua_newtable(L);

    struct CGarbageCollector {
        lua_State* L;
        int IncludeTables;
        int Count;
    } Gct{ L, IncludeTables, 0 };

    luaM_visitgco(L, &Gct, [](void* Context, lua_Page* Page, GCObject* Gco) {
        auto Gct = static_cast<CGarbageCollector*>(Context);

        if (!((Gco->gch.marked ^ WHITEBITS) & otherwhite(Gct->L->global)))
            return false;

        auto tt = Gco->gch.tt;
        if (tt == LUA_TFUNCTION || tt == LUA_TUSERDATA || (Gct->IncludeTables && tt == LUA_TTABLE))
        {
            Gct->L->top->value.gc = Gco;
            Gct->L->top->tt = Gco->gch.tt;
            Gct->L->top++;

            lua_rawseti(Gct->L, -2, ++Gct->Count);
        }
        
        return false;
    });

    return 1;
}

inline int getgenv(lua_State* L) {
    const auto our_state = L;

    if (our_state == L) {
        lua_pushvalue(L, LUA_ENVIRONINDEX);
        return 1;
    }

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);
    luaC_threadbarrier(our_state);

    lua_pushvalue(our_state, LUA_ENVIRONINDEX);
    lua_xmove(our_state, L, 1);

    return 1;
}

inline int getreg(lua_State* L) {
    const auto our_state = L;

    if (our_state == L) {
        lua_pushvalue(L, LUA_REGISTRYINDEX);
        return 1;
    }

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);
    luaC_threadbarrier(our_state);

    lua_pushvalue(our_state, LUA_REGISTRYINDEX);
    lua_xmove(our_state, L, 1);

    return 1;
}

inline int gettenv(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTHREAD);
    lua_State* luaState = (lua_State*)lua_topointer(L, 1);
    LuaTable* table = hvalue(luaA_toobject(luaState, LUA_GLOBALSINDEX));

    sethvalue(L, L->top, table);
    L->top++;

    return 1;
};

inline int getrenv(lua_State* L) {
    const auto RobloxState = L->global->mainthread;

    if (!RobloxState->isactive)
        luaC_threadbarrier(RobloxState);

    lua_pushvalue(RobloxState, LUA_GLOBALSINDEX);
    lua_xmove(RobloxState, L, 1);

    lua_normalisestack(L, 0);
    lua_preparepushcollectable(L, 2);
    lua_createtable(L, 2, 2);
    lua_rawgeti(L, LUA_REGISTRYINDEX, 2);
    lua_setfield(L, -2, OBF("_G"));
    lua_rawgeti(L, LUA_REGISTRYINDEX, 4);
    lua_setfield(L, -2, OBF("shared"));

    return 1;
}

inline int getsenv(lua_State* L) {
    luaL_checktype(L, 1, LUA_TUSERDATA);

    uintptr_t script = *(uintptr_t*)lua_touserdata(L, 1);

    if (!RBX::CheckMemory(script) || !RBX::CheckMemory(*(uintptr_t*)(script + Offsets::Instance::ClassDescriptor))) {
        lua_pushnil(L);
        return 1;
    }

    const char* className = *(const char**)(*(uintptr_t*)(script + Offsets::Instance::ClassDescriptor) + Offsets::Instance::ClassName);

    bool decrypt = true;
    std::string bytecode = "";

    if (strcmp(className, "LocalScript") != 0) {
        luaL_argerror(L, 1, "localscript");
    }

    const auto script_node = *reinterpret_cast<uintptr_t*>(script + Offsets::Scripts::weak_thread_node);
    const auto node_weak_thread_ref = script_node ? *reinterpret_cast<uintptr_t*>(script_node + Offsets::Scripts::weak_thread_ref) : NULL;
    const auto live_thread_ref = node_weak_thread_ref ? *reinterpret_cast<uintptr_t*>(node_weak_thread_ref + Offsets::Scripts::weak_thread_ref_live) : NULL;
    const auto script_thread = live_thread_ref ? *reinterpret_cast<lua_State**>(live_thread_ref + Offsets::Scripts::weak_thread_ref_live_thread) : NULL;

    if (!script_thread)
        luaL_error(L, "could not get script environment - localscript not running");

    lua_pushvalue(script_thread, LUA_GLOBALSINDEX);
    lua_xmove(script_thread, L, 1);

    return 1;
}

inline int getscripthash(lua_State* L) {
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

    if (strcmp(className, "LocalScript") == 0) {
        uintptr_t bytecodePointer = *(uintptr_t*)(script + Offsets::Scripts::LocalScriptEmbedded);
        std::string bytecode = *(std::string*)(bytecodePointer + 0x10);

        std::string hashed = sha384(bytecode);

        lua_pushlstring(L, hashed.data(), hashed.size());
        return 1;
    }
    else if (strcmp(className, "ModuleScript") == 0) {
        uintptr_t bytecodePointer = *(uintptr_t*)(script + Offsets::Scripts::ModuleScriptEmbedded);
        std::string bytecode = *(std::string*)(bytecodePointer + 0x10);

        std::string hashed = sha384(bytecode);

        lua_pushlstring(L, hashed.data(), hashed.size());
        return 1;
    }

    lua_pushnil(L);

    return 1;
}

inline int getfunctionhash(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    Closure* cl = (Closure*)lua_topointer(L, 1);

    // closure info
    uint8_t nupvalues = cl->nupvalues;
    Proto* p = (Proto*)cl->l.p;

    std::string result =
        std::to_string((int)p->sizep) + "," +
        std::to_string((int)p->sizelocvars) + "," +
        std::to_string((int)p->sizeupvalues) + "," +
        std::to_string((int)p->sizek) + "," +
        std::to_string((int)p->sizelineinfo) + "," +
        std::to_string((int)p->linegaplog2) + "," +
        std::to_string((int)p->linedefined) + "," +
        std::to_string((int)p->bytecodeid) + "," +
        std::to_string((int)p->sizetypeinfo) + "," +
        std::to_string(nupvalues);

    std::string hash = SHA256::hash(result);

    lua_pushstring(L, hash.c_str());

    return 1;
}

inline HINTERNET hInternet = nullptr;
inline HINTERNET hConnect = nullptr;

inline int decompile(lua_State* L) {
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

    bool decrypt = true;
    std::string bytecode = "";

    if (strcmp(className, "LocalScript") == 0) {
        uintptr_t bytecodePointer = *(uintptr_t*)(script + Offsets::Scripts::LocalScriptEmbedded);
        bytecode = *(std::string*)(bytecodePointer + 0x10);

        if (bytecode.size() <= 8) {
            bytecode = "";
            decrypt = false;
        }
    }
    else if (strcmp(className, "ModuleScript") == 0) {
        uintptr_t bytecodePointer = *(uintptr_t*)(script + Offsets::Scripts::ModuleScriptEmbedded);
        bytecode = *(std::string*)(bytecodePointer + 0x10);

        if (bytecode.size() <= 8) {
            bytecode = "";
            decrypt = false;
        }
    }
    else if (strcmp(className, "Script") == 0) {
        bytecode = "";
        decrypt = false;
    }
    else {
        bytecode = "";
        decrypt = false;
    }

    if (!decrypt) {
        lua_pushnil(L);
        return 1;
    }

    const auto decompressedBytecode = RBX::DecompressBytecode(bytecode);
    if (decompressedBytecode.empty()) {
        lua_pushstring(L, "Invalid bytecode format");
        return 1;
    }

    if (!hInternet) {
        hInternet = InternetOpenA("HTTP Client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) return 0;
    }

    if (!hConnect) {
        hConnect = InternetConnectA(hInternet, "127.0.0.1", 9002, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (!hConnect) return 0;
    }

    const char* headers = "Content-Type: text/plain\r\n";

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", NULL, NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0);
    if (!hRequest) {
        lua_pushstring(L, "Failed to open HTTP request");
        return 1;
    }

    std::string decompiledresult = base64_encode(decompressedBytecode);

    BOOL sendResult = HttpSendRequestA(hRequest, headers, (DWORD)strlen(headers), (LPVOID)decompiledresult.c_str(), (DWORD)decompiledresult.size());
    if (!sendResult) {
        lua_pushstring(L, "Failed to send HTTP request");
        InternetCloseHandle(hRequest);
        return 1;
    }

    std::vector<char> responseBuffer;
    char buffer[8192];
    DWORD bytesRead;

    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        responseBuffer.insert(responseBuffer.end(), buffer, buffer + bytesRead);
    }

    InternetCloseHandle(hRequest);

    lua_pushlstring(L, responseBuffer.data(), responseBuffer.size());

    return 1;
}

inline int getidentity(lua_State* L) {
    lua_pushnumber(L, RBX::GetThreadIdentity(reinterpret_cast<uintptr_t>(L)));
    return 1;
}

inline uintptr_t IdentityToCapabilities(uintptr_t Identity) {
    uintptr_t obfuscated = (Identity ^ 0xABC) & 0xFFFF;
    uintptr_t result = 0;

    struct Pair {
        uintptr_t key;
        uintptr_t value;
    };

    static const Pair mapping[] = {
        { (4 ^ 0xABC) & 0xFFFF,  0x2000000000000003LL },
        { (3 ^ 0xABC) & 0xFFFF,  0x200000000000000BLL },
        { (5 ^ 0xABC) & 0xFFFF,  0x2000000000000001LL },
        { (6 ^ 0xABC) & 0xFFFF,  0x600000000000000BLL },
        { (8 ^ 0xABC) & 0xFFFF,  0x200000000000003FLL },
        { (9 ^ 0xABC) & 0xFFFF,  12LL },
        { (10 ^ 0xABC) & 0xFFFF, 0x6000000000000003LL },
        { (11 ^ 0xABC) & 0xFFFF, 0x2000000000000000LL },
    };

    for (auto& pair : mapping) {
        if (pair.key == obfuscated) {
            result = pair.value;
            break;
        }
    }

    return (result ? result : 0) | 0xFFFFFFFFFFFFFFF0uLL;
}

inline int64_t rbx_get_identity_struct(int64_t a1)
{
    int64_t v2;
    int64_t v3;
    int64_t v4;
    int64_t v5;
    int64_t v6;

    v2 = reinterpret_cast<int64_t(__fastcall*)()>(Offsets::GetCurrentThreadId)();

    v3 = v2 + 1008;

    v4 = v2 + *reinterpret_cast<uint16_t*>(a1 + 26);

    uint16_t bitField = *reinterpret_cast<uint16_t*>(a1 + 24);
    int64_t arrayIndex = bitField >> 6;
    int bitPosition = bitField & 0x3F;
    int64_t* bitArray = reinterpret_cast<int64_t*>(v3);
    if ((((bitArray[arrayIndex]) >> bitPosition) & 1) == 0)
    {
        using FuncPtr = void (*)(int64_t, int64_t);
        FuncPtr functionPointer = *reinterpret_cast<FuncPtr*>(a1 + 8);
        functionPointer(v4, *reinterpret_cast<int64_t*>(a1));

        v5 = v3 + 8 * arrayIndex;
        v6 = *reinterpret_cast<int64_t*>(v5);
        _bittestandset64(reinterpret_cast<long long*>(&v6), bitPosition);
        *reinterpret_cast<int64_t*>(v5) = v6;
    }

    return v4;
}

inline int setidentity(lua_State* L) {
    luaL_checktype(L, 1, LUA_TNUMBER);

    uintptr_t userData = (uintptr_t)L->userdata;

    uintptr_t identityStruct = RBX::GetIdentityStruct(*reinterpret_cast<uintptr_t*>(Offsets::IdentityPtr));
    if (!identityStruct)
        return 0;

    int identity = lua_tonumber(L, 1);
    *reinterpret_cast<uintptr_t*>(userData + Offsets::ExtraSpace::Identity) = identity;
    *reinterpret_cast<uintptr_t*>(userData + Offsets::ExtraSpace::Capabilities) = IdentityToCapabilities(identity);

    *reinterpret_cast<int32_t*>(identityStruct) = (int32_t)(identity);
    *reinterpret_cast<uintptr_t*>(identityStruct + 32) = IdentityToCapabilities(identity);

    return 0;
}

namespace Library {
    namespace Script {
        inline void RegisterLibrary(lua_State* L) {
            RegisterFunction(L, getscriptbytecode, "getscriptbytecode");
            RegisterFunction(L, getscriptbytecode, "dumpstring");

            RegisterFunction(L, getgenv, "getgenv");
            RegisterFunction(L, getsenv, "getsenv");
            RegisterFunction(L, getreg, "getreg");

            /*RegisterFunction(L, getscriptclosure, "getscriptclosure");
            RegisterFunction(L, getgc, "getgc");
            RegisterFunction(L, gettenv, "gettenv");
            RegisterFunction(L, getrenv, "getrenv");
            RegisterFunction(L, getscripthash, "getscripthash");
            RegisterFunction(L, getfunctionhash, "getfunctionhash");
            RegisterFunction(L, getfunctionhash, "getfunctionbytecode");
            RegisterFunction(L, decompile, "decompile");*/

            RegisterFunction(L, getidentity, "getidentity");
            RegisterFunction(L, getidentity, "getthreadidentity");
            RegisterFunction(L, getidentity, "getthreadcontext");

            RegisterFunction(L, setidentity, "setidentity");
            RegisterFunction(L, setidentity, "setthreadidentity");
            RegisterFunction(L, setidentity, "setthreadcontext");
        }
    }
}