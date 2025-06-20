#pragma once

#include "../../Miscellaneous/Includes.h"
#include "../../Miscellaneous/Update/Offsets.hpp"
#include "../../Execution/Execution.hpp"

#define RegisterFunction(L, Func, Name) lua_pushcclosure(L, Func, Name, 0); \
lua_setglobal(L, Name);

#define RegisterMember(L, Func, Name) lua_pushcclosure(L, Func, Name, 0); \
lua_setfield(L, -2, Name);

inline int loadstring(lua_State* L) {
    luaL_checktype(L, 1, LUA_TSTRING);
    auto source = lua_tostring(L, 1);
    lua_pop(L, 1); // for optimization (optional)
    auto chunk_name = luaL_optstring(L, 2, "@SkidBase");
    lua_pop(L, 1); // for optimization (optional)

    auto compressed_bytecode = Execution->CompileScript(source);

    if (RBX::LuaVM__Load(L, &compressed_bytecode, chunk_name, 0) != LUA_OK) {
        std::string Error_Message = luaL_checklstring(L, -1, nullptr);
        //lua_pop(L, 1); // add back if removing the first 2 pops
        lua_pushnil(L);
        lua_pushstring(L, Error_Message.data());
        return 2;
    }

    Closure* Function = lua_toclosure(L, -1);
    if (Function && Function->l.p) {
        Execution->SetProtoCapabilities(Function->l.p);
    }

    luaA_pushobject(L, luaA_toobject(L, -1)); // optional

    lua_setsafeenv(L, LUA_GLOBALSINDEX, false);
    return 1;
};

inline int iscclosure(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushboolean(L, lua_iscfunction(L, 1));
    return 1;
}

inline int islclosure(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushboolean(L, lua_isLfunction(L, 1));
    return 1;
}

inline int isexecutorclosure(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    if (Closure* cl = clvalue(luaA_toobject(L, 1)); cl->isC) {
        lua_pushboolean(L, cl->c.debugname == nullptr);
        return 1;
    }
    else {
        lua_pushboolean(L, cl->l.p->linedefined == -1);
        return 1;
    }
}

inline int checkcaller(lua_State* L) {
    lua_pushboolean(L, L->userdata->Script.expired() || L->userdata->Script.lock() == nullptr);
    return 1;
}

inline std::map<Closure*, Closure*> newcc_map;
inline std::map<Closure*, Closure*> hooks_map;
inline std::uintptr_t max_caps = 0xFFFFFFFFFFFFFFFF;

inline void set_closure_capabilities(Proto* proto, uintptr_t* caps) {
    if (!proto)
        return;

    proto->userdata = caps;
    for (int i = 0; i < proto->sizep; ++i)
        set_closure_capabilities(proto->p[i], caps);
}

inline static void handler_run(lua_State* L, void* ud) {
    luaD_call(L, (StkId)(ud), LUA_MULTRET);
}

inline std::string strip_error_message(const std::string& message) {
    static auto callstack_regex = std::regex(R"(.*"\]:(\d)*: )", std::regex::optimize | std::regex::icase);
    if (std::regex_search(message.begin(), message.end(), callstack_regex)) {
        const auto fixed = std::regex_replace(message, callstack_regex, "");
        return fixed;
    }

    return message;
};


inline int newcc_proxy(lua_State* L) {
    const auto closure = newcc_map.contains(clvalue(L->ci->func)) ? newcc_map.at(clvalue(L->ci->func)) : nullptr;

    if (!closure)
        luaL_error(L, OBF("unable to find closure"));

    setclvalue(L, L->top, closure);
    L->top++;

    lua_insert(L, 1);

    StkId function = L->base;
    L->ci->flags |= LUA_CALLINFO_HANDLE;

    L->baseCcalls++;
    int status = luaD_pcall(L, handler_run, function, savestack(L, function), 0);
    L->baseCcalls--;

    if (status == LUA_ERRRUN) {
        const auto regexed_error = strip_error_message(lua_tostring(L, -1));
        lua_pop(L, 1);

        lua_pushlstring(L, regexed_error.c_str(), regexed_error.size());
        lua_error(L);
        return 0;
    }

    expandstacklimit(L, L->top);

    if (status == 0 && (L->status == LUA_YIELD || L->status == LUA_BREAK))
        return -1;

    return lua_gettop(L);
};

