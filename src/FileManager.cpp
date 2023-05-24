#include "../includes/FileManager.h"
#include "../includes/Utility.hpp"
#include "../includes/User.h"
#include "../includes/FileSystem.h"
#include "../includes/DeviceManager.h"
/*==========================class FileManager===============================*/
FileManager::FileManager()
{
    // nothing to do here
}

FileManager::~FileManager()
{
    // nothing to do here
    // debug ����ʱͬ������
    m_FileSystem->Update();
}

void FileManager::Initialize()
{
    this->m_FileSystem = FileSystem::getInst();

    this->m_InodeTable = &g_InodeTable;
    this->m_OpenFileTable = &g_OpenFileTable;

    this->m_InodeTable->Initialize();
    this->m_FileSystem->Initialize();
    /*���ø�Ŀ¼*/
}

/*
 * ���ܣ����ļ�
 * Ч�����������ļ��ṹ���ڴ�i�ڵ㿪�� ��i_count Ϊ������i_count ++��
 * */
int FileManager::Open(const char *path, int mode)
{
    Inode *pInode;
    User &u = *User::getInst();
    u.u_dirp = path;
    pInode = this->NameI(NextChar, FileManager::OPEN_); /* 0 = Open, not create */
    /* û���ҵ���Ӧ��Inode */
    if (NULL == pInode)
    {
        return -1;
    }
    this->Open1(pInode, mode, 0);
    return u.u_ar0[User::EAX];
}
int FileManager::MkNod(const char *path, int mode)
{
    Inode *pInode;
    User &u = *User::getInst();
    u.u_dirp = path;
    int number = -1;
    /* ���uid�Ƿ���root����ϵͳ����ֻ��uid==rootʱ�ſɱ����� */

    pInode = this->NameI(FileManager::NextChar, FileManager::CREATE_);
    /* Ҫ�������ļ��Ѿ�����,���ﲢ����ȥ���Ǵ��ļ� */
    if (pInode != NULL)
    {
        // u.u_error = User::EEXIST_;
        number = pInode->i_number;
        this->m_InodeTable->IPut(pInode);
        return number;
    }

    pInode = this->MakNode(mode);
    if (NULL == pInode)
    {
        return -1;
    }

    number = pInode->i_number;
    this->m_InodeTable->IPut(pInode);
    return number;
}
/*
 * ���ܣ�����һ���µ��ļ�(���ļ���)
 * Ч�����������ļ��ṹ���ڴ�i�ڵ㿪�� ��i_count Ϊ������Ӧ���� 1��
 * ���أ�fd
 * */
int FileManager::Creat(const char *path, int mode)
{
    Inode *pInode;
    User &u = *User::getInst();
    u.u_dirp = path;
    unsigned int newACCMode = mode;

    /* ����Ŀ¼��ģʽΪ1����ʾ����������Ŀ¼����д�������� */
    pInode = this->NameI(NextChar, FileManager::CREATE_);
    /* û���ҵ���Ӧ��Inode����NameI���� */
    if (NULL == pInode)
    {
        if (u.u_error)
            return -1;
        /* ����Inode */
        pInode = this->MakNode(newACCMode);
        /* ����ʧ�� */
        if (NULL == pInode)
        {
            return -1;
        }

        /*
         * �����ϣ�������ֲ����ڣ�ʹ�ò���trf = 2������open1()��
         * ����Ҫ����Ȩ�޼�飬��Ϊ�ոս������ļ���Ȩ�޺ʹ������mode
         * ����ʾ��Ȩ��������һ���ġ�
         */
        this->Open1(pInode, File::FWRITE, 2);
    }
    else
    {
        // if (mode & Inode::IFDIR)
        // {
        //     printf("directory%s exits\n", path);
        //     this->Open1(pInode, File::FWRITE, 0);
        // }
        // else
        {
            this->Open1(pInode, File::FWRITE, 1);
            pInode->i_mode |= newACCMode;
        }
    }
    return u.u_ar0[User::EAX];
}

/*
 * trf == 0��open����
 * trf == 1��creat���ã�creat�ļ���ʱ��������ͬ�ļ������ļ�
 * trf == 2��creat���ã�creat�ļ���ʱ��δ������ͬ�ļ������ļ��������ļ�����ʱ��һ������
 * mode���������ļ�ģʽ����ʾ�ļ������� ����д���Ƕ�д
 */
