#pragma once
#include <cstdint>

class CChat {
public:
    static CChat* Get();

    // Вывод сообщения с ником/префиксом (как сообщения игроков)
    void AddChatMessage(const char* szPrefix, uint32_t prefixColor, const char* szText);
    
    // Вывод обычной строки в чат (формат цвета: 0xAARRGGBB)
    void AddMessage(uint32_t color, const char* szText);
};
