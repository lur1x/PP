#include <windows.h>
#include <iostream>
#include <string>

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
    unsigned int threadNum = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(lpParam));
    std::cout << "Поток №" << threadNum << " выполняет свою работу" << std::endl;
    return 0;
}

int main(int argc, char* argv[])
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    if (argc != 2)
    {
        std::cout << "Использование: program.exe <количество_потоков>" << std::endl;
        return 1;
    }

    unsigned int N = 0;

    try {
        int temp = std::stoi(argv[1]);
        if (temp < 0) 
        {
            std::cerr << "Ошибка: количество потоков не может быть отрицательным." << std::endl;
            return 1;
        }
        N = static_cast<unsigned int>(temp);
    }
    catch (const std::invalid_argument&) 
    {
        std::cerr << "Ошибка: аргумент не является числом." << std::endl;
        return 1;
    }
    catch (const std::out_of_range&) 
    {
        std::cerr << "Ошибка: слишком большое число." << std::endl;
        return 1;
    }

    if (N == 0)
    {
        std::cerr << "Ошибка: количество потоков должно быть больше 0." << std::endl;
        return 1;
    }

    if (N > MAXIMUM_WAIT_OBJECTS) 
    { 
        std::cerr << "Ошибка: максимальное количество потоков — " << MAXIMUM_WAIT_OBJECTS << std::endl;
        return 1;
    }

    HANDLE* handles = new HANDLE[N];

    for (unsigned int i = 0; i < N; ++i)
    {
        handles[i] = CreateThread(
            NULL, 0,
            ThreadProc,
            reinterpret_cast<LPVOID>(static_cast<uintptr_t>(i + 1)),
            CREATE_SUSPENDED, 
            NULL
        );

        if (handles[i] == NULL)
        {
            std::cerr << "Ошибка при создании потока №" << i + 1 << std::endl;
            for (unsigned int j = 0; j < i; ++j)
                CloseHandle(handles[j]);
            delete[] handles;
            return 1;
        }
    }
    for (unsigned int j = 0; j < N; j++)
    {
        ResumeThread(handles[j]);
    }

    WaitForMultipleObjects(N, handles, TRUE, INFINITE);

    for (unsigned int i = 0; i < N; ++i)
    {
        CloseHandle(handles[i]);
    }

    delete[] handles;

    std::cout << "Все потоки завершили работу." << std::endl;
    return 0;
}
