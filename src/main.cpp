
#include <windows.h>
#include <cstdio>
#include <cstring>
#include "MinHook.h"
#include "CSettings.h"

namespace Core {
    HMODULE hSelf = nullptr;
    uintptr_t sampBase = 0;
    CSettings* pSettings = nullptr;

    using LoadLibraryA_t = HMODULE(WINAPI*)(LPCSTR);
    LoadLibraryA_t origLoadLibraryA = nullptr;

    bool IsSampR1(uintptr_t base) {
        auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
        auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);
        return nt->Signature == IMAGE_NT_SIGNATURE && 
               nt->OptionalHeader.AddressOfEntryPoint == 0x31DF13;
    }

    DWORD WINAPI UnloadThread(LPVOID) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        FreeLibraryAndExitThread(hSelf, 0);
        return 0;
    }

    void WriteTestLog(const stSettings& cfg) {
        FILE* f = fopen(".\\SAMP\\samp_test.txt", "w");
        if (f) {
            fprintf(f, "Plugin loaded successfully! SA-MP 0.3.7-R1 confirmed.\n");
            fprintf(f, "--------------------------------------------------\n");
            fprintf(f, "NickName: %s\n", cfg.szNickName);
            fprintf(f, "Host:     %s\n", cfg.szHost);
            fprintf(f, "Port:     %d\n", cfg.iPort);
            fprintf(f, "Password: %s\n", cfg.szPassword ? cfg.szPassword : "(empty)");
            fclose(f);
        }
    }


    void OnSampLoaded() {
        pSettings = new CSettings();
        WriteTestLog(pSettings->GetReadOnly());
    }

    HMODULE WINAPI Hooked_LoadLibraryA(LPCSTR lpLibFileName) {
        HMODULE hMod = origLoadLibraryA(lpLibFileName);

        if (hMod && !sampBase && lpLibFileName && strstr(lpLibFileName, "samp.dll")) {
            sampBase = reinterpret_cast<uintptr_t>(hMod);
            MH_DisableHook(&LoadLibraryA);

            if (!IsSampR1(sampBase)) {
                CreateThread(nullptr, 0, UnloadThread, nullptr, 0, nullptr);
                return hMod;
            }

            OnSampLoaded();
        }
        return hMod;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        Core::hSelf = hModule;
        DisableThreadLibraryCalls(hModule);

        if (MH_Initialize() == MH_OK) {
            MH_CreateHook(&LoadLibraryA, &Core::Hooked_LoadLibraryA, reinterpret_cast<void**>(&Core::origLoadLibraryA));
            MH_EnableHook(&LoadLibraryA);
        }
    }
    else if (reason == DLL_PROCESS_DETACH) {
        delete Core::pSettings;
        Core::pSettings = nullptr;
        MH_Uninitialize();
    }
    return TRUE;
}