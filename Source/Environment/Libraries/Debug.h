#pragma once

#include "../../Miscellaneous/Includes.h"

inline Closure* header_get_function(lua_State* L, bool allowCclosure = false, bool popcl = true) {
    luaL_checkany(L, 1);

    if (!(lua_isfunction(L, 1) || lua_isnumber(L, 1)))
    {
        luaL_argerror(L, 1, "function or number");
    }

    int level = 0;
    if (lua_isnumber(L, 1))
    {
        level = lua_tointeger(L, 1);

        if (level <= 0)
        {
            luaL_argerror(L, 1, "level out of range");
        }
    }
    else if (lua_isfunction(L, 1))
    {
        level = -lua_gettop(L);
    }

    lua_Debug ar;
    if (!lua_getinfo(L, level, "f", &ar))
    {
        luaL_argerror(L, 1, "invalid level");
    }

    if (!lua_isfunction(L, -1))
        luaL_argerror(L, 1, "level does not point to a function");
    if (lua_iscfunction(L, -1) && !allowCclosure)
        luaL_argerror(L, 1, "level points to c function");

    if (!allowCclosure && lua_iscfunction(L, -1))
    {
        luaL_argerror(L, 1, "luau function expected");
    }

    auto function = clvalue(luaA_toobject(L, -1));
    if (popcl) lua_pop(L, 1);

    return function;
}

inline int debug_getupvalues(lua_State* L) {
    luaL_checkany(L, 1);

    Closure* function = header_get_function(L, true, false);
    lua_newtable(L);


    for (int i = 0; i < function->nupvalues; i++) {
        TValue* upval;

        const char* upvalue_name = aux_upvalue_2(function, i + 1, &upval);

        if (upvalue_name) {
            if (iscollectable(upval))
                luaC_threadbarrier(L);

            lua_rawcheckstack(L, 1);

            L->top->value = upval->value;
            L->top->tt = upval->tt;
            L->top++;

            lua_rawseti(L, -2, i + 1);
        }

    }

    return 1;
}

inline int debug_getupvalue(lua_State* L)
{
    const auto function = header_get_function(L, true, false);
    const auto idx = lua_tointeger(L, 2);

    int level = -lua_gettop(L);;

    if (function->nupvalues <= 0)
    {
        luaL_argerror(L, 1, "function has no upvalues");
    }


    lua_Debug ar;
    if (!lua_getinfo(L, level, ("f"), &ar))
        luaL_error(L, "invalid level");

    const char* name = lua_getupvalue(L, -1, idx);
    if (!name) {
        lua_pushnil(L);
        return 1;
    }

    return 1;
}

inline int debug_setupvalue(lua_State* L)
{
    const auto function = header_get_function(L, false, false);
    const auto idx = lua_tointeger(L, 2);

    if (function->nupvalues <= 0)
    {
        luaL_argerror(L, 1, "function has no upvalues");
    }

    if (!(idx >= 1 && idx <= function->nupvalues))
    {
        luaL_argerror(L, 2, "index out of range");
    }

    lua_pushvalue(L, 3);
    lua_setupvalue(L, -2, idx);
    return 1;
}