inline int newcc_continuation(lua_State* L, int Status) {
    if (Status != LUA_OK) {
        const auto regexed_error = strip_error_message(lua_tostring(L, -1));
        lua_pop(L, 1);

        lua_pushlstring(L, regexed_error.c_str(), regexed_error.size());
        lua_error(L);
    }

    return lua_gettop(L);
};

inline int wrap_closure(lua_State* L, int index) {
    luaL_checktype(L, index, LUA_TFUNCTION);

    lua_ref(L, index);
    lua_pushcclosurek(L, newcc_proxy, nullptr, 0, newcc_continuation);
    lua_ref(L, -1);

    newcc_map[clvalue(luaA_toobject(L, -1))] = clvalue(luaA_toobject(L, index));

    return 1;
};

inline int newlclosure(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    /// if (!lua_iscfunction(L, -1))
    ///     lua_ref(L, -1);

    lua_rawcheckstack(L, 3);
    luaC_threadbarrier(L);

    lua_newtable(L);
    lua_newtable(L); // Meta

    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);

    Closure* closure = clvalue(luaA_toobject(L, -3));

    L->top->value.p = closure->env;
    L->top->tt = ::lua_Type::LUA_TTABLE;
    L->top++;
    lua_setfield(L, -2, OBF("__index"));
    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);
    L->top->value.p = closure->env;
    L->top->tt = ::lua_Type::LUA_TTABLE;
    L->top++;
    lua_setfield(L, -2, OBF("__newindex"));
    lua_setreadonly(L, -1, true);
    lua_setmetatable(L, -2);

    lua_pushvalue(L, -2);
    lua_rawsetfield(L, -2, OBF("new_l_closure_wrap"));

    std::string bytecode = Execution->CompileScript(OBF("return new_l_closure_wrap(...)"));
    RBX::LuaVM__Load(L, &bytecode, OBF("="), -1);

    set_closure_capabilities(clvalue(luaA_toobject(L, -1))->l.p, &max_caps);

    lua_remove(L, lua_gettop(L) - 1); // Balance lua stack.
    return 1;
}

inline int newcclosure(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    if (!lua_iscfunction(L, 1))
        return wrap_closure(L, 1);

    lua_pushnil(L);
    return 1;
};

enum closure_type_t {
    lclosure, cclosure,
    newcc, unidentified
};

inline closure_type_t identify_closure(Closure* closure) {
    if (!closure->isC)
        return closure_type_t::lclosure;
    else if (closure->c.f == newcc_proxy)
        return closure_type_t::newcc;
    else if (closure->isC)
        return closure_type_t::cclosure;
    else
        return closure_type_t::unidentified;
}

inline int clonefunction(lua_State* L) {

    luaL_checktype(L, 1, LUA_TFUNCTION);

    Closure* cl = clvalue(luaA_toobject(L, 1));

    closure_type_t closure_type = identify_closure(cl);

    if (closure_type == cclosure) {
        lua_clonecfunction(L, 1);
    }
    else if (closure_type == lclosure) {
        lua_clonefunction(L, 1);
    }
    else if (closure_type == newcc) {
        const auto unwrapped = newcc_map.contains(cl) ? newcc_map.at(cl) : nullptr;

        if (unwrapped) {
            lua_checkstack(L, 1);

            lua_pushcclosurek(L, newcc_proxy, nullptr, 0, newcc_continuation);
            lua_ref(L, -1);

            newcc_map[clvalue(luaA_toobject(L, -1))] = unwrapped;
        }
        else
            lua_pushnil(L);
    }
    else {
        lua_pushnil(L);
    }

    return 1;
}

