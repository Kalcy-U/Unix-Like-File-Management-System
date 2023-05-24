#include "../includes/shell.h"
#include "../includes/Utility.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <string.h>
using namespace std;
Shell::Shell()
{
}
void Shell::Start(int mode)
{
    m_user = User::getInst();
    m_fileManager = FileManager::getInst();
    m_deviceManager = DeviceManager::getInst();
    m_fileSystem = FileSystem::getInst();

    m_user->Debug = mode;
    Interface();
}
void Shell::Usage()
{
    printf("ls                              list directorys and files in current path\n");
    printf("mkdir <path>                    create a directory\n");
    printf("cd <path>                       change current path\n");
    printf("fcreate <path>                  create a file\n");
    printf("fdelete <path>                  delete a file\n");
    printf("fopen <path>                    open a file\n");
    printf("fclose <fd>                     close a file\n");
    printf("fread <fd> <count>              read file(fd={fd}) for {count} byte\n");
    printf("fwrite <fd> <count> <content>   write file(fd={fd}) for {count} byte\n");
    printf("flseek <fd> <pos> [mode]        set the read/write pointer of file(fd={fd}) at {pos}\n");
    printf("                                mode :  -b      [defalt]pos is the offset from beginning\n");
    printf("                                        -c      pos is the offset from current position\n");
    printf("                                        -e      pos is the offset from end\n");
}
// 开始shell服务
//  开始前要实例化内核 文件系统LoadSuperblock
void Shell::Interface()
{

    while (1)
    {
        std::vector<std::string> input;
        std::string str;
        int count = 0;
        cout << "[" << m_user->Pwd() << "]# ";
        while (cin >> str)
        {
            input.push_back(str);
            count++;
            if ('\n' == getchar())
                break;
        }
        cin.clear();

        if (count == 0)
            continue;

        if (count >= 1 && input[0] == "help")
        {
            Usage();
        }
        else if (count == 1 && input[0] == "quit")
            break;
        else if (input[0] == "ls")
        {
            /*查看当前路径下的文件*/

            int fd = m_fileManager->Open(m_user->Pwd(), File::FREAD); /*返回fd写入u.u_ar0 嵌套层数太多不好直接返回*/
            int size = m_user->u_ofiles.GetF(fd)->f_inode->i_size;
            if (size > 31 * 32)
                size = 31 * 32;
            DirectoryEntry etr[31];
            m_fileManager->Read(fd, (unsigned char *)buffer, size);
            Utility::DWordCopy((int *)buffer, (int *)etr, size);

            for (int i = 0; i < size / 32; i++)
            {
                if (etr[i].m_ino != 0)
                    printf("%s   ", etr[i].m_name);
            }
            printf("\n");
        }
        else if (input[0] == "cd")
        {
            /*修改当前工作路径*/
            if (count == 2)
            {
                m_fileManager->ChDir(input[1].c_str());
                if (m_user->u_error == User::ENOTDIR_)
                {
                    printf("not a directory.\n");
                }
                else if (m_user->u_error != User::NOERROR_)
                {
                    printf("change directory failed.\n");
                }
                m_user->clear();
            }
            else
                Usage();
        }
        else if (input[0] == "mkdir")
        {
            /*新建文件夹*/
            if (count == 2)
            {
                m_fileManager->MkNod(input[1].c_str(), Inode::IFDIR | Inode::IRWXU);
                if (m_user->u_error != User::NOERROR_)
                {
                    printf("create directory failed.\n");
                    m_user->clear();
                }
            }
            else
                Usage();
        }
        else if (input[0] == "fcreate")
        {
            if (count == 2)
            {
                int fd = m_fileManager->Creat(input[1].c_str(), Inode::IRWXU);
                if (fd < 0 || m_user->u_error != User::NOERROR_)
                {
                    printf("create failed.\n");
                    m_user->clear();
                }
                else
                {
                    printf("create successfully.fd=%d!\n", fd);
                }
            }
            else
                Usage();
        }
        else if (input[0] == "fopen")
        {
            if (count == 2)
            {
                int fd = m_fileManager->Open(input[1].c_str(), Inode::IRWXU);
                if (fd < 0 || m_user->u_error != User::NOERROR_)
                {
                    printf("open failed.\n");
                    m_user->clear();
                }
                else
                {
                    printf("open successfully.fd=%d!\n", fd);
                }
            }
            else
                Usage();
        }
        else if (input[0] == "fclose")
        {
            if (count == 2)
            {

                int fd = -1;
                if (Utility::AllisNum(input[1].c_str()))
                {
                    fd = atoi(input[1].c_str());
                    m_fileManager->Close(fd);
                    if (fd < 0 || m_user->u_error != User::NOERROR_)
                    {
                        printf("close failed.\n");
                        m_user->clear();
                    }
                    else
                    {
                        printf("open successfully.fd=%d!\n", fd);
                    }
                }
                else
                {
                    printf("fclose <fd> \n");
                    printf("fd should be a number!\n");
                }
            }
            else
                Usage();
        }
        else if (input[0] == "fread")
        {
            if (count == 3)
            {
                int fd = -1, ncount = -1;
                if (Utility::AllisNum(input[1].c_str()) && Utility::AllisNum(input[2].c_str()))
                {
                    fd = atoi(input[1].c_str());
                    ncount = atoi(input[2].c_str());
                    if (ncount > 1000 || ncount < 0)
                    {
                        printf("count should be within (0,1000)\n");
                        continue;
                    }
                    memset((void *)buffer, 0, 1000);
                    ncount = m_fileManager->Read(fd, (unsigned char *)buffer, ncount);
                    if (fd < 0 || m_user->u_error != User::NOERROR_)
                    {
                        printf("read failed.\n");
                        m_user->clear();
                    }
                    else
                    {
                        /*输出内容很怪，但是为了测试只能这样*/
                        printf("read successfully:actual count=%d,tellp=%d,content=%s\n", ncount, m_fileManager->Tellp(fd), buffer);
                    }
                }
                else
                {
                    printf("input is illegal!\n");
                }
            }
            else
                Usage();
        }
        else if (input[0] == "fwrite")
        {
            if (count == 4)
            {
                int fd = -1, ncount = -1;
                if (Utility::AllisNum(input[1].c_str()) && Utility::AllisNum(input[2].c_str()))
                {
                    fd = atoi(input[1].c_str());
                    ncount = atoi(input[2].c_str());
                    if (ncount > 1000 || ncount < 0)
                    {
                        printf("count should be within (0,1000)\n");
                        continue;
                    }

                    strcpy(buffer, input[3].c_str());
                    ncount = m_fileManager->Write(fd, (unsigned char *)buffer, ncount);
                    if (fd < 0 || m_user->u_error != User::NOERROR_)
                    {
                        printf("write failed.\n");
                        m_user->clear();
                    }
                    else
                    {
                        printf("write successfully:actual count=%d,tellp=%d\n", ncount, m_fileManager->Tellp(fd));
                    }
                }
                else
                {
                    Usage();
                }
            }
            else
                Usage();
        }
        else if (input[0] == "flseek")
        {
            if (count == 4)
            {
                int fd = -1, pos = -1;
                if (Utility::AllisNum(input[1].c_str()) && Utility::AllisNum(input[2].c_str()))
                {
                    fd = atoi(input[1].c_str());
                    pos = atoi(input[2].c_str());
                    int mode = 0;
                    if (input[3] == "-c")
                        mode = 1;
                    else if (input[3] == "-e")
                        mode = 2;
                    m_fileManager->Seek(fd, pos, mode);
                    if (m_user->u_error != User::NOERROR_)
                    {
                        printf("fseek failed.\n");
                        m_user->clear();
                    }
                    else
                    {
                        printf("changed r/w pointer successfully:tellp=%d\n", m_fileManager->Tellp(fd));
                    }
                }
                else
                {
                    Usage();
                }
            }
            else
                Usage();
        }
        // else if (input[0] == "fdelete")
        // {
        // }
        else
            Usage();
    }
    return;
}
