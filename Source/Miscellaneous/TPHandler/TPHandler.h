#pragma once

#include "../Manager/Manager.h"
#include "../Scheduler/Scheduler.hpp"
#include "../Update/Offsets.hpp"
#include "../Includes.h"

std::atomic<uintptr_t> lastState{ 0 };
std::atomic<uintptr_t> lastPlaceId{ 0 };
std::atomic<bool> teleportMonitoringActive{ true };

uintptr_t GlobalState() {
    auto ScriptContext = Scheduler->GetScriptContext();
    uintptr_t GlobalState = ScriptContext + Offsets::LuaUserData::GlobalState;
    return GlobalState;
}


void TeleportState() {
    try {
        uintptr_t DataModel = Scheduler->GetDataModel();
        uintptr_t currentPlaceId = *(uintptr_t*)(DataModel + Offsets::DataModel::PlaceId);

        lastPlaceId.store(currentPlaceId);
        lastState.store(GlobalState());

        bool TP = false;

        while (teleportMonitoringActive.load()) {
            DataModel = Scheduler->GetDataModel();
            currentPlaceId = *(uintptr_t*)(DataModel + Offsets::DataModel::PlaceId);

            if (currentPlaceId == 0 || GlobalState == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            if (currentPlaceId != lastPlaceId.load() || GlobalState() != lastState.load()) {
                TP = true;
                lastPlaceId.store(currentPlaceId);
                lastState.store(GlobalState());
            }
            else if (TP && currentPlaceId == lastPlaceId.load() && GlobalState() == lastState.load()) {
                TP = false;

                while (true) {
                    if (Scheduler->IsGameLoaded(Scheduler->GetDataModel()) != 1) {
                        //Roblox::Print(1, "waiting for datamodel");
                        Sleep(500);
                    }
                    else {
                        break;
                    }
                }

                std::this_thread::sleep_for(std::chrono::seconds(5));
                //Roblox::Print(1, "ingame");

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

            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
    }
    catch (const std::exception& ex) {
        MessageBoxA(NULL, ex.what(), "SkidBase - TPHandler", MB_ICONWARNING);
    }
}
