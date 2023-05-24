#include <cstdarg>
#include "../includes/Utility.hpp"
#include "../includes/User.h"

void Utility::IOMove(unsigned char *from, unsigned char *to, int count)
{
    while (count--)
    {
        *to++ = *from++;
    }
    return;
}
void Utility::DWordCopy(int *src, int *dst, int count)
{
    while (count--)
    {
        *dst++ = *src++;
    }
    return;
}
void Utility::Panic(const char *s)
{
    printf(s);
}
void Utility::StringCopy(char const *src, char *dst)
{
    while ((*dst++ = *src++) != 0)
        ;
}

int Utility::StringLength(char *pString)
{
    int length = 0;
    char *pChar = pString;

    while (*pChar++)
    {
        length++;
    }

    /* 返回字符串长度 */
    return length;
}

std::vector<std::string> Utility::Split(const std::string &str, char c)
{
    std::vector<std::string> list;
    std::string temp = "";
    for (auto it : str)
    {
        if (it == c)
        {
            if (temp.size() > 0)
                list.push_back(temp);
            temp = "";
        }
        else
        {
            temp += it;
        }
    }
    if (temp.size() > 0)
    {
        list.push_back(temp);
    }

    return list;
}
void Utility::DebugInfo(const char *format, ...)
{
    va_list args;
    if (User::getInst()->Debug == 1)
    {
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

bool Utility::AllisNum(std::string str)
{
    for (int i = 0; i < str.size(); i++)
    {
        int tmp = (int)str[i];
        if (tmp >= 48 && tmp <= 57)
        {
            continue;
        }
        else
        {
            return false;
        }
    }
    return true;
}