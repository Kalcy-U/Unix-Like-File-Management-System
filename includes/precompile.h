#ifndef PRECOMPILE_SETINGS_H
#define PRECOMPILE_SETINGS_H
#ifdef __linux__
// Linux-specific code
#include <unistd.h>
void sleepms(int t)
{
	Sleep(t * 1000);
}
#elif defined(_WIN32) || defined(_WIN64)
// Windows-specific code
#include<Windows.h>
void sleepms(int t)
{
	Sleep(t);
}
#endif

#endif