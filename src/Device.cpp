
#include "../includes/Device.h"
#include "../includes/BufferManager.h"
#include "../includes/Utility.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
void VirtualDist::quit()
{
    if (fstr.is_open())
        fstr.close();
    printf("VirtualDist::~VirtualDist : close dist %s\n", fstr.fail() ? "failed" : "succeed");
}
void VirtualDist::reuse()
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

VirtualDist::VirtualDist(int _devId, int _NSECTOR, char const *name) : BlockDevice(_devId)
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
int VirtualDist::bnoToMem(int bno)
{
    // return MANAGEMENT_SIZE + bno * SECTOR_SIZE;
    return bno * SECTOR_SIZE;
}
int VirtualDist::Read(Buf *bp)
{

    int bnum = bp->b_blkno;
    int mno = bnoToMem(bnum);
    fstr.seekg(mno, std::ios::beg);
    // debug 救命为什么是getline

    // fstr.getline((char *)bp->b_addr, SECTOR_SIZE, '\0');
    fstr.read((char *)bp->b_addr, SECTOR_SIZE);
    printf("buffer read no=%d,pos=mno=%d count=%d\n", bp->b_blkno, mno, fstr.gcount());
    return 0;
}
void VirtualDist::Write(Buf *bp)
{
    int bnum = bp->b_blkno;
    int mno = bnoToMem(bnum);
    fstr.seekp(mno, std::ios::beg);
    printf("VirtualDist::Write : pos=mno=%d %s\n", mno, fstr.fail() ? "fail" : "succeed");
    fstr.write((char *)bp->b_addr, SECTOR_SIZE);
    printf("VirtualDist::Write : %s tellp=%d,count=%d\n", fstr.fail() ? "fail" : "succeed", int(fstr.tellp()), int(fstr.tellp()) - mno);
}
VirtualDist::~VirtualDist()
{
    // debug:析构前需要将延迟写的缓存写入磁盘
    BufferManager::getInst()->Bflush(devId);
    if (fstr.is_open())
        fstr.close();
    printf("VirtualDist::~VirtualDist : close dist %s\n", fstr.fail() ? "failed" : "succeed");
}