#include <windows.h>
#include "CNetGameHook.h"
#include "MinHook.h"
#include "CLogger.h"
#include "CSettings.h"
#include "game/Offsets.h"

namespace Core {
    extern CSettings* pSettings;
}

namespace CNetGameHook {
    using SetLocalPlayerName_t = void(__thiscall*)(void* pThis, const char* szName);

    SetLocalPlayerName_t origSetLocalPlayerName = nullptr;
    LPVOID pTargetSetNick = nullptr;

    void __fastcall SetLocalPlayerNameHook(void* pThis, void* /*edx*/, const char* szName) {
        if (Core::pSettings) {
            const auto& cfg = Core::pSettings->GetReadOnly();
            if (cfg.szNickName[0] != '\0') {
                szName = cfg.szNickName;
                CLogger::Log("Nickname hooked on first init: %s", szName);
            }
        }
        origSetLocalPlayerName(pThis, szName);
    }

    void Install() {
        if (Offsets::Addr.fnSetLocalPlayerName) {
            pTargetSetNick = reinterpret_cast<LPVOID>(Offsets::Addr.fnSetLocalPlayerName);
            if (MH_CreateHook(pTargetSetNick, &SetLocalPlayerNameHook, reinterpret_cast<void**>(&origSetLocalPlayerName)) == MH_OK) {
                MH_EnableHook(pTargetSetNick);
                CLogger::Log("SetLocalPlayerNameHook installed at 0x%p", pTargetSetNick);
            }
        }
    }

    void Remove() {
        if (pTargetSetNick) {
            MH_DisableHook(pTargetSetNick);
            pTargetSetNick = nullptr;
        }
    }
}
