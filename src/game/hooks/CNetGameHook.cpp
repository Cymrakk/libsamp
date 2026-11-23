#include <windows.h>
#include <cstring>
#include <string>

#include "CNetGameHook.h"
#include "MinHook.h"
#include "CLogger.h"
#include "CSettings.h"
#include "game/Offsets.h"
#include "game/samp/CNetGame.h"

namespace Core {
    extern CSettings* pSettings;
}

namespace CNetGameHook {
    using RakConnect_t = bool(__thiscall*)(
        void* pThis,
        const char* host,
        unsigned short serverPort,
        unsigned short clientPort,
        unsigned int depreciated,
        int threadSleepTimer
    );
    using RakSetPassword_t = void(__thiscall*)(void* pThis, const char* password);

    RakConnect_t origRakConnect = nullptr;
    LPVOID pTargetConnect = nullptr;
    bool bStopThread = false;

    void WriteSampStdString(uintptr_t addr, const char* newStr) {
        size_t len = strlen(newStr);
        if (len > 24) len = 24; // Лимит ника в SA-MP
        if (len < 16) {
            memcpy(reinterpret_cast<char*>(addr), newStr, len + 1);
            *reinterpret_cast<size_t*>(addr + 16) = len;
            *reinterpret_cast<size_t*>(addr + 20) = 15;
        } else {
            // Используем CRT malloc из игры (msvcrt), чтобы не было краша при освобождении
            using malloc_t = void* (__cdecl*)(size_t);
            HMODULE hCrt = GetModuleHandleA("msvcrt.dll");
            auto gameMalloc = hCrt ? reinterpret_cast<malloc_t>(GetProcAddress(hCrt, "malloc")) : nullptr;
            char* buf = gameMalloc ? reinterpret_cast<char*>(gameMalloc(len + 1)) : nullptr;
            if (buf) {
                memcpy(buf, newStr, len + 1);
                *reinterpret_cast<char**>(addr) = buf;
                *reinterpret_cast<size_t*>(addr + 16) = len;
                *reinterpret_cast<size_t*>(addr + 20) = len;
            }
        }
    }

    bool __fastcall ConnectedHook(
        void* pThis,
        void* /*edx*/,
        const char* host,
        unsigned short serverPort,
        unsigned short clientPort,
        unsigned int depreciated,
        int threadSleepTimer
    ) {
        if (Core::pSettings) {
            const auto& cfg = Core::pSettings->GetReadOnly();

            if (cfg.szHost[0] != '\0') host = cfg.szHost;
            if (cfg.iPort > 0) serverPort = static_cast<unsigned short>(cfg.iPort);

            if (cfg.szPassword[0] != '\0') {
                void** vtable = *reinterpret_cast<void***>(pThis);
                if (vtable && vtable[3]) {
                    reinterpret_cast<RakSetPassword_t>(vtable[3])(pThis, cfg.szPassword);
                }
            }

            uintptr_t netGame = reinterpret_cast<uintptr_t>(CNetGame::Get());
            if (netGame) {
                if (Offsets::Addr.offHostOrIp && cfg.szHost[0] != '\0') {
                    strncpy_s(reinterpret_cast<char*>(netGame + Offsets::Addr.offHostOrIp), 257, cfg.szHost, _TRUNCATE);
                }
                if (Offsets::Addr.offPort && cfg.iPort > 0) {
                    *reinterpret_cast<int*>(netGame + Offsets::Addr.offPort) = cfg.iPort;
                }
                if (cfg.szNickName[0] != '\0' && Offsets::Addr.offPools && Offsets::Addr.offPlayerPool && Offsets::Addr.offLocalPlayerName) {
                    uintptr_t pools = *reinterpret_cast<uintptr_t*>(netGame + Offsets::Addr.offPools);
                    if (pools) {
                        uintptr_t playerPool = *reinterpret_cast<uintptr_t*>(pools + Offsets::Addr.offPlayerPool);
                        if (playerPool) {
                            WriteSampStdString(playerPool + Offsets::Addr.offLocalPlayerName, cfg.szNickName);
                        }
                    }
                }
            }

            CLogger::Log("Redirecting RakNet to %s:%u (Nick: %s)", host, serverPort, cfg.szNickName);
        }

        return origRakConnect(pThis, host, serverPort, clientPort, depreciated, threadSleepTimer);
    }

    DWORD WINAPI WaitAndHookThread(LPVOID) {
        while (!bStopThread) {
            uintptr_t netGame = reinterpret_cast<uintptr_t>(CNetGame::Get());
            if (netGame) {
                uintptr_t rakClient = *reinterpret_cast<uintptr_t*>(netGame + Offsets::Addr.offRakClientInterface);
                if (rakClient) {
                    void** vtable = *reinterpret_cast<void***>(rakClient);
                    if (vtable && vtable[1] && vtable[3]) {
                        pTargetConnect = vtable[1];
                        if (MH_CreateHook(pTargetConnect, &ConnectedHook, reinterpret_cast<void**>(&origRakConnect)) == MH_OK) {
                            MH_EnableHook(pTargetConnect);
                            CLogger::Log("CNetGameHook::ConnectedHook installed at 0x%p", pTargetConnect);
                        }
                        break;
                    }
                }
            }
            Sleep(1);
        }
        return 0;
    }

    void Install() {
        bStopThread = false;
        HANDLE hThread = CreateThread(nullptr, 0, WaitAndHookThread, nullptr, 0, nullptr);
        if (hThread) CloseHandle(hThread);
    }

    void Remove() {
        bStopThread = true;
        if (pTargetConnect) {
            MH_DisableHook(pTargetConnect);
            pTargetConnect = nullptr;
        }
    }
}
