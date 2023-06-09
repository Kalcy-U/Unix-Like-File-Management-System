#ifndef BUF_H
#define BUF_H

/*
 * 缓存控制块buf定义
 * 记录了相应缓存的使用情况等信息；
 * 同时兼任I/O请求块，记录该缓存
 * 相关的I/O请求和执行结果。
 */
class Buf
{
public:
    enum BufFlag /* b_flags中标志位 */
    {
        B_WRITE = 0x1, /* 写操作。将缓存中的信息写到硬盘上去 */
        B_READ = 0x2,  /* 读操作。从盘读取信息到缓存中 */
        B_DONE = 0x4,  /* I/O操作结束 这个标识说明块可以重用*/
        // B_ERROR = 0x8,   /* I/O因出错而终止 */
        // B_BUSY = 0x10,   /* 相应缓存正在使用中 */
        // B_WANTED = 0x20, /* 有进程正在等待使用该buf管理的资源，清B_BUSY标志时，要唤醒这种进程 */
        B_ASYNC = 0x40,  /* 异步I/O，不需要等待其结束 */
        B_DELWRI = 0x80, /* 延迟写，在相应缓存要移做他用时，再将其内容写到相应块设备上 */
        B_CLEAR = 0x8,   /*新增标志：块是干净的*/
        B_USING = 0x10,  /*新增标志：块正在被进程使用*/
        // B_IO = 0x20
    };

public:
    unsigned int b_flags; /* 缓存控制块标志位 */
    /* 缓存控制块队列勾连指针 */
    Buf *b_forw;
    Buf *b_back;
    // Buf *av_forw;
    // Buf *av_back;
    int b_no;              /*Buffer在BufferManager的15个缓存中的序数*/
    int b_dev;             /* 主、次设备号，其中高8位是主设备号，低8位是次设备号 */
    int b_wcount;          /* 需传送的字节数 */
    unsigned char *b_addr; /* 指向该缓存控制块所管理的缓冲区的首地址 */
    int b_blkno;           /* 磁盘逻辑块号 */
    int b_error;           /* I/O出错时信息 */
    int b_resid;           /* I/O出错时尚未传送的剩余字节数 */

public:
    Buf()
    {
        b_no = -1;
        b_flags = B_CLEAR;
        b_forw = nullptr;
        b_back = nullptr;
        b_dev = -1;
        b_wcount = -1;
        b_addr = nullptr;
        b_blkno = -1;
        b_error = 0;
        b_resid = 0;
    };
};

#endif
