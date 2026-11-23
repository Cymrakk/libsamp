#include "CPlayerPool.h"
#include "CNetGame.h"
#include "game/Offsets.h"

CPlayerPool* CPlayerPool::Get() {
    uintptr_t netGame = reinterpret_cast<uintptr_t>(CNetGame::Get());
    if (!netGame || !Offsets::Addr.offPools) return nullptr;

    uintptr_t pools = *reinterpret_cast<uintptr_t*>(netGame + Offsets::Addr.offPools);
    if (!pools) return nullptr;

    return *reinterpret_cast<CPlayerPool**>(pools + Offsets::Addr.offPlayerPool);
}

const char* CPlayerPool::GetLocalPlayerName() {
    uintptr_t pool = reinterpret_cast<uintptr_t>(Get());
    if (!pool || !Offsets::Addr.offLocalPlayerName) return "";

    uintptr_t strAddr = pool + Offsets::Addr.offLocalPlayerName;
    size_t capacity = *reinterpret_cast<size_t*>(strAddr + 20);
    if (capacity >= 16) {
        const char* ptr = *reinterpret_cast<const char**>(strAddr);
        return ptr ? ptr : "";
    }
    return reinterpret_cast<const char*>(strAddr);
}

void CPlayerPool::SetLocalPlayerName(const char* szName) {
    CPlayerPool* pool = Get();
    if (!pool || !Offsets::Addr.fnSetLocalPlayerName || !szName) return;

    using SetLocalName_t = void(__thiscall*)(CPlayerPool*, const char*);
    reinterpret_cast<SetLocalName_t>(Offsets::Addr.fnSetLocalPlayerName)(pool, szName);
}
