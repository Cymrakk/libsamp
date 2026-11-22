#define _CRT_SECURE_NO_WARNINGS // Отключает ошибку C4996 для fopen в MSVC
#include "CSettings.h"
#include <windows.h>
#include <cstdio>
#include <cstring>

CSettings::CSettings() {
    Load();
}

bool CSettings::ReadBool(const char* section, const char* key, bool defVal) {
    char szBuf[16] = { 0 };
    GetPrivateProfileStringA(section, key, defVal ? "true" : "false", szBuf, sizeof(szBuf), CONFIG_PATH);
    return (_stricmp(szBuf, "true") == 0 || strcmp(szBuf, "1") == 0);
}

void CSettings::ToDefaults() {
    snprintf(m_Settings.szNickName, sizeof(m_Settings.szNickName), "Player_Cymrakk");
    snprintf(m_Settings.szHost, sizeof(m_Settings.szHost), "127.0.0.1");
    m_Settings.iPort = 7777;
    m_Settings.szPassword[0] = '\0';
    m_Settings.bFPSCounter = false;
    m_Settings.iChatLines = 10;
    m_Settings.iChatFontSize = 0;

    Save();
}

void CSettings::Save() {
    CreateDirectoryA(".\\SAMP", nullptr);

    FILE* pFile = fopen(CONFIG_PATH, "w");
    if (pFile) {
        fprintf(pFile, "; made by Cymrakk\n");
        fprintf(pFile, "[client]\n");
        fprintf(pFile, "NickName=%s\n", m_Settings.szNickName);
        fprintf(pFile, "host=%s\n", m_Settings.szHost);
        fprintf(pFile, "port=%d\n", m_Settings.iPort);
        fprintf(pFile, "\n");
        fprintf(pFile, "password=%s\n", m_Settings.szPassword);
        fprintf(pFile, "\n");
        fprintf(pFile, "[gui]\n");
        fprintf(pFile, "FPSCounter=%s\n", m_Settings.bFPSCounter ? "true" : "false");
        fprintf(pFile, "ChatLines=%d\n", m_Settings.iChatLines);
        fprintf(pFile, "ChatFontSize=%d\n", m_Settings.iChatFontSize);
        fclose(pFile);
    }
}

void CSettings::Load() {
    DWORD fileAttr = GetFileAttributesA(CONFIG_PATH);
    if (fileAttr == INVALID_FILE_ATTRIBUTES || (fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
        ToDefaults();
        return;
    }
    GetPrivateProfileStringA("client", "NickName", "Player_Cymrakk", m_Settings.szNickName, sizeof(m_Settings.szNickName), CONFIG_PATH);
    GetPrivateProfileStringA("client", "host", "127.0.0.1", m_Settings.szHost, sizeof(m_Settings.szHost), CONFIG_PATH);
    m_Settings.iPort = GetPrivateProfileIntA("client", "port", 7777, CONFIG_PATH);
    GetPrivateProfileStringA("client", "password", "", m_Settings.szPassword, sizeof(m_Settings.szPassword), CONFIG_PATH);

    m_Settings.bFPSCounter = ReadBool("gui", "FPSCounter", false);
    m_Settings.iChatLines = GetPrivateProfileIntA("gui", "ChatLines", 10, CONFIG_PATH);
    m_Settings.iChatFontSize = GetPrivateProfileIntA("gui", "ChatFontSize", 0, CONFIG_PATH);
}
