#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstring>
#include <string>

#include "CNetGameHook.h"
#include "MinHook.h"
#include "CLogger.h"
#include "CSettings.h"
#include "game/Offsets.h"
#include "game/samp/CNetGame.h"

#pragma comment(lib, "ws2_32.lib")

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
    using sendto_t = int(WSAAPI*)(SOCKET, const char*, int, int, const struct sockaddr*, int);

    RakConnect_t origRakConnect = nullptr;
    sendto_t origSendTo = nullptr;
    LPVOID pTargetConnect = nullptr;
    bool bStopThread = false;

    // Исходный адрес сервера, который подменяем на уровне UDP
    unsigned long g_OrigIp = 0;
    unsigned short g_OrigPort = 0;

    // Безопасная перезапись MSVC std::string (24 байта) в памяти SA-MP
    void WriteSampStdString(uintptr_t addr, const char* newStr) {
        size_t len = strlen(newStr);
        size_t capacity = *reinterpret_cast<size_t*>(addr + 20);
        char* oldPtr = (capacity >= 16) ? *reinterpret_cast<char**>(addr) : nullptr;

        if (len < 16) {
            memcpy(reinterpret_cast<char*>(addr), newStr, len + 1);
            *reinterpret_cast<size_t*>(addr + 16) = len;
            *reinterpret_cast<size_t*>(addr + 20) = 15;
        } else {
            // Выделяем буфер через HeapAlloc процесса, чтобы SA-MP мог безопасно его читать
            char* buf = reinterpret_cast<char*>(HeapAlloc(GetProcessHeap(), 0, len + 1));
            if (buf) {
                memcpy(buf, newStr, len + 1);
                *reinterpret_cast<char**>(addr) = buf;
                *reinterpret_cast<size_t*>(addr + 16) = len;
                *reinterpret_cast<size_t*>(addr + 20) = len;
            }
        }
    }

    int WSAAPI Hooked_sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen) {
        if (to && to->sa_family == AF_INET && Core::pSettings && g_OrigPort != 0) {
            const auto& cfg = Core::pSettings->GetReadOnly();
            const sockaddr_in* sin = reinterpret_cast<const sockaddr_in*>(to);

            if (sin->sin_port == htons(g_OrigPort) && (g_OrigIp == 0 || sin->sin_addr.s_addr == g_OrigIp)) {
                sockaddr_in redirected = *sin;
                if (cfg.szHost[0] != '\0') {
                    unsigned long newIp = inet_addr(cfg.szHost);
                    if (newIp == INADDR_NONE) {
                        hostent* he = gethostbyname(cfg.szHost);
                        if (he && he->h_addr_list[0]) {
                            newIp = *reinterpret_cast<unsigned long*>(he->h_addr_list[0]);
                        }
                    }
                    if (newIp != INADDR_NONE) {
                        redirected.sin_addr.s_addr = newIp;
                    }
                }
                if (cfg.iPort > 0) {
                    redirected.sin_port = htons(static_cast<unsigned short>(cfg.iPort));
                }
                return origSendTo(s, buf, len, flags, reinterpret_cast<const sockaddr*>(&redirected), sizeof(redirected));
            }
        }
        return origSendTo(s, buf, len, flags, to, tolen);
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

            // Запоминаем исходный IP и порт при первом подключении
            if (g_OrigPort == 0 && host && serverPort > 0) {
                g_OrigIp = inet_addr(host);
                if (g_OrigIp == INADDR_NONE) g_OrigIp = 0;
                g_OrigPort = serverPort;
            }

            if (cfg.szHost[0] != '\0') host = cfg.szHost;
            if (cfg.iPort > 0) serverPort = static_cast<unsigned short>(cfg.iPort);

            // Пароль сервера
            if (cfg.szPassword[0] != '\0') {
                void** vtable = *reinterpret_cast<void***>(pThis);
                if (vtable && vtable[3]) {
                    reinterpret_cast<RakSetPassword_t>(vtable[3])(pThis, cfg.szPassword);
                }
            }

            uintptr_t netGame = reinterpret_cast<uintptr_t>(CNetGame::Get());
            if (netGame) {
                // Обновляем IP и порт в структуре CNetGame
                if (Offsets::Addr.offHostOrIp && cfg.szHost[0] != '\0') {
                    strncpy_s(reinterpret_cast<char*>(netGame + Offsets::Addr.offHostOrIp), 257, cfg.szHost, _TRUNCATE);
                }
                if (Offsets::Addr.offPort && cfg.iPort > 0) {
                    *reinterpret_cast<int*>(netGame + Offsets::Addr.offPort) = cfg.iPort;
                }

                // Перезаписываем ник прямо в памяти CPlayerPool
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

            CLogger::Log("Redirecting UDP & RakNet to %s:%u (Nick: %s)", host, serverPort, cfg.szNickName);
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
            Sleep(5);
        }
        return 0;
    }

    void Install() {
        MH_CreateHookApi(L"ws2_32", "sendto", &Hooked_sendto, reinterpret_cast<void**>(&origSendTo));
        MH_EnableHook(MH_ALL_HOOKS);

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
