#include "pch.h"

extern "C" __declspec(dllexport) void MyProc2(int* buffer, long long length, int gain)
{
    for (long long i = 0; i < length; ++i)
    {
        buffer[i] *= gain;
    }
