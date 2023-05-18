#include "./includes/BufferManager.h"
#include "./includes/DeviceManager.h"
#include "./includes/User.h"
#include "./includes/FileSystem.h"
#include "./includes/Utility.hpp"
#include <cstring>

#define TEST_BUFFER false
#define TEST_FILE false
#define TEST_SUPERBLOCK true
#define INIT_SUPERBLOCK_EMPTY true
#define TEST_FLUSH false
DeviceManager DeviceManager::inst;
BufferManager BufferManager::inst;
FileSystem FileSystem::inst;
FileManager FileManager::inst;
User User::inst;

int main()
{
    DeviceManager &deviceManager = *(DeviceManager::getInst());
    BufferManager &bufferManager = *(BufferManager::getInst());
    FileManager &fileManager = *(FileManager::getInst());

    if (TEST_BUFFER)
    {
        bufferManager.showFreeList();
        Buf *b0 = bufferManager.Bread(0, 0);
        Buf *b1 = bufferManager.GetBlk(0, 1);
        bufferManager.Brelse(b0);
        strcpy((char *)b1->b_addr, "abcdefg");
        b1->b_flags |= Buf::BufFlag::B_ASYNC;
        bufferManager.Bwrite(b1);
        printf("Ask to read (0,1) again.\n");
        bufferManager.Bread(0, 1);
        bufferManager.showFreeList();
    }
    if (TEST_FLUSH)
    {
        bufferManager.showFreeList();
        Buf *b0 = bufferManager.GetBlk(0, 202);
        Buf *b1 = bufferManager.GetBlk(0, 203);
        Buf *b2 = bufferManager.GetBlk(0, 204);

        strcpy((char *)b0->b_addr, "abcdefg");
        strcpy((char *)b1->b_addr, "hijklmn");
        strcpy((char *)b2->b_addr, "hello");
        bufferManager.Bdwrite(b0);
        bufferManager.Bdwrite(b1);
        bufferManager.Bdwrite(b2);
        bufferManager.showFreeList();
        bufferManager.Bflush(0);
        bufferManager.showFreeList();
    }
    if (TEST_SUPERBLOCK)
    {
        if (INIT_SUPERBLOCK_EMPTY)
        {
            SuperBlock sb;
            sb.s_isize = 200;                   /* 外存Inode区占用的盘块数 */
            sb.s_fsize = 1024 * 1024 * 8 / 512; /* 盘块总数 */
            sb.s_nfree = 100;                   /* 直接管理的空闲盘块数量 */
            for (int i = 0; i < 100; i++)
            {
                sb.s_free[i] = 1024 + i;
            }
            sb.s_ninode = 100; /* 直接管理的空闲外存Inode数量 */
            for (int i = 0; i < 100; i++)
            {
                sb.s_inode[i] = i;
            }
            sb.s_flock = 0;            /* 封锁空闲盘块索引表标志 */
            sb.s_ilock = 0;            /* 封锁空闲Inode表标志 */
            sb.s_fmod = 1;             /* 内存中super block副本被修改标志，意味着需要更新外存对应的Super Block */
            sb.s_ronly = 0;            /* 本文件系统只能读出 */
            sb.s_time = time(nullptr); /* 最近一次更新时间 */
            fileManager.m_FileSystem->Update();
        }
    }
    if (TEST_FILE)
    {
    }
    return 0;
}