#include "Hooks.h"
#include "MinHook.h"
#include "hooks/CChatHook.h"

namespace Hooks {
    void Init() {
        MH_Initialize();
        CChatHook::Install();
    }

    void Shutdown() {
        CChatHook::Remove();
    }
}