void FileManager::Open1(Inode *pInode, int mode, int trf)
{
    User &u = *User::getInst();

    /*
     * ����ϣ�����ļ��Ѵ��ڵ�����£���trf == 0��trf == 1����Ȩ�޼��
     * �����ϣ�������ֲ����ڣ���trf == 2������Ҫ����Ȩ�޼�飬��Ϊ�ս���
     * ���ļ���Ȩ�޺ʹ���Ĳ���mode������ʾ��Ȩ��������һ���ġ�
     */
    if (trf != 2)
    {
        if (mode & File::FREAD)
        {
            /* ����Ȩ�� */
            this->Access(pInode, Inode::IREAD);
        }
        if (mode & File::FWRITE)
        {
            /* ���дȨ�� */
            this->Access(pInode, Inode::IWRITE);
        }
    }

    if (u.u_error)
    {
        this->m_InodeTable->IPut(pInode);
        return;
    }

    /* ��creat�ļ���ʱ��������ͬ�ļ������ļ����ͷŸ��ļ���ռ�ݵ������̿� */
    if (1 == trf)
    {
        pInode->ITrunc();
    }

    /* ������ļ����ƿ�File�ṹ */
    File *pFile = this->m_OpenFileTable->FAlloc();
    if (NULL == pFile)
    {
        this->m_InodeTable->IPut(pInode);
        return;
    }
    /* ���ô��ļ���ʽ������File�ṹ���ڴ�Inode�Ĺ�����ϵ */
    pFile->f_flag = mode & (File::FREAD | File::FWRITE);
    pFile->f_inode = pInode;

    /* �����豸�򿪺��� */
    pInode->OpenI(mode & File::FWRITE);

    /* Ϊ�򿪻��ߴ����ļ��ĸ�����Դ���ѳɹ����䣬�������� */
    if (u.u_error == 0)
    {
        return;
    }
    else /* ����������ͷ���Դ */
    {
        /* �ͷŴ��ļ������� */
        int fd = u.u_ar0[User::EAX];
        if (fd != -1)
        {
            u.u_ofiles.SetF(fd, NULL);
            /* �ݼ�File�ṹ��Inode�����ü��� ,File�ṹû���� f_countΪ0�����ͷ�File�ṹ��*/
            pFile->f_count--;
        }
        this->m_InodeTable->IPut(pInode);
    }
}

void FileManager::Close(int fd)
{
    User &u = *User::getInst();

    /* ��ȡ���ļ����ƿ�File�ṹ */
    File *pFile = u.u_ofiles.GetF(fd);
    if (NULL == pFile)
    {
        return;
    }

    /* �ͷŴ��ļ�������fd���ݼ�File�ṹ���ü��� */
    u.u_ofiles.SetF(fd, NULL);
    this->m_OpenFileTable->CloseF(pFile);
}

int FileManager::Tellp(int fd)
{
    File *pFile;
    User &u = *User::getInst();

    pFile = u.u_ofiles.GetF(fd);
    if (NULL == pFile)
    {
        return -1; /* ��FILE�����ڣ�GetF��������� */
    }
    return pFile->f_offset;
}
void FileManager::Seek(int fd, int offset, int mode)
{
    File *pFile;
    User &u = *User::getInst();

    pFile = u.u_ofiles.GetF(fd);
    if (NULL == pFile)
    {
        return; /* ��FILE�����ڣ�GetF��������� */
    }

    switch (mode)
    {
    /* ��дλ������Ϊoffset */
    case 0:
        pFile->f_offset = offset;
        break;
    /* ��дλ�ü�offset(�����ɸ�) */
    case 1:
        pFile->f_offset += offset;
        break;
    /* ��дλ�õ���Ϊ�ļ����ȼ�offset */
    case 2:
        pFile->f_offset = pFile->f_inode->i_size + offset;
        break;
    }
}

void FileManager::Dup(int fd)
{
    File *pFile;
    User &u = *User::getInst();

    pFile = u.u_ofiles.GetF(fd);
    if (NULL == pFile)
    {
        return;
    }

    int newFd = u.u_ofiles.AllocFreeSlot();
    if (newFd < 0)
    {
        return;
    }
    /* ���˷�����������newFd�ɹ� */
    u.u_ofiles.SetF(newFd, pFile);
    pFile->f_count++;
}

// void FileManager::FStat(int fd)
// {
//     File *pFile;
//     User &u = *User::getInst();

