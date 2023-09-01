#ifndef DEVICE_H
#define DEVICE_H
#include "Buf.h"
#include <fstream>
#include <iostream>
#include<thread>
#include <atomic>

class BlockDevice;
class VirtualDisk;

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
    virtual void addToATA(Buf *bp){};
    virtual void ATARun(){}; // 启动ATA磁盘驱动器
};

// 将.img大文件视为虚拟块设备
class VirtualDisk : public BlockDevice
{
public:
    int NSECTOR; /* ATA磁盘扇区数 */
    static const int SECTOR_SIZE = 512;
    char devname[100];
    std::fstream fstr;
    Buf devList;
    std::thread t_ATA;//守护进程
    std::atomic<bool> shouldTerminate;//守护进程结束的通知
    std::condition_variable condATA;
    std::mutex mtxATA;
    int bnoToMem(int bno);
    virtual int Read(Buf *bp);
    virtual void Write(Buf *bp);

public:
    VirtualDisk(int _devId, int _NSECTOR, char const *name);
    virtual ~VirtualDisk();
 
    virtual void ATARun();//启动ATA磁盘驱动器

    virtual void quit();
    virtual void reuse();
    virtual void addToATA(Buf *bp);
};

#endif
