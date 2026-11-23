#include "Hooks.h"
#include "hooks/CChatHook.h"
#include "hooks/CNetGameHook.h"

namespace Hooks {
    void Init() {
        CChatHook::Install();
        CNetGameHook::Install();
    }

    void Shutdown() {
        CChatHook::Remove();
        CNetGameHook::Remove();
    }
}
