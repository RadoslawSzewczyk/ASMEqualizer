#include <iostream>

extern "C" void _fastcall MyProc1(int* buffer, long long length, int gain);

int main() {
    int audioBuffer[] = { 1000, 2000, 3000, 4000, 5000 };
    long long length = sizeof(audioBuffer) / sizeof(audioBuffer[0]);
    int gain = 2;

    MyProc1(audioBuffer, length, gain);

    for (int i = 0; i < length; i++) {
        std::cout << audioBuffer[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}

