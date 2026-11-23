#include "CNetGameHook.h"
#include "MinHook.h"
#include "CLogger.h"
#include "CSettings.h"
#include "game/Offsets.h"
#include <cstring>

namespace Core {
    extern CSettings* pSettings;
}

namespace CNetGameHook {
    // Сигнатура InitSamp: void __cdecl InitSamp(const char* szHost, int iPort, const char* szNick, const char* szPass)
    using InitSamp_t = void(__cdecl*)(const char*, int, const char*, const char*);
    InitSamp_t origInitSamp = nullptr;

    void __cdecl ConnectedHook(
        const char* szHostOrIp,
        int iPort,
        const char* szPlayerName,
        const char* szPass
    ) {
        CLogger::Log("ConnectedHook triggered! Original: %s:%d (%s)", szHostOrIp, iPort, szPlayerName);

        if (Core::pSettings) {
            const auto& cfg = Core::pSettings->GetReadOnly();

            if (cfg.szHost[0] != '\0') {
                szHostOrIp = cfg.szHost;
            }
            if (cfg.iPort > 0) {
                iPort = cfg.iPort;
            }
            if (cfg.szNickName[0] != '\0') {
                szPlayerName = cfg.szNickName;
            }
            if (cfg.szPassword[0] != '\0') {
                szPass = cfg.szPassword;
            } else {
                szPass = "";
            }

            CLogger::Log("Redirecting connection to %s:%d as %s", szHostOrIp, iPort, szPlayerName);
        }

        origInitSamp(szHostOrIp, iPort, szPlayerName, szPass);
    }

    void Install() {
        if (Offsets::Addr.fnInitSamp) {
            LPVOID target = reinterpret_cast<LPVOID>(Offsets::Addr.fnInitSamp);
            MH_STATUS status = MH_CreateHook(target, &ConnectedHook, reinterpret_cast<void**>(&origInitSamp));
            if (status == MH_OK) {
                MH_EnableHook(target);
                CLogger::Log("CNetGameHook::ConnectedHook installed at 0x%X", Offsets::Addr.fnInitSamp);
            } else {
                CLogger::Log("Failed to install CNetGameHook::ConnectedHook! MH_STATUS = %d", status);
            }
        }
    }

    void Remove() {
        if (Offsets::Addr.fnInitSamp) {
            MH_DisableHook(reinterpret_cast<LPVOID>(Offsets::Addr.fnInitSamp));
        }
    }
}
