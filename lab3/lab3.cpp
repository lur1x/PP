#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

struct ThreadData
{
    int threadNumber;
    DWORD startTime;
    int operationsCount;
};

bool isPrime(int n)
{
    if (n <= 1)
        return false;
    if (n <= 3)
        return true;
    if (n % 2 == 0 || n % 3 == 0)
        return false;

    for (int i = 5; i * i <= n; i += 6)
    {
        if (n % i == 0 || n % (i + 2) == 0)
        {
            return false;
        }
    }
    return true;
}

DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
    ThreadData *data = (ThreadData *)lpParam;

    std::string filename = "thread_" + std::to_string(data->threadNumber) + ".txt";
    std::ofstream outFile(filename);

    if (!outFile.is_open())
    {
        std::cerr << "Ошибка открытия файла для потока " << data->threadNumber << std::endl;
        return 1;
    }

    std::cout << "Поток " << data->threadNumber << " начал работу" << std::endl;

    for (int i = 0; i < data->operationsCount; i++)
    {
        int primeCount = 0;
        for (int num = 1000000 + (data->threadNumber * 1000); num < 1005000 + (data->threadNumber * 1000); num++)
        {
            if (isPrime(num))
            {
                primeCount++;
            }
        }

        DWORD currentTime = timeGetTime();
        DWORD elapsedTime = currentTime - data->startTime;

        outFile << elapsedTime << std::endl;

        Sleep(10 + (data->threadNumber * 5));
    }

    outFile.close();
    std::cout << "Поток " << data->threadNumber << " завершил работу. Данные сохранены в " << filename << std::endl;

    return 0;
}

int main()
{
    setlocale(LC_ALL, "RU");

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    std::cout << "Всего процессоров в системе: " << sysInfo.dwNumberOfProcessors << std::endl;

    system("pause");

    DWORD_PTR processMask, systemMask;
    GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask);

    if (processMask == 1)
    {
        std::cout << "Процесс ограничен одним процессором" << std::endl;
    }
    else
    {
        std::cout << "Процесс НЕ ограничен. Доступно несколько процессоров." << std::endl;
    }

    system("pause");

    const int THREAD_COUNT = 2;
    const int OPERATIONS_COUNT = 20;

    DWORD startTime = timeGetTime();

    ThreadData *threadData = new ThreadData[THREAD_COUNT];
    HANDLE *handles = new HANDLE[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        threadData[i].threadNumber = i + 1;
        threadData[i].startTime = startTime;
        threadData[i].operationsCount = OPERATIONS_COUNT;

        handles[i] = CreateThread(NULL, 0, &ThreadProc, &threadData[i], CREATE_SUSPENDED, NULL);

        if (handles[i] == NULL)
        {
            std::cerr << "Ошибка при создании потока " << (i + 1) << std::endl;

            for (int j = 0; j < i; j++)
            {
                if (handles[j] != NULL)
                {
                    CloseHandle(handles[j]);
                }
            }

            delete[] handles;
            delete[] threadData;

            return 1;
        }
    }

    /*SetThreadPriority(handles[0], THREAD_PRIORITY_NORMAL);
    SetThreadPriority(handles[1], THREAD_PRIORITY_NORMAL);*/

    SetThreadPriority(handles[0], THREAD_PRIORITY_NORMAL);
    SetThreadPriority(handles[1], THREAD_PRIORITY_HIGHEST);

    std::cout << "\nПриоритеты установлены" << std::endl;
    std::cout << "\nЗапускаем потоки..." << std::endl;

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        ResumeThread(handles[i]);
    }

    WaitForMultipleObjects(THREAD_COUNT, handles, TRUE, INFINITE);

    DWORD endTime = timeGetTime();
    DWORD totalTime = endTime - startTime;
    std::cout << "Общее время выполнения: " << totalTime << " мс" << std::endl;

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        CloseHandle(handles[i]);
    }

    delete[] handles;
    delete[] threadData;

    std::cout << "\nВсе потоки завершили работу" << std::endl;
    std::cout << "Данные сохранены в файлы" << std::endl;

    return 0;
}