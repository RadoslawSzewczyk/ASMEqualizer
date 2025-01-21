#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <sndfile.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <Windows.h>
#include <filesystem> 

// Declare the DLL functions
extern "C" __declspec(dllimport) void MyProc1(int* buffer, long long length, int lowGain, int midGain, int highGain);
extern "C" __declspec(dllimport) void MyProc2(int* buffer, long long length, int lowGain, int midGain, int highGain);

typedef void (*MyProcFunc)(int* buffer, long long length, int lowGain, int midGain, int highGain);

// Global mutex for multithreading
std::mutex mtx;

// Function to process a segment of the buffer
void processBufferSegment(MyProcFunc procFunc, int* buffer, long long start, long long end, int lowGain, int midGain, int highGain) {
    procFunc(buffer + start, end - start, lowGain, midGain, highGain);
}

int main() {
    sf::RenderWindow window(sf::VideoMode(600, 400), "Audio Processor");

    // Variables for file selection, processing function, and multithreading
    std::string inputFilePath;
    std::string outputFilePath = "output_sample_equalized.wav";
    MyProcFunc selectedProc = nullptr;
    int numThreads = 1;

    bool fileSelected = false;
    bool processingComplete = false;

    // Load font and check if it's loaded successfully

    sf::Font font;
    std::string fontPath = "C:\\Windows\\Fonts\\arial.ttf";

    // Use filesystem to get the absolute path for debugging
    std::filesystem::path fontFullPath = std::filesystem::absolute(fontPath);
    std::cout << "Attempting to load font from: " << fontFullPath.string() << std::endl;

    if (!std::filesystem::exists(fontPath)) {
        std::cerr << "Font file does not exist at: " << fontPath << std::endl;
        return -1;
    }


    if (!font.loadFromFile(fontPath))
    {
        std::cerr << "Failed to load font from: " << fontFullPath.string() << std::endl;
        std::cerr << "Error code: " << GetLastError() << std::endl; // Get Windows error code
        return -1;
    }

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Draw the GUI
        window.clear(sf::Color::White);

        // File selection button
        sf::RectangleShape fileButton(sf::Vector2f(200, 50));
        fileButton.setPosition(200, 50);
        fileButton.setFillColor(sf::Color::Blue);

        sf::Text fileButtonText("Choose File", font, 20);
        fileButtonText.setPosition(210, 60);
        fileButtonText.setFillColor(sf::Color::White);

        window.draw(fileButton);
        window.draw(fileButtonText);

        // Processing function buttons
        sf::Text proc1Text("MyProc1", font, 20);
        sf::Text proc2Text("MyProc2", font, 20);
        proc1Text.setPosition(200, 120);
        proc2Text.setPosition(200, 160);

        // Threads selector
        sf::Text threadText("Threads: " + std::to_string(numThreads), font, 20);
        threadText.setPosition(200, 200);

        window.draw(proc1Text);
        window.draw(proc2Text);
        window.draw(threadText);

        // Handle mouse input for GUI
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);

            // File button
            if (fileButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                // Prompt user to input file path
                std::cout << "Enter file path: ";
                std::cin >> inputFilePath;
                fileSelected = true;
            }

            // MyProc1 button
            if (proc1Text.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                selectedProc = MyProc1;
            }

            // MyProc2 button
            if (proc2Text.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                selectedProc = MyProc2;
            }

            // Thread adjustment (just an example, use keyboard inputs for finer control)
            if (threadText.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                numThreads = (numThreads % std::thread::hardware_concurrency()) + 1;
            }
        }

        // If all inputs are selected, process the file
        if (fileSelected && selectedProc) {
            SF_INFO sfInfo;
            SNDFILE* sndFile = sf_open(inputFilePath.c_str(), SFM_READ, &sfInfo);
            if (!sndFile) {
                std::cerr << "Error opening file!" << std::endl;
                break;
            }

            // Read audio data
            std::vector<int16_t> audioBuffer(sfInfo.frames * sfInfo.channels);
            sf_read_short(sndFile, audioBuffer.data(), audioBuffer.size());
            sf_close(sndFile);

            // Convert to 32-bit
            std::vector<int> audioBuffer32(audioBuffer.begin(), audioBuffer.end());

            long long bufferLength = audioBuffer32.size();
            int lowGain = 1, midGain = 1, highGain = 1;

            // Divide buffer into segments for multithreading
            std::vector<std::thread> threads;
            long long segmentSize = bufferLength / numThreads;

            for (int i = 0; i < numThreads; ++i) {
                long long start = i * segmentSize;
                long long end = (i == numThreads - 1) ? bufferLength : start + segmentSize;

                threads.emplace_back(processBufferSegment, selectedProc, audioBuffer32.data(), start, end, lowGain, midGain, highGain);
            }

            // Join threads
            for (auto& thread : threads) {
                thread.join();
            }

            // Convert back to 16-bit
            std::vector<int16_t> processedBuffer(audioBuffer32.size());
            for (size_t i = 0; i < audioBuffer32.size(); ++i) {
                if (audioBuffer32[i] > 32767) processedBuffer[i] = 32767;
                else if (audioBuffer32[i] < -32768) processedBuffer[i] = -32768;
                else processedBuffer[i] = static_cast<int16_t>(audioBuffer32[i]);
            }

            // Save the file
            SNDFILE* outFile = sf_open(outputFilePath.c_str(), SFM_WRITE, &sfInfo);
            sf_write_short(outFile, processedBuffer.data(), processedBuffer.size());
            sf_close(outFile);

            processingComplete = true;
        }

        window.display();
    }

    return 0;
}
