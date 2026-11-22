#include "Offsets.h"
#include <windows.h>

namespace Offsets {

    bool Init(uintptr_t sampBase) {
        Base = sampBase;

        auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(sampBase);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;

        auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(sampBase + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return false;

        DWORD ep = nt->OptionalHeader.AddressOfEntryPoint;

        if (ep == 0x31DF13) {
            Version = SampVersion::R1;

            // Глобальные указатели (R1)
            Addr.pSampInfo   = sampBase + 0x21A0F8;
            Addr.pChatInfo   = sampBase + 0x21A0E4;
            Addr.pInputInfo  = sampBase + 0x21A0E8;
            Addr.pKillInfo   = sampBase + 0x21A0EC;
            Addr.pDialogInfo = sampBase + 0x21A0B8;

            // Функции (R1)
            Addr.fnAddEntry                 = sampBase + 0x64010;
            Addr.fnAddMessage            = sampBase + 0x645A0;
            Addr.fnRegisterClientCommand = sampBase + 0x65AD0;
            Addr.fnSay                   = sampBase + 0x57F0;
            Addr.fnSendCommand           = sampBase + 0x65C60;
            Addr.fnShowDialog            = sampBase + 0x6B9C0;

            // Смещения структур (R1)
            Addr.offRakClientInterface = 0x3C9;
            Addr.offPools              = 0x3CD;

            return true;
        }
        else if (ep == 0xCC4D0) {
            Version = SampVersion::R3_1;

            // Глобальные указатели (R3-1)
            Addr.pSampInfo   = sampBase + 0x26E8DC;
            Addr.pChatInfo   = sampBase + 0x26E8C8;
            Addr.pInputInfo  = sampBase + 0x26E8CC;
            Addr.pKillInfo   = sampBase + 0x26E8D0;
            Addr.pDialogInfo = sampBase + 0x26E898;

            // Функции (R3-1)
            Addr.fnAddEntry        = sampBase + 0x67460;
            Addr.fnAddMessage            = sampBase + 0x679F0;
            Addr.fnRegisterClientCommand = sampBase + 0x69000;
            Addr.fnSay                   = sampBase + 0x5820;
            Addr.fnSendCommand           = sampBase + 0x691B0;
            Addr.fnShowDialog            = sampBase + 0x6F8C0;

            // Смещения структур (R3-1)
            Addr.offRakClientInterface = 0x2C;
            Addr.offPools              = 0x3DE;

            return true;
        }

        Version = SampVersion::Unknown;
        return false;
    }

    const char* GetVersionName() {
        switch (Version) {
            case SampVersion::R1:   return "0.3.7-R1";
            case SampVersion::R3_1: return "0.3.7-R3-1";
            default:                return "Unknown";
        }
    }
}
