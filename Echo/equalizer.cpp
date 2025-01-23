#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <sndfile.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <Windows.h>
#include <filesystem>
#include <chrono>

// Declare the DLL functions
extern "C" __declspec(dllimport) void MyProc1(int* buffer, long long length, int lowGain, int midGain, int highGain);
extern "C" __declspec(dllimport) void MyProc2(int* buffer, long long length, int lowGain, int midGain, int highGain);

typedef void (*MyProcFunc)(int* buffer, long long length, int lowGain, int midGain, int highGain);

// Global mutex for multithreading
std::mutex mtx;

// Function to process a segment of the buffer
void processBufferSegment(MyProcFunc procFunc, int* buffer, long long start, long long end, int lowGain, int midGain, int highGain)
{
    procFunc(buffer + start, end - start, lowGain, midGain, highGain);
}

// Function to open file dialog and get selected file path
std::string openFileDialog()
{
    OPENFILENAMEW ofn;                  // Common dialog box structure
    wchar_t filePath[MAX_PATH] = L"";  // Buffer to hold the file path

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;           // No specific owner window
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"WAV Files\0*.wav\0All Files\0*.*\0"; // Wide-character filter
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) // Use the wide version of the function
    {
        char narrowPath[MAX_PATH];
        size_t convertedChars = 0;

        errno_t err = wcstombs_s(&convertedChars, narrowPath, MAX_PATH, filePath, MAX_PATH - 1);
        if (err != 0)
        {
            std::cerr << "Error converting file path to narrow string." << std::endl;
            return "";
        }
        return std::string(narrowPath);
    }
    return "";
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(600, 700), "Audio Processor");

    // Variables for file selection, processing function, and multithreading
    std::string inputFilePath;
    std::string outputFilePath = "output_sample_equalized.wav";
    MyProcFunc selectedProc = nullptr;
    int numThreads = 1;

    int lowGain = 1;
    int midGain = 1;
    int highGain = 1;

    bool fileSelected = false;

    // Load font
    sf::Font font;
    std::string fontPath = "C:\\Windows\\Fonts\\arial.ttf";

    if (!font.loadFromFile(fontPath))
    {
        std::cerr << "Failed to load font from: " << fontPath << std::endl;
        return -1;
    }

    // Buttons and associated text
    sf::RectangleShape fileButton(sf::Vector2f(200, 50));
    fileButton.setPosition(200, 50);
    fileButton.setFillColor(sf::Color::Blue);

    sf::Text fileButtonText("Choose File", font, 20);
    fileButtonText.setPosition(210, 60);
    fileButtonText.setFillColor(sf::Color::White);

    sf::Text filePathText("", font, 15);
    filePathText.setPosition(50, 20);
    filePathText.setFillColor(sf::Color::Black);

    sf::RectangleShape proc1Button(sf::Vector2f(200, 50));
    proc1Button.setPosition(200, 120);
    proc1Button.setFillColor(sf::Color::Green);

    sf::Text proc1Text("ASM", font, 20);
    proc1Text.setPosition(230, 130);
    proc1Text.setFillColor(sf::Color::White);

    sf::RectangleShape proc2Button(sf::Vector2f(200, 50));
    proc2Button.setPosition(200, 190);
    proc2Button.setFillColor(sf::Color::Green);

    sf::Text proc2Text("C++", font, 20);
    proc2Text.setPosition(230, 200);
    proc2Text.setFillColor(sf::Color::White);

    // Threads adjustment buttons
    sf::RectangleShape threadPlus(sf::Vector2f(50, 50));
    threadPlus.setPosition(400, 260);
    threadPlus.setFillColor(sf::Color::Cyan);

    sf::Text threadPlusText("+", font, 30);
    threadPlusText.setPosition(415, 265);
    threadPlusText.setFillColor(sf::Color::Black);

    sf::RectangleShape threadMinus(sf::Vector2f(50, 50));
    threadMinus.setPosition(300, 260);
    threadMinus.setFillColor(sf::Color::Cyan);

    sf::Text threadMinusText("-", font, 30);
    threadMinusText.setPosition(315, 265);
    threadMinusText.setFillColor(sf::Color::Black);

    sf::Text threadCountText("Threads: " + std::to_string(numThreads), font, 20);
    threadCountText.setPosition(150, 270);
    threadCountText.setFillColor(sf::Color::Black);

    // Gain adjustment buttons and text
    sf::RectangleShape lowGainPlus(sf::Vector2f(50, 50));
    lowGainPlus.setPosition(400, 330);
    lowGainPlus.setFillColor(sf::Color::Yellow);

    sf::Text lowGainPlusText("+", font, 30);
    lowGainPlusText.setPosition(415, 335);
    lowGainPlusText.setFillColor(sf::Color::Black);

    sf::RectangleShape lowGainMinus(sf::Vector2f(50, 50));
    lowGainMinus.setPosition(300, 330);
    lowGainMinus.setFillColor(sf::Color::Yellow);

    sf::Text lowGainMinusText("-", font, 30);
    lowGainMinusText.setPosition(315, 335);
    lowGainMinusText.setFillColor(sf::Color::Black);

    sf::Text lowGainText("Low: " + std::to_string(lowGain), font, 20);
    lowGainText.setPosition(150, 340);
    lowGainText.setFillColor(sf::Color::Black);

    sf::RectangleShape midGainPlus(sf::Vector2f(50, 50));
    midGainPlus.setPosition(400, 400);
    midGainPlus.setFillColor(sf::Color::Yellow);

    sf::Text midGainPlusText("+", font, 30);
    midGainPlusText.setPosition(415, 405);
    midGainPlusText.setFillColor(sf::Color::Black);

    sf::RectangleShape midGainMinus(sf::Vector2f(50, 50));
    midGainMinus.setPosition(300, 400);
    midGainMinus.setFillColor(sf::Color::Yellow);

    sf::Text midGainMinusText("-", font, 30);
    midGainMinusText.setPosition(315, 405);
    midGainMinusText.setFillColor(sf::Color::Black);

    sf::Text midGainText("Mid: " + std::to_string(midGain), font, 20);
    midGainText.setPosition(150, 410);
    midGainText.setFillColor(sf::Color::Black);

    sf::RectangleShape highGainPlus(sf::Vector2f(50, 50));
    highGainPlus.setPosition(400, 470);
    highGainPlus.setFillColor(sf::Color::Yellow);

    sf::Text highGainPlusText("+", font, 30);
    highGainPlusText.setPosition(415, 475);
    highGainPlusText.setFillColor(sf::Color::Black);

    sf::RectangleShape highGainMinus(sf::Vector2f(50, 50));
    highGainMinus.setPosition(300, 470);
    highGainMinus.setFillColor(sf::Color::Yellow);

    sf::Text highGainMinusText("-", font, 30);
    highGainMinusText.setPosition(315, 475);
    highGainMinusText.setFillColor(sf::Color::Black);

    sf::Text highGainText("High: " + std::to_string(highGain), font, 20);
    highGainText.setPosition(150, 480);
    highGainText.setFillColor(sf::Color::Black);

    sf::RectangleShape startButton(sf::Vector2f(200, 50));
    startButton.setPosition(200, 550);
    startButton.setFillColor(sf::Color::Red);

    sf::Text startButtonText("Start Processing", font, 20);
    startButtonText.setPosition(210, 560);
    startButtonText.setFillColor(sf::Color::White);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            // File selection
            if (event.type == sf::Event::MouseButtonPressed)
            {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);

                // Handle file selection
                if (fileButton.getGlobalBounds().contains(mousePos.x, mousePos.y))
                {
                    inputFilePath = openFileDialog();
                    if (!inputFilePath.empty())
                    {
                        filePathText.setString("Selected File: " + inputFilePath);
                        fileSelected = true;
                    }
                }

                // Handle processing function selection
                if (proc1Button.getGlobalBounds().contains(mousePos.x, mousePos.y))
                    selectedProc = MyProc1;

                if (proc2Button.getGlobalBounds().contains(mousePos.x, mousePos.y))
                    selectedProc = MyProc2;

                // Adjust thread count
                if (threadPlus.getGlobalBounds().contains(mousePos.x, mousePos.y))
                    ++numThreads;
                if (threadMinus.getGlobalBounds().contains(mousePos.x, mousePos.y) && numThreads > 1)
                    --numThreads;
                threadCountText.setString("Threads: " + std::to_string(numThreads));

                // Adjust gain values
                if (lowGainPlus.getGlobalBounds().contains(mousePos.x, mousePos.y))
                    ++lowGain;
                if (lowGainMinus.getGlobalBounds().contains(mousePos.x, mousePos.y) && lowGain > 0)
                    --lowGain;
                if (midGainPlus.getGlobalBounds().contains(mousePos.x, mousePos.y))
                    ++midGain;
                if (midGainMinus.getGlobalBounds().contains(mousePos.x, mousePos.y) && midGain > 0)
                    --midGain;
                if (highGainPlus.getGlobalBounds().contains(mousePos.x, mousePos.y))
                    ++highGain;
                if (highGainMinus.getGlobalBounds().contains(mousePos.x, mousePos.y) && highGain > 0)
                    --highGain;

                lowGainText.setString("Low: " + std::to_string(lowGain));
                midGainText.setString("Mid: " + std::to_string(midGain));
                highGainText.setString("High: " + std::to_string(highGain));

                if (proc1Button.getGlobalBounds().contains(mousePos.x, mousePos.y))
                {
                    selectedProc = MyProc1;
                    std::cout << "ASM selected for processing." << std::endl;
                }

                if (proc2Button.getGlobalBounds().contains(mousePos.x, mousePos.y))
                {
                    selectedProc = MyProc2;
                    std::cout << "C++ selected for processing." << std::endl;
                }

                if (startButton.getGlobalBounds().contains(mousePos.x, mousePos.y) && fileSelected && selectedProc)
                {
                    // Open the file
                    SF_INFO sfInfo;
                    SNDFILE* sndFile = sf_open(inputFilePath.c_str(), SFM_READ, &sfInfo);
                    if (!sndFile)
                    {
                        std::cerr << "Error opening input file!" << std::endl;
                        continue;
                    }

                    // Read the audio data
                    std::vector<int16_t> audioBuffer(sfInfo.frames * sfInfo.channels);
                    sf_read_short(sndFile, audioBuffer.data(), audioBuffer.size());
                    sf_close(sndFile);

                    // Convert 16-bit to 32-bit
                    std::vector<int> audioBuffer32(audioBuffer.begin(), audioBuffer.end());

                    // Start timing
                    auto startTime = std::chrono::high_resolution_clock::now();

                    // Process the audio data using threads
                    long long bufferLength = audioBuffer32.size();
                    long long segmentSize = bufferLength / numThreads;
                    std::vector<std::thread> threads;

                    for (int i = 0; i < numThreads; ++i)
                    {
                        long long start = i * segmentSize;
                        long long end = (i == numThreads - 1) ? bufferLength : start + segmentSize;

                        threads.emplace_back(processBufferSegment, selectedProc, audioBuffer32.data(), start, end, lowGain, midGain, highGain);
                    }

                    for (auto& thread : threads)
                    {
                        thread.join();
                    }

                    // Stop timing
                    auto endTime = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> duration = endTime - startTime;

                    // Convert back to 16-bit and save
                    std::vector<int16_t> processedBuffer(audioBuffer32.begin(), audioBuffer32.end());
                    SF_INFO outSfInfo = sfInfo;
                    SNDFILE* outFile = sf_open(outputFilePath.c_str(), SFM_WRITE, &outSfInfo);

                    if (!outFile)
                    {
                        std::cerr << "Error opening output file!" << std::endl;
                        continue;
                    }

                    sf_write_short(outFile, processedBuffer.data(), processedBuffer.size());
                    sf_close(outFile);

                    std::cout << "Processing complete. Output saved to " << outputFilePath << std::endl;

                    // Display the time taken for processing
                    std::cout << "Time taken for processing: " << duration.count() << " seconds." << std::endl;

                    window.close();
                }

            }
        }

        window.clear(sf::Color::White);

        // Draw all elements
        window.draw(fileButton);
        window.draw(fileButtonText);
        window.draw(filePathText);
        window.draw(proc1Button);
        window.draw(proc1Text);
        window.draw(proc2Button);
        window.draw(proc2Text);
        window.draw(threadPlus);
        window.draw(threadPlusText);
        window.draw(threadMinus);
        window.draw(threadMinusText);
        window.draw(threadCountText);
        window.draw(lowGainPlus);
        window.draw(lowGainPlusText);
        window.draw(lowGainMinus);
        window.draw(lowGainMinusText);
        window.draw(lowGainText);
        window.draw(midGainPlus);
        window.draw(midGainPlusText);
        window.draw(midGainMinus);
        window.draw(midGainMinusText);
        window.draw(midGainText);
        window.draw(highGainPlus);
        window.draw(highGainPlusText);
        window.draw(highGainMinus);
        window.draw(highGainMinusText);
        window.draw(highGainText);
        window.draw(startButton);
        window.draw(startButtonText);

        window.display();
    }

    return 0;
}
