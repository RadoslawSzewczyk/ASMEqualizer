#include <sndfile.h>
#include <iostream>
#include <vector>
#include <thread>

// External assembly function declaration
extern "C" void _fastcall MyProc1(int* buffer, long long length, int gain);
extern "C" void _fastcall MyProc2(int buffer, int gain);

// Function to process a chunk of audio data
void processChunk(int* buffer, long long length, int gain) {
    MyProc1(buffer, length, gain);
}

int main()
{
    MyProc2(1, 2);
    const char* inputFilePath = "C:\\Users\\radek\\Downloads\\sample.wav";
    const char* outputFilePath = "C:\\Users\\radek\\Downloads\\output_sample.wav";

    // Open the audio file using libsndfile
    SF_INFO sfInfo;
    SNDFILE* sndFile = sf_open(inputFilePath, SFM_READ, &sfInfo);
    if (!sndFile) {
        std::cerr << "Error: Unable to open audio file: " << sf_strerror(nullptr) << std::endl;
        return 1;
    }

    // Validate the format
    if ((sfInfo.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV) {
        std::cerr << "Error: Not a WAV file." << std::endl;
        sf_close(sndFile);
        return 1;
    }

    // Ensure the file contains PCM data
    if ((sfInfo.format & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
        std::cerr << "Error: Only 16-bit PCM data is supported." << std::endl;
        sf_close(sndFile);
        return 1;
    }

    // Read audio data as 16-bit values (int16_t)
    std::vector<int16_t> audioBuffer(sfInfo.frames * sfInfo.channels);
    sf_read_short(sndFile, audioBuffer.data(), audioBuffer.size());
    sf_close(sndFile);

    // Convert audioBuffer from 16-bit (int16_t) to 32-bit (int) values
    std::vector<int> audioBuffer32(audioBuffer.begin(), audioBuffer.end());

    // Process the audio data in chunks using threads
    long long bufferLength = audioBuffer32.size();
    int gain = 2;

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 1;

    long long chunkSize = bufferLength / numThreads;
    long long remaining = bufferLength % numThreads;

    std::vector<std::thread> threads;

    long long offset = 0;
    for (unsigned int i = 0; i < numThreads; ++i) {
        long long currentChunkSize = chunkSize + (i < remaining ? 1 : 0);
        threads.emplace_back(processChunk, &audioBuffer32[offset], currentChunkSize, gain);
        offset += currentChunkSize;
    }

    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Convert the processed audio buffer back to 16-bit values for saving
    std::vector<int16_t> processedAudioBuffer(audioBuffer32.size());
    for (size_t i = 0; i < audioBuffer32.size(); ++i) {
        // Clip the audio to ensure it stays within the 16-bit range
        if (audioBuffer32[i] > 32767) {
            processedAudioBuffer[i] = 32767;  // Max 16-bit value
        }
        else if (audioBuffer32[i] < -32768) {
            processedAudioBuffer[i] = -32768; // Min 16-bit value
        }
        else {
            processedAudioBuffer[i] = static_cast<int16_t>(audioBuffer32[i]);
        }
    }

    // Create a new SF_INFO structure for the output file
    SF_INFO outputSfInfo = sfInfo;  // Copy the input file info
    outputSfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    // Open the output file for writing
    SNDFILE* outFile = sf_open(outputFilePath, SFM_WRITE, &outputSfInfo);
    if (!outFile) {
        std::cerr << "Error: Unable to open output file: " << sf_strerror(nullptr) << std::endl;
        return 1;
    }

    // Write the processed audio data to the output file
    sf_write_short(outFile, processedAudioBuffer.data(), processedAudioBuffer.size());
    sf_close(outFile);

    std::cout << "Audio processed and saved to " << outputFilePath << std::endl;

    return 0;
}
