#include "../includes/FileSystem.h"
#include "../includes/DeviceManager.h"
#include "../includes/User.h"
#include "../includes/Utility.hpp"

/*==============================class SuperBlock===================================*/
/* ϵͳȫ�ֳ�����SuperBlock���� */
SuperBlock g_spb;

SuperBlock::SuperBlock()
{
	// nothing to do here
}

SuperBlock::~SuperBlock()
{
	// nothing to do here
}

/*==============================class Mount===================================*/
Mount::Mount()
{
	this->m_dev = -1;
	this->m_spb = NULL;
	this->m_inodep = NULL;
}

Mount::~Mount()
{
	this->m_dev = -1;
	this->m_inodep = NULL;
	// �ͷ��ڴ�SuperBlock����
	if (this->m_spb != NULL)
	{
		// delete this->m_spb;
		this->m_spb = NULL;
	}
}

/*==============================class FileSystem===================================*/
FileSystem::FileSystem()
{
	// nothing to do here
	for (int i = 0; i < FileSystem::NMOUNT; i++)
	{
		this->m_Mount[i].m_spb = NULL;
		this->m_Mount[i].m_dev = -1;
		this->m_Mount[i].m_inodep = NULL;
	}
}

FileSystem::~FileSystem()
{
	// nothing to do here
}

void FileSystem::Initialize()
{

	this->m_BufferManager = BufferManager::getInst();

	this->updlock = 0;
}
void FileSystem::formatDisk(int dev)
{
	DeviceManager &dv = *DeviceManager::getInst();
	// ϵͳ���ú����������������ذ�
	//  VirtualDisk *disk = dynamic_cast<VirtualDisk *>(dv.GetBlockDevice(DEFALT_DEV));
	//  disk->quit();
	//  system("cd img & del disk.img & fsutil file createnew disk.img 8388608");

	// disk->reuse();

	SuperBlock sb;
	this->m_Mount[0].m_dev = DeviceManager::ROOTDEV;
	this->m_Mount[0].m_spb = &g_spb;

	sb.s_isize = 200;		/* ���Inode��ռ�õ��̿��� */
	sb.s_fsize = BLOCK_NUM; /* �̿����� */
	sb.s_nfree = 0;			/* ֱ�ӹ���Ŀ����̿����� */
	for (int i = 0; i < 100; i++)
	{
		sb.s_free[i] = 0;
	}
	sb.s_ninode = 0; /* ֱ�ӹ���Ŀ������Inode���� */
	/*�����̿����s_free*/

	for (int i = 0; i < 100; i++)
	{
		sb.s_inode[i] = 0;
	}
	sb.s_flock = 0;			   /* ���������̿��������־ */
	sb.s_ilock = 0;			   /* ��������Inode���־ */
	sb.s_fmod = 0;			   /* �ڴ���super block�������޸ı�־����ζ����Ҫ��������Ӧ��Super Block */
	sb.s_ronly = 0;			   /* ���ļ�ϵͳֻ�ܶ��� */
	sb.s_time = time(nullptr); /* ���һ�θ���ʱ�� */
	for (int i = 0; i < sizeof(sb.padding) / sizeof(int); i++)
	{
		sb.padding[i] = 0;
	}
	for (int j = 0; j < 2; j++)
	{
		/* ��һ��pָ��SuperBlock�ĵ�0�ֽڣ��ڶ���pָ���512�ֽ� */
		int *p = (int *)&sb + j * 128;

		/* ��Ҫд�뵽�豸dev�ϵ�SUPER_BLOCK_SECTOR_NUMBER + j������ȥ */
		Buf *pBuf = m_BufferManager->GetBlk(dev, FileSystem::SUPER_BLOCK_SECTOR_NUMBER + j);

		/* ��SuperBlock�е�0 - 511�ֽ�д�뻺���� */
		Utility::DWordCopy(p, (int *)pBuf->b_addr, 128);

		/* ���������е�����д�������� */
		m_BufferManager->Bwrite(pBuf);
	}
	m_BufferManager->Bflush(dev);
	LoadSuperBlock();

	for (int i = sb.s_fsize - 1; i >= DATA_ZONE_START_SECTOR; --i)
	{
		this->Free(dev, i);
	}
	Inode rootdir;
	rootdir.i_dev = 0;
	rootdir.i_size = 0;
	rootdir.i_nlink = 1;
	rootdir.i_count = 1;
	rootdir.i_flag = Inode::IUPD;
	rootdir.i_number = ROOTINO;
	rootdir.i_mode |= Inode::IFDIR | Inode::IALLOC | Inode::IRWXU;

	rootdir.IUpdate(time(nullptr));
	Update();

	FileManager::getInst()->Initialize();
}

