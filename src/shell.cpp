#include "../includes/shell.h"
#include <string>
#include <iostream>
#include <vector>
using namespace std;
Shell::Shell()
{
}
void Shell::Start(int mode)
{
    m_user = User::getInst();
    m_fileManager = FileManager::getInst();
    m_user->Debug = mode;
    Interface();
}

void Shell::Interface()
{
    std::vector<string> input;
    while (1)
    {
        vector<string> temp;
        input.swap(temp);
        string str;
        int count = 0;
        cout << endl
             << "> ";
        while (cin >> str)
        {
            if ('\n' == getchar())
                break;
            input.push_back(str);
            count++;
        }
        if (count == 1 && input[0] == "quit")
            break;
        else if (input[0] == "ls")
        {
        }
        else if (input[0] == "mkdir")
        {
            if (count == 2)
            {
            }
        }
        else if (input[0] == "fcreate")
        {
        }
    }
    return;
}
