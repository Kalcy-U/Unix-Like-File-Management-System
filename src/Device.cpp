
#include "../includes/Device.h"
#include "../includes/BufferManager.h"
#include "../includes/Utility.hpp"

#include <cstring>
#include <fstream>
#include <iostream>


void VirtualDisk::ATARun()
{
    
    while(shouldTerminate.load()==false)
    {
        Buf *p;
       
        if (true)
        { // 制造排他锁作用域 在这里修改设备队列
            std::unique_lock<std::mutex> lk(mtxATA);
            condATA.wait(lk, [&]
                         { return shouldTerminate.load() ||devList.b_back != nullptr; });
            Utility::DebugInfo("VirtualDisk::ATARun : wake up\n");
            if (shouldTerminate.load())
                return;
            p = devList.b_back;
            devList.b_back = p->b_back;
            if (p->b_back)
                p->b_back->b_forw = p->b_forw;
        }
        if(true){
            
            Utility::DebugInfo("VirtualDisk::ATARun : befor rw\n");
            p->b_back = nullptr;
            p->b_forw = nullptr;

            if (p->b_flags & Buf::BufFlag::B_WRITE)
            {
                this->Write(p);
            }
            else if (p->b_flags & Buf::BufFlag::B_READ)
            {
                this->Read(p);
            }
            Utility::DebugInfo("VirtualDisk::ATARun : after\n");
            std::unique_lock<std::mutex> lkblk(BufferManager::getInst()->buf_mutex[p->b_no]);
            p->b_flags |= Buf::BufFlag::B_DONE;

            if(p->b_flags&Buf::BufFlag::B_ASYNC)
            {
                //直接释放块
                bool isWanted = p->b_flags & Buf::BufFlag::B_WANTED;
                BufferManager::getInst()->Brelse(p); // 去B_USING、B_ASYNC
                if (isWanted)
                {
                    p->condv.notify_all();
                }
            }
            else
            {
                //唤醒同步等待的进程 由bufferManger接管
                p->condv.notify_all();
            }
        }
        
    }
    return;
}

void VirtualDisk::quit()
{
    // debug:析构前需要将延迟写的缓存写入磁盘
    BufferManager::getInst()->Bflush(devId);
    shouldTerminate.store(true);
    condATA.notify_all();
    t_ATA.join();
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
/*把缓存加到队尾*/
void VirtualDisk::addToATA(Buf * bp)
{
    std::unique_lock<std::mutex> lk(mtxATA);
    Buf* p = &devList;
    bool wakeUpFlag = false;
    wakeUpFlag=(p->b_back == nullptr);
    while(p->b_back)
    {
        p=p->b_back;
    }

    p->b_back=bp;
    bp->b_forw=p;
    bp->b_back = nullptr;

    if (wakeUpFlag)
    {
        condATA.notify_one();
    }
    return;
}
VirtualDisk::VirtualDisk(int _devId, int _NSECTOR, char const *name) : BlockDevice(_devId), shouldTerminate(false)
{
    NSECTOR = _NSECTOR;
    if (name != nullptr && strlen(name) != 0)
    {
        strcpy(devname, name);
    }
    
    fstr.open(name, std::ios::binary | std::ios::in | std::ios::out);
    if (fstr.is_open())
    {
        t_ATA=std::thread(&VirtualDisk::ATARun,this);
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
    quit();
    
}
