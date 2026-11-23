#include "Hooks.h"
#include "MinHook.h"
#include "hooks/CChatHook.h"
#include "hooks/CNetGameHook.h"
#include "hooks/CPlayerPoolHook.h"

namespace Hooks {
    void Init() {
        MH_Initialize();
        CChatHook::Install();
        CNetGameHook::Install();
        CPlayerPoolHook::Install();
    }

    void Shutdown() {
        CChatHook::Remove();
        CNetGameHook::Remove();
        CPlayerPoolHook::Remove();
    }
}
