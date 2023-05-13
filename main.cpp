#include "./includes/BufferManager.h"
#include "./includes/DeviceManager.h"
#include "./includes/User.h"
#include <cstring>
// User user;
DeviceManager DeviceManager::inst;
BufferManager BufferManager::inst;
FileSystem FileSystem::inst;
User User::inst;
int main()
{

    DeviceManager &deviceManager = *(DeviceManager::getInst());
    BufferManager &bufferManager = *(BufferManager::getInst());
    bufferManager.showFreeList();
    Buf *b0 = bufferManager.Bread(1, 0);
    Buf *b1 = bufferManager.GetBlk(1, 1);
    bufferManager.Brelse(b0);
    strcpy((char *)b1->b_addr, "abcdefg");
    b1->b_flags |= Buf::BufFlag::B_ASYNC;
    bufferManager.Bwrite(b1);
    printf("Ask to read (1,1) again.\n");
    bufferManager.Bread(1, 1);
    bufferManager.showFreeList();
    return 0;
}