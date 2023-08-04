
#include "../includes/Device.h"
#include "../includes/BufferManager.h"
#include "../includes/Utility.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
void VirtualDisk::quit()
{
    if (fstr.is_open())
        fstr.close();
    Utility::DebugInfo("VirtualDisk::~VirtualDisk : close dist %s\n", fstr.fail() ? "failed" : "succeed");
}
void VirtualDisk::reuse()
{
    if (!fstr.is_open())
    {
        fstr.open("img/disk.img", std::ios::binary | std::ios::in | std::ios::out);
        if (!fstr.is_open())
        {
            
            std::cout << "disk activate failed." << std::endl;
            return;
        }
        else
            std::cout << "disk is acitvated." << std::endl;
    }
    else
        std::cout << "disk is open." << std::endl;
}

VirtualDisk::VirtualDisk(int _devId, int _NSECTOR, char const *name) : BlockDevice(_devId)
{
    NSECTOR = _NSECTOR;
    if (name != nullptr && strlen(name) != 0)
    {
        strcpy(devname, name);
    }
    // TODO:OPEN_ MODE
    fstr.open(name, std::ios::binary | std::ios::in | std::ios::out);
    if (fstr.is_open())
    {
        std::cout << "disk [" << name << "] is ready." << std::endl;
        return;
    }
    std::cout << "disk activate failed." << std::endl;
}
int VirtualDisk::bnoToMem(int bno)
{
    // return MANAGEMENT_SIZE + bno * SECTOR_SIZE;
    return bno * SECTOR_SIZE;
}
int VirtualDisk::Read(Buf *bp)
{

    int bnum = bp->b_blkno;
    int mno = bnoToMem(bnum);
    fstr.seekg(mno, std::ios::beg);
    // debug 救命为什么是getline

    // fstr.getline((char *)bp->b_addr, SECTOR_SIZE, '\0');
    fstr.read((char *)bp->b_addr, SECTOR_SIZE);
    Utility::DebugInfo("VirtualDisk::Read : blk=%d,pos=mno=%d count=%d\n", bp->b_blkno, mno, fstr.gcount());
    return 0;
}
void VirtualDisk::Write(Buf *bp)
{
    int bnum = bp->b_blkno;
    int mno = bnoToMem(bnum);
    fstr.seekp(mno, std::ios::beg);
    Utility::DebugInfo("VirtualDisk::Write : blk=%d,pos=mno=%d %s\n", bp->b_blkno, mno, fstr.fail() ? "fail" : "succeed");
    fstr.write((char *)bp->b_addr, SECTOR_SIZE);
}
VirtualDisk::~VirtualDisk()
{
    // debug:析构前需要将延迟写的缓存写入磁盘
    BufferManager::getInst()->Bflush(devId);
    if (fstr.is_open())
        fstr.close();
    Utility::DebugInfo("VirtualDisk::~VirtualDisk : close dist %s\n", fstr.fail() ? "failed" : "succeed");
}
