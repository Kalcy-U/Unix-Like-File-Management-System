#include "./includes/BufferManager.h"
#include "./includes/DeviceManager.h"
#include "./includes/User.h"
#include "./includes/FileSystem.h"
#include "./includes/Utility.hpp"
#include <cstring>

#define TEST_BUFFER false
#define TEST_FILE false
#define TEST_SUPERBLOCK false
#define INIT_SUPERBLOCK_EMPTY false
#define TEST_FLUSH true
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
    FileSystem &fileSystem = *(FileSystem::getInst());
    fileManager.Initialize();
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
            fileSystem.formatDisk(DEFALT_DEV);
        }
        /*读取根设备的sb*/
        fileSystem.LoadSuperBlock();
        Buf *b0 = fileSystem.Alloc(DEFALT_DEV);
        Inode *pI = fileSystem.IAlloc(DEFALT_DEV);

        SuperBlock *sb1 = fileSystem.m_Mount[0].m_spb; // 断点调试查看内容
        fileSystem.Update();
        bufferManager.Bflush(DEFALT_DEV);
    }
    if (TEST_FILE)
    {
    }
    return 0;
}