//     pFile = u.u_ofiles.GetF(fd);
//     if (NULL == pFile)
//     {
//         return;
//     }

//     /* u.u_arg[1] = pStatBuf */
//     this->Stat1(pFile->f_inode, u.u_arg[1]);
// }

// void FileManager::Stat(int fd)
// {
//     Inode *pInode;
//     User &u = *User::getInst();

//     pInode = this->NameI(FileManager::NextChar, FileManager::OPEN_);
//     if (NULL == pInode)
//     {
//         return;
//     }
//     this->Stat1(pInode, u.u_arg[1]);
//     this->m_InodeTable->IPut(pInode);
// }

// void FileManager::Stat1(Inode *pInode, unsigned long statBuf)
// {
//     Buf *pBuf;
//     BufferManager &bufMgr = *BufferManager::getInst();

//     pInode->IUpdate(time(nullptr));
//     pBuf = bufMgr.Bread(pInode->i_dev, FileSystem::INODE_ZONE_START_SECTOR + pInode->i_number / FileSystem::INODE_NUMBER_PER_SECTOR);

//     /* ��pָ�򻺴����б��Ϊinumber���Inode��ƫ��λ�� */
//     unsigned char *p = pBuf->b_addr + (pInode->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
//     Utility::DWordCopy((int *)p, (int *)statBuf, sizeof(DiskInode) / sizeof(int));

//     bufMgr.Brelse(pBuf);
// }

int FileManager::Read(int fd, unsigned char *buffer, int count)
{
    /* ֱ�ӵ���Rdwr()�������� */
    return this->Rdwr(File::FREAD, fd, buffer, count);
}

int FileManager::Write(int fd, unsigned char *buffer, int count)
{
    /* ֱ�ӵ���Rdwr()�������� */
    return this->Rdwr(File::FWRITE, fd, buffer, count);
}

int FileManager::Rdwr(enum File::FileFlags mode, int fd, unsigned char *buffer, int count)
{
    File *pFile;
    User &u = *User::getInst();

    /* ����Read()/Write()��ϵͳ���ò���fd��ȡ���ļ����ƿ�ṹ */
    pFile = u.u_ofiles.GetF(fd); /* fd */
    if (NULL == pFile)
    {
        /* �����ڸô��ļ���GetF�Ѿ����ù������룬�������ﲻ��Ҫ�������� */
        /*	u.u_error = User::EBADF_;	*/
        return -1;
    }

    /* ��д��ģʽ��� ɾ��*/

    u.u_IOParam.m_Base = buffer; /* Ŀ�껺������ַ */
    u.u_IOParam.m_Count = count; /* Ҫ���/д���ֽ��� */
    u.u_segflg = 0;              /* User Space I/O�����������Ҫ�����ݶλ��û�ջ�� */

    /* �����ļ���ʼ��λ�� */
    u.u_IOParam.m_Offset = pFile->f_offset;
    if (File::FREAD == mode)
    {
        pFile->f_inode->ReadI();
    }
    else
    {
        pFile->f_inode->WriteI();
    }

    /* ���ݶ�д�������ƶ��ļ���дƫ��ָ�� */
    pFile->f_offset += (count - u.u_IOParam.m_Count);

    /* ����ʵ�ʶ�д���ֽ������޸Ĵ��ϵͳ���÷���ֵ�ĺ���ջ��Ԫ */
    return count - u.u_IOParam.m_Count;
}

