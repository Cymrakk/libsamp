#pragma once
#include <cstdint>

class CPlayerPool {
public:
    static CPlayerPool* Get();
    static const char* GetLocalPlayerName();
    static void SetLocalPlayerName(const char* szName);
};
