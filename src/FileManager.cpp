#include "../includes/FileManager.h"
#include "../includes/Utility.hpp"
#include "../includes/User.h"
#include "../includes/FileSystem.h"
#include "../includes/precompile.h"
/*==========================class FileManager===============================*/
FileManager::FileManager()
{
    // nothing to do here
}

FileManager::~FileManager()
{
    // nothing to do here
}

void FileManager::Initialize()
{
    this->m_FileSystem = FileSystem::getInst();

    this->m_InodeTable = &g_InodeTable;
    this->m_OpenFileTable = &g_OpenFileTable;

    this->m_InodeTable->Initialize();
    this->m_FileSystem->Initialize();
}

/*
 * ���ܣ����ļ�
 * Ч�����������ļ��ṹ���ڴ�i�ڵ㿪�� ��i_count Ϊ������i_count ++��
 * */
void FileManager::Open()
{
    Inode *pInode;
    User &u = *User::getInst();

    pInode = this->NameI(NextChar, FileManager::OPEN_); /* 0 = Open, not create */
    /* û���ҵ���Ӧ��Inode */
    if (NULL == pInode)
    {
        return;
    }
    this->Open1(pInode, u.u_arg[1], 0);
}

/*
 * ���ܣ�����һ���µ��ļ�
 * Ч�����������ļ��ṹ���ڴ�i�ڵ㿪�� ��i_count Ϊ������Ӧ���� 1��
 * */
