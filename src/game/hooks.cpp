#include "Hooks.h"
#include "MinHook.h"
#include "hooks/CChatHook.h"
#include "hooks/CNetGameHook.h"

namespace Hooks {
    void Init() {
        MH_Initialize();
        CChatHook::Install();
        CNetGameHook::Install();
    }

    void Shutdown() {
        CChatHook::Remove();
        CNetGameHook::Remove();
    }
}
