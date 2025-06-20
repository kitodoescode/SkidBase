#pragma once

#include "../Miscellaneous/Includes.h"

#define PatchCFG(page) (*reinterpret_cast<uint8_t*>((bitmap) + ((page) >> 0x13)) |= (1 << (((page) >> 16) & 7)))

namespace RBX {
    __forceinline bool CheckMemory(uintptr_t address) {
        if (address < 0x10000 || address > 0x7FFFFFFFFFFF) {
            return false;
        }

        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi)) == 0) {
            return false;
        }

        if (mbi.Protect & PAGE_NOACCESS || mbi.State != MEM_COMMIT) {
            return false;
        }

        return true;
    }

    const auto DecompressBytecode = [&](std::string_view CompressedInput) -> std::string {
        const uint8_t BytecodeSignature[4] = { 'R', 'S', 'B', '1' }; /* roblox secure bytecode v1 */
        const int BytecodeHashMultiplier = 41;
        const int BytecodeHashSeed = 42;

        if (CompressedInput.size() < 8)
            return "Compressed data too short";

        std::vector<uint8_t> CompressedData(CompressedInput.begin(), CompressedInput.end());
        std::vector<uint8_t> HeaderBuffer(4);

        for (size_t i = 0; i < 4; ++i)
        {
            HeaderBuffer[i] = CompressedData[i] ^ BytecodeSignature[i];
            HeaderBuffer[i] = (HeaderBuffer[i] - i * BytecodeHashMultiplier) % 256;
        }

        for (size_t i = 0; i < CompressedData.size(); ++i)
            CompressedData[i] ^= (HeaderBuffer[i % 4] + i * BytecodeHashMultiplier) % 256;

        uint32_t HashValue = 0;
        for (size_t i = 0; i < 4; ++i)
            HashValue |= HeaderBuffer[i] << (i * 8);

        uint32_t Rehash = XXH32(CompressedData.data(), CompressedData.size(), BytecodeHashSeed);
        if (Rehash != HashValue)
            return "Hash mismatch during decompression";

        uint32_t DecompressedSize = 0;
        for (size_t i = 4; i < 8; ++i)
            DecompressedSize |= CompressedData[i] << ((i - 4) * 8);

        CompressedData = std::vector<uint8_t>(CompressedData.begin() + 8, CompressedData.end());
        std::vector<uint8_t> Result(DecompressedSize);

        size_t const ActualDecompressdSize = ZSTD_decompress(Result.data(), DecompressedSize, CompressedData.data(), CompressedData.size());
        if (ZSTD_isError(ActualDecompressdSize))
            return "ZSTD decompression error: " + std::string(ZSTD_getErrorName(ActualDecompressdSize));

        Result.resize(ActualDecompressdSize);

        return std::string(Result.begin(), Result.end());
        };
}