
#include "../includes/Device.h"
#include <cstring>
#include <fstream>
#include <iostream>

VirtualDist::VirtualDist(int _devId, int _NSECTOR, char const *name) : BlockDevice(_devId)
{
    NSECTOR = _NSECTOR;
    if (name != nullptr && strlen(name) != 0)
    {
        strcpy(devname, name);
    }
    // TODO:OPEN MODE
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
    fstr.getline((char *)bp->b_addr, SECTOR_SIZE, '\0');

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
    if (fstr.is_open())
        fstr.close();
    printf("VirtualDist::~VirtualDist : close dist %s\n", fstr.fail() ? "failed" : "succeed");
}