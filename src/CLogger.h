#pragma once
#include <windows.h>

class CLogger {
public:
    static void Init();
    static void Shutdown();
    static void Log(const char* format, ...);

private:
    static CRITICAL_SECTION m_cs;
    static bool m_bInitialized;
    static constexpr const char* LOG_PATH = ".\\SAMP\\libsamp.log";
};
