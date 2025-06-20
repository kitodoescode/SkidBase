#pragma once

#include <Windows.h>
#include <iostream>
#include <vector>

#define REBASE(x) x + (uintptr_t)GetModuleHandle(nullptr)

struct SignalT;
struct lua_State;
struct Proto;

/* 
* version - 82f8ee8d17124507 
*/

namespace Offsets {
    const uintptr_t Print = REBASE(0x15977E0); //
    const uintptr_t RawScheduler = REBASE(0x6884DC8); //
    const uintptr_t TaskSchedulerTargetFps = REBASE(0x6394020); //
    const uintptr_t GetGlobalState = REBASE(0xDC7E20); //
    const uintptr_t DecryptLuaState = REBASE(0xB4D320); //
    const uintptr_t LuaVM__Load = REBASE(0xB503A0); //
    const uintptr_t RequestCode = REBASE(0x906AC0); //
    const uintptr_t Impersonator = REBASE(0x335F140); //
    const uintptr_t PushInstance = REBASE(0xEA2D70); //
    const uintptr_t PushInstance2 = REBASE(0xEA2DC0); //
    const uintptr_t Luau_Execute = REBASE(0x26D4300); //
    const uintptr_t TaskDefer = REBASE(0xFEFA00); //
    const uintptr_t Task__Spawn = REBASE(0xFEFD20); //
    const uintptr_t LuaD_throw = REBASE(0x26A1550); //
    const uintptr_t LuaO_NilObject = REBASE(0x4740458); //
    const uintptr_t LuaH_DummyNode = REBASE(0x473FB88); //
    const uintptr_t KTable = REBASE(0x63940A0); //
    const uintptr_t FireMouseClick = REBASE(0x1CA01D0); //
    const uintptr_t FireRightMouseClick = REBASE(0x1CA0370); //
    const uintptr_t FireMouseHoverEnter = REBASE(0x1CA1770); //
    const uintptr_t FireMouseHoverLeave = REBASE(0x1CA1910); //
    const uintptr_t FireTouchInterest = REBASE(0x145F4D0); //
    const uintptr_t GetIdentityStruct = REBASE(0x38530A0); //
    const uintptr_t IdentityPtr = REBASE(0x63C9D48); //
    const uintptr_t GetProperty = REBASE(0xA5C7E0); //
    const uintptr_t FireProximityPrompt = REBASE(0x1D710E0); //
    const uintptr_t EncryptedState = 0x88;

    const uintptr_t RaiseEventInvocation = REBASE(0x15425E0); //
    const uintptr_t LockViolationInstanceCrash = REBASE(0x6014620); //
    const uintptr_t GetValues = REBASE(0xBE0D60); //
    const uintptr_t GetCurrentThreadId = REBASE(0x3852E70); //

    namespace InternalFastFlags {
        const uintptr_t EnableLoadModule = REBASE(0x6007F08); //
        const uintptr_t DebugCheckRenderThreading = REBASE(0x6030E68); //
        const uintptr_t RenderDebugCheckThreading2 = REBASE(0x605B230); //
        const uintptr_t DisableCorescriptLoadstring = REBASE(0x6007EE8); //
        const uintptr_t LuaStepIntervalMsOverrideEnabled = REBASE(0x600A700); //
    }

    namespace LuaUserData {
        const uintptr_t GlobalState = 0x140;
        const uintptr_t DecryptState = 0x88;
        const uintptr_t ScriptContext = 0x3B0;
        const uintptr_t ScriptInstance = 0x50;
        const uintptr_t IsGameLoaded = 0x650;
    }

    const uintptr_t FakeDataModelToDataModel = 0x1B8;
    const uintptr_t PlaceId = 0x1A0;
    const uintptr_t ScriptContext = 0x3B0;

    namespace Instance {
        const uintptr_t ClassDescriptor = 0x18;
        const uintptr_t PropertyDescriptor = 0x3B8;
        const uintptr_t ClassName = 0x8;
        const uintptr_t Name = 0x78;
        const uintptr_t Children = 0x80;
    }

    namespace Scripts {
        const uintptr_t LocalScriptEmbedded = 0x1B0;
        const uintptr_t ModuleScriptEmbedded = 0x158;
        const uintptr_t weak_thread_node = 0x188;
        const uintptr_t weak_thread_ref = 0x8;
        const uintptr_t weak_thread_ref_live = 0x20;
        const uintptr_t weak_thread_ref_live_thread = 0x8;
    }

    namespace ExtraSpace {
        const uintptr_t Identity = 0x30;
        const uintptr_t Capabilities = 0x48;
    }

    namespace TaskScheduler {
        const uintptr_t FpsCap = 0x1B0;
        const uintptr_t JobEnd = 0x1D8;
        const uintptr_t JobId = 0x140;
        const uintptr_t JobStart = 0x1D0;
        const uintptr_t Job_Name = 0x18;
    }


    namespace DataModel {
        namespace TypeStructure {
            enum PropertyType : uint32_t {
                BinaryString = 0x1E,
                SystemAddress = 0x1D,
                SharedString = 0x3D,
                Enum = 0x20,
                String = 0x6,
                Instance = 0x8
            };

            constexpr uintptr_t typeId = 0x38;
        }
        constexpr uintptr_t PropertiesStart = 0x30;
        constexpr uintptr_t PropertiesEnd = 0x38;