void FileManager::Creat()
{
    Inode *pInode;
    User &u = *User::getInst();
    unsigned int newACCMode = u.u_arg[1] & (Inode::IRWXU | Inode::IRWXG | Inode::IRWXO);

    /* ����Ŀ¼��ģʽΪ1����ʾ����������Ŀ¼����д�������� */
    pInode = this->NameI(NextChar, FileManager::CREATE_);
    /* û���ҵ���Ӧ��Inode����NameI���� */
    if (NULL == pInode)
    {
        if (u.u_error)
            return;
        /* ����Inode */
        pInode = this->MakNode(newACCMode & (~Inode::ISVTX));
        /* ����ʧ�� */
        if (NULL == pInode)
        {
            return;
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
        /* ���NameI()�������Ѿ�����Ҫ�������ļ�������ո��ļ������㷨ITrunc()����UIDû�иı�
         * ԭ��UNIX��������������ļ�����ȥ�����½����ļ�һ����Ȼ�������ļ������ߺ����Ȩ��ʽû�䡣
         * Ҳ����˵creatָ����RWX������Ч��
         * ������Ϊ���ǲ�����ģ�Ӧ�øı䡣
         * ���ڵ�ʵ�֣�creatָ����RWX������Ч */
        this->Open1(pInode, File::FWRITE, 1);
        pInode->i_mode |= newACCMode;
    }
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
            /* ϵͳ����ȥдĿ¼�ļ��ǲ������ */
            if ((pInode->i_mode & Inode::IFMT) == Inode::IFDIR)
            {
                u.u_error = User::EISDIR_;
            }
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

void FileManager::Close()
{
    User &u = *User::getInst();
    int fd = u.u_arg[0];

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

void FileManager::Seek()
{
    File *pFile;
    User &u = *User::getInst();
    int fd = u.u_arg[0];

    pFile = u.u_ofiles.GetF(fd);
    if (NULL == pFile)
    {
        return; /* ��FILE�����ڣ�GetF��������� */
    }

    /* �ܵ��ļ�������seek */
    if (pFile->f_flag & File::FPIPE)
    {
        u.u_error = User::ESPIPE_;
        return;
    }

    int offset = u.u_arg[1];

    /* ���u.u_arg[2]��3 ~ 5֮�䣬��ô���ȵ�λ���ֽڱ�Ϊ512�ֽ� */
    if (u.u_arg[2] > 2)
    {
        offset = offset << 9;
        u.u_arg[2] -= 3;
    }

    switch (u.u_arg[2])
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

void FileManager::Dup()
{
    File *pFile;
    User &u = *User::getInst();
    int fd = u.u_arg[0];

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

void FileManager::FStat()
{
    File *pFile;
    User &u = *User::getInst();
    int fd = u.u_arg[0];

    pFile = u.u_ofiles.GetF(fd);
    if (NULL == pFile)
    {
        return;
    }

    /* u.u_arg[1] = pStatBuf */
    this->Stat1(pFile->f_inode, u.u_arg[1]);
}

void FileManager::Stat()
{
    Inode *pInode;
    User &u = *User::getInst();

    pInode = this->NameI(FileManager::NextChar, FileManager::OPEN_);
    if (NULL == pInode)
    {
        return;
    }
    this->Stat1(pInode, u.u_arg[1]);
    this->m_InodeTable->IPut(pInode);
}

void FileManager::Stat1(Inode *pInode, unsigned long statBuf)
{
    Buf *pBuf;
    BufferManager &bufMgr = *BufferManager::getInst();

    pInode->IUpdate(time(nullptr));
    pBuf = bufMgr.Bread(pInode->i_dev, FileSystem::INODE_ZONE_START_SECTOR + pInode->i_number / FileSystem::INODE_NUMBER_PER_SECTOR);

    /* ��pָ�򻺴����б��Ϊinumber���Inode��ƫ��λ�� */
    unsigned char *p = pBuf->b_addr + (pInode->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
    Utility::DWordCopy((int *)p, (int *)statBuf, sizeof(DiskInode) / sizeof(int));

    bufMgr.Brelse(pBuf);
}

void FileManager::Read()
{
    /* ֱ�ӵ���Rdwr()�������� */
    this->Rdwr(File::FREAD);
}

void FileManager::Write()
{
    /* ֱ�ӵ���Rdwr()�������� */
    this->Rdwr(File::FWRITE);
}

void FileManager::Rdwr(enum File::FileFlags mode)
{
    File *pFile;
    User &u = *User::getInst();

    /* ����Read()/Write()��ϵͳ���ò���fd��ȡ���ļ����ƿ�ṹ */
    pFile = u.u_ofiles.GetF(u.u_arg[0]); /* fd */
    if (NULL == pFile)
    {
        /* �����ڸô��ļ���GetF�Ѿ����ù������룬�������ﲻ��Ҫ�������� */
        /*	u.u_error = User::EBADF_;	*/
        return;
    }

    /* ��д��ģʽ����ȷ */
    if ((pFile->f_flag & mode) == 0)
    {
        u.u_error = User::EACCES_;
        return;
    }

    u.u_IOParam.m_Base = (unsigned char *)u.u_arg[1]; /* Ŀ�껺������ַ */
    u.u_IOParam.m_Count = u.u_arg[2];                 /* Ҫ���/д���ֽ��� */
    u.u_segflg = 0;                                   /* User Space I/O�����������Ҫ�����ݶλ��û�ջ�� */

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
    pFile->f_offset += (u.u_arg[2] - u.u_IOParam.m_Count);

    /* ����ʵ�ʶ�д���ֽ������޸Ĵ��ϵͳ���÷���ֵ�ĺ���ջ��Ԫ */
    u.u_ar0[User::EAX] = u.u_arg[2] - u.u_IOParam.m_Count;
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
        u.u_error = User::ENOENT_;
        goto out;
    }

    /* ���ѭ��ÿ�δ���pathname��һ��·������ */
    while (true)
    {
        /* ����������ͷŵ�ǰ��������Ŀ¼�ļ�Inode�����˳� */
        if (u.u_error != User::NOERROR_)
        {
            break; /* goto out; */
        }

        /* ����·��������ϣ�������ӦInodeָ�롣Ŀ¼�����ɹ����ء� */
        if ('\0' == curchar)
        {
            return pInode;
        }

        /* ���Ҫ���������Ĳ���Ŀ¼�ļ����ͷ����Inode��Դ���˳� */
        if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
        {
            u.u_error = User::ENOTDIR_;
            break; /* goto out; */
        }

        /* ����Ŀ¼����Ȩ�޼��,IEXEC��Ŀ¼�ļ��б�ʾ����Ȩ�� */
        if (this->Access(pInode, Inode::IEXEC))
        {
            u.u_error = User::EACCES_;
            break; /* ���߱�Ŀ¼����Ȩ�ޣ�goto out; */
        }

        /*
         * ��Pathname�е�ǰ׼������ƥ���·������������u.u_dbuf[]�У�
         * ���ں�Ŀ¼����бȽϡ�
         */
        pChar = &(u.u_dbuf[0]);
        while ('/' != curchar && '\0' != curchar && u.u_error == User::NOERROR_)
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

        if (u.u_error != User::NOERROR_)
        {
            break; /* goto out; */
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
    // pInode->i_gid = u.u_gid;
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

void FileManager::SetCurDir(char *pathname)
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
    User &u = *User::getInst();

    /* ����д��Ȩ�ޣ���������ļ�ϵͳ�Ƿ���ֻ���� */
    if (Inode::IWRITE == mode)
    {
        if (this->m_FileSystem->GetFS(pInode->i_dev)->s_ronly != 0)
        {
            u.u_error = User::EROFS_;
            return 1;
        }
    }
    /*
     * ���ڳ����û�����д�κ��ļ����������
     * ��Ҫִ��ĳ�ļ�ʱ��������i_mode�п�ִ�б�־
     */
    if (u.u_uid == 0)
    {
        if (Inode::IEXEC == mode && (pInode->i_mode & (Inode::IEXEC | (Inode::IEXEC >> 3) | (Inode::IEXEC >> 6))) == 0)
        {
            u.u_error = User::EACCES_;
            return 1;
        }
        return 0; /* Permission Check Succeed! */
    }

    u.u_error = User::EACCES_;
    return 1;
}

void FileManager::ChDir()
{
    Inode *pInode;
    User &u = *User::getInst();

    pInode = this->NameI(FileManager::NextChar, FileManager::OPEN_);
    if (NULL == pInode)
    {
        return;
    }
    /* ���������ļ�����Ŀ¼�ļ� */
    if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
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
    this->SetCurDir((char *)u.u_arg[0] /* pathname */);
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
