#pragma once

#include "../Includes.h"
#include "../Update/Offsets.hpp"

class TaskScheduler {
public:
    lua_State* L = nullptr;
    lua_State* GetLuaState();
    uintptr_t GetDataModel();
    uintptr_t GetScriptContext();
    uintptr_t GetJobByName(std::string JobName);
    int IsGameLoaded(uintptr_t DataModel);
};

inline auto Scheduler = std::make_unique<TaskScheduler>();