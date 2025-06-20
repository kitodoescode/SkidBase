#pragma once

#include "../Miscellaneous/Includes.h"

#include <Luau/Compiler.h>
#include <Luau/BytecodeBuilder.h>
#include <Luau/BytecodeUtils.h>
#include <Luau/Bytecode.h>

#include "../Miscellaneous/Update/Offsets.hpp"

static class BytecodeEncoderClass : public Luau::BytecodeEncoder {
    inline void encode(uint32_t* data, size_t count) override
    {
        for (auto i = 0u; i < count;) {

            auto& opcode = *(uint8_t*)(data + i);

            i += Luau::getOpLength(LuauOpcode(opcode));

            opcode *= 227;
        }
    }
};

static BytecodeEncoderClass Encoder;

static uintptr_t MaxCaps = 0xFFFFFFFFFFFFFFFF;

class CExecution {
private:
public:
    lua_State* NewThread(lua_State* L);
    std::string CompileScript(const std::string Source);
    void Execute(lua_State* state, std::string Source);
    void SetProtoCapabilities(Proto* Proto);
};

inline auto Execution = std::make_unique<CExecution>();
