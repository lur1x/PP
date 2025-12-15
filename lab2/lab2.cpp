#include "Bmp.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#define _USE_MATH_DEFINES
#include <math.h>

using namespace std::chrono;

struct Params 
{
    Bitmap* in;
    uint32_t startHeight;
    uint32_t endHeight;
    uint32_t startWidth;
    uint32_t endWidth;
};


void Blur(int radius, Params* params) 
{
    if (!params || !params->in) return;

    Bitmap* in = params->in;
    int width = in->GetWidth();
    int height = in->GetHeight();

    float rs = ceil(radius * 2.5f); 
    float sigma = radius / 2.0f;
    float sigmaSquare = sigma * sigma;
    float twoPiSigmaSquare = 2.0f * M_PI * sigmaSquare;

    for (auto i = params->startHeight; i < params->endHeight; ++i) 
    {
        for (auto j = params->startWidth; j < params->endWidth; ++j) 
        {
            double r = 0, g = 0, b = 0;
            double weightSum = 0;

            for (int iy = static_cast<int>(i) - static_cast<int>(rs);
                iy <= static_cast<int>(i) + static_cast<int>(rs); ++iy) 
            {
                for (int ix = static_cast<int>(j) - static_cast<int>(rs);
                    ix <= static_cast<int>(j) + static_cast<int>(rs); ++ix) 
                {

                    int x = min(width - 1, max(0, ix));
                    int y = min(height - 1, max(0, iy));

                    float dx = static_cast<float>(ix - static_cast<int>(j));
                    float dy = static_cast<float>(iy - static_cast<int>(i));
                    float distanceSquare = (dx * dx) + (dy * dy);
                    float weight = exp(-distanceSquare / (2.0f * sigmaSquare)) / twoPiSigmaSquare;

                    rgb32* pixel = params->in->GetPixel(x, y);
                    if (!pixel) continue; 

                    r += pixel->r * weight;
                    g += pixel->g * weight;
                    b += pixel->b * weight;
                    weightSum += weight;
                }
            }

            if (weightSum > 0)
            {
                rgb32* pixel = params->in->GetPixel(j, i);
                if (pixel) 
                {
                    pixel->r = static_cast<uint8_t>(min(255.0, max(0.0, r / weightSum)));
                    pixel->g = static_cast<uint8_t>(min(255.0, max(0.0, g / weightSum)));
                    pixel->b = static_cast<uint8_t>(min(255.0, max(0.0, b / weightSum)));
                }
            }
        }
    }
}


DWORD WINAPI ThreadProc(LPVOID lpParam) 
{
    Params* params = (Params*)lpParam;
    Blur(4, params);
    return 0;
}

HANDLE CreateThreadWithAffinity(Params* params, int threadIndex, int coresCount) 
{
    HANDLE threadHandle = CreateThread(NULL, 0, &ThreadProc, params, CREATE_SUSPENDED, NULL);

    if (threadHandle != NULL) 
    {
        DWORD_PTR affinityMask = (static_cast<DWORD_PTR>(1) << (threadIndex % coresCount));
        SetThreadAffinityMask(threadHandle, affinityMask);
        ResumeThread(threadHandle);
    }

    return threadHandle;
}

void WaitForAllThreads(HANDLE* handles, int threadsCount) 
{
    WaitForMultipleObjects(static_cast<DWORD>(threadsCount), handles, TRUE, INFINITE);
}

void CleanupThreadResources(HANDLE* handles, Params* params, int threadsCount) 
{
    for (int i = 0; i < threadsCount; i++) 
    {
        if (handles[i] != NULL) 
        {
            CloseHandle(handles[i]);
        }
    }
    delete[] handles;
    delete[] params;
}

void SequentialBlur(Bitmap* in) 
{
    Params params;
    params.in = in;
    params.startWidth = 0;
    params.endWidth = in->GetWidth();
    params.startHeight = 0;
    params.endHeight = in->GetHeight();

    Blur(4, &params);
}

void ParallelBlur(Bitmap* in, int threadsCount, int coresCount) 
{
    int height = in->GetHeight();
    int width = in->GetWidth();

    int partHeight = height / threadsCount;
    int heightRemaining = height % threadsCount;

    Params* paramsArray = new Params[threadsCount];
    HANDLE* handles = new HANDLE[threadsCount];

    int currentStart = 0;
    for (int i = 0; i < threadsCount; i++) 
    {
        paramsArray[i].in = in;
        paramsArray[i].startWidth = 0;
        paramsArray[i].endWidth = width;
        paramsArray[i].startHeight = currentStart;

        int currentEnd = currentStart + partHeight;
        if (i < heightRemaining) 
        {
            currentEnd++;
        }
        paramsArray[i].endHeight = min(currentEnd, height);

        currentStart = paramsArray[i].endHeight;

        handles[i] = CreateThreadWithAffinity(&paramsArray[i], i, coresCount);
        if (!handles[i]) 
        {
            std::cerr << "Error: Failed to create thread " << i << std::endl;

            for (int j = 0; j < i; j++) 
            {
                if (handles[j]) CloseHandle(handles[j]);
            }
            delete[] handles;
            delete[] paramsArray;
            throw std::runtime_error("Failed to create thread");
        }
    }

    WaitForAllThreads(handles, threadsCount);

    CleanupThreadResources(handles, paramsArray, threadsCount);
}

