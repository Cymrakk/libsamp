#include "Hooks.h"
#include "MinHook.h"
#include "hooks/CChatHook.h"
#include "hooks/CNetGameHook.h" // <-- Добавили

namespace Hooks {
    void Init() {
        MH_Initialize();
        CChatHook::Install();
        CNetGameHook::Install(); // <-- Добавили
    }

    void Shutdown() {
        CChatHook::Remove();
        CNetGameHook::Remove(); // <-- Добавили
    }
}
