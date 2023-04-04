using namespace std;

#include "./includes/BufferManager.h"
#include "./includes/DeviceManager.h"
#include "./includes/User.h"
// #include <Windows.h>
#include <cstring>
// User user;
DeviceManager DeviceManager::inst;
BufferManager BufferManager::inst;
User User::inst;
int main()
{

    DeviceManager &deviceManager = *(DeviceManager::getInst());
    BufferManager &bufferManager = *(BufferManager::getInst());

    Buf *b0 = bufferManager.Bread(1, 0);
    Buf *b1 = bufferManager.GetBlk(1, 1);
    strcpy((char *)b1->b_addr, (char *)b0->b_addr);
    b1->b_flags |= Buf::BufFlag::B_ASYNC;
    bufferManager.Bwrite(b1);
    printf("Ask to read (1,1) again.\n");
    bufferManager.Bread(1, 1);
    bufferManager.showFreeList();
    return 0;
}