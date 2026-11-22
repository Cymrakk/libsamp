#pragma once
#include <cstdint>

class CChat {
public:
    static CChat* Get();
    void AddChatMessage(const char* szPrefix, uint32_t prefixColor, const char* szText);
    void AddMessage(uint32_t color, const char* szText);
};
