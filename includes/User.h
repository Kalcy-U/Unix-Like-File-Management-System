#ifndef USER_H
#define USER_H

#include "File.h"
#include "FileManager.h"
#include "Inode.h"

/*
 * @comment 该类与Unixv6中 struct user结构对应，因此只改变
 * 类名，不修改成员结构名字，关于数据类型的对应关系如下:
 */
class User
{
public:
    static const int EAX = 0; /* u.u_ar0[EAX]；访问现场保护区中EAX寄存器的偏移量 */

public:
    /* 文件系统相关成员 */
    Inode *u_cdir; /* 指向当前目录的Inode指针 */
    Inode *u_pdir; /* 指向父目录的Inode指针 */

    // DirectoryEntry u_dent;               /* 当前目录的目录项 */
    // char u_dbuf[DirectoryEntry::DIRSIZ]; /* 当前路径分量 */
    char u_curdir[128]; /* 当前工作目录完整路径 */

    int u_segflg; /* 表明I/O针对用户或系统空间 */

    /* 进程的用户标识 */
    short u_uid; /* 有效用户ID */

    /* 文件系统相关成员 */
    OpenFiles u_ofiles; /* 进程打开文件描述符表对象 */

    /* 文件I/O操作 */
    IOParameter u_IOParam; /* 记录当前读、写文件的偏移量，用户目标区域和剩余字节数参数 */

    /* Member Functions */
public:
    /* 获取当前用户工作目录 */
    void Pwd();
};

#endif