        constexpr uintptr_t Type = 0x60;
        constexpr uintptr_t TypeGetSetDescriptor = 0x98;
        constexpr uintptr_t getVFtableFunc = 0x10;
        const uintptr_t FakeDataModelPointer = REBASE(0x67C2958); //
        constexpr uintptr_t FakeDataModelToDataModel = 0x1B8;

        constexpr uintptr_t GameLoaded = 0x650;
        constexpr uintptr_t PlaceId = 0x1A0;
        constexpr uintptr_t GameId = 0x198;
        constexpr uintptr_t ModuleFlags = 0x6E0 - 0x4;
        constexpr uintptr_t IsCoreScript = 0x6E0;
        constexpr uintptr_t Children = 0x80;
        constexpr uintptr_t ChildrenEnd = 0x8;

        constexpr uintptr_t Name = 0x78;
        constexpr uintptr_t ClassDescriptor = 0x18;
        constexpr uintptr_t PropDescriptor = 0x3B8;
        constexpr uintptr_t ClassName = 0x8;
        constexpr uintptr_t PrimitiveTouch = 0x178LL;
        constexpr uintptr_t Overlap = 0x1d0;

        const uintptr_t weak_thread_node = 0x188;
        const uintptr_t weak_thread_ref = 0x8;
        const uintptr_t weak_thread_ref_live = 0x20;
        const uintptr_t weak_thread_ref_live_thread = 0x8;
    }
}

namespace RBX {
    using Print_T = uintptr_t(__fastcall*)(int, const char*, ...);
    using LuaVM__Load_T = int(__fastcall*)(lua_State*, void*, const char*, int);

    using TGetIdentityStruct = uintptr_t(__fastcall*)(uintptr_t);
    inline auto GetIdentityStruct = (TGetIdentityStruct)Offsets::GetIdentityStruct;

    inline auto Print = (Print_T)(Offsets::Print);
    inline auto LuaVM__Load = (LuaVM__Load_T)(Offsets::LuaVM__Load);

    inline auto LuaVMLoad2 = (uintptr_t(__fastcall*)(int64_t, std::string*, const char*, int))Offsets::LuaVM__Load;

    inline auto RequestCode = (uintptr_t(__fastcall*)(uintptr_t, uintptr_t))Offsets::RequestCode;
    inline auto GetState = (uintptr_t(__fastcall*)(uintptr_t, int32_t*, uintptr_t*))Offsets::GetGlobalState;

    inline auto DecryptState = (lua_State * (__fastcall*)(uintptr_t))Offsets::DecryptLuaState;

    inline auto PushInstance = (uintptr_t * (__fastcall*)(lua_State*, uintptr_t))Offsets::PushInstance;

    inline auto FireProximityPrompt = (uintptr_t * (__thiscall*)(uintptr_t))Offsets::FireProximityPrompt;

    inline auto FireMouseClick = (void(__fastcall*)(__int64 a1, float a2, __int64 a3))Offsets::FireMouseClick;
    inline auto FireRightMouseClick = (void(__fastcall*)(__int64 a1, float a2, __int64 a3))Offsets::FireRightMouseClick;

    inline auto FireMouseHoverEnter = (void(__fastcall*)(__int64 a1, __int64 a2))Offsets::FireMouseHoverEnter;

    inline auto FireMouseHoverLeave = (void(__fastcall*)(__int64 a1, __int64 a2))Offsets::FireMouseHoverLeave;

    inline auto FireTouchInterest = (void(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, bool, bool))Offsets::FireTouchInterest;

    inline auto KTable = reinterpret_cast<uintptr_t*>(Offsets::KTable);

    inline auto GetProperty = (uintptr_t * (__thiscall*)(uintptr_t, uintptr_t*))Offsets::GetProperty;
    inline auto Impersonator = (void(__fastcall*)(std::int64_t*, std::int32_t*, std::int64_t))Offsets::Impersonator;
    inline auto TaskDefer = (int(__fastcall*)(lua_State*))Offsets::TaskDefer;

    inline __int64 GetThreadIdentity(uintptr_t L) {
        uintptr_t Userdata = *(uintptr_t*)(L + 0x78);
        return *(__int64*)(Userdata + Offsets::ExtraSpace::Identity);
    }

    inline void SetThreadIdentity(uintptr_t L, uintptr_t Identity) {
        uintptr_t Userdata = *(uintptr_t*)(L + 0x78);
        *(__int64*)(Userdata + Offsets::ExtraSpace::Identity) = Identity;
    }

    inline std::string RequestBytecode(uintptr_t scriptPtr) {
        uintptr_t code[0x4];
        std::memset(code, 0, sizeof(code));

        RequestCode((std::uintptr_t)code, scriptPtr);

        std::uintptr_t bytecodePtr = code[1];

        if (!bytecodePtr) { return "Failed to get bytecode"; }

        std::uintptr_t str = bytecodePtr + 0x10;
        std::uintptr_t data;

        if (*reinterpret_cast<std::size_t*>(str + 0x18) > 0xf) {
            data = *reinterpret_cast<std::uintptr_t*>(str);
        }
        else {
            data = str;
        }

        std::string x;
        std::size_t len = *reinterpret_cast<std::size_t*>(str + 0x10);
        x.reserve(len);

        for (unsigned i = 0; i < len; i++) {
            x += *reinterpret_cast<char*>(data + i);
        }

        if (x.size() <= 8) { "Failed to get bytecode"; }

        return x;
    }
}