/* ����NULL��ʾĿ¼����ʧ�ܣ������Ǹ�ָ�룬ָ���ļ����ڴ��i�ڵ� ���������ڴ�i�ڵ�  */
Inode *FileManager::NameI(char (*func)(), enum DirectorySearchMode mode)
{
    Inode *pInode;
    Buf *pBuf;
    char curchar;
    char *pChar;
    int freeEntryOffset; /* �Դ����ļ�ģʽ����Ŀ¼ʱ����¼����Ŀ¼���ƫ���� */
    User &u = *User::getInst();
    BufferManager &bufMgr = *BufferManager::getInst();

    /*
     * �����·����'/'��ͷ�ģ��Ӹ�Ŀ¼��ʼ������
     * ����ӽ��̵�ǰ����Ŀ¼��ʼ������
     */
    pInode = u.u_cdir;
    if ('/' == (curchar = (*func)()))
    {
        pInode = this->rootDirInode;
    }

    /* ����Inode�Ƿ����ڱ�ʹ�ã��Լ���֤������Ŀ¼���������и�Inode�����ͷ� */
    this->m_InodeTable->IGet(pInode->i_dev, pInode->i_number);

    /* �������////a//b ����·�� ����·���ȼ���/a/b */
    while ('/' == curchar)
    {
        curchar = (*func)();
    }
    /* �����ͼ���ĺ�ɾ����ǰĿ¼�ļ������ */
    if ('\0' == curchar && mode != FileManager::OPEN_)
    {

        goto out;
    }

    /* ���ѭ��ÿ�δ���pathname��һ��·������ */
    while (true)
    {

        /* ����·��������ϣ�������ӦInodeָ�롣Ŀ¼�����ɹ����ء� */
        if ('\0' == curchar)
        {
            return pInode;
        }

        /* ���Ҫ���������Ĳ���Ŀ¼�ļ����ͷ����Inode��Դ���˳� */
        if (!(pInode->i_mode & Inode::IFDIR))
        {
            u.u_error = User::ENOTDIR_;
            break; /* goto out; */
        }

        /* ����Ŀ¼����Ȩ�޼��,IEXEC��Ŀ¼�ļ��б�ʾ����Ȩ�� */
        if (this->Access(pInode, Inode::IEXEC))
        {

            break; /* ���߱�Ŀ¼����Ȩ�ޣ�goto out; */
        }

        /*
         * ��Pathname�е�ǰ׼������ƥ���·������������u.u_dbuf[]�У�
         * ���ں�Ŀ¼����бȽϡ�
         */
        pChar = &(u.u_dbuf[0]);
        while ('/' != curchar && '\0' != curchar)
        {
            if (pChar < &(u.u_dbuf[DirectoryEntry::DIRSIZ]))
            {
                *pChar = curchar;
                pChar++;
            }
            curchar = (*func)();
        }
        /* ��u_dbufʣ��Ĳ������Ϊ'\0' */
        while (pChar < &(u.u_dbuf[DirectoryEntry::DIRSIZ]))
        {
            *pChar = '\0';
            pChar++;
        }

        /* �������////a//b ����·�� ����·���ȼ���/a/b */
        while ('/' == curchar)
        {
            curchar = (*func)();
        }

        /* �ڲ�ѭ�����ֶ���u.u_dbuf[]�е�·���������������Ѱƥ���Ŀ¼�� */
        u.u_IOParam.m_Offset = 0;
        /* ����ΪĿ¼����� �����հ׵�Ŀ¼��*/
        u.u_IOParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ + 4);
        freeEntryOffset = 0;
        pBuf = NULL;

        while (true)
        {
            /* ��Ŀ¼���Ѿ�������� */
            if (0 == u.u_IOParam.m_Count)
            {
                if (NULL != pBuf)
                {
                    bufMgr.Brelse(pBuf);
                }
                /* ����Ǵ������ļ� */
                if (FileManager::CREATE_ == mode && curchar == '\0')
                {
                    /* �жϸ�Ŀ¼�Ƿ��д */
                    if (this->Access(pInode, Inode::IWRITE))
                    {
                        goto out; /* Failed */
                    }

                    /* ����Ŀ¼Inodeָ�뱣���������Ժ�дĿ¼��WriteDir()�������õ� */
                    u.u_pdir = pInode;

                    if (freeEntryOffset) /* �˱�������˿���Ŀ¼��λ��Ŀ¼�ļ��е�ƫ���� */
                    {
                        /* ������Ŀ¼��ƫ��������u���У�дĿ¼��WriteDir()���õ� */
                        u.u_IOParam.m_Offset = freeEntryOffset - (DirectoryEntry::DIRSIZ + 4);
                    }
                    else /*���⣺Ϊ��if��֧û����IUPD��־��  ������Ϊ�ļ��ĳ���û�б�ѽ*/
                    {
                        pInode->i_flag |= Inode::IUPD;
                    }
                    /* �ҵ�����д��Ŀ���Ŀ¼��λ�ã�NameI()�������� */
                    return NULL;
                }

                /* Ŀ¼��������϶�û���ҵ�ƥ����ͷ����Inode��Դ�����Ƴ� */
                u.u_error = User::ENOENT_;
                goto out;
            }

            /* �Ѷ���Ŀ¼�ļ��ĵ�ǰ�̿飬��Ҫ������һĿ¼�������̿� */
            if (0 == u.u_IOParam.m_Offset % Inode::BLOCK_SIZE)
            {
                if (NULL != pBuf)
                {
                    bufMgr.Brelse(pBuf);
                }
                /* ����Ҫ���������̿�� */
                int phyBlkno = pInode->Bmap(u.u_IOParam.m_Offset / Inode::BLOCK_SIZE);
                pBuf = bufMgr.Bread(pInode->i_dev, phyBlkno);
            }

            /* û�ж��굱ǰĿ¼���̿飬���ȡ��һĿ¼����u.u_dent */
            int *src = (int *)(pBuf->b_addr + (u.u_IOParam.m_Offset % Inode::BLOCK_SIZE));
            Utility::DWordCopy(src, (int *)&u.u_dent, sizeof(DirectoryEntry) / sizeof(int));

            u.u_IOParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);
            u.u_IOParam.m_Count--;

            /* ����ǿ���Ŀ¼���¼����λ��Ŀ¼�ļ���ƫ���� */
            if (0 == u.u_dent.m_ino)
            {
                if (0 == freeEntryOffset)
                {
                    freeEntryOffset = u.u_IOParam.m_Offset;
                }
                /* ��������Ŀ¼������Ƚ���һĿ¼�� */
                continue;
            }

            int i;
            for (i = 0; i < DirectoryEntry::DIRSIZ; i++)
            {
                if (u.u_dbuf[i] != u.u_dent.m_name[i])
                {
                    break; /* ƥ����ĳһ�ַ�����������forѭ�� */
                }
            }

            if (i < DirectoryEntry::DIRSIZ)
            {
                /* ����Ҫ������Ŀ¼�����ƥ����һĿ¼�� */
                continue;
            }
            else
            {
                /* Ŀ¼��ƥ��ɹ����ص����While(true)ѭ�� */
                break;
            }
        }

        /*
         * ���ڲ�Ŀ¼��ƥ��ѭ�������˴���˵��pathname��
         * ��ǰ·������ƥ��ɹ��ˣ�����ƥ��pathname����һ·��
         * ������ֱ������'\0'������
         */
        if (NULL != pBuf)
        {
            bufMgr.Brelse(pBuf);
        }

        /* �����ɾ���������򷵻ظ�Ŀ¼Inode����Ҫɾ���ļ���Inode����u.u_dent.m_ino�� */
        if (FileManager::DELETE_ == mode && '\0' == curchar)
        {
            /* ����Ը�Ŀ¼û��д��Ȩ�� */
            if (this->Access(pInode, Inode::IWRITE))
            {
                u.u_error = User::EACCES_;
                break; /* goto out; */
            }
            return pInode;
        }

        /*
         * ƥ��Ŀ¼��ɹ������ͷŵ�ǰĿ¼Inode������ƥ��ɹ���
         * Ŀ¼��m_ino�ֶλ�ȡ��Ӧ��һ��Ŀ¼���ļ���Inode��
         */
        short dev = pInode->i_dev;
        this->m_InodeTable->IPut(pInode);
        pInode = this->m_InodeTable->IGet(dev, u.u_dent.m_ino);
        /* �ص����While(true)ѭ��������ƥ��Pathname����һ·������ */

        if (NULL == pInode) /* ��ȡʧ�� */
        {
            return NULL;
        }
    }
