#include <windows.h>
#include <string>
#include <iostream>
#include <tchar.h>
CRITICAL_SECTION cs;

DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
	int threadNum = *(int*)lpParam;
	EnterCriticalSection(&cs);
	std::cout << "Поток №" << threadNum << " выполняет свою работу" << std::endl;
	LeaveCriticalSection(&cs);
	ExitThread(0);
}

int _tmain(int argc, _TCHAR* argv[])
{
	setlocale(LC_ALL, "RU");

	if (argc != 2)
	{
		std::cerr << "Ошибка, для запуска программы используйте: program.exe <количество_потоков>" << std::endl;
		return 1;
	}

	unsigned int N = 0;

	try 
	{
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
	unsigned int* threadIds = new unsigned int[N];

	InitializeCriticalSection(&cs);

	HANDLE* handles = new HANDLE[N];
	for (unsigned int i = 0; i < N; i++)
	{
		threadIds[i] = i + 1;
		handles[i] = CreateThread(NULL, 0, &ThreadProc, &threadIds[i], 0, NULL);
	
		if (handles[i] == NULL)
		{
			std::cerr << "Ошибка при создании потока " << (i + 1) << std::endl;
			
			for (unsigned int j = 0; j < i; ++j)
			{
				CloseHandle(handles[j]);
			}
			delete[] handles;

			return 1;
		}

	}

	for (unsigned int i = 0; i < N; i++)
	{
		ResumeThread(handles[i]);
	}

	WaitForMultipleObjects(N, handles, true, INFINITE);

	for (unsigned int i = 0; i < N; i++)
	{
		CloseHandle(handles[i]);
	}

	delete[] handles;

	std::cout << "Все потоки завершили работу" << std::endl;
	return 0;
}
