#pragma once
#include <stdio.h>
#include <string>
#include <vector>
namespace Utility
{
    void IOMove(unsigned char *from, unsigned char *to, int count);

    void DWordCopy(int *src, int *dst, int count);

    void Panic(const char *s);
    int StringLength(char *pString);
    void StringCopy(char const *src, char *dst);
    std::vector<std::string> Split(const std::string &str, char c);
    void DebugInfo(const char *format, ...);
    bool AllisNum(std::string str);
}