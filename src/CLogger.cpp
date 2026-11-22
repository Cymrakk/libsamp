#include "CLogger.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>

CRITICAL_SECTION CLogger::m_cs{};
bool CLogger::m_bInitialized = false;

void CLogger::Init() {
    if (m_bInitialized) return;

    InitializeCriticalSection(&m_cs);
    CreateDirectoryA(".\\SAMP", nullptr);

    // Очищаем файл при каждом новом запуске игры ("w")
    FILE* pFile = fopen(LOG_PATH, "w");
    if (pFile) {
        fclose(pFile);
    }

    m_bInitialized = true;
}

void CLogger::Shutdown() {
    if (!m_bInitialized) return;

    DeleteCriticalSection(&m_cs);
    m_bInitialized = false;
}

void CLogger::Log(const char* format, ...) {
    if (!m_bInitialized) return;

    EnterCriticalSection(&m_cs);

    FILE* pFile = fopen(LOG_PATH, "a");
    if (pFile) {
        SYSTEMTIME st{};
        GetLocalTime(&st);

        fprintf(pFile, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond);

        va_list args;
        va_start(args, format);
        vfprintf(pFile, format, args);
        va_end(args);

        fprintf(pFile, "\n");
        fclose(pFile);
    }

    LeaveCriticalSection(&m_cs);
}