void SetProcessCores(int coresCount) 
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int maxCores = sysInfo.dwNumberOfProcessors;

    if (coresCount > maxCores) 
    {
        coresCount = maxCores;
        std::cout << "Warning: Requested " << coresCount << " cores, but system has only " << maxCores << std::endl;
    }

    DWORD_PTR processAffinityMask = (static_cast<DWORD_PTR>(1) << coresCount) - 1;
    if (!SetProcessAffinityMask(GetCurrentProcess(), processAffinityMask)) 
    {
        std::cerr << "Warning: Failed to set process affinity mask" << std::endl;
    }

    std::cout << "Process limited to " << coresCount << " core(s)" << std::endl;
}

int CalculateRequiredRepeats(long long durationMs) 
{
    if (durationMs < 500) 
    {
        return max(2, 500 / max(1, static_cast<int>(durationMs)));
    }
    return 1;
}

void PrintUsage(const char* programName) 
{
    std::cout << "Usage: " << programName << " <input.bmp> <output.bmp> <threads> <cores>" << std::endl;
    std::cout << "  threads: 1 for sequential, >1 for parallel" << std::endl;
    std::cout << "  cores: 1-4" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << programName << " input.bmp output.bmp 1 1   (sequential)" << std::endl;
    std::cout << "  " << programName << " input.bmp output.bmp 4 2   (parallel, 4 threads, 2 cores)" << std::endl;
}

bool ValidateArguments(int argc, char* argv[]) 
{

    if (argc != 5)
    {
        PrintUsage(argv[0]);
        return false;
    }

    int threadsCount = atoi(argv[2]);
    int coresCount = atoi(argv[3]);

    if (threadsCount <= 0)
    {
        std::cerr << "Error: Threads count must be positive" << std::endl;
        return false;
    }

    if (coresCount <= 0 || coresCount > 4)
    {
        std::cerr << "Error: Cores count must be between 1 and 4" << std::endl;
        return false;
    }

    return true;
}


int main(int argc, char* argv[]) 
{
    if (!ValidateArguments(argc, argv)) 
    {
        return 1;
    }


    const char* inputFile = argv[1];
    const char* outputFile = argv[2];
    int threadsCount = atoi(argv[3]);
    int coresCount = atoi(argv[4]);


    auto startTime = high_resolution_clock::now();

    try 
    {
        std::cout << "=== Image Blur Processor ===" << std::endl;
        std::cout << "Input file: " << inputFile << std::endl;
        std::cout << "Output file: " << outputFile << std::endl;
        std::cout << "Threads: " << threadsCount << std::endl;
        std::cout << "Cores: " << coresCount << std::endl;

        SetProcessCores(coresCount);

        std::cout << "Loading image..." << std::endl;
        Bitmap bmp(inputFile);
        std::cout << "Image loaded. Size: " << bmp.GetWidth() << "x" << bmp.GetHeight() << std::endl;

        std::cout << "Performing test run..." << std::endl;
        auto testStart = high_resolution_clock::now();

        if (threadsCount == 1) 
        {
            SequentialBlur(&bmp);
        }
        else 
        {
            ParallelBlur(&bmp, threadsCount, coresCount);
        }

        auto testEnd = high_resolution_clock::now();
        auto testDuration = duration_cast<milliseconds>(testEnd - testStart);
        std::cout << "Test completed in " << testDuration.count() << " ms" << std::endl;

        int repeats = CalculateRequiredRepeats(testDuration.count());

        if (repeats > 1) 
        {
            std::cout << "Test was too fast (" << testDuration.count() << "ms), applying blur "
                << repeats << " times" << std::endl;

            bmp = Bitmap(inputFile);

            auto blurStart = high_resolution_clock::now();

            for (int i = 0; i < repeats; i++) 
            {
                std::cout << "Blur iteration " << (i + 1) << " of " << repeats << std::endl;
                if (threadsCount == 1) 
                {
                    SequentialBlur(&bmp);
                }
                else 
                {
                    ParallelBlur(&bmp, threadsCount, coresCount);
                }
            }

            auto blurEnd = high_resolution_clock::now();
            auto blurTime = duration_cast<milliseconds>(blurEnd - blurStart);
            std::cout << "All blur iterations completed in " << blurTime.count() << " ms" << std::endl;
        }
        else 
        {
            std::cout << "Single blur applied (duration: " << testDuration.count() << " ms)" << std::endl;
        }

        std::cout << "Saving result to: " << outputFile << std::endl;
        bmp.Save(outputFile);
        std::cout << "Image saved successfully" << std::endl;

        auto endTime = high_resolution_clock::now();
        auto totalTime = duration_cast<milliseconds>(endTime - startTime);

        std::cout << "\n=== Final Results ===" << std::endl;
        std::cout << "Mode: " << (threadsCount == 1 ? "Sequential" : "Parallel") << std::endl;
        std::cout << "Threads: " << threadsCount << std::endl;
        std::cout << "Cores: " << coresCount << std::endl;
        std::cout << "Total execution time: " << totalTime.count() << " ms" << std::endl;

        std::cout << "\n" << totalTime.count() << " ms" << std::endl;

    }
    catch (const std::exception& e)
    {
        auto endTime = high_resolution_clock::now();
        auto totalTime = duration_cast<milliseconds>(endTime - startTime);

        std::cerr << "\nERROR: " << e.what() << std::endl;
        std::cerr << "Program terminated after " << totalTime.count() << " ms" << std::endl;

        std::cout << totalTime.count() << " ms" << std::endl;
        return 1;
    }
    catch (...) 
    {
        auto endTime = high_resolution_clock::now();
        auto totalTime = duration_cast<milliseconds>(endTime - startTime);

        std::cerr << "\nERROR: Unknown exception occurred" << std::endl;
        std::cerr << "Program terminated after " << totalTime.count() << " ms" << std::endl;

        std::cout << totalTime.count() << " ms" << std::endl;
        return 1;
    }

    return 0;
}