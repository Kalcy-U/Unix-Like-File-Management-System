#ifndef PRECOMPILE_SETINGS_H
#define PRECOMPILE_SETINGS_H
#ifdef __linux__
// Linux-specific code
#elif defined(_WIN32) || defined(_WIN64)
// Windows-specific code

void tasksleep(int t)
{
}
#endif

#endif