out:
    this->m_InodeTable->IPut(pInode);
    return NULL;
}

Inode *FileManager::NameI(std::string name, enum DirectorySearchMode mode)
{
    Inode *pInode;
    Buf *pBuf;
    char curchar;
    char *pChar;
    int freeEntryOffset; /* �Դ����ļ�ģʽ����Ŀ¼ʱ����¼����Ŀ¼���ƫ���� */
    User &u = *User::getInst();
    BufferManager &bufMgr = *BufferManager::getInst();
    std::vector<std::string> dirlist;
    /*
     * �����·����'/'��ͷ�ģ��Ӹ�Ŀ¼��ʼ������
     * ����ӽ��̵�ǰ����Ŀ¼��ʼ������
     */
    pInode = u.u_cdir;
    if ('/' == name[0])
    {
        pInode = this->rootDirInode;
    }

    /* ����Inode�Ƿ����ڱ�ʹ�ã��Լ���֤������Ŀ¼���������и�Inode�����ͷ� */
    this->m_InodeTable->IGet(pInode->i_dev, pInode->i_number);
    if (!(pInode->i_mode & Inode::IFDIR))
    {
        u.u_error = User::ENOTDIR_;
        goto out; /* goto out; */
    }
    dirlist = Utility::Split(name, '/');

    for (int i = 0; i < dirlist.size() - 2; i++)
    {
        if (u.u_error != User::NOERROR_)
            break;

        if (dirlist[i].size() > DirectoryEntry::DIRSIZ - 1)
            break;
        char *dst = u.u_dbuf, *p = dst;
        const char *src = dirlist[i].c_str();
        while ((*dst++ = *src++) != 0)
            ;
        while (dst < &(u.u_dbuf[DirectoryEntry::DIRSIZ]))
            *dst++ = '\0';
        /* �ڲ�ѭ�����ֶ���u.u_dbuf[]�е�·���������������Ѱƥ���Ŀ¼�� */
        u.u_IOParam.m_Offset = 0;
        /* ����ΪĿ¼����� �����հ׵�Ŀ¼��*/
        u.u_IOParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ + 4);
        freeEntryOffset = 0;
        pBuf = NULL;
    }
    /* ���ѭ��ÿ�δ���pathname��һ��·������ */
    while (true)
    {
        /* ����������ͷŵ�ǰ��������Ŀ¼�ļ�Inode�����˳� */

        /* ����·��������ϣ�������ӦInodeָ�롣Ŀ¼�����ɹ����ء� */
        if ('\0' == curchar)
        {
            return pInode;
        }

        /* �ڲ�ѭ�����ֶ���u.u_dbuf[]�е�·���������������Ѱƥ���Ŀ¼�� */
        u.u_IOParam.m_Offset = 0;
        /* ����ΪĿ¼����� �����հ׵�Ŀ¼��*/
        u.u_IOParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ + 4);
        freeEntryOffset = 0;
        pBuf = NULL;

        while (true)
        {
            /* ��Ŀ¼���Ѿ�������� */
            if (0 == u.u_IOParam.m_Count)
            {
                if (NULL != pBuf)
                {
                    bufMgr.Brelse(pBuf);
                }
                /* ����Ǵ������ļ� */
                if (FileManager::CREATE_ == mode && curchar == '\0')
                {
                    /* �жϸ�Ŀ¼�Ƿ��д */
                    if (this->Access(pInode, Inode::IWRITE))
                    {
                        u.u_error = User::EACCES_;
                        goto out; /* Failed */
                    }

                    /* ����Ŀ¼Inodeָ�뱣���������Ժ�дĿ¼��WriteDir()�������õ� */
                    u.u_pdir = pInode;

                    if (freeEntryOffset) /* �˱�������˿���Ŀ¼��λ��Ŀ¼�ļ��е�ƫ���� */
                    {
                        /* ������Ŀ¼��ƫ��������u���У�дĿ¼��WriteDir()���õ� */
                        u.u_IOParam.m_Offset = freeEntryOffset - (DirectoryEntry::DIRSIZ + 4);
                    }
                    else /*���⣺Ϊ��if��֧û����IUPD��־��  ������Ϊ�ļ��ĳ���û�б�ѽ*/
                    {
                        pInode->i_flag |= Inode::IUPD;
                    }
                    /* �ҵ�����д��Ŀ���Ŀ¼��λ�ã�NameI()�������� */
                    return NULL;
                }

                /* Ŀ¼��������϶�û���ҵ�ƥ����ͷ����Inode��Դ�����Ƴ� */
                u.u_error = User::ENOENT_;
                goto out;
            }

            /* �Ѷ���Ŀ¼�ļ��ĵ�ǰ�̿飬��Ҫ������һĿ¼�������̿� */
            if (0 == u.u_IOParam.m_Offset % Inode::BLOCK_SIZE)
            {
                if (NULL != pBuf)
                {
                    bufMgr.Brelse(pBuf);
                }
                /* ����Ҫ���������̿�� */
                int phyBlkno = pInode->Bmap(u.u_IOParam.m_Offset / Inode::BLOCK_SIZE);
                pBuf = bufMgr.Bread(pInode->i_dev, phyBlkno);
            }

            /* û�ж��굱ǰĿ¼���̿飬���ȡ��һĿ¼����u.u_dent */
            int *src = (int *)(pBuf->b_addr + (u.u_IOParam.m_Offset % Inode::BLOCK_SIZE));
            Utility::DWordCopy(src, (int *)&u.u_dent, sizeof(DirectoryEntry) / sizeof(int));

            u.u_IOParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);
            u.u_IOParam.m_Count--;

            /* ����ǿ���Ŀ¼���¼����λ��Ŀ¼�ļ���ƫ���� */
            if (0 == u.u_dent.m_ino)
            {
                if (0 == freeEntryOffset)
                {
                    freeEntryOffset = u.u_IOParam.m_Offset;
                }
                /* ��������Ŀ¼������Ƚ���һĿ¼�� */
                continue;
            }

            int i;
            for (i = 0; i < DirectoryEntry::DIRSIZ; i++)
            {
                if (u.u_dbuf[i] != u.u_dent.m_name[i])
                {
                    break; /* ƥ����ĳһ�ַ�����������forѭ�� */
                }
            }

            if (i < DirectoryEntry::DIRSIZ)
            {
                /* ����Ҫ������Ŀ¼�����ƥ����һĿ¼�� */
                continue;
            }
            else
            {
                /* Ŀ¼��ƥ��ɹ����ص����While(true)ѭ�� */
                break;
            }
        }

        /*
         * ���ڲ�Ŀ¼��ƥ��ѭ�������˴���˵��pathname��
         * ��ǰ·������ƥ��ɹ��ˣ�����ƥ��pathname����һ·��
         * ������ֱ������'\0'������
         */
        if (NULL != pBuf)
        {
            bufMgr.Brelse(pBuf);
        }

        /* �����ɾ���������򷵻ظ�Ŀ¼Inode����Ҫɾ���ļ���Inode����u.u_dent.m_ino�� */
        if (FileManager::DELETE_ == mode && '\0' == curchar)
        {
            return pInode;
        }

        /*
         * ƥ��Ŀ¼��ɹ������ͷŵ�ǰĿ¼Inode������ƥ��ɹ���
         * Ŀ¼��m_ino�ֶλ�ȡ��Ӧ��һ��Ŀ¼���ļ���Inode��
         */
        short dev = pInode->i_dev;
        this->m_InodeTable->IPut(pInode);
        pInode = this->m_InodeTable->IGet(dev, u.u_dent.m_ino);
        /* �ص����While(true)ѭ��������ƥ��Pathname����һ·������ */

        if (NULL == pInode) /* ��ȡʧ�� */
        {
            return NULL;
        }
    }