inline int hookfunction(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    Closure* original = clvalue(luaA_toobject(L, 1));
    Closure* hook = clvalue(luaA_toobject(L, 2));
    lua_ref(L, 1);
    lua_ref(L, 2);

    closure_type_t original_type = identify_closure(original);
    closure_type_t hook_type = identify_closure(hook);

    lua_rawcheckstack(L, 2);
    luaC_threadbarrier(L);
    lua_pushcclosurek(L, clonefunction, nullptr, 0, 0);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);
    lua_ref(L, -1);
    Closure* cloned = clvalue(luaA_toobject(L, -1));
    lua_pop(L, 1);


    // C CLOSURES
    if (original_type == cclosure && hook_type == cclosure) {
        // C->C
        if (hook->nupvalues > original->nupvalues)
            luaL_argerrorL(L, 2, OBF("hook has more upvalues than the function you are hooking"));

        for (int i = 0; i < hook->nupvalues; ++i)
            setobj2n(L, &original->c.upvals[i], &hook->c.upvals[i]);

        original->env = hook->env;
        original->stacksize = hook->stacksize;
        original->preload = hook->preload;

        original->c.cont = hook->c.cont;
        original->c.f = hook->c.f;

        lua_rawcheckstack(L, 1);
        luaC_threadbarrier(L);

        L->top->value.p = cloned;
        L->top->tt = LUA_TFUNCTION;
        L->top++;

        hooks_map[original] = clvalue(luaA_toobject(L, -1));
        return 1;
    }
    else if (original_type == cclosure && hook_type == lclosure) {
        // C->L
        original->c.f = newcc_proxy;
        original->c.cont = newcc_continuation;
        newcc_map[original] = hook;

        lua_rawcheckstack(L, 1);
        luaC_threadbarrier(L);

        L->top->value.p = cloned;
        L->top->tt = LUA_TFUNCTION;
        L->top++;

        hooks_map[original] = clvalue(luaA_toobject(L, -1));
        return 1;
    }

    // L CLOSURE
    else if (original_type == lclosure && hook_type == lclosure) {
        // L->L
        original->env = hook->env;
        original->nupvalues = hook->nupvalues;
        original->stacksize = hook->stacksize;
        original->preload = hook->preload;

        original->l.p = hook->l.p;

        for (int i = 0; i < hook->nupvalues; i++) {
            setobj2n(L, &original->l.uprefs[i], &hook->l.uprefs[i]);
        }

        lua_rawcheckstack(L, 1);
        luaC_threadbarrier(L);

        L->top->value.p = cloned;
        L->top->tt = LUA_TFUNCTION;
        L->top++;

        hooks_map[original] = clvalue(luaA_toobject(L, -1));
        return 1;
    }
    else if (original_type == lclosure && hook_type == cclosure) {
        // L->C
        lua_rawcheckstack(L, 2);
        luaC_threadbarrier(L);

        lua_pushcclosure(L, newlclosure, nullptr, 0);
        lua_pushvalue(L, 2);
        lua_call(L, 1, 1);

        Closure* new_l_closure = clvalue(luaA_toobject(L, -1));
        lua_pop(L, 1);

        original->env = new_l_closure->env;
        original->nupvalues = new_l_closure->nupvalues;
        original->stacksize = new_l_closure->stacksize;
        original->preload = new_l_closure->preload;

        original->l.p = new_l_closure->l.p;

        for (int i = 0; i < new_l_closure->nupvalues; i++) {
            setobj2n(L, &original->l.uprefs[i], &new_l_closure->l.uprefs[i]);
        }

        lua_rawcheckstack(L, 1);
        luaC_threadbarrier(L);

        L->top->value.p = cloned;
        L->top->tt = LUA_TFUNCTION;
        L->top++;

        hooks_map[original] = clvalue(luaA_toobject(L, -1));
        return 1;
    }
    else if (original_type == lclosure && hook_type == newcc) {
        if (!newcc_map.contains(hook))
            luaL_argerrorL(L, 2, OBF("failed to get unwrapped closure of hook"));

        Closure* non_wrapped = newcc_map.at(hook);

        lua_rawcheckstack(L, 2);
        luaC_threadbarrier(L);

        lua_pushcclosure(L, newlclosure, nullptr, 0);
        L->top->value.p = non_wrapped;
        L->top->tt = LUA_TFUNCTION;
        L->top++;
        lua_call(L, 1, 1);

        Closure* new_l_closure = clvalue(luaA_toobject(L, -1));
        lua_pop(L, 1);

        original->env = new_l_closure->env;
        original->nupvalues = new_l_closure->nupvalues;
        original->stacksize = new_l_closure->stacksize;
        original->preload = new_l_closure->preload;

        original->l.p = new_l_closure->l.p;

        for (int i = 0; i < new_l_closure->nupvalues; i++) {
            setobj2n(L, &original->l.uprefs[i], &new_l_closure->l.uprefs[i]);
        }

        lua_rawcheckstack(L, 1);
        luaC_threadbarrier(L);

        L->top->value.p = cloned;
        L->top->tt = LUA_TFUNCTION;
        L->top++;

        hooks_map[original] = clvalue(luaA_toobject(L, -1));
        return 1;
    }
    else if (original_type == newcc && hook_type == newcc) {
        if (!newcc_map.contains(hook))
            luaL_argerrorL(L, 2, OBF("failed to get unwrapped closure of hook"));

        Closure* non_wrapped = newcc_map.at(hook);

        newcc_map[original] = non_wrapped;

        lua_rawcheckstack(L, 1);
        luaC_threadbarrier(L);

        L->top->value.p = cloned;
        L->top->tt = LUA_TFUNCTION;
        L->top++;

        hooks_map[original] = clvalue(luaA_toobject(L, -1));
        return 1;
    }
    else if (original_type == newcc && hook_type == lclosure) {
        if (!newcc_map.contains(original))
            luaL_argerrorL(L, 2, OBF("failed to get unwrapped closure of original closure"));

        Closure* non_wrapped = newcc_map.at(original);

        newcc_map[original] = hook;

        lua_rawcheckstack(L, 1);
        luaC_threadbarrier(L);

        L->top->value.p = cloned;
        L->top->tt = LUA_TFUNCTION;
        L->top++;

        hooks_map[original] = clvalue(luaA_toobject(L, -1));
        return 1;
    }
    else if (original_type == newcc && hook_type == cclosure) {
        if (!newcc_map.contains(original))
            luaL_argerrorL(L, 2, OBF("failed to get unwrapped closure of original closure"));

        Closure* non_wrapped = newcc_map.at(original);

        newcc_map[original] = hook;

        lua_rawcheckstack(L, 1);
        luaC_threadbarrier(L);

        L->top->value.p = cloned;
        L->top->tt = LUA_TFUNCTION;
        L->top++;

        hooks_map[original] = clvalue(luaA_toobject(L, -1));
        return 1;
    }
    else if (original_type == cclosure && hook_type == newcc) {
        if (!newcc_map.contains(hook))
            luaL_argerrorL(L, 2, OBF("failed to get unwrapped closure of hook"));

        Closure* non_wrapped = newcc_map.at(hook);

        original->c.f = newcc_proxy;
        original->c.cont = newcc_continuation;

        newcc_map[original] = non_wrapped;

        lua_rawcheckstack(L, 1);
        luaC_threadbarrier(L);

        L->top->value.p = cloned;
        L->top->tt = LUA_TFUNCTION;
        L->top++;

        hooks_map[original] = clvalue(luaA_toobject(L, -1));
        return 1;
    }
    else {
        luaL_error(L, OBF("unsupported hooking pair"));
    }

    return 0;

}

