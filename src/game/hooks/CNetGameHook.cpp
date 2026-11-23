#include "CNetGameHook.h"
#include "MinHook.h"
#include "CLogger.h"
#include "CSettings.h"
#include "game/Offsets.h"
#include "game/samp/CNetGame.h"
#include <cstring>

// Подтягиваем указатель на настройки из main.cpp
namespace Core {
    extern CSettings* pSettings;
}

namespace CNetGameHook {
    // Сигнатура конструктора CNetGame::__thiscall
    using CNetGameCtor_t = CNetGame* (__thiscall*)(CNetGame*, const char*, int, const char*, const char*);
    CNetGameCtor_t origCNetGameCtor = nullptr;

    CNetGame* __fastcall Hooked_CNetGameCtor(
        CNetGame* pThis,
        void* /*edx*/,
        const char* szHostOrIp,
        int iPort,
        const char* szPlayerName,
        const char* szPass
    ) {
        if (Core::pSettings) {
            const auto& cfg = Core::pSettings->GetReadOnly();

            // Подменяем хост и порт, если они заданы в samp.ini
            if (cfg.szHost[0] != '\0') {
                szHostOrIp = cfg.szHost;
            }
            if (cfg.iPort > 0) {
                iPort = cfg.iPort;
            }

            // Подменяем ник, если он задан в конфиге
            if (cfg.szNickName[0] != '\0') {
                szPlayerName = cfg.szNickName;
            }

            // Подменяем пароль (если в конфиге пусто — оставляем пустым или как передал лаунчер)
            if (cfg.szPassword[0] != '\0') {
                szPass = cfg.szPassword;
            } else {
                szPass = "";
            }

            CLogger::Log("Redirecting connection to %s:%d as %s", szHostOrIp, iPort, szPlayerName);
        }

        return origCNetGameCtor(pThis, szHostOrIp, iPort, szPlayerName, szPass);
    }

    void Install() {
        if (!Offsets::Addr.fnCNetGameConstructor) return;

        LPVOID target = reinterpret_cast<LPVOID>(Offsets::Addr.fnCNetGameConstructor);
        if (MH_CreateHook(target, &Hooked_CNetGameCtor, reinterpret_cast<void**>(&origCNetGameCtor)) == MH_OK) {
            MH_EnableHook(target);
            CLogger::Log("CNetGameHook installed at 0x%X", Offsets::Addr.fnCNetGameConstructor);
        } else {
            CLogger::Log("Failed to install CNetGameHook!");
        }
    }

    void Remove() {
        if (!Offsets::Addr.fnCNetGameConstructor) return;
        MH_DisableHook(reinterpret_cast<LPVOID>(Offsets::Addr.fnCNetGameConstructor));
    }
}
