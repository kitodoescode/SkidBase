#include "Execution.hpp"
#include "../Miscellaneous/Scheduler/Scheduler.hpp"

std::string CExecution::CompileScript(const std::string Source) { 

    std::string Bytecode = Luau::compile(Source, { 2, 1, 2 }, { true, true }, &Encoder);

    size_t DataSize = Bytecode.size();
    size_t MaxSize = ZSTD_compressBound(DataSize);
    std::vector<char> Buffer(MaxSize + 8);

    memcpy(Buffer.data(), "RSB1", 4);
    memcpy(Buffer.data() + 4, &DataSize, sizeof(DataSize));

    size_t CompressedSize = ZSTD_compress(Buffer.data() + 8, MaxSize, Bytecode.data(), DataSize, ZSTD_maxCLevel());
    size_t TotalSize = CompressedSize + 8;

    uint32_t Key = XXH32(Buffer.data(), TotalSize, 42);
    uint8_t* KeyBytes = (uint8_t*)&Key;

    for (size_t i = 0; i < TotalSize; ++i) Buffer[i] ^= KeyBytes[i % 4] + i * 41;

    return std::string(Buffer.data(), TotalSize);
}

void CExecution::SetProtoCapabilities(Proto* Proto) {
    Proto->userdata = &MaxCaps;
    for (int i = 0; i < Proto->sizep; i++)
    {
        SetProtoCapabilities(Proto->p[i]);
    }
}


lua_State* CExecution::NewThread(lua_State* L) {
    if (L == nullptr || L->tt != LUA_TTHREAD)
        return nullptr;

    luaC_checkGC(L);
    luaC_threadbarrier(L);

    int StackCount = lua_gettop(L);
    if (StackCount > 0)
        lua_pop(L, StackCount);

    lua_State* L1 = luaE_newthread(L);
    if (L1 == nullptr)
        return nullptr;

    if (L1->tt != LUA_TTHREAD)
        return nullptr;

    setthvalue(L, L->top, L1);
    incr_top(L);

    global_State* G = L->global;
    if (G->cb.userthread != nullptr)
        G->cb.userthread(L, L1);

    return L1;
}

void CExecution::Execute(lua_State* state, std::string Source)
{
    if (Source.empty() || !state) return;

    std::string Script = CompileScript(Source);

    if (Script[0] == '\0' || Script.empty()) {
        RBX::Print(3, "Failed to compile script");
        return;
    }

    lua_settop(state, 0);
    lua_gc(state, LUA_GCSTOP, 0);

    lua_State* Thread = lua_newthread(state);
    luaL_sandboxthread(Thread);
    if (!Thread) {
        RBX::Print(3, "Failed to create Lua thread");
        lua_gc(state, LUA_GCRESTART, 0);
        return;
    }

    lua_settop(Thread, 0);

    lua_getglobal(Thread, "task");
    if (lua_isnil(Thread, -1)) {
        RBX::Print(3, "Global 'task' not found");
        lua_gc(state, LUA_GCRESTART, 0);
        return;
    }
    lua_getfield(Thread, -1, "defer");

    uintptr_t Userdata = (uintptr_t)Thread->userdata;
    if (!Userdata) {
        RBX::Print(3, "Invalid userdata in thread");
        lua_gc(state, LUA_GCRESTART, 0);
        return;
    }

    uintptr_t identityPtr = Userdata + Offsets::ExtraSpace::Identity;
    uintptr_t capabilitiesPtr = Userdata + Offsets::ExtraSpace::Capabilities;

    if (identityPtr && capabilitiesPtr) {
        *(uintptr_t*)identityPtr = 8;
        *(int64_t*)capabilitiesPtr = ~0ULL;
    }
    else {
        RBX::Print(3, "Invalid memory addresses for identity or capabilities");
        lua_gc(state, LUA_GCRESTART, 0);
        return;
    }

    int LoadResult = RBX::LuaVM__Load(Thread, &Script, "@SkidBase", 0);
    if (LoadResult != LUA_OK) {
        std::string Error = luaL_checklstring(Thread, -1, nullptr);
        lua_pop(Thread, 1);

        RBX::Print(3, Error.data());
        lua_gc(state, LUA_GCRESTART, 0);
        return;
    }

    Closure* closure = clvalue(luaA_toobject(Thread, -1));
    if (!closure) {
        RBX::Print(3, "Failed to retrieve closure");
        lua_gc(state, LUA_GCRESTART, 0);
        return;
    }

    SetProtoCapabilities(closure->l.p);

    if (lua_pcall(Thread, 1, 0, 0) != LUA_OK) {
        std::string Error = luaL_checklstring(Thread, -1, nullptr);
        lua_pop(Thread, 1);
        RBX::Print(3, Error.data());
        lua_gc(state, LUA_GCRESTART, 0);
        return;
    }

    lua_pop(Thread, 1);

    lua_gc(state, LUA_GCRESTART, 0);
}