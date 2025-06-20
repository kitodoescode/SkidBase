#include "Miscellaneous/Includes.h"
#include "Miscellaneous/Scheduler/Scheduler.hpp"
#include "Execution/Execution.hpp"
#include "Environment/Environment.hpp"
#include "Miscellaneous/Communication.hpp"
#include "ThreadPool.hpp"
#include "Miscellaneous/TPHandler/TPHandler.h"
#include "Miscellaneous/Manager/Manager.h"

void Init(HMODULE Module) {
    RBX::Print(1, "Module Loaded.");

    uintptr_t dataModel = Scheduler->GetDataModel();
    uintptr_t placeId = *(uintptr_t*)(dataModel + Offsets::DataModel::PlaceId);

    if (placeId) {

        int sIndex[] = { 2 };
        uintptr_t aIndex[] = { 0, 0 };

        lua_State* L = RBX::DecryptState(
            RBX::GetState(GlobalState(), sIndex, aIndex) +
            Offsets::EncryptedState
        );

        //lua_State* expThread = Execution->NewThread(L);

        luaL_sandboxthread(L);

        Manager->SetLuaState(L);

        L->userdata->Identity = 8;
        L->userdata->Capabilities = ~0ULL;
        Communication::ChangeState(L);
        Environment->Initialize(L);
        ThreadPool->Run(Communication::Initialize);
    }

    ThreadPool->Run(TeleportState);
};

BOOL APIENTRY DllMain(HINSTANCE Module, int Reason)
{
    if (Reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(Module);
        MessageBoxA(NULL, "Module Attached.", "", MB_OK);
        Init(Module);
    };

    return TRUE;
}