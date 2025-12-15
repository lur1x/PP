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
    Bitmap* out;
    uint32_t startHeight;
    uint32_t endHeight;
    uint32_t startWidth;
    uint32_t endWidth;
};

void Blur(int radius, Params* params)
{
    if (!params || !params->in || !params->out) return;

    Bitmap* in = params->in;
    Bitmap* out = params->out;
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

                    rgb32* pixel = in->GetPixel(x, y);
                    if (!pixel) continue;

                    r += pixel->r * weight;
                    g += pixel->g * weight;
                    b += pixel->b * weight;
                    weightSum += weight;
                }
            }

            if (weightSum > 0)
            {
                rgb32* srcPixel = in->GetPixel(j, i);
                rgb32* dstPixel = out->GetPixel(j, i);
                if (srcPixel && dstPixel)
                {
                    dstPixel->r = static_cast<uint8_t>(min(255.0, max(0.0, r / weightSum)));
                    dstPixel->g = static_cast<uint8_t>(min(255.0, max(0.0, g / weightSum)));
                    dstPixel->b = static_cast<uint8_t>(min(255.0, max(0.0, b / weightSum)));
                    dstPixel->a = srcPixel->a; 
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

void SequentialBlur(Bitmap* in, Bitmap* out)
{
    Params params;
    params.in = in;
    params.out = out;
    params.startWidth = 0;
    params.endWidth = in->GetWidth();
    params.startHeight = 0;
    params.endHeight = in->GetHeight();

    Blur(4, &params);
}

void ParallelBlur(Bitmap* in, Bitmap* out, int threadsCount, int coresCount)
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
        paramsArray[i].out = out;
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
            std::cerr << "Ошибка: Не удалось создать поток " << i << std::endl;

            for (int j = 0; j < i; j++)
            {
                if (handles[j]) CloseHandle(handles[j]);
            }
            delete[] handles;
            delete[] paramsArray;
            throw std::runtime_error("Не удалось создать поток");
        }
    }

    WaitForAllThreads(handles, threadsCount);

    CleanupThreadResources(handles, paramsArray, threadsCount);
}

Bitmap* CreateBitmapCopy(Bitmap* src)
{
    static int tempCounter = 0;
    std::string tempFile = "temp_copy_" + std::to_string(tempCounter++) + ".bmp";

    src->Save(tempFile.c_str());

    Bitmap* copy = new Bitmap(tempFile.c_str());

    remove(tempFile.c_str());

    return copy;
}

void SetProcessCores(int coresCount)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int maxCores = sysInfo.dwNumberOfProcessors;

    if (coresCount > maxCores)
    {
        coresCount = maxCores;
        std::cout << "Предупреждение: Запрошено " << coresCount << " ядер, но в системе только " << maxCores << std::endl;
    }

    DWORD_PTR processAffinityMask = (static_cast<DWORD_PTR>(1) << coresCount) - 1;
    if (!SetProcessAffinityMask(GetCurrentProcess(), processAffinityMask))
    {
        std::cerr << "Предупреждение: Не удалось установить маску сходства процесса" << std::endl;
    }

    std::cout << "Процесс ограничен " << coresCount << " ядром(ами)" << std::endl;
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
    std::cout << "Использование: " << programName << " <input.bmp> <output.bmp> <потоки> <ядра>" << std::endl;
    std::cout << "  потоки: 1 для последовательной обработки, >1 для параллельной" << std::endl;
    std::cout << "  ядра: 1-4" << std::endl;
    std::cout << std::endl;
    std::cout << "Примеры:" << std::endl;
    std::cout << "  " << programName << " input.bmp output.bmp 1 1   (последовательно)" << std::endl;
    std::cout << "  " << programName << " input.bmp output.bmp 4 2   (параллельно, 4 потока, 2 ядра)" << std::endl;
}

