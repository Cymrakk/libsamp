// 0.3.7-R1

#include <windows.h>
#include <cstdio>
#include <cwchar>
#include "MinHook.h"
#include "CSettings.h"

namespace Core {
    HMODULE hSelf = nullptr;
    uintptr_t sampBase = 0;
    CSettings* pSettings = nullptr;

    using LoadLibraryExW_t = HMODULE(WINAPI*)(LPCWSTR, HANDLE, DWORD);
    LoadLibraryExW_t origLoadLibraryExW = nullptr;

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
    void OnSampLoaded(HMODULE hMod) {
        sampBase = reinterpret_cast<uintptr_t>(hMod);
        if (!IsSampR1(sampBase)) {
            HANDLE hThread = CreateThread(nullptr, 0, UnloadThread, nullptr, 0, nullptr);
            if (hThread) CloseHandle(hThread);
            return;
        }
        pSettings = new CSettings();
    }

    bool IsSampDllW(LPCWSTR path) {
        if (!path) return false;
        const wchar_t* fileName = wcsrchr(path, L'\\');
        if (!fileName) fileName = wcsrchr(path, L'/');
        fileName = fileName ? fileName + 1 : path;
        return _wcsicmp(fileName, L"samp.dll") == 0;
    }

    HMODULE WINAPI Hooked_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
        HMODULE hMod = origLoadLibraryExW(lpLibFileName, hFile, dwFlags);

        if (hMod && !sampBase && IsSampDllW(lpLibFileName)) {
            OnSampLoaded(hMod);
        }
        return hMod;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        Core::hSelf = hModule;
        DisableThreadLibraryCalls(hModule);

        HMODULE hSamp = GetModuleHandleA("samp.dll");
        if (hSamp) {
            Core::OnSampLoaded(hSamp);
            return TRUE;
        }

        if (MH_Initialize() == MH_OK) {
            MH_CreateHookApi(L"kernel32", "LoadLibraryExW", &Core::Hooked_LoadLibraryExW, reinterpret_cast<void**>(&Core::origLoadLibraryExW));
            MH_EnableHook(MH_ALL_HOOKS);
        }
    }
    else if (reason == DLL_PROCESS_DETACH) {
        delete Core::pSettings;
        Core::pSettings = nullptr;
        MH_Uninitialize();
    }
    return TRUE;
}