inline int isfunctionhooked(lua_State* L) {

    luaL_checktype(L, 1, LUA_TFUNCTION);

    Closure* closure = clvalue(luaA_toobject(L, 1));

    lua_pushboolean(L, hooks_map.contains(closure));
    return 1;
}

inline int restorefunction(lua_State* L) {

    luaL_checktype(L, 1, LUA_TFUNCTION);

    Closure* original = clvalue(luaA_toobject(L, 1));

    if (!hooks_map.contains(original))
        luaL_argerrorL(L, 1, OBF("function is not hooked"));

    Closure* hook = hooks_map.at(original);
    hooks_map.erase(original);

    closure_type_t original_type = identify_closure(original);
    closure_type_t hook_type = identify_closure(hook);

    if (original_type == cclosure && hook_type == cclosure) {
        // C->C
        //if (hook->nupvalues > original->nupvalues)
        //    luaL_argerrorL(L, 2, OBF("hook has more upvalues than the function you are hooking"));

        for (int i = 0; i < hook->nupvalues; ++i)
            setobj2n(L, &original->c.upvals[i], &hook->c.upvals[i]);

        original->env = hook->env;
        original->stacksize = hook->stacksize;
        original->preload = hook->preload;

        original->c.cont = hook->c.cont;
        original->c.f = hook->c.f;

    }
    else if (original_type == cclosure && hook_type == lclosure) {
        // C->L
        original->c.f = newcc_proxy;
        original->c.cont = newcc_continuation;
        newcc_map[original] = hook;
    }

    // L CLOSURE
    else if (original_type == lclosure && hook_type == lclosure) {
        // L->L
        original->env = hook->env;
        original->nupvalues = hook->nupvalues;
        original->stacksize = hook->stacksize;
        original->preload = hook->preload;

        original->l.p = hook->l.p;

        for (int i = 0; i < hook->nupvalues; i++) {
            setobj2n(L, &original->l.uprefs[i], &hook->l.uprefs[i]);
        }
    }
    else if (original_type == lclosure && hook_type == cclosure) {
        // L->C
        lua_rawcheckstack(L, 2);
        luaC_threadbarrier(L);

        lua_pushcclosure(L, newlclosure, nullptr, 0);
        lua_pushvalue(L, 2);
        lua_call(L, 1, 1);

        Closure* new_l_closure = clvalue(luaA_toobject(L, -1));
        lua_pop(L, 1);

        original->env = new_l_closure->env;
        original->nupvalues = new_l_closure->nupvalues;
        original->stacksize = new_l_closure->stacksize;
        original->preload = new_l_closure->preload;

        original->l.p = new_l_closure->l.p;

        for (int i = 0; i < new_l_closure->nupvalues; i++) {
            setobj2n(L, &original->l.uprefs[i], &new_l_closure->l.uprefs[i]);
        }

    }
    else if (original_type == newcc && hook_type == newcc) {
        if (!newcc_map.contains(hook))
            luaL_argerrorL(L, 2, OBF("failed to get unwrapped closure of hook"));

        Closure* non_wrapped = newcc_map.at(hook);

        newcc_map[original] = non_wrapped;
    }
    else if (original_type == newcc && hook_type == lclosure) {
        if (!newcc_map.contains(original))
            luaL_argerrorL(L, 2, OBF("failed to get unwrapped closure of original closure"));

        Closure* non_wrapped = newcc_map.at(original);

        newcc_map[original] = hook;
    }
    else if (original_type == newcc && hook_type == cclosure) {
        if (!newcc_map.contains(original))
            luaL_argerrorL(L, 2, OBF("failed to get unwrapped closure of original closure"));

        Closure* non_wrapped = newcc_map.at(original);

        newcc_map[original] = hook;
    }
    else if (original_type == cclosure && hook_type == newcc) {
        if (!newcc_map.contains(hook))
            luaL_argerrorL(L, 2, OBF("failed to get unwrapped closure of hook"));

        Closure* non_wrapped = newcc_map.at(hook);

        original->c.f = newcc_proxy;
        original->c.cont = newcc_continuation;

        newcc_map[original] = non_wrapped;
    }
    else {
        /*
         *         const char *hook_type_1 = "";
                const char *hook_type_2 = "";

                if (original_type == cclosure)
                    hook_type_1 = "C";
                else if (original_type == lclosure)
                    hook_type_1 = "L";
                else if (original_type == newcc)
                    hook_type_1 = "NewCC";

                if (hook_type == cclosure)
                    hook_type_2 = "C";
                else if (hook_type == lclosure)
                    hook_type_2 = "L";
                else if (hook_type == newcc)
                    hook_type_2 = "NewCC";


                luaL_error(L, "failed to restore function: %s->%s", hook_type_1, hook_type_2);
         */
        luaL_error(L, OBF("failed to restore function"));
    }

    return 0;
}

