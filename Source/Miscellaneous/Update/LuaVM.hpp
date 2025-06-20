#pragma once

#include <Windows.h>
#include "Structs/Structs.h"

#define REBASE(x) x + (uintptr_t)GetModuleHandle(nullptr)

#define LUAU_COMMA_SEP ,
#define LUAU_SEMICOLON_SEP ;

/*
* version-82f8ee8d17124507
*/

#define LUAU_SHUFFLE3(s, a1, a2, a3) a1 s a3 s a2
#define LUAU_SHUFFLE4(s, a1, a2, a3, a4) a3 s a1 s a2 s a4
#define LUAU_SHUFFLE5(s, a1, a2, a3, a4, a5) a5 s a4 s a1 s a3 s a2
#define LUAU_SHUFFLE6(s, a1, a2, a3, a4, a5, a6) a2 s a4 s a3 s a1 s a5 s a6
#define LUAU_SHUFFLE7(s, a1, a2, a3, a4, a5, a6, a7) a4 s a5 s a1 s a7 s a2 s a3 s a6
#define LUAU_SHUFFLE8(s, a1, a2, a3, a4, a5, a6, a7, a8) a6 s a4 s a1 s a2 s a8 s a5 s a7 s a3
#define LUAU_SHUFFLE9(s, a1, a2, a3, a4, a5, a6, a7, a8, a9) a1 s a8 s a5 s a3 s a4 s a6 s a7 s a9 s a2

#define PROTO_MEMBER1_ENC VMValue0
#define PROTO_MEMBER2_ENC VMValue4
#define PROTO_DEBUGISN_ENC VMValue3
#define PROTO_TYPEINFO_ENC VMValue2
#define PROTO_DEBUGNAME_ENC VMValue1

#define LSTATE_STACKSIZE_ENC VMValue2
#define LSTATE_GLOBAL_ENC VMValue0

#define CLOSURE_FUNC_ENC VMValue0
#define CLOSURE_CONT_ENC VMValue3
#define CLOSURE_DEBUGNAME_ENC VMValue4

#define TABLE_MEMBER_ENC VMValue0
#define TABLE_META_ENC VMValue0

#define UDATA_META_ENC VMValue3

#define TSTRING_HASH_ENC VMValue1
#define TSTRING_LEN_ENC VMValue0

#define GSTATE_TTNAME_ENC VMValue0
#define GSTATE_TMNAME_ENC VMValue0

namespace LuaVM {
    const uintptr_t LuaO_NilObject = REBASE(0x4740458); //
    const uintptr_t LuaH_DummyNode = REBASE(0x473FB88); //
    const uintptr_t Luau_Execute = REBASE(0x26D4300); //
    const uintptr_t LuaD_Throw = REBASE(0x26A1550); //
}
