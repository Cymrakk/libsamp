#include "CPlayerPoolHook.h"
#include "MinHook.h"
#include "CLogger.h"
#include "CSettings.h"
#include "game/Offsets.h"
#include "game/samp/CPlayerPool.h"

namespace Core {
    extern CSettings* pSettings;
}

namespace CPlayerPoolHook {
    using SetLocalName_t = void(__thiscall*)(CPlayerPool*, const char*);
    SetLocalName_t origSetLocalName = nullptr;

    void __fastcall SetLocalPlayerNameHook(CPlayerPool* pThis, void* /*edx*/, const char* szName) {
        if (Core::pSettings) {
            const auto& cfg = Core::pSettings->GetReadOnly();
            if (cfg.szNickName[0] != '\0') {
                CLogger::Log("SetLocalPlayerNameHook: '%s' -> '%s'", szName ? szName : "", cfg.szNickName);
                szName = cfg.szNickName;
            }
        }
        origSetLocalName(pThis, szName);
    }

    void Install() {
        if (!Offsets::Addr.fnSetLocalPlayerName) return;

        LPVOID target = reinterpret_cast<LPVOID>(Offsets::Addr.fnSetLocalPlayerName);
        MH_STATUS status = MH_CreateHook(
            target,
            &SetLocalPlayerNameHook,
            reinterpret_cast<void**>(&origSetLocalName)
        );
        if (status == MH_OK) {
            MH_EnableHook(target);
            CLogger::Log("CPlayerPoolHook::SetLocalPlayerNameHook installed at 0x%X", Offsets::Addr.fnSetLocalPlayerName);
        } else {
            CLogger::Log("Failed to install CPlayerPoolHook! MH_STATUS = %d", status);
        }
    }

    void Remove() {
        if (Offsets::Addr.fnSetLocalPlayerName) {
            MH_DisableHook(reinterpret_cast<LPVOID>(Offsets::Addr.fnSetLocalPlayerName));
        }
    }
}