out:
    this->m_InodeTable->IPut(pInode);
    return NULL;
}
char FileManager::NextChar()
{
    User &u = *User::getInst();

    /* u.u_dirpָ��pathname�е��ַ� */
    return *u.u_dirp++;
}

/* ��creat���á�
 * Ϊ�´������ļ�д�µ�i�ڵ���µ�Ŀ¼��
 * ���ص�pInode�����������ڴ�i�ڵ㣬���е�i_count�� 1��
 *
 * �ڳ����������� WriteDir��������������Լ���Ŀ¼��д����Ŀ¼���޸ĸ�Ŀ¼�ļ���i�ڵ� ������д�ش��̡�
 *
 */
Inode *FileManager::MakNode(unsigned int mode)
{
    Inode *pInode;
    User &u = *User::getInst();

    /* ����һ������DiskInode������������ȫ����� */
    pInode = this->m_FileSystem->IAlloc(u.u_pdir->i_dev);
    if (NULL == pInode)
    {
        return NULL;
    }

    pInode->i_flag |= (Inode::IACC | Inode::IUPD);
    pInode->i_mode = mode | Inode::IALLOC;
    pInode->i_nlink = 1;
    pInode->i_uid = u.u_uid;
    pInode->i_gid = 0;
    /* ��Ŀ¼��д��u.u_dent�����д��Ŀ¼�ļ� */
    this->WriteDir(pInode);
    return pInode;
}

