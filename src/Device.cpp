
#include "../includes/Device.h"
#include <cstring>
#include <fstream>
#include <iostream>
const int MANAGEMENT_SIZE = 512 * 128;

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
    return MANAGEMENT_SIZE + bno * NSECTOR;
}
int VirtualDist::Read(Buf *bp)
{

    int bnum = bp->b_blkno;
    int mno = bnoToMem(bnum);
    fstr.getline((char *)bp->b_addr, NSECTOR, '\0');

    return 0;
}
void VirtualDist::Write(Buf *bp)
{
    int bnum = bp->b_blkno;
    int mno = bnoToMem(bnum);
    fstr.seekp(mno, std::ios::beg);
    fstr.write((char *)bp->b_addr, 512);
    printf("VirtualDist::Write : %s tellp=%d\n", fstr.fail() ? "fail" : "succeed", fstr.tellp());
}
VirtualDist::~VirtualDist()
{
    if (fstr.is_open())
        fstr.close();
}