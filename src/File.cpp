#include "../includes/File.h"
#include "../includes/User.h"

/*==============================class File===================================*/
File::File()
{
	this->f_count = 0;
	this->f_flag = 0;
	this->f_offset = 0;
	this->f_inode = NULL;
}

File::~File()
{
	// nothing to do here
}

/*==============================class OpenFiles===================================*/
OpenFiles::OpenFiles()
{
	for (int i = 0; i < NOFILES; i++)
	{
		ProcessOpenFileTable[i] = NULL;
	}
}

OpenFiles::~OpenFiles()
{
}

int OpenFiles::AllocFreeSlot()
{
	int i;
	User &u = *User::getInst();

	for (i = 0; i < OpenFiles::NOFILES; i++)
	{
		if (this->ProcessOpenFileTable[i] == NULL)
		{
			return i;
		}
	}
	return -1;
}

int OpenFiles::Clone(int fd)
{
	return 0;
}

File *OpenFiles::GetF(int fd)
{
	File *pFile;
	User &u = *User::getInst();

	/* 如果打开文件描述符的值超出了范围 */
	if (fd < 0 || fd >= OpenFiles::NOFILES)
	{
		u.u_error = User::EBADF_;
		return NULL;
	}

	pFile = this->ProcessOpenFileTable[fd];
	if (pFile == NULL)
	{
		u.u_error = User::EBADF_;
	}

	return pFile; /* 即使pFile==NULL也返回它，由调用GetF的函数来判断返回值 */
}
/*将OpenFileTable中第fd条指向pFile*/
void OpenFiles::SetF(int fd, File *pFile)
{
	if (fd < 0 || fd >= OpenFiles::NOFILES)
	{
		return;
	}
	/* 打开文件描述符 = 相应的File结构*/
	this->ProcessOpenFileTable[fd] = pFile;
}

/*==============================class IOParameter===================================*/
IOParameter::IOParameter()
{
	this->m_Base = 0;
	this->m_Count = 0;
	this->m_Offset = 0;
}

IOParameter::~IOParameter()
{
	// nothing to do here
}