inline int isnewcclosure(lua_State* L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    Closure* closure = clvalue(luaA_toobject(L, 1));

    lua_pushboolean(L, newcc_map.contains(closure));
    return 1;
}

namespace Library {
    namespace ClosureLib {
        inline void RegisterLibrary(lua_State* L) {
            RegisterFunction(L, loadstring, "loadstring");
            RegisterFunction(L, checkcaller, "checkcaller");
            RegisterFunction(L, islclosure, "islclosure");
            RegisterFunction(L, iscclosure, "iscclosure");
            RegisterFunction(L, newcclosure, "newcclosure");

            RegisterFunction(L, isnewcclosure, "isnewcclosure");
            RegisterFunction(L, isnewcclosure, "iscustomcclosure");

            RegisterFunction(L, hookfunction, "hookfunction");
            RegisterFunction(L, hookfunction, "replaceclosure");
            RegisterFunction(L, hookfunction, "hookfunc");

            RegisterFunction(L, isexecutorclosure, "isexecutorclosure");
            RegisterFunction(L, isexecutorclosure, "checkclosure");
            RegisterFunction(L, isexecutorclosure, "isourclosure");

            RegisterFunction(L, restorefunction, "restorefunction");
            RegisterFunction(L, restorefunction, "restorefunc");

            /*
            RegisterFunction(L, isfunctionhooked, "isfunctionhooked");

            RegisterFunction(L, checkcaller, "checkcaller");

            RegisterFunction(L, clonefunction, "clonefunction");
            */
        }
    }
}