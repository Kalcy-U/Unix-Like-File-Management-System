#ifndef DEVICE_H
#define DEVICE_H
#include "Buf.h"
#include <fstream>
#include <iostream>
class BlockDevice;
class VirtualDist;

class BlockDevice
{
public:
    int devId;
    BlockDevice(int _devId) : devId(_devId){};
    virtual ~BlockDevice(){};
    /*
     * 定义为虚函数，由派生类进行override实现设备
     * 特定操作。正常情况下，基类中函数不应被调用到。
     */
    virtual int Read(Buf *bp) { return 0; };
    virtual void Write(Buf *bp) { std::cout << "block device write" << std::endl; };
};

// 将.img大文件视为虚拟块设备
class VirtualDist : public BlockDevice
{
public:
    int NSECTOR; /* ATA磁盘扇区数 */
    static const int MANAGEMENT_SIZE = 512 * 1024;
    static const int SECTOR_SIZE = 512;
    char devname[100];
    std::fstream fstr;

public:
    VirtualDist(int _devId, int _NSECTOR, char const *name);
    virtual ~VirtualDist();
    int bnoToMem(int bno);
    virtual int Read(Buf *bp);
    virtual void Write(Buf *bp);
    virtual void quit();
    virtual void reuse();
};

#endif
