#include "CChatHook.h"
#include "MinHook.h"
#include "CLogger.h"
#include "game/Offsets.h"
#include "game/samp/CChat.h"
#include <cstring>
#include <cstdio>

namespace CChatHook {
    using AddEntry_t = void(__thiscall*)(CChat*, int, const char*, const char*, uint32_t, uint32_t);
    AddEntry_t origAddEntry = nullptr;

    void __fastcall Hooked_AddEntry(CChat* pThis, void* /*edx*/, int nType, const char* szText, const char* szPrefix, uint32_t textColor, uint32_t prefixColor) {
        if (nType != 8 || !szText) {
            origAddEntry(pThis, nType, szText, szPrefix, textColor, prefixColor);
            return;
        }

        char szTranslated[256] = { 0 };
        const char* outText = szText;
        int outType = nType;

        if (strcmp(szText, "-> OnFoot position saved") == 0) {
            outText = "-> Позиция пешком сохранена";
        }
        /*
            TODO: Add hook for message: SA-MP 0.3.7 Started
        */
        else if (strcmp(szText, "-> InCar position saved") == 0) {
            outText = "-> Позиция в транспорте сохранена";
        }
        else if (strcmp(szText, "Connected. Joining the game...") == 0) {
            outText = "Подключено. Вход в игру...";
        }
        else if (strcmp(szText, "The server didn't respond. Retrying..") == 0) {
            outText = "Сервер не отвечает. Повторная попытка...";
        }
        else if (strcmp(szText, "Server closed the connection.") == 0) {
            outText = "Сервер закрыл соединение.";
        }
        else if (strcmp(szText, "Lost connection to the server. Reconnecting..") == 0) {
            outText = "Соединение с сервером потеряно. Переподключение...";
        }
        else if (strcmp(szText, "You are banned from this server.") == 0) {
            outText = "Вы забанены на этом сервере.";
        }
        else if (strcmp(szText, "Wrong server password.") == 0) {
            outText = "Неверный пароль сервера.";
        }
        else if (strcmp(szText, "The server is full. Retrying...") == 0) {
            outText = "Сервер полон. Повторная попытка...";
        }
        else if (strcmp(szText, "Connection attempt failed. Reconnecting..") == 0) {
            outText = "Не удалось подключиться. Переподключение...";
        }
        else if (strcmp(szText, "Use /quit to exit or press ESC and select Quit Game") == 0) {
            outText = "Используйте /quit для выхода или нажмите ESC и выберите Выйти из игры";
        }
        else if (strcmp(szText, "I'm sorry, that's an invalid command.") == 0) {
            outText = "Извините, такой команды не существует.";
        }
        else if (strncmp(szText, "Connecting to ", 14) == 0) {
            //outText = "Подключение к серверу...";         // IN RELEASE
            snprintf(szTranslated, sizeof(szTranslated), "Подключение к %s", szText + 14);  // IN DEBUG
            outText = szTranslated;                                                         // IN DEBUG
        }
        else if (strncmp(szText, "Connected to ", 13) == 0) {
            snprintf(szTranslated, sizeof(szTranslated), "Вы подключились к %s", szText + 13);
            outText = szTranslated;
        }
        else if (strncmp(szText, "Screenshot Taken - ", 19) == 0) {
            snprintf(szTranslated, sizeof(szTranslated), "Скриншот сохранён - %s", szText + 19);
            outText = szTranslated;
        }

        origAddEntry(pThis, outType, outText, szPrefix, textColor, prefixColor);
    }

    void Install() {
        if (!Offsets::Addr.fnAddEntry) return;

        LPVOID target = reinterpret_cast<LPVOID>(Offsets::Addr.fnAddEntry);
        if (MH_CreateHook(target, &Hooked_AddEntry, reinterpret_cast<void**>(&origAddEntry)) == MH_OK) {
            MH_EnableHook(target);
            CLogger::Log("CChatHook installed at 0x%X", Offsets::Addr.fnAddEntry);
        } else {
            CLogger::Log("Failed to install CChatHook!");
        }
    }

    void Remove() {
        if (!Offsets::Addr.fnAddEntry) return;
        MH_DisableHook(reinterpret_cast<LPVOID>(Offsets::Addr.fnAddEntry));
    }
}
