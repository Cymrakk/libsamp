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
    using recvfrom_t = int(WSAAPI*)(SOCKET, char*, int, int, struct sockaddr*, int*);

    RakConnect_t origRakConnect = nullptr;
    sendto_t origSendTo = nullptr;
    recvfrom_t origRecvFrom = nullptr;
    LPVOID pTargetConnect = nullptr;
    bool bStopThread = false;

    unsigned long g_OrigIp = 0;
    unsigned short g_OrigPort = 0;
    unsigned long g_TargetIp = 0;
    unsigned short g_TargetPort = 0;

    unsigned long ResolveIp(const char* host) {
        if (!host || host[0] == '\0') return INADDR_NONE;
        unsigned long ip = inet_addr(host);
        if (ip == INADDR_NONE) {
            hostent* he = gethostbyname(host);
            if (he && he->h_addr_list && he->h_addr_list[0]) {
                ip = *reinterpret_cast<unsigned long*>(he->h_addr_list[0]);
            }
        }
        return ip;
    }

    void WriteSampStdString(uintptr_t addr, const char* newStr) {
        size_t len = strlen(newStr);
        if (len < 16) {
            memcpy(reinterpret_cast<char*>(addr), newStr, len + 1);
            *reinterpret_cast<size_t*>(addr + 16) = len;
            *reinterpret_cast<size_t*>(addr + 20) = 15;
        } else {
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
        if (to && to->sa_family == AF_INET && g_OrigPort != 0 && g_TargetPort != 0) {
            const sockaddr_in* sin = reinterpret_cast<const sockaddr_in*>(to);
            if (sin->sin_port == htons(g_OrigPort) && (g_OrigIp == 0 || sin->sin_addr.s_addr == g_OrigIp)) {
                sockaddr_in redirected = *sin;
                if (g_TargetIp != INADDR_NONE && g_TargetIp != 0) {
                    redirected.sin_addr.s_addr = g_TargetIp;
                }
                redirected.sin_port = htons(g_TargetPort);
                return origSendTo(s, buf, len, flags, reinterpret_cast<const sockaddr*>(&redirected), sizeof(redirected));
            }
        }
        return origSendTo(s, buf, len, flags, to, tolen);
    }

    int WSAAPI Hooked_recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen) {
        int ret = origRecvFrom(s, buf, len, flags, from, fromlen);
        if (ret > 0 && from && from->sa_family == AF_INET && g_OrigPort != 0 && g_TargetPort != 0) {
            sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(from);
            if (sin->sin_port == htons(g_TargetPort) && (g_TargetIp == 0 || sin->sin_addr.s_addr == g_TargetIp)) {
                if (g_OrigIp != 0) {
                    sin->sin_addr.s_addr = g_OrigIp;
                }
                sin->sin_port = htons(g_OrigPort);
            }
        }
        return ret;
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

            if (g_OrigPort == 0 && host && serverPort > 0) {
                unsigned long ip = ResolveIp(host);
                g_OrigIp = (ip != INADDR_NONE) ? ip : 0;
                g_OrigPort = serverPort;
            }

            if (cfg.szHost[0] != '\0') {
                host = cfg.szHost;
                unsigned long tip = ResolveIp(cfg.szHost);
                g_TargetIp = (tip != INADDR_NONE) ? tip : g_OrigIp;
            } else {
                g_TargetIp = g_OrigIp;
            }

            g_TargetPort = (cfg.iPort > 0) ? static_cast<unsigned short>(cfg.iPort) : g_OrigPort;
            if (cfg.iPort > 0) serverPort = g_TargetPort;

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
        MH_CreateHookApi(L"ws2_32", "recvfrom", &Hooked_recvfrom, reinterpret_cast<void**>(&origRecvFrom));
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
