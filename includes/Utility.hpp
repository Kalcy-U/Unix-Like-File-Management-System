#pragma once
namespace Utility
{
    void IOMove(unsigned char *from, unsigned char *to, int count);

    void DWordCopy(int *src, int *dst, int count);
    void Panic(const char *s);
}