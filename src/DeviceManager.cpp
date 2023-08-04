
#include "../includes/DeviceManager.h"
#include "../includes/properties.h"

DeviceManager::DeviceManager()
{
    nblkdev = 1;
    DevTable[0] = new VirtualDisk(DEFALT_DEV, 1024 * 32, (std::string(PATH) + "/disk.img").c_str());
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
    for (int i = 0; i < MAX_DEVICE_NUM; i++)
    {
        if (major == DevTable[i]->devId)
            return DevTable[i];
    }
    return nullptr;
}