void FileManager::WriteDir(Inode *pInode)
{
    User &u = *User::getInst();

    /* ����Ŀ¼����Inode��Ų��� */
    u.u_dent.m_ino = pInode->i_number;

    /* ����Ŀ¼����pathname�������� */
    for (int i = 0; i < DirectoryEntry::DIRSIZ; i++)
    {
        u.u_dent.m_name[i] = u.u_dbuf[i];
    }

    u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
    u.u_IOParam.m_Base = (unsigned char *)&u.u_dent;
    u.u_segflg = 1;

    /* ��Ŀ¼��д�븸Ŀ¼�ļ� */
    u.u_pdir->WriteI();
    this->m_InodeTable->IPut(u.u_pdir);
}

void FileManager::SetCurDir(const char *pathname)
{
    User &u = *User::getInst();

    /* ·�����ǴӸ�Ŀ¼'/'��ʼ����������u.u_curdir������ϵ�ǰ·������ */
    if (pathname[0] != '/')
    {
        int length = Utility::StringLength(u.u_curdir);
        if (u.u_curdir[length - 1] != '/')
        {
            u.u_curdir[length] = '/';
            length++;
        }
        Utility::StringCopy(pathname, u.u_curdir + length);
    }
    else /* ����ǴӸ�Ŀ¼'/'��ʼ����ȡ��ԭ�й���Ŀ¼ */
    {
        Utility::StringCopy(pathname, u.u_curdir);
    }
}

