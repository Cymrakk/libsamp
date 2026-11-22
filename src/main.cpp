// 0.3.7-R1

#include <windows.h>
#include <cwchar>
#include "MinHook.h"
#include "CSettings.h"
#include "CLogger.h"
#include "game/Offsets.h"

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
        Sleep(50);
        CLogger::Log("LibSamp unload");
        CLogger::Shutdown();

        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        FreeLibraryAndExitThread(hSelf, 0);
        return 0;
    }

    void OnSampLoaded(HMODULE hMod) {
        sampBase = reinterpret_cast<uintptr_t>(hMod);
        CLogger::Log("samp.dll base - 0x%X", sampBase);

        if (!Offsets::Init(sampBase)) {
            CLogger::Log("Unsupported SA-MP version! Unloading...");
            HANDLE hThread = CreateThread(nullptr, 0, UnloadThread, nullptr, 0, nullptr);
            if (hThread) CloseHandle(hThread);
            return;
        }

        CLogger::Log("Detected SA-MP version: %s", Offsets::GetVersionName());

        CLogger::Log("Loading settings...");
        pSettings = new CSettings();

        const auto& cfg = pSettings->GetReadOnly();
        CLogger::Log("Settings loaded!");
    }

    DWORD WINAPI InitThread(LPVOID lpParam) {
        OnSampLoaded(static_cast<HMODULE>(lpParam));
        return 0;
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

        if (hMod && !sampBase && !(dwFlags & (LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE))) {
            if (IsSampDllW(lpLibFileName)) {
                OnSampLoaded(hMod);
            }
        }
        return hMod;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        Core::hSelf = hModule;
        DisableThreadLibraryCalls(hModule);

        CLogger::Init();
        CLogger::Log("LibSamp started!");
        CLogger::Log("made by Cymrakk");
        CLogger::Log("GitHub: https://github.com/Cymrakk/libsamp");
        CLogger::Log(" ");
        HMODULE hSamp = GetModuleHandleA("samp.dll");
        if (hSamp) {
            HANDLE hThread = CreateThread(nullptr, 0, Core::InitThread, hSamp, 0, nullptr);
            if (hThread) CloseHandle(hThread);
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

        CLogger::Log("LibSamp unload");
        CLogger::Shutdown();

        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }
    return TRUE;
}

