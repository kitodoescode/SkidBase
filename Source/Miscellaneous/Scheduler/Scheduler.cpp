#include "Scheduler.hpp"
#include "../../Execution/Execution.hpp"

uintptr_t TaskScheduler::GetJobByName(std::string JobName) {
    uintptr_t TaskScheduler = *reinterpret_cast<uintptr_t*>(Offsets::RawScheduler);
    uintptr_t Job = 0;
    uintptr_t JobStart = *(uintptr_t*)(TaskScheduler + Offsets::TaskScheduler::JobStart);

    while (JobStart != *(uintptr_t*)(TaskScheduler + Offsets::TaskScheduler::JobEnd)) {
        if (*(std::string*)(*(uintptr_t*)(JobStart)+Offsets::TaskScheduler::Job_Name) == JobName) {
            Job = *(uintptr_t*)JobStart;
        }
        JobStart += 0x10;
    }

    return Job;
}

uintptr_t TaskScheduler::GetDataModel() {
	uintptr_t fakeDM = *(uintptr_t*)Offsets::DataModel::FakeDataModelPointer;
    uintptr_t dataModel = *(uintptr_t*)(fakeDM + Offsets::DataModel::FakeDataModelToDataModel);

	return dataModel;
}

int TaskScheduler::IsGameLoaded(uintptr_t DataModel) {
    uint64_t isgameloaded = *reinterpret_cast<uint64_t*>(DataModel + Offsets::LuaUserData::IsGameLoaded);
    if (isgameloaded != 31) {
        return 0;
    }

    int value = isgameloaded;
    std::ostringstream ss;
    ss << "Game Loaded: " << value;
    RBX::Print(1, ss.str().c_str());

    return 1;
}

uintptr_t TaskScheduler::GetScriptContext() {
    uintptr_t dataModel = GetDataModel();
    if (!dataModel) {
        RBX::Print(1, "Invalid DataModel.");
        return 0;
    }

    uintptr_t children = *(uintptr_t*)(dataModel + 0x80);
    uintptr_t start = *(uintptr_t*)(children + 0x0);
    uintptr_t end = *(uintptr_t*)(children + 0x8);

    for (uintptr_t ptr = start; ptr < end; ptr += sizeof(uintptr_t)) {
        uintptr_t instance = *(uintptr_t*)ptr;
        if (!instance) continue;

        uintptr_t classDesc = *(uintptr_t*)(instance + 0x18);
        if (!classDesc) continue;

        const char* name = *(const char**)(classDesc + 0x8);
        if (!name) continue;

        if (strcmp(name, "ScriptContext") == 0) {
            return instance;
        }
    }

    RBX::Print(1, "Invalid ScriptContext.");
    return 0;
}

lua_State* TaskScheduler::GetLuaState()
{
	if (!L)
	{
		int32_t ignore1 = 2;
		uintptr_t ignore2 = { 0 };
		uintptr_t LuaState = RBX::GetState(*(uintptr_t*)(**(uintptr_t**)(*(uintptr_t*)(*(uintptr_t*)(Offsets::DataModel::FakeDataModelPointer)+Offsets::DataModel::FakeDataModelToDataModel) + Offsets::DataModel::Children) + Offsets::LuaUserData::ScriptContext) + Offsets::LuaUserData::GlobalState, &ignore1, &ignore2);
		if (!LuaState)
		{
            RBX::Print(3, "Failed to get obfuscated luastate.");
			return 0;
		}

        L = lua_newthread((lua_State*)RBX::DecryptState(LuaState + Offsets::LuaUserData::DecryptState));
	}

    std::ostringstream ss;
    ss << "LuaState: " << L;
    RBX::Print(1, ss.str().c_str());

	return L;
}
