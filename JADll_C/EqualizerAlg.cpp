#include <cmath>
#include <algorithm>
#include <cstdint>
#include "pch.h"
// Clamp function
template <typename T>
T clamp(T value, T min, T max) {
    return (value < min) ? min : (value > max ? max : value);
}

// Improved Low-pass filter (2nd-order approximation for sharper roll-off)
double lowPassFilter(double sample, double cutoff, double sampleRate, double& prev1, double& prev2) {
    double alpha = 2 * 3.14159 * cutoff / sampleRate;
    double result = alpha * sample + (1.0 - alpha) * prev1;
    prev2 = prev1;  // Shift previous samples
    prev1 = result;
    return result;
}

// Improved High-pass filter (2nd-order approximation for sharper roll-off)
double highPassFilter(double sample, double cutoff, double sampleRate, double& prev1, double& prev2) {
    double alpha = cutoff / (cutoff + sampleRate);
    double result = alpha * (prev1 + sample - prev2);
    prev2 = prev1;  // Shift previous samples
    prev1 = result;
    return result;
}

// Mid-pass filter (band-pass using refined low and high-pass filters)
double bandPassFilter(double sample, double lowCutoff, double highCutoff, double sampleRate, double& lowPrev1, double& lowPrev2, double& highPrev1, double& highPrev2) {
    double lowFiltered = lowPassFilter(sample, highCutoff, sampleRate, lowPrev1, lowPrev2);  // Remove high frequencies
    return highPassFilter(lowFiltered, lowCutoff, sampleRate, highPrev1, highPrev2);        // Remove low frequencies
}

// Equalizer processing function
extern "C" __declspec(dllexport) void MyProc2(int* buffer, long long length, int lowGain, int midGain, int highGain) {
    const double sampleRate = 44100.0; // Standard sample rate
    const double lowCutoff = 200.0;    // Low band cutoff (Hz)
    const double highCutoff = 5000.0;  // High band cutoff (Hz)

    double lowPrev1 = 0.0, lowPrev2 = 0.0;
    double highPrev1 = 0.0, highPrev2 = 0.0;
    double midLowPrev1 = 0.0, midLowPrev2 = 0.0;
    double midHighPrev1 = 0.0, midHighPrev2 = 0.0;

    for (long long i = 0; i < length; ++i) {
        double sample = static_cast<double>(buffer[i]);

        // Low band processing
        double lowSample = lowPassFilter(sample, lowCutoff, sampleRate, lowPrev1, lowPrev2);
        double lowBand = lowSample * (lowGain / 100.0);

        // High band processing
        double highSample = highPassFilter(sample, highCutoff, sampleRate, highPrev1, highPrev2);
        double highBand = highSample * (highGain / 100.0);

        // Mid band processing
        double midSample = bandPassFilter(sample, lowCutoff, highCutoff, sampleRate, midLowPrev1, midLowPrev2, midHighPrev1, midHighPrev2);
        double midBand = midSample * (midGain / 100.0);

        // Combine all bands
        double processedSample = lowBand + midBand + highBand;

        // Clamp the result to 32-bit integer range
        buffer[i] = static_cast<int>(clamp(processedSample, -2147483648.0, 2147483647.0));
    }
}
