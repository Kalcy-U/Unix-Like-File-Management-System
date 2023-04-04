
#include "../includes/DeviceManager.h"

#define PATH "E:\\courses\\OS\\FileManageSys\\myFileSys\\img"
DeviceManager::DeviceManager()
{
    nblkdev = 1;
    DevTable[0] = new VirtualDist(1, 1024 * 32, (std::string(PATH) + "\\disk.img").c_str());
    for (int i = 1; i < MAX_DEVICE_NUM; i++)
    {
        DevTable[i] = nullptr;
    }
}

DeviceManager::~DeviceManager()
{
    for (int i = 0; i < MAX_DEVICE_NUM; i++)
    {
        if (DevTable[i] != nullptr)
            delete DevTable[i];
    }
}

int DeviceManager::GetNBlkDev()
{
    return nblkdev;
}

BlockDevice *DeviceManager::GetBlockDevice(int major)
{
    // TODO: 在此处插入 return 语句
    for (int i = 0; i < MAX_DEVICE_NUM; i++)
    {
        if (major == DevTable[i]->devId)
            return DevTable[i];
    }
    return nullptr;
}
