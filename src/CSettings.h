
#pragma once
#include <cstdint>

struct stSettings {
    char szNickName[24];
    char szHost[64];
    int  iPort;
    char szPassword[64];

    bool bFPSCounter;
    int  iChatLines;
    int  iChatFontSize;
};

class CSettings {
public:
    CSettings();
    ~CSettings() = default;

    void Load();
    void Save();
    void ToDefaults();

    const stSettings& GetReadOnly() const { return m_Settings; }
    stSettings& GetWrite() { return m_Settings; }

private:
    stSettings m_Settings{};
    static constexpr const char* CONFIG_PATH = ".\\SAMP\\samp.ini";
};
