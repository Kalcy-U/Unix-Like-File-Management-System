#include "./includes/BufferManager.h"
#include "./includes/DeviceManager.h"
#include "./includes/User.h"
#include "./includes/FileSystem.h"
#include "./includes/Utility.hpp"
#include <cstring>

#define TEST_BUFFER 0
#define TEST_FLUSH 0
#define FORMAT_DISK 0
#define TEST_CREATE 0
#define TEST_WRITE 0
#define TEST_READ 1
// 静态对象实例化
//  涉及到析构时资源有序释放，因此实例化的顺序不能改
DeviceManager DeviceManager::inst;
BufferManager BufferManager::inst;
User User::inst;
FileSystem FileSystem::inst;
FileManager FileManager::inst;

int main()
{
    DeviceManager &deviceManager = *(DeviceManager::getInst());
    BufferManager &bufferManager = *(BufferManager::getInst());
    User &user = *User::getInst();
    FileSystem &fileSystem = *(FileSystem::getInst());
    FileManager &fileManager = *(FileManager::getInst());
    fileManager.Initialize();
    Inode *pRootInode = FORMAT_DISK ? NULL : fileManager.rootDirInode = g_InodeTable.IGet(DeviceManager::ROOTDEV, FileSystem::ROOTINO);
    User &us = *User::getInst();
    us.u_cdir = pRootInode;
    us.u_pdir = pRootInode;
    Utility::StringCopy("/", us.u_curdir);

    if (TEST_BUFFER)
    {
        bufferManager.showFreeList();
        Buf *b0 = bufferManager.Bread(0, 202);

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
    if (FORMAT_DISK)
    {

        fileSystem.formatDisk(DEFALT_DEV);

        /*读取根设备的sb*/
        // fileSystem.LoadSuperBlock();
        // Buf *b0 = fileSystem.Alloc(DEFALT_DEV);
        // Inode *pI = fileSystem.IAlloc(DEFALT_DEV);

        SuperBlock *sb1 = fileSystem.m_Mount[0].m_spb; // 断点调试查看内容
        // fileSystem.Update();
        bufferManager.Bflush(DEFALT_DEV);

        return 0;
    }
    if (TEST_CREATE)
    {
        fileSystem.LoadSuperBlock();

        // debug:之前创建了目录项，但磁盘Inode中的模式没有写对，导致读取目录文件失败。后面虽然修改了模式，但因为目录项已经存在，所以跳过了Inode模式更新.只能格式化重来了
        // 有时候会出现很神秘的问题
        // debug：使用create创建并打开文件，不包括修改内存Inode。导致Inode并没有写回.

        int inode_u = fileManager.MkNod("/user", Inode::IFDIR | Inode::IRWXU);
        int inode_h = fileManager.MkNod("/home", Inode::IFDIR | Inode::IRWXU);
        Inode *pI = fileManager.m_InodeTable->IGet(0, inode_h);
        int inode_d = fileManager.MkNod("/dev", Inode::IFDIR | Inode::IRWXU);
        int inode_t = fileManager.MkNod("/home/texts", Inode::IFDIR | Inode::IRWXU);
        fileManager.MkNod("/home/reports", Inode::IFDIR | Inode::IRWXU);
        fileManager.MkNod("/home/photos", Inode::IFDIR | Inode::IRWXU);
        fileManager.ChDir("/home/texts");
        int fd1 = fileManager.Creat("Jerry.txt", Inode::IRWXU);
        fileManager.Close(fd1);
        bufferManager.Bflush(DEFALT_DEV);

        // 读取根目录表项
        unsigned char buffer[201] = {0};

        int fd2 = fileManager.Open("/", File::FREAD); /*返回fd写入u.u_ar0 嵌套层数太多不好直接返回*/
        DirectoryEntry etr[3];
        fileManager.Read(fd2, buffer, 32 * 3);
        Utility::DWordCopy((int *)buffer, (int *)etr, 32 * 3);
        printf("entry0 ino=%d,name=%s\n", etr[0].m_ino, etr[0].m_name);
    }
    if (TEST_WRITE)
    {
        fileSystem.LoadSuperBlock();
        int fd = fileManager.Open("/home/texts/Jerry.txt", File::FWRITE | File::FREAD);
        if (fd >= 0)
        {
            char str[200] = "0123456789012345678901234567890123456789";
            int count = fileManager.Write(fd, (unsigned char *)str, 30);
            Inode *pInode = user.u_ofiles.GetF(fd)->f_inode;
            printf("inode.number=%d,isize=%d\n", pInode->i_number, pInode->i_size);
            fileManager.Close(fd);
        }
    }
    if (TEST_WRITE)
    {
        fileSystem.LoadSuperBlock();
        int fd = fileManager.Open("/home/texts/Jerry.txt", File::FWRITE | File::FREAD);
        if (fd >= 0)
        {
            char str[200] = "0123456789012345678901234567890123456789";
            int count = fileManager.Write(fd, (unsigned char *)str, 30);
            File *pFile = user.u_ofiles.GetF(fd);
            Inode *pInode = pFile->f_inode;
            printf("inode.number=%d,isize=%d,tellp=%d\n", pInode->i_number, pInode->i_size, fileManager.Tellp(fd));
            fileManager.Close(fd);
        }
    }
    if (TEST_READ)
    {
        fileSystem.LoadSuperBlock();
    }
    return 0;
}