/*
 * ����ֵ��0����ʾӵ�д��ļ���Ȩ�ޣ�1��ʾû������ķ���Ȩ�ޡ��ļ�δ�ܴ򿪵�ԭ���¼��u.u_error�����С�
 */
int FileManager::Access(Inode *pInode, unsigned int mode)
{
    // User &u = *User::getInst();
    //  /* ����д��Ȩ�ޣ���������ļ�ϵͳ�Ƿ���ֻ���� */
    //  if (Inode::IWRITE == mode)
    //  {
    //      if (this->m_FileSystem->GetFS(pInode->i_dev)->s_ronly != 0)
    //      {
    //          u.u_error = User::EROFS_;
    //          return 1;
    //      }
    //  }
    //  /*
    //   * ���ڳ����û�����д�κ��ļ����������
    //   * ��Ҫִ��ĳ�ļ�ʱ��������i_mode�п�ִ�б�־
    //   */
    //  if (u.u_uid == 0)
    //  {
    //      if (Inode::IEXEC == mode && (pInode->i_mode & (Inode::IEXEC | (Inode::IEXEC >> 3) | (Inode::IEXEC >> 6))) == 0)
    //      {
    //          u.u_error = User::EACCES_;
    //          return 1;
    //      }
    //      return 0; /* Permission Check Succeed! */
    //  }

    // u.u_error = User::EACCES_;
    return 0;
}

void FileManager::ChDir(const char *path)
{
    Inode *pInode;
    User &u = *User::getInst();
    u.u_dirp = path;
    pInode = this->NameI(FileManager::NextChar, FileManager::OPEN_);
    if (NULL == pInode)
    {
        return;
    }
    /* ���������ļ�����Ŀ¼�ļ� */
    if (!(pInode->i_mode & Inode::IFDIR))
    {
        u.u_error = User::ENOTDIR_;
        this->m_InodeTable->IPut(pInode);
        return;
    }
    if (this->Access(pInode, Inode::IEXEC))
    {
        this->m_InodeTable->IPut(pInode);
        return;
    }
    this->m_InodeTable->IPut(u.u_cdir);
    u.u_cdir = pInode;
    this->SetCurDir(path /* pathname */);
}
void FileManager::UnLink(const char *path)
{
    Inode *pInode;
    Inode *pDeleteInode;
    User &u = *User::getInst();
    u.u_dirp = path;
    /*unlink�ļ��ĸ�Ŀ¼*/
    pDeleteInode = this->NameI(FileManager::NextChar, FileManager::DELETE_);
    if (NULL == pDeleteInode)
    {
        return;
    }
    /*unlink�ļ�����*/
    pInode = this->m_InodeTable->IGet(pDeleteInode->i_dev, u.u_dent.m_ino);
    if (NULL == pInode)
    {
        Utility::Panic("unlink -- iget");
    }

    /* д��������Ŀ¼�� */
    u.u_IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
    u.u_IOParam.m_Base = (unsigned char *)&u.u_dent;
    u.u_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;

    u.u_dent.m_ino = 0;
    pDeleteInode->WriteI();

    /* �޸�inode�� */
    pInode->i_nlink--;
    pInode->i_flag |= Inode::IUPD;

    this->m_InodeTable->IPut(pDeleteInode);
    this->m_InodeTable->IPut(pInode);
}
/*==========================class DirectoryEntry===============================*/
DirectoryEntry::DirectoryEntry()
{
    this->m_ino = 0;
    this->m_name[0] = '\0';
}

DirectoryEntry::~DirectoryEntry()
{
    // nothing to do here
}