bool ValidateArguments(int argc, char* argv[])
{
    if (argc != 5)
    {
        PrintUsage(argv[0]);
        return false;
    }

    int threadsCount = atoi(argv[3]);
    int coresCount = atoi(argv[4]);

    if (threadsCount <= 0)
    {
        std::cerr << "Ошибка: Количество потоков должно быть положительным" << std::endl;
        return false;
    }

    if (coresCount <= 0 || coresCount > 4)
    {
        std::cerr << "Ошибка: Количество ядер должно быть от 1 до 4" << std::endl;
        return false;
    }

    return true;
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "RU");

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
        std::cout << "=== Обработчик размытия изображений ===" << std::endl;
        std::cout << "Входной файл: " << inputFile << std::endl;
        std::cout << "Выходной файл: " << outputFile << std::endl;
        std::cout << "Потоки: " << threadsCount << std::endl;
        std::cout << "Ядра: " << coresCount << std::endl;

        SetProcessCores(coresCount);

        std::cout << "Загрузка изображения..." << std::endl;
        Bitmap* originalBmp = new Bitmap(inputFile);
        std::cout << "Изображение загружено. Размер: " << originalBmp->GetWidth() << "x" << originalBmp->GetHeight() << std::endl;

        Bitmap* workingBmp = CreateBitmapCopy(originalBmp);

        std::cout << "Выполнение тестового запуска..." << std::endl;
        auto testStart = high_resolution_clock::now();

        Bitmap* outputBmp = CreateBitmapCopy(originalBmp);

        if (threadsCount == 1)
        {
            SequentialBlur(workingBmp, outputBmp);
            std::cout << "Последовательный режим завершен" << std::endl;
        }
        else
        {
            ParallelBlur(workingBmp, outputBmp, threadsCount, coresCount);
            std::cout << "Параллельный режим завершен" << std::endl;
        }

        auto testEnd = high_resolution_clock::now();
        auto testDuration = duration_cast<milliseconds>(testEnd - testStart);
        std::cout << "Тест выполнен за " << testDuration.count() << " мс" << std::endl;

        int repeats = CalculateRequiredRepeats(testDuration.count());

        if (repeats > 1)
        {
            std::cout << "Тест был слишком быстрым (" << testDuration.count() << "мс), применяем размытие "
                << repeats << " раз(а)" << std::endl;

            delete workingBmp;
            delete outputBmp;

            workingBmp = CreateBitmapCopy(originalBmp);
            outputBmp = CreateBitmapCopy(originalBmp);

            auto blurStart = high_resolution_clock::now();

            for (int i = 0; i < repeats; i++)
            {
                std::cout << "Итерация размытия " << (i + 1) << " из " << repeats << std::endl;

                if (threadsCount == 1)
                {
                    SequentialBlur(workingBmp, outputBmp);
                    std::cout << "Последовательный режим завершен" << std::endl;
                }
                else
                {
                    ParallelBlur(workingBmp, outputBmp, threadsCount, coresCount);
                    std::cout << "Параллельный режим завершен" << std::endl;
                }

                std::swap(workingBmp, outputBmp);
            }

            auto blurEnd = high_resolution_clock::now();
            auto blurTime = duration_cast<milliseconds>(blurEnd - blurStart);
            std::cout << "Все итерации размытия выполнены за " << blurTime.count() << " мс" << std::endl;

            std::swap(workingBmp, outputBmp);
        }
        else
        {
            std::cout << "Применено однократное размытие (длительность: " << testDuration.count() << " мс)" << std::endl;
        }

        std::cout << "Сохранение результата в: " << outputFile << std::endl;
        outputBmp->Save(outputFile);
        std::cout << "Изображение успешно сохранено" << std::endl;

        auto endTime = high_resolution_clock::now();
        auto totalTime = duration_cast<milliseconds>(endTime - startTime);

        std::cout << "\n=== Итоговые результаты ===" << std::endl;
        std::cout << "Режим: " << (threadsCount == 1 ? "Последовательный" : "Параллельный") << std::endl;
        std::cout << "Потоки: " << threadsCount << std::endl;
        std::cout << "Ядра: " << coresCount << std::endl;
        std::cout << "Общее время выполнения: " << totalTime.count() << " мс" << std::endl;

        std::cout << "\n" << totalTime.count() << " мс" << std::endl;

        delete originalBmp;
        delete workingBmp;
        delete outputBmp;

    }
    catch (const std::exception& e)
    {
        auto endTime = high_resolution_clock::now();
        auto totalTime = duration_cast<milliseconds>(endTime - startTime);

        std::cerr << "\nОШИБКА: " << e.what() << std::endl;
        std::cerr << "Программа завершена через " << totalTime.count() << " мс" << std::endl;

        std::cout << totalTime.count() << " мс" << std::endl;
        return 1;
    }
    catch (...)
    {
        auto endTime = high_resolution_clock::now();
        auto totalTime = duration_cast<milliseconds>(endTime - startTime);

        std::cerr << "\nОШИБКА: Произошла неизвестная ошибка" << std::endl;
        std::cerr << "Программа завершена через " << totalTime.count() << " мс" << std::endl;

        std::cout << totalTime.count() << " мс" << std::endl;
        return 1;
    }

    return 0;
}