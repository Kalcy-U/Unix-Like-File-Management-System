#pragma once
#include "User.h"
#include "Filesystem.h"
#include "FileManager.h"
class Shell
{
    enum DebugMode
    {
        ON,
        OFF
    };

public:
    Shell();
    void Start(int mode = DebugMode::OFF);
    void Interface();

private:
    User *m_user;
    FileManager *m_fileManager;
};