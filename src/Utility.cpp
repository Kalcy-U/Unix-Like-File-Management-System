#include "../includes/Utility.hpp"
#include <stdio.h>
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