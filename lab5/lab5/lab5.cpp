
//#include <windows.h>
//#include <string>
//#include <iostream>
//#include "tchar.h"
//#include <fstream>
//#include <semaphore>

//CRITICAL_SECTION FileLockingCriticalSection;

//const int DEPOSIT_AMOUNT = 230;
//const int WITHDRAW_AMOUNT = 1000;

//int ReadFromFile()
//{
//	//EnterCriticalSection(&FileLockingCriticalSection);   
//	std::fstream myfile("balance.txt", std::ios_base::in);
//	int result;
//	myfile >> result;
//	myfile.close();
//	//LeaveCriticalSection(&FileLockingCriticalSection);
//	return result;
//}

//void WriteToFile(int data)
//{
//	//EnterCriticalSection(&FileLockingCriticalSection);
//	std::fstream myfile("balance.txt", std::ios_base::out);
//	myfile << data << std::endl;
//	myfile.close();
//	//LeaveCriticalSection(&FileLockingCriticalSection);
//}

//int GetBalance() {
//	int balance = ReadFromFile();
//	return balance;
//}

//void Deposit(int money) {
//	int balance = GetBalance();
//	balance += money;
//
//	WriteToFile(balance);
//	printf("Balance after deposit: %d\n", balance);
//}

//void Withdraw(int money) {
//	if (GetBalance() < money) {
//		printf("Cannot withdraw money, balance lower than %d\n", money);
//		return;
//	}
//
//	Sleep(20);
//	int balance = GetBalance();
//	balance -= money;
//	WriteToFile(balance);
//
//	printf("Balance after withdraw: %d\n", balance);
//}

//DWORD WINAPI DoDeposit(CONST LPVOID lpParameter)
//{
//	EnterCriticalSection(&FileLockingCriticalSection);
//	Deposit((int)lpParameter);
//	LeaveCriticalSection(&FileLockingCriticalSection);
//	ExitThread(0);
//}

//DWORD WINAPI DoWithdraw(CONST LPVOID lpParameter)
//{
//	EnterCriticalSection(&FileLockingCriticalSection);
//	Withdraw((int)lpParameter);
//	LeaveCriticalSection(&FileLockingCriticalSection);
//	ExitThread(0);
//}

//int _tmain(int argc, _TCHAR* argv[])
//{
//	HANDLE* handles = new HANDLE[50];
//
//	/*InitializeCriticalSection(&FileLockingCriticalSection);*/
//	InitializeCriticalSectionAndSpinCount(&FileLockingCriticalSection, 1);
//
//	WriteToFile(0);
//
//	SetProcessAffinityMask(GetCurrentProcess(), 1);
//	for (int i = 0; i < 50; i++) {
//		handles[i] = (i % 2 == 0)
//			? CreateThread(NULL, 0, &DoDeposit, (LPVOID)DEPOSIT_AMOUNT, CREATE_SUSPENDED, NULL)
//			: CreateThread(NULL, 0, &DoWithdraw, (LPVOID)WITHDRAW_AMOUNT, CREATE_SUSPENDED, NULL);
//		ResumeThread(handles[i]);
//	}
//
//	WaitForMultipleObjects(50, handles, true, INFINITE);
//	printf("Final Balance: %d\n", GetBalance());
//
//
//	delete[] handles;
//
//	DeleteCriticalSection(&FileLockingCriticalSection);
//
//	getchar();
//
//	return 0;
//}


#include <windows.h>
#include <string>
#include <iostream>
#include "tchar.h"
#include <fstream>

const int DEPOSIT_AMOUNT = 230;
const int WITHDRAW_AMOUNT = 1000;


CRITICAL_SECTION FileLockingCriticalSection;
CRITICAL_SECTION BalanceCriticalSection;

int ReadFromFile() 
{
	EnterCriticalSection(&FileLockingCriticalSection);
	std::fstream myfile("balance.txt", std::ios_base::in);
	int result = 0;
	if (myfile.is_open()) 
	{
		myfile >> result;
		myfile.close();
	}
	LeaveCriticalSection(&FileLockingCriticalSection);

	return result;
}

void WriteToFile(int data) 
{
	EnterCriticalSection(&FileLockingCriticalSection);
	std::fstream myfile("balance.txt", std::ios_base::out);
	myfile << data << std::endl;
	myfile.close();
	LeaveCriticalSection(&FileLockingCriticalSection);
}

int GetBalance() 
{
	return ReadFromFile();
}

void Deposit(int money) 
{
	EnterCriticalSection(&BalanceCriticalSection);
	int balance = GetBalance();
	balance += money;

	WriteToFile(balance);
	printf("Balance after deposit: %d\n", balance);
	LeaveCriticalSection(&BalanceCriticalSection);
}

void Withdraw(int money) 
{
	EnterCriticalSection(&BalanceCriticalSection);
	int balance = GetBalance();

	if (balance < money)
	{
		printf("Cannot withdraw money, balance lower than %d\n", money);
		LeaveCriticalSection(&BalanceCriticalSection);
		return;
	}

	Sleep(20);
	balance -= money;
	WriteToFile(balance);
	printf("Balance after withdraw: %d\n", balance);
	LeaveCriticalSection(&BalanceCriticalSection);
}

DWORD WINAPI DoDeposit(CONST LPVOID lpParameter)
{
	Deposit((int)lpParameter);
	ExitThread(0);
}

DWORD WINAPI DoWithdraw(CONST LPVOID lpParameter)
{
	Withdraw((int)lpParameter);
	ExitThread(0);
}

int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE* handles = new HANDLE[50];

	InitializeCriticalSection(&FileLockingCriticalSection);
	InitializeCriticalSection(&BalanceCriticalSection);

	WriteToFile(0);

	SetProcessAffinityMask(GetCurrentProcess(), 1);
	for (int i = 0; i < 50; i++) {
		handles[i] = (i % 2 == 0)
			? CreateThread(NULL, 0, &DoDeposit, (LPVOID)DEPOSIT_AMOUNT, CREATE_SUSPENDED, NULL)
			: CreateThread(NULL, 0, &DoWithdraw, (LPVOID)WITHDRAW_AMOUNT, CREATE_SUSPENDED, NULL);
		ResumeThread(handles[i]);
	}

	WaitForMultipleObjects(50, handles, true, INFINITE);
	printf("Final Balance: %d\n", GetBalance());

	system("pause");

	DeleteCriticalSection(&BalanceCriticalSection);
	DeleteCriticalSection(&FileLockingCriticalSection);

	delete[] handles;

	return 0;
}
