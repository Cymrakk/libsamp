#pragma once
#include <cstdint>

enum class SampVersion {
    Unknown = 0,
    R1,
    R3_1
};

struct stOffsets {
    // Глобальные указатели (с прибавленным sampBase)
    uintptr_t pSampInfo   = 0;
    uintptr_t pChatInfo   = 0;
    uintptr_t pInputInfo  = 0;
    uintptr_t pKillInfo   = 0;
    uintptr_t pDialogInfo = 0;

    // Функции (с прибавленным sampBase)
    uintptr_t fnAddEntry              = 0;
    uintptr_t fnAddMessage            = 0;
    uintptr_t fnRegisterClientCommand = 0;
    uintptr_t fnSay                   = 0;
    uintptr_t fnSendCommand           = 0;
    uintptr_t fnShowDialog            = 0;
    uintptr_t fnSetLocalPlayerName    = 0;

    // Смещения внутри структур (относительные)
    uintptr_t offRakClientInterface = 0;
    uintptr_t offPools              = 0;
    uintptr_t offHostOrIp           = 0;
    uintptr_t offPort               = 0;
    uintptr_t offPlayerPool         = 0;
    uintptr_t offLocalPlayerName    = 0;
};

namespace Offsets {
    inline SampVersion Version = SampVersion::Unknown;
    inline uintptr_t Base = 0;
    inline stOffsets Addr{};

    bool Init(uintptr_t sampBase);
    const char* GetVersionName();
}
