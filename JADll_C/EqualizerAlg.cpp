#include "pch.h"
#include <cmath>
#include <vector>
#include <Windows.h>


constexpr double M_PI{ 3.14159265358979323846 };

// DLL Export Macro
extern "C" __declspec(dllexport) void MyProc2(int* buffer, long long length, int lowGain, int midGain, int highGain) {
    // Create buffers for filtered data
    std::vector<int> lowPassBuffer(length);
    std::vector<int> bandPassBuffer(length);
    std::vector<int> highPassBuffer(length);

    // Filter coefficients (example cutoff frequencies)
    constexpr float lowCutoff{ 200.0f };    // Low-pass cutoff frequency (Hz)
    constexpr float highCutoff{ 3000.0f };  // High-pass cutoff frequency (Hz)
    constexpr float sampleRate{ 44100.0f }; // Assuming standard sample rate

    // Simplistic filter coefficients
    float lowAlpha = 2.0f * M_PI * lowCutoff / sampleRate;
    float highAlpha = 2.0f * M_PI * highCutoff / sampleRate;

    // Previous samples for low-pass and high-pass filters
    float lowYPrev = 0.0f;
    float lowXPrev = 0.0f;
    float highYPrev = 0.0f;
    float highXPrev = 0.0f;

    // Process the audio sample by sample
    for (long long i = 0; i < length; ++i) {
        // Low-pass filter
        lowYPrev = (lowAlpha * buffer[i] + (1 - lowAlpha) * lowYPrev);
        lowPassBuffer[i] = static_cast<int>(lowYPrev);

        // High-pass filter
        highYPrev = (highAlpha * (highYPrev + buffer[i] - highXPrev));
        highPassBuffer[i] = static_cast<int>(highYPrev);
        highXPrev = buffer[i];

        // Band-pass filter (input - low-pass - high-pass)
        bandPassBuffer[i] = buffer[i] - lowPassBuffer[i] - highPassBuffer[i];
    }

    // Apply gains
    for (long long i = 0; i < length; ++i) {
        buffer[i] = (lowPassBuffer[i] * lowGain) +
            (bandPassBuffer[i] * midGain) +
            (highPassBuffer[i] * highGain);

        // Clip to ensure the value stays within the 32-bit integer range
        if (buffer[i] > INT_MAX) buffer[i] = INT_MAX;
        if (buffer[i] < INT_MIN) buffer[i] = INT_MIN;
    }
}
