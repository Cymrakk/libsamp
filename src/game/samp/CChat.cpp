// CChat.cpp
#include "CChat.h"
#include "game/Offsets.h"

CChat* CChat::Get() {
    if (!Offsets::Addr.pChatInfo) return nullptr;
    return *reinterpret_cast<CChat**>(Offsets::Addr.pChatInfo);
}

void CChat::AddEntry(int nType, const char* szText, const char* szPrefix, uint32_t textColor, uint32_t prefixColor) {
    if (!this || !Offsets::Addr.fnAddEntry) return;

    using AddEntry_t = void(__thiscall*)(CChat*, int, const char*, const char*, uint32_t, uint32_t);
    auto fn = reinterpret_cast<AddEntry_t>(Offsets::Addr.fnAddEntry);

    fn(this, nType, szText, szPrefix, textColor, prefixColor);
}

void CChat::AddMessage(uint32_t color, const char* szText) {
    if (!this || !Offsets::Addr.fnAddMessage) return;

    using AddMessage_t = void(__thiscall*)(CChat*, uint32_t, const char*);
    auto fn = reinterpret_cast<AddMessage_t>(Offsets::Addr.fnAddMessage);

    fn(this, color, szText);
}
