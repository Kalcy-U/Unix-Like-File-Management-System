#pragma once
#include "User.h"
#include "FileSystem.h"
#include "FileManager.h"
#include "BufferManager.h"
#include "DeviceManager.h"
class Shell
{

public:
    enum DebugMode
    {
        OFF,
        ON
    };

    Shell();
    void Start(int mode = DebugMode::OFF);

    void Usage();

protected:
    User *m_user;
    FileManager *m_fileManager;
    FileSystem *m_fileSystem;
    DeviceManager *m_deviceManager;
    BufferManager *m_bufferManager;
    char buffer[1000];
    void Interface();
};