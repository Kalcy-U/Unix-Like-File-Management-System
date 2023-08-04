
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
    static User inst;

    enum ErrorCode
    {
        NOERROR_ = 0, /* No error */
        EPERM_ = 1,   /* Operation not permitted */
        ENOENT_ = 2,  /* No such file or directory */
        ESRCH_ = 3,   /* No such process */

        ENXIO_ = 6, /* No such device or address */

        EBADF_ = 9, /* Bad file number */

        EACCES_ = 13, /* Permission denied */
        EFAULT_ = 14, /* Bad address */
     

        EEXIST_ = 17, /* File exists */
        EXDEV_ = 18,  /* Cross-device link */

        ENOTDIR_ = 20, /* Not a directory */
        EISDIR_ = 21,  /* Is a directory */
        EINVAL_ = 22,  /* Invalid argument */
        ENFILE_ = 23,  /* File table overflow */
        EMFILE_ = 24,  /* Too many open files */
        ENOTTY_ = 25,  /* Not a typewriter(terminal) */
        ETXTBSY_ = 26, /* Text file busy */
        EFBIG_ = 27,   /* File too large */
        ENOSPC_ = 28,  /* No space left on device */
        ESPIPE_ = 29,  /* Illegal seek */
        EROFS_ = 30,   /* Read-only file system */
        EMLINK_ = 31,  /* Too many links */
        EPIPE_ = 32,   /* Broken pipe */
        ENOSYS_ = 100
        // EFAULT_	= 106
    };

public:
    /* 系统调用相关成员 */

    unsigned int u_ar0[5]; /* 指向核心栈现场保护区中EAX寄存器
    //                      存放的栈单元，本字段存放该栈单元的地址。
    //                      在V6中r0存放系统调用的返回值给用户程序，
    //                      x86平台上使用EAX存放返回值，替代u.u_ar0[R0] */

    int u_arg[5];       /* 存放当前系统调用参数 */
    const char *u_dirp; /* 系统调用参数(一般用于Pathname)的指针 */

    /* 文件系统相关成员 */
    Inode *u_cdir;                       /* 指向当前目录的Inode指针 */
    Inode *u_pdir;                       /* 指向父目录的Inode指针 */
    ErrorCode u_error;                   /* 存放错误码 */
    DirectoryEntry u_dent;               /* 当前目录的目录项 */
    char u_dbuf[DirectoryEntry::DIRSIZ]; /* 当前路径分量 */
    char u_curdir[128];                  /* 当前工作目录完整路径 */

    int u_segflg; /* 表明I/O针对用户或系统空间 */

    /* 进程的用户标识 */
    short u_uid; /* 有效用户ID */

    /* 文件系统相关成员 */
    OpenFiles u_ofiles; /* 进程打开文件描述符表对象 */

    /* 文件I/O操作 */
    IOParameter u_IOParam; /* 记录当前读、写文件的偏移量，用户目标区域和剩余字节数参数 */

    /*debug*/
    int Debug;
    /* Member Functions */
public:
    /* 获取当前用户工作目录 */
    const char *Pwd() { return u_curdir; };
    User(int debug = 1)
    {
        Debug = debug;
        u_uid = 0;
    };
    static User *getInst() { return &inst; };
    void clear() { u_error = NOERROR_; };
};

#endif