void FileSystem::LoadSuperBlock()
{
	User &u = *User::getInst();

	BufferManager &bufMgr = *BufferManager::getInst();
	Buf *pBuf;

	for (int i = 0; i < 2; i++)
	{
		int *p = (int *)&g_spb + i * 128;

		pBuf = bufMgr.Bread(DeviceManager::ROOTDEV, FileSystem::SUPER_BLOCK_SECTOR_NUMBER + i);

		Utility::DWordCopy((int *)pBuf->b_addr, p, 128);

		bufMgr.Brelse(pBuf);
	}

	this->m_Mount[0].m_dev = DeviceManager::ROOTDEV;
	this->m_Mount[0].m_spb = &g_spb;

	g_spb.s_flock = 0;
	g_spb.s_ilock = 0;
	g_spb.s_ronly = 0;
	g_spb.s_time = std::time(nullptr);
}
/*��ȡ�豸��Superblock*/
SuperBlock *FileSystem::GetFS(int dev)
{
	SuperBlock *sb;

	/* ����ϵͳװ����Ѱ���豸��Ϊdev���豸���ļ�ϵͳ��SuperBlock */
	for (int i = 0; i < FileSystem::NMOUNT; i++)
	{
		if (this->m_Mount[i].m_spb != NULL && this->m_Mount[i].m_dev == dev)
		{
			/* ��ȡSuperBlock�ĵ�ַ */
			sb = this->m_Mount[i].m_spb;
			if (sb->s_nfree > 100 || sb->s_ninode > 100)
			{
				sb->s_nfree = 0;
				sb->s_ninode = 0;
			}
			return sb;
		}
	}

	Utility::Panic("No File System!");
	return NULL;
}
/* SuperBlock��Inode���´���ӳ��*/
void FileSystem::Update()
{
	int i;
	SuperBlock *sb;
	Buf *pBuf;

	// /* ��һ�������ڽ���ͬ������ֱ�ӷ��� */
	// if (this->updlock)
	// {
	// 	return;
	// }

	// /* ����Update()�����Ļ���������ֹ������������ */
	// this->updlock++;

	/* ͬ��SuperBlock������ */
	for (i = 0; i < FileSystem::NMOUNT; i++)
	{
		if (this->m_Mount[i].m_spb != NULL) /* ��Mountװ����Ӧĳ���ļ�ϵͳ */
		{
			sb = this->m_Mount[i].m_spb;

			/* �����SuperBlock�ڴ渱��û�б��޸ģ�ֱ�ӹ���inode�Ϳ����̿鱻��������ļ�ϵͳ��ֻ���ļ�ϵͳ */
			if (sb->s_fmod == 0)
			{
				continue;
			}

			/* ��SuperBlock�޸ı�־ */
			sb->s_fmod = 0;
			/* д��SuperBlock�����ʱ�� */
			sb->s_time = std::time(nullptr);

			/*
			 * Ϊ��Ҫд�ص�������ȥ��SuperBlock����һ�黺�棬���ڻ�����СΪ512�ֽڣ�
			 * SuperBlock��СΪ1024�ֽڣ�ռ��2��������������������Ҫ2��д�������
			 */
			for (int j = 0; j < 2; j++)
			{
				/* ��һ��pָ��SuperBlock�ĵ�0�ֽڣ��ڶ���pָ���512�ֽ� */
				int *p = (int *)sb + j * 128;

				/* ��Ҫд�뵽�豸dev�ϵ�SUPER_BLOCK_SECTOR_NUMBER + j������ȥ */
				pBuf = this->m_BufferManager->GetBlk(this->m_Mount[i].m_dev, FileSystem::SUPER_BLOCK_SECTOR_NUMBER + j);

				/* ��SuperBlock�е�0 - 511�ֽ�д�뻺���� */
				Utility::DWordCopy(p, (int *)pBuf->b_addr, 128);

				/* ���������е�����д�������� */
				this->m_BufferManager->Bwrite(pBuf);
			}
		}
	}

	/* ͬ���޸Ĺ����ڴ�Inode����Ӧ���Inode */
	g_InodeTable.UpdateInodeTable();

	/* ���Update()������ */
	// this->updlock = 0;

	// /* ���ӳ�д�Ļ����д�������� */

	this->m_BufferManager->Bflush(DeviceManager::NODEV);
}
/*���豸�Ϸ���һ������Inode*/
Inode *FileSystem::IAlloc(int dev)
{
	SuperBlock *sb;
	Buf *pBuf;
	Inode *pNode;
	User &u = *User::getInst();
	int ino; /* ���䵽�Ŀ������Inode��� */

	/* ��ȡ��Ӧ�豸��SuperBlock�ڴ渱�� */
	sb = this->GetFS(dev);

	if (sb->s_ninode <= 0)
	{

		/* ���Inode��Ŵ�0��ʼ���ⲻͬ��Unix V6�����Inode��1��ʼ��� */
		ino = -1;

		/* ���ζ������Inode���еĴ��̿飬�������п������Inode���������Inode������ */
		for (int i = 0; i < sb->s_isize; i++)
		{
			pBuf = this->m_BufferManager->Bread(dev, FileSystem::INODE_ZONE_START_SECTOR + i);

			/* ��ȡ��������ַ */
			int *p = (int *)pBuf->b_addr;

			/* ���û�������ÿ�����Inode��i_mode != 0����ʾ�Ѿ���ռ�� */
			for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
			{
				ino++;

				int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

				/* �����Inode�ѱ�ռ�ã����ܼ������Inode������ */
				if (mode != 0)
				{
					continue;
				}
				/*
				 * ������inode��i_mode==0����ʱ������ȷ��
				 * ��inode�ǿ��еģ���Ϊ�п������ڴ�inodeû��д��
				 * ������,����Ҫ���������ڴ�inode���Ƿ�����Ӧ����
				 */
				if (g_InodeTable.IsLoaded(dev, ino) == -1)
				{
					/* �����Inodeû�ж�Ӧ���ڴ濽��������������Inode������ */
					sb->s_inode[sb->s_ninode++] = ino;

					/* ��������������Ѿ�װ�����򲻼������� */
					if (sb->s_ninode >= 100)
					{
						break;
					}
				}
			}

			/* �����Ѷ��굱ǰ���̿飬�ͷ���Ӧ�Ļ��� */
			this->m_BufferManager->Brelse(pBuf);

			/* ��������������Ѿ�װ�����򲻼������� */
			if (sb->s_ninode >= 100)
			{
				break;
			}
		}

		/* ����ڴ�����û���������κο������Inode������NULL */
		if (sb->s_ninode <= 0)
		{

			u.u_error = User::ErrorCode::ENOSPC_;
			return NULL;
		}
	}

	/*
	 * ���沿���Ѿ���֤������ϵͳ��û�п������Inode��
	 * �������Inode�������бض����¼�������Inode�ı�š�
	 */
	while (true)
	{
		/* ��������ջ������ȡ�������Inode��� */
		ino = sb->s_inode[--sb->s_ninode];

		/* ������Inode�����ڴ� */
		pNode = g_InodeTable.IGet(dev, ino);
		/* δ�ܷ��䵽�ڴ�inode */
		if (NULL == pNode)
		{
			return NULL;
		}

		/* �����Inode����,���Inode�е����� */
		if (0 == pNode->i_mode)
		{
			pNode->Clean();
			/* ����SuperBlock���޸ı�־ */
			sb->s_fmod = 1;
			return pNode;
		}
		else /* �����Inode�ѱ�ռ�� */
		{
			g_InodeTable.IPut(pNode);
			continue; /* whileѭ�� */
		}
	}
	return NULL; /* GCC likes it! */
}
/*�ͷ��豸�ϵ�number��Inode*/
void FileSystem::IFree(int dev, int number)
{
	SuperBlock *sb;

	sb = this->GetFS(dev); /* ��ȡ��Ӧ�豸��SuperBlock�ڴ渱�� */

	/*
	 * ���������ֱ�ӹ���Ŀ������Inode����100����
	 * ͬ�����ͷŵ����Inodeɢ���ڴ���Inode���С�
	 */
	if (sb->s_ninode >= 100)
	{
		return;
	}

	sb->s_inode[sb->s_ninode++] = number;

	/* ����SuperBlock���޸ı�־ */
	sb->s_fmod = 1;
}
/*��dev�豸�з���һ�����д��̿飬������Buf��*/
Buf *FileSystem::Alloc(int dev)
{
	int blkno; /* ���䵽�Ŀ��д��̿��� */
	SuperBlock *sb;
	Buf *pBuf;
	User &u = *User::getInst();

	/* ��ȡSuperBlock������ڴ渱�� */
	sb = this->GetFS(dev);

	/*
	 * ������д��̿����������ڱ���������������������
	 * ���ڲ������д��̿����������������������ͨ��
	 * ������������̵���Free()��Alloc()��ɵġ�
	 */
	// while (sb->s_flock)
	// {
	// 	/* ����˯��ֱ����ø����ż��� */
	// 	u.u_procp->Sleep((unsigned long)&sb->s_flock, ProcessManager::PINOD);
	// }

	/* ��������ջ������ȡ���д��̿��� */
	blkno = sb->s_free[--sb->s_nfree];

	/*
	 * ����ȡ���̿���Ϊ�㣬���ʾ�ѷ��価���еĿ��д��̿顣
	 * ���߷��䵽�Ŀ��д��̿��Ų����������̿�������(��BadBlock()���)��
	 * ����ζ�ŷ�����д��̿����ʧ�ܡ�
	 */
	if (0 == blkno)
	{
		sb->s_nfree = 0;
		printf("No Space On %d !\n", dev);
		u.u_error = User::ENOSPC_;
		return NULL;
	}
	if (this->BadBlock(sb, dev, blkno))
	{
		return NULL;
	}

	/*
	 * ջ�ѿգ��·��䵽���д��̿��м�¼����һ����д��̿�ı��,
	 * ����һ����д��̿�ı�Ŷ���SuperBlock�Ŀ��д��̿�������s_free[100]�С�
	 */
	if (sb->s_nfree <= 0)
	{
		/*
		 * �˴���������Ϊ����Ҫ���ж��̲������п��ܷ��������л���
		 * ����̨�Ľ��̿��ܶ�SuperBlock�Ŀ����̿���������ʣ��ᵼ�²�һ���ԡ�
		 */
		sb->s_flock++;

		/* ����ÿ��д��̿� */
		pBuf = this->m_BufferManager->Bread(dev, blkno);

		/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		int *p = (int *)pBuf->b_addr;

		/* ���ȶ��������̿���s_nfree */
		sb->s_nfree = *p++;

		/* ��ȡ�����к���λ�õ����ݣ�д�뵽SuperBlock�����̿�������s_free[100]�� */
		Utility::DWordCopy(p, sb->s_free, 100);

		/* ����ʹ����ϣ��ͷ��Ա㱻��������ʹ�� */
		this->m_BufferManager->Brelse(pBuf);

		// /* ����Կ��д��̿������������������Ϊ�ȴ�����˯�ߵĽ��� */
		// sb->s_flock = 0;
		// Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)&sb->s_flock);
	}

	/* ��ͨ����³ɹ����䵽һ���д��̿� */
	pBuf = this->m_BufferManager->GetBlk(dev, blkno); /* Ϊ�ô��̿����뻺�� */
	this->m_BufferManager->ClrBuf(pBuf);			  /* ��ջ����е����� */
	sb->s_fmod = 1;									  /* ����SuperBlock���޸ı�־ */

	return pBuf;
}
/*�ͷ�һ���̿�*/
void FileSystem::Free(int dev, int blkno)
{
	SuperBlock *sb;
	Buf *pBuf;
	User &u = *User::getInst();

	sb = this->GetFS(dev);

	/*
	 * ��������SuperBlock���޸ı�־���Է�ֹ���ͷ�
	 * ���̿�Free()ִ�й����У���SuperBlock�ڴ渱��
	 * ���޸Ľ�������һ�룬�͸��µ�����SuperBlockȥ
	 */
	sb->s_fmod = 1;

	/* ����ͷŴ��̿�ĺϷ��� */
	if (this->BadBlock(sb, dev, blkno))
	{
		return;
	}

	/*
	 * �����ǰϵͳ���Ѿ�û�п����̿飬
	 * �����ͷŵ���ϵͳ�е�1������̿�
	 */
	if (sb->s_nfree <= 0)
	{
		sb->s_nfree = 1;
		sb->s_free[0] = 0; /* ʹ��0��ǿ����̿���������־ */
	}

	/* SuperBlock��ֱ�ӹ�����д��̿�ŵ�ջ���� */
	if (sb->s_nfree >= 100)
	{
		// sb->s_flock++;

		/*
		 * ʹ�õ�ǰFree()������Ҫ�ͷŵĴ��̿飬���ǰһ��100������
		 * ���̿��������
		 */
		pBuf = this->m_BufferManager->GetBlk(dev, blkno); /* Ϊ��ǰ��Ҫ�ͷŵĴ��̿���仺�� */

		/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		int *p = (int *)pBuf->b_addr;

		/* ����д������̿��������˵�һ��Ϊ99�飬����ÿ�鶼��100�� */
		*p++ = sb->s_nfree;
		/* ��SuperBlock�Ŀ����̿�������s_free[100]д�뻺���к���λ�� */
		Utility::DWordCopy(sb->s_free, p, 100);

		sb->s_nfree = 0;
		/* ����ſ����̿�������ġ���ǰ�ͷ��̿顱д����̣���ʵ���˿����̿��¼�����̿�ŵ�Ŀ�� */
		this->m_BufferManager->Bwrite(pBuf);

		sb->s_flock = 0;
		// Kernel::Instance().GetProcessManager().WakeUpAll((unsigned long)&sb->s_flock);
	}
	// s_free�׸�λ�ü�¼��ǰ�ͷ��̿��
	sb->s_free[sb->s_nfree++] = blkno; /* SuperBlock�м�¼�µ�ǰ�ͷ��̿�� */
	sb->s_fmod = 1;
}

bool FileSystem::BadBlock(SuperBlock *spb, int dev, int blkno)
{
	return blkno < DATA_ZONE_START_SECTOR;
}
