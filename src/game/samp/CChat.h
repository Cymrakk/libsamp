// CChat.h
#pragma once
#include <cstdint>

class CChat {
public:
    static CChat* Get();

    void AddEntry(int nType, const char* szText, const char* szPrefix, uint32_t textColor, uint32_t prefixColor);
    void AddMessage(uint32_t color, const char* szText);
};