inline int debug_getconstants(lua_State* L)
{
    const auto function = header_get_function(L);
    const auto p = (Proto*)function->l.p;

    lua_createtable(L, p->sizek, 0);

    for (int i = 0; i < p->sizek; i++)
    {
        TValue k = p->k[i];

        if (k.tt == LUA_TNIL || k.tt == LUA_TFUNCTION || k.tt == LUA_TTABLE)
        {
            lua_pushnil(L);
        }
        else
        {
            luaC_threadbarrier(L)
                luaA_pushobject(L, &k);
        }
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

inline int debug_getconstant(lua_State* L)
{
    const auto function = header_get_function(L);
    const auto idx = luaL_checkinteger(L, 2);
    const auto p = (Proto*)function->l.p;

    const auto level = -lua_gettop(L);

    if (p->sizek <= 0)
    {
        luaL_argerror(L, 1, "function has no constants");
    }

    lua_Debug ar;
    if (!lua_getinfo(L, level, ("f"), &ar))
        luaL_error(L, "invalid level");

    if (!(idx >= 1 && idx <= p->sizek))
    {
        luaL_argerror(L, 2, "index out of range");
    }


    const auto k = &p->k[idx - 1];

    if (k->tt == LUA_TNIL || k->tt == LUA_TTABLE || k->tt == LUA_TFUNCTION) {
        lua_pushnil(L);
        return 1;
    }

    luaA_pushobject(L, k);
    return 1;
}

inline int debug_setconstant(lua_State* L)
{
    luaL_checkany(L, 3);

    const auto function = header_get_function(L);
    const auto idx = luaL_checkinteger(L, 2);
    const auto p = (Proto*)function->l.p;

    if (p->sizek <= 0)
    {
        luaL_argerror(L, 1, "function has no constants");
    }

    if (!(idx >= 1 && idx <= p->sizek))
    {
        luaL_argerror(L, 2, "index out of range");
    }

    TValue* k = &p->k[idx - 1];

    if (k->tt == LUA_TFUNCTION || k->tt == LUA_TTABLE)
    {
        return 0;
    }
    else
    {
        if (k->tt == luaA_toobject(L, 3)->tt)
        {
            setobj2s(L, k, luaA_toobject(L, 3));
        }
    }

    return 0;
}

inline int debug_getprotos(lua_State* L)
{
    const auto function = header_get_function(L);
    const auto p = (Proto*)function->l.p;

    lua_createtable(L, p->sizep, 0);
    for (int i = 0; i < p->sizep; i++)
    {
        Proto* proto = p->p[i];
        Closure* pcl = luaF_newLclosure(L, function->nupvalues, function->env, proto);

        luaC_threadbarrier(L) setclvalue(L, L->top, pcl) L->top++;

        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

inline int debug_getproto(lua_State* L)
{

    Closure* cl = header_get_function(L);

    int index = luaL_checkinteger(L, 2);
    bool active = false;
    if (!lua_isnoneornil(L, 3))
        active = luaL_checkboolean(L, 3);
    if (!active) {
        if (index < 1 || index > cl->l.p->sizep)
            luaL_argerror(L, 2, "index out of range");

        Proto* p = cl->l.p->p[index - 1];

        std::unique_ptr<TValue> function(new TValue{});
        setclvalue(L, function.get(), luaF_newLclosure(L, 0, cl->env, p));
        luaA_pushobject(L, function.get());
    }
    else {
        lua_newtable(L);

        struct Ctx {
            lua_State* L;
            int count;
            Closure* cl;
        } ctx{ L, 0, cl };

        luaM_visitgco(L, &ctx, [](void* pctx, lua_Page* page, GCObject* gco) -> bool {
            Ctx* ctx = static_cast<Ctx*>(pctx);
            if (!((gco->gch.marked ^ WHITEBITS) & otherwhite(ctx->L->global)))
                return false;

            uint8_t tt = gco->gch.tt;
            if (tt == LUA_TFUNCTION) {
                Closure* cl = (Closure*)gco;
                if (!cl->isC && cl->l.p == ctx->cl->l.p->p[ctx->count]) {
                    setclvalue(ctx->L, ctx->L->top, cl);
                    ctx->L->top++;
                    lua_rawseti(ctx->L, -2, ++ctx->count);
                }
            }
            return false;
            });
    }
    return 1;
}

inline int debug_getstack(lua_State* L)
{
    luaL_checkany(L, 1);

    if (!(lua_isfunction(L, 1) || lua_isnumber(L, 1)))
    {
        luaL_argerror(L, 1, "function or number");
    }

    int level = 0;
    if (lua_isnumber(L, 1))
    {
        level = lua_tointeger(L, 1);

        if (level <= 0)
        {
            luaL_argerror(L, 1, "level out of range");
        }
    }
    else if (lua_isfunction(L, 1))
    {
        level = -lua_gettop(L);
    }

    lua_Debug ar;
    if (!lua_getinfo(L, level, "f", &ar))
    {
        luaL_argerror(L, 1, "invalid level");
    }

    if (!lua_isfunction(L, -1))
    {
        luaL_argerror(L, 1, "level does not point to a function");
    }

    if (lua_iscfunction(L, -1))
    {
        luaL_argerror(L, 1, "luau function expected");
    }

    lua_pop(L, 1);

    auto ci = L->ci[-level];

    if (lua_isnumber(L, 2))
    {
        const auto idx = lua_tointeger(L, 2) - 1;

        if (idx >= cast_int(ci.top - ci.base) || idx < 0)
        {
            luaL_argerror(L, 2, "index out of range");
        }

        auto val = ci.base + idx;
        luaC_threadbarrier(L) luaA_pushobject(L, val);
    }
    else
    {
        int idx = 0;
        lua_newtable(L);

        for (auto val = ci.base; val < ci.top; val++)
        {
            lua_pushinteger(L, idx++ + 1);

            luaC_threadbarrier(L) luaA_pushobject(L, val);

            lua_settable(L, -3);
        }
    }

    return 1;
}

inline int debug_setstack(lua_State* L)
{
    luaL_checkany(L, 1);

    if (!(lua_isfunction(L, 1) || lua_isnumber(L, 1)))
    {
        luaL_argerror(L, 1, ("function or number"));
    }

    int level = 0;
    if (lua_isnumber(L, 1))
    {
        level = lua_tointeger(L, 1);

        if (level <= 0)
        {
            luaL_argerror(L, 1, "level out of range");
        }
    }
    else if (lua_isfunction(L, 1))
    {
        level = -lua_gettop(L);
    }

    lua_Debug ar;
    if (!lua_getinfo(L, level, "f", &ar))
    {
        luaL_argerror(L, 1, "invalid level");
    }

    if (!lua_isfunction(L, -1))
    {
        luaL_argerror(L, 1, "level does not point to a function");
    }

    if (lua_iscfunction(L, -1))
    {
        luaL_argerror(L, 1, "luau function expected");
    }

    lua_pop(L, 1);

    luaL_checkany(L, 3);

    auto ci = L->ci[-level];

    const auto idx = luaL_checkinteger(L, 2) - 1;
    if (idx >= cast_int(ci.top - ci.base) || idx < 0)
    {
        luaL_argerror(L, 2, "index out of range");
    }

    if ((ci.base + idx)->tt != luaA_toobject(L, 3)->tt)
    {
        luaL_argerror(L, 3, "new value type does not match previous value type");
    }

    setobj2s(L, (ci.base + idx), luaA_toobject(L, 3))
        return 0;
}

inline int debug_getinfo(lua_State* L)
{
    luaL_checkany(L, 1);

    if (!(lua_isfunction(L, 1) || lua_isnumber(L, 1)))
    {
        luaL_argerror(L, 1, "function or number");
    }

    int level;
    if (lua_isnumber(L, 1))
    {
        level = lua_tointeger(L, 1);
    }
    else
    {
        level = -lua_gettop(L);
    }

    auto desc = "sluanf";

    lua_Debug ar;
    if (!lua_getinfo(L, level, desc, &ar))
    {
        luaL_argerror(L, 1, "invalid level");
    }

    if (!lua_isfunction(L, -1))
    {
        luaL_argerror(L, 1, "level does not point to a function.");
    }

    lua_newtable(L);
    {
        if (std::strchr(desc, 's'))
        {
            lua_pushstring(L, ar.source);
            lua_setfield(L, -2, "source");

            lua_pushstring(L, ar.short_src);
            lua_setfield(L, -2, "short_src");

            lua_pushstring(L, ar.what);
            lua_setfield(L, -2, "what");

            lua_pushinteger(L, ar.linedefined);
            lua_setfield(L, -2, "linedefined");
        }

        if (std::strchr(desc, 'l'))
        {
            lua_pushinteger(L, ar.currentline);
            lua_setfield(L, -2, "currentline");
        }

        if (std::strchr(desc, 'u'))
        {
            lua_pushinteger(L, ar.nupvals);
            lua_setfield(L, -2, "nups");
        }

        if (std::strchr(desc, 'a'))
        {
            lua_pushinteger(L, ar.isvararg);
            lua_setfield(L, -2, "is_vararg");

            lua_pushinteger(L, ar.nparams);
            lua_setfield(L, -2, "numparams");
        }

        if (std::strchr(desc, 'n'))
        {
            lua_pushstring(L, ar.name);
            lua_setfield(L, -2, "name");
        }

        if (std::strchr(desc, 'f'))
        {
            lua_pushvalue(L, -2);
            lua_remove(L, -3);
            lua_setfield(L, -2, "func");
        }
    }

    return 1;
}

inline int debug_getregistry(lua_State* L)
{
    lua_rawcheckstack(L, 1);
    luaC_threadbarrier(L);

    lua_pushvalue(L, LUA_REGISTRYINDEX);
    return 1;
}

extern std::map<Closure*, Closure*> hooks_map;
extern int getgenv(lua_State* L);
inline lua_CFunction old_getfenv = nullptr;
inline LuaTable* our_env = nullptr;

inline int getfenv_stub(lua_State* L) {
    if (L->userdata == nullptr || L->userdata->Script.expired())
        return old_getfenv(L);

    if (lua_isfunction(L, 1)) {
        const Closure* closure = clvalue(luaA_toobject(L, 1));
        if (closure->env == our_env) {
            //lua_newtable(L);
            lua_rawcheckstack(L, 1);
            luaC_threadbarrier(L);

            lua_newtable(L);
            return 1;
        }
    }
    else if (lua_isnumber(L, 1)) {
        lua_Debug ar;
        const int level = luaL_optinteger(L, 1, 1);
        luaL_argcheck(L, level >= 0, 1, OBF("level must be non-negative"));
        if (lua_getinfo(L, level, OBF("f"), &ar) == 0)
            luaL_argerror(L, 1, OBF("invalid level"));
        if (lua_isnil(L, -1))
            luaL_error(L, OBF("no function environment for tail call at level %d"), level);

        Closure* closure = clvalue(luaA_toobject(L, -1));
        lua_pop(L, 1);

        if (closure->env == our_env) {
            //lua_newtable(L);
            lua_rawcheckstack(L, 1);
            luaC_threadbarrier(L);

            //lua_pushvalue(L, LUA_GLOBALSINDEX);
            lua_newtable(L);
            return 1;
        }
    }

    return old_getfenv(L);
}

namespace Library {
    namespace Debug {
        inline void RegisterLibrary(lua_State* L) {
            RegisterFunction(L, debug_getconstant, "getconstant");
            RegisterFunction(L, debug_getconstants, "getconstants");
            RegisterFunction(L, debug_setconstant, "setconstant");
            RegisterFunction(L, debug_getupvalue, "getupvalue");
            RegisterFunction(L, debug_getupvalues, "getupvalues");
            RegisterFunction(L, debug_setupvalue, "setupvalue");
            RegisterFunction(L, debug_getstack, "getstack");
            RegisterFunction(L, debug_setstack, "setstack");
            RegisterFunction(L, debug_getproto, "getproto");
            RegisterFunction(L, debug_getprotos, "getprotos");
            RegisterFunction(L, debug_getinfo, "getinfo");
            RegisterFunction(L, debug_getregistry, "getregistry");

            lua_newtable(L);
            RegisterMember(L, debug_getconstant, "getconstant");
            RegisterMember(L, debug_getconstants, "getconstants");
            RegisterMember(L, debug_setconstant, "setconstant");
            RegisterMember(L, debug_getupvalue, "getupvalue");
            RegisterMember(L, debug_getupvalues, "getupvalues");
            RegisterMember(L, debug_setupvalue, "setupvalue");
            RegisterMember(L, debug_getstack, "getstack");
            RegisterMember(L, debug_setstack, "setstack");
            RegisterMember(L, debug_getproto, "getproto");
            RegisterMember(L, debug_getprotos, "getprotos");
            RegisterMember(L, debug_getinfo, "getinfo");
            RegisterMember(L, debug_getregistry, "getregistry");
            lua_setglobal(L, "debug");
        }
    }
}