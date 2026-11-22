// This is CNetGame.cpp
#include "CNetGame.h"
#include "game/Offsets.h"

CNetGame* CNetGame::Get() {
    if (!Offsets::Addr.pSampInfo) return nullptr;
    return *reinterpret_cast<CNetGame**>(Offsets::Addr.pSampInfo);
}
