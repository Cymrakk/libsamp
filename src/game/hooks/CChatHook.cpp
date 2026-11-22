#include "CChatHook.h"
#include "MinHook.h"
#include "CLogger.h"
#include "game/Offsets.h"
#include "game/samp/CChat.h"
#include <cstring>
#include <cstdio>

namespace CChatHook {
    using AddMessage_t = void(__thiscall*)(CChat*, uint32_t, const char*);
    AddMessage_t origAddMessage = nullptr;

    void __fastcall Hooked_AddMessage(CChat* pThis, void* /*edx*/, uint32_t color, const char* szText) {
        if (!szText) {
            origAddMessage(pThis, color, szText);
            return;
        }

        char szTranslated[256] = { 0 };
        const char* outText = szText;

        // 1. Точные совпадения
        if (strcmp(szText, "Connected. Joining the game...") == 0) {
            outText = "Подключено. Подготовка к игре...";
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
        // 2. Шаблоны с параметрами (IP, порт, название сервера, скриншот)
        else if (strncmp(szText, "SA-MP ", 6) == 0 && strstr(szText, "Started")) {
            snprintf(szTranslated, sizeof(szTranslated), "SA-MP %s запущен (LibSamp)", Offsets::GetVersionName());
            outText = szTranslated;
        }
        else if (strncmp(szText, "Connecting to ", 14) == 0) {
            // Оригинал: "Connecting to 127.0.0.1:7777..."
            const char* addrPart = szText + 14;
            snprintf(szTranslated, sizeof(szTranslated), "Подключение к %s", addrPart);
            outText = szTranslated;
        }
        else if (strncmp(szText, "Connected to ", 13) == 0) {
            // Оригинал: "Connected to Server Name"
            const char* serverName = szText + 13;
            snprintf(szTranslated, sizeof(szTranslated), "Вы подключились к %s", serverName);
            outText = szTranslated;
        }
        else if (strncmp(szText, "Screenshot Taken - ", 19) == 0) {
            // Оригинал: "Screenshot Taken - sa-mp-000.png"
            const char* fileName = szText + 19;
            snprintf(szTranslated, sizeof(szTranslated), "Скриншот сохранён - %s", fileName);
            outText = szTranslated;
        }

        // Вызываем оригинал, сохраняя исходный color
        origAddMessage(pThis, color, outText);
    }

    void Install() {
        if (!Offsets::Addr.fnAddMessage) return;

        LPVOID target = reinterpret_cast<LPVOID>(Offsets::Addr.fnAddMessage);
        if (MH_CreateHook(target, &Hooked_AddMessage, reinterpret_cast<void**>(&origAddMessage)) == MH_OK) {
            MH_EnableHook(target);
            CLogger::Log("CChatHook installed at 0x%X", Offsets::Addr.fnAddMessage);
        } else {
            CLogger::Log("Failed to install CChatHook!");
        }
    }

    void Remove() {
        if (!Offsets::Addr.fnAddMessage) return;
        MH_DisableHook(reinterpret_cast<LPVOID>(Offsets::Addr.fnAddMessage));
    }
}
