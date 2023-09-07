#include "../includes/Buf.h"
#include "../includes/BufferManager.h"
#include "../includes/DeviceManager.h"
#include "../includes/precompile.h"
#include "../includes/Utility.hpp"
#include <iomanip>
#include <string.h>
#include <exception>

BufferManager::BufferManager()
{
    memset((void *)Buffer, 0, NBUF * BUFFER_SIZE);
    // 初始化Buf实际地址
    for (int i = 0; i < this->NBUF; i++)
    {
        m_Buf[i].b_addr = Buffer[i];
        m_Buf[i].b_back = ((i + 1) < NBUF ? (&m_Buf[i + 1]) : nullptr);
        m_Buf[i].b_forw = (i - 1) >= 0 ? &m_Buf[i - 1] : &bFreeList;
        m_Buf[i].b_no = i;
        m_Buf[i].b_flags = Buf::BufFlag::B_USING;
        Brelse(m_Buf+i);
    }

    bFreeList.b_back = &m_Buf[0];
    bFreeList.b_forw = nullptr;
}
void BufferManager::showFreeList()
{
    Buf *buf_first = bFreeList.b_back;
    int count = 0;
    printf("b_no ,blkno,flags,WRITE ,READ  ,DONE  ,WANTED,ASYNC ,DELWRI,USING \n");
   
    while (buf_first != nullptr)
    {
        //翻译B_FLAG
        int B_WRITE = (buf_first->b_flags & Buf::BufFlag::B_WRITE) ? 1 : 0;
        int B_READ = (buf_first->b_flags & Buf::BufFlag::B_READ)?1:0;
        int B_DONE = (buf_first->b_flags & Buf::BufFlag::B_DONE) ? 1 : 0;
        int B_WANTED =( buf_first->b_flags & Buf::BufFlag::B_WANTED) ? 1 : 0;
        int B_ASYNC= (buf_first->b_flags & Buf::BufFlag::B_ASYNC) ? 1 : 0;
        int B_DELWRI = (buf_first->b_flags & Buf::BufFlag::B_DELWRI) ? 1 : 0;
        int B_USING = (buf_first->b_flags & Buf::BufFlag::B_USING) ? 1 : 0;

        printf("5d,%5d,%5d,%6d,%6d,%6d,%6d,%6d,%6d,%6d\n", buf_first->b_no, buf_first->b_dev, buf_first->b_blkno, 
            buf_first->b_flags, B_WRITE, B_READ, B_DONE, B_WANTED, B_ASYNC, B_DELWRI, B_USING);
        buf_first = buf_first->b_back;
        count++;
    }
    printf("free buffer count=%d\n", count);
    
}
void BufferManager::NotAvail(Buf *bp)
{
    
    // 如果bp是队尾bp->b_back为null
    if (bp->b_back)
        bp->b_back->b_forw = bp->b_forw;
    bp->b_forw->b_back = bp->b_back;
    bp->b_back = nullptr;
    bp->b_forw = nullptr;
    /* 设置B_BUSY标志 */
    bp->b_flags |= Buf::B_USING;
    
    return;
}
/// @brief 获得一个缓存块。
/// @param dev 设备
/// @param blkno 块号
/// @return
Buf *BufferManager::GetBlk(int dev, int blkno)
{
    // 搜索全部缓存，看有没有可重用的

    Buf *buf_reuse = nullptr;
    if (blkno < 0)
    {
        Utility::Panic("blk值无效\n");
    }

    for (int i = 0; i < NBUF; i++)
    {
        if (m_Buf[i].b_dev == dev && m_Buf[i].b_blkno == blkno)
        {
            buf_reuse = &m_Buf[i];
            break;
        }
    }

    // 找不到可重用块，取出自由队列队首
    Buf *buf_first = bFreeList.b_back;
   
    if (buf_reuse == nullptr && buf_first)
    {
        NotAvail(buf_first);
        if (buf_first->b_flags & Buf::BufFlag::B_DELWRI)
        {
            // 直接送Bwrite
            buf_first->b_flags |= Buf::BufFlag::B_ASYNC;
            Bwrite(buf_first); // 包含了块释放
            return GetBlk(dev, blkno);
        }
        buf_first->b_blkno = blkno;
        buf_first->b_dev = dev;
        // debug 之前没有把flag覆盖，重用的块带有B_DONE标记，误以为IO已完成
        buf_first->b_flags = Buf::BufFlag::B_USING;

        return buf_first;
    }
    // 找不到任何空闲块（？）
    else if (buf_reuse == nullptr && !bFreeList.b_back)
    {
        // 睡会儿算了 UNIX其实是在自由队列头指针加了B_WANTED，等有缓存被释放时唤醒
        sleepms(200);
        return GetBlk(dev, blkno);
    }
    /* 已重构
    // 找到了可重用的缓存
    // 如果是自由的，加B_USING标志，移出队列
    // B_USING同时Getblk？应该是块正在进行异步IO。等开锁。
    if (buf_reuse != nullptr && (buf_reuse->b_flags & (Buf::BufFlag::B_USING)))
    {
        buf_mutex[buf_reuse->b_no].lock();
        buf_mutex[buf_reuse->b_no].unlock();
        NotAvail(buf_reuse);
        return buf_reuse;
    }*/
    // 找到了可重用的缓存
    // 如果是自由的，加B_USING标志，移出队列
    // B_USING同时Getblk？应该是块正在进行异步IO。加B_WANTED等开锁
    if (buf_reuse != nullptr && (buf_reuse->b_flags & (Buf::BufFlag::B_USING)))
    {
        
        //加标志
        buf_reuse->b_flags |= Buf::BufFlag::B_WANTED;

        IOwait(buf_reuse);
        return GetBlk(dev,blkno);
    }

    else if (buf_reuse != nullptr )
    {
        // 可重用
        NotAvail(buf_reuse);
        return buf_reuse;
    }

    return nullptr;
}
/// @brief 释放缓存，放入自由队列队尾 带有锁;如果B_WANTED，那么唤醒并清除WANTED
/// @param bp
void BufferManager::Brelse(Buf *bp)
{
    std::unique_lock<std::mutex>lk(buf_mutex[bp->b_no]);
    if (bp != nullptr && (bp->b_flags & Buf::B_USING)) // 如果B_USING=false，不再执行
    {
        
        // 放入队尾
        Buf *freeP = &bFreeList;
        while (freeP->b_back != nullptr)
            freeP = freeP->b_back;
        freeP->b_back = bp;
        bp->b_forw = freeP;
        bp->b_back = nullptr;
        /*一定要最后再改标志*/
        bp->b_flags &= ~(Buf::B_USING | Buf::B_ASYNC);
        //如果B_WANTED 唤醒一个等待的进程
        
        if (bp->b_flags & Buf::B_WANTED)
        {
            bp->b_flags &= ~Buf::B_WANTED;
            lk.unlock();
            bp->condv.notify_all();
        }
    }
}
/// @brief 读一个块，包含请求块、同步读、加done标，不含块释放
/// @param dev
/// @param blkno
/// @return
Buf *BufferManager::Bread(int dev, int blkno)
{
    // 查设备表
    Buf *bp = GetBlk(dev, blkno);
    /* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
    if (bp->b_flags & Buf::B_DONE)
    {
        return bp;
    }
    bp->b_flags |= Buf::BufFlag::B_READ;
   
    DeviceManager::getInst()->GetBlockDevice(dev)->addToATA(bp);
    //同步等待IO结束
    IOwait(bp);
    Utility::DebugInfo("Bread return\n");
    return bp;
}

Buf &BufferManager::GetBFreeList()
{
    return bFreeList;
}

void BufferManager::IOwait(Buf*bp)
{
    std::unique_lock<std::mutex> lk(buf_mutex[bp->b_no]);
    bp->condv.wait(lk);
}


/// @brief 拿到一个块并已经对缓存完成处理后，加B_USING,完成同步或异步写；如果是同步，设置B_DONE标志并释放
/// @param bp
void BufferManager::Bwrite(Buf *bp)
{
    unsigned int flags;

    
    flags = bp->b_flags;
    bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_DELWRI);
    bp->b_flags |= (Buf::B_WRITE | Buf::B_USING);
    bp->b_wcount = BufferManager::BUFFER_SIZE;
    DeviceManager::getInst()->GetBlockDevice(bp->b_dev)->addToATA(bp);//送IO队列并唤醒
    if ((flags & Buf::B_ASYNC) == 0)
    {
        /* 同步写，需要等待I/O操作结束 */
        IOwait(bp);
        Brelse(bp);
    }
    //异步不负责释放
    
    return;
}
/// @brief 延迟写，调整记号并释放。
/// @param bp
void BufferManager::Bdwrite(Buf *bp)
{
    /* 置上B_DONE允许其它进程使用该磁盘块内容 */
    bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
    bp->b_wcount = BufferManager::BUFFER_SIZE;
    this->Brelse(bp);
    return;
}
/// @brief 异步写，完成后释放。
/// @param bp
void BufferManager::Bawrite(Buf *bp)
{
    /* 标记为异步写 */
    bp->b_flags |= (Buf::B_WRITE|Buf::B_ASYNC | Buf::B_USING);
    DeviceManager::getInst()->GetBlockDevice(bp->b_dev)->addToATA(bp);
    Brelse(bp);
    return;
}
/// @brief 清缓存
/// @param bp
void BufferManager::ClrBuf(Buf *bp)
{
    void *pvoid = bp->b_addr;

    /* 将缓冲区中数据清零 */
    memset((void *)pvoid, 0, BUFFER_SIZE);

    return;
}
void BufferManager::Bflush(short dev)
{
    Buf *freeP = bFreeList.b_back;
    while (freeP != nullptr)
    {

        if (freeP->b_flags & Buf::B_DELWRI)
        {
            this->NotAvail(freeP);
            this->Bwrite(freeP);
        }
        freeP = freeP->b_back;
    }
    return;
}