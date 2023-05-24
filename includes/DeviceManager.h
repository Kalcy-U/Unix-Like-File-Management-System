#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "Device.h"
#define DEFALT_DEV 0
class DeviceManager
{
    /* static const member */
public:
    static const int MAX_DEVICE_NUM = 10; /* 系统允许最大块设备数量 */
    static const int NODEV = -1;          /* NODEV设备号 */
    static const short ROOTDEV = 0;       /* 磁盘的主、从设备号都为0 */
    static DeviceManager inst;

public:
    DeviceManager();
    ~DeviceManager();

    /* 初始化块设备基类指针数组。相当于是对块设备开关表bdevsw的初始化。*/

    int GetNBlkDev(); /* 获取系统中实际块设备数量nblkdev */

    BlockDevice *GetBlockDevice(int major); /* 根据主设备号major获取相应块设备对象的指针 */
    static DeviceManager *getInst() { return &inst; };

protected:
    int nblkdev;
    BlockDevice *DevTable[MAX_DEVICE_NUM]; /* 指向块设备基类的指针数组，相当于Unix V6中块设备开关表 */
};

#endif
