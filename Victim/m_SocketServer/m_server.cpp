
# include "ihead.h"

const char*IPADDR = "127.0.0.1";
u_short PORT = 9000;

DWORD WINAPI MyThreadFunction(LPVOID lpParam);
//void ErrorHandler(LPTSTR lpszFunction);用于处理与客户端的通信，包涵功能函数


int main(void){
	int count = 0;//record the socket num
	int iResult;

	SOCKET MySocket;
	SOCKET ClientSocket;

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	//本函数必须是应用程序或DLL调用的第一个Windows Sockets函数。
	//它允许应用程序或DLL指明Windows Sockets API的版本号及获得特定Windows Sockets实现的细节。
	//应用程序或DLL只能在一次成功的WSAStartup()调用之后才能调用进一步的Windows Sockets API函数

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error\n");
		return ERROR;
	}

	MySocket = socket(
		PF_INET,
		SOCK_STREAM,
		IPPROTO_TCP);
	if (MySocket == INVALID_SOCKET)
	{
		printf("new socket error\n");
		WSACleanup();
		return ERROR;
	}
	//printf("%s\n", MySocket);

	sockaddr_in sockaddr;
	sockaddr.sin_family = PF_INET;
	sockaddr.sin_addr.S_un.S_addr = inet_addr(IPADDR);
	sockaddr.sin_port = htons(PORT);

	iResult = bind(MySocket, (SOCKADDR*)&sockaddr, sizeof(SOCKADDR));
	if (iResult != 0)//or iResult == SOCKET_ERROR
	{
		printf("bind MySocket error\n");
		closesocket(MySocket);
		WSACleanup();
		return ERROR;
	}

	iResult = listen(MySocket, SOMAXCONN);
	if (iResult != 0)
	{
		printf(" MySocket listen error\n");
		closesocket(MySocket);
		WSACleanup();
		return ERROR;
	}
	printf("Listening on port [%d].\n", PORT);

	HANDLE ThreadArray[MAX_THREADS];

	while (true)
	{
		SOCKADDR clientaddr;
		int size = sizeof(SOCKADDR);
		ClientSocket = accept(MySocket, &clientaddr, &size);

		if (ClientSocket == INVALID_SOCKET)
		{
			printf("MySocket accept error \n ");
			closesocket(MySocket);
			WSACleanup();
			return 1;
		}

		printf("New client touched:%d.\n", ClientSocket);

		if (count >= MAX_THREADS)
		{
			char* msg = "Exceeded max clients.Can not connect the server now.\n ";
			iResult = send(ClientSocket, msg, strlen(msg) + sizeof(char), NULL);
			if (iResult == 0)
			{
				printf("send refuse error.\n");
				closesocket(MySocket);
				WSACleanup();
				return ERROR;
			}
			closesocket(ClientSocket);
			break;
		}//if count

		ThreadArray[count] = CreateThread(
			NULL,
			0,
			MyThreadFunction,
			&ClientSocket,
			0,
			NULL);

		count++;

	}//while
	WaitForMultipleObjects(MAX_THREADS + 1, ThreadArray, true, INFINITE);
	closesocket(MySocket);

	for (int i = 0; i < MAX_THREADS; i++)
	{
		CloseHandle(ThreadArray[i]);
	}
	WSACleanup();
	printf("Clean all.\n");

	system("pause");


}

DWORD WINAPI MyThreadFunction(LPVOID lpParam){
	int iResult;

	SOCKET ClientSocket = *(SOCKET*)lpParam;
	char*msg = "Hi,my client.Now you can send your massage.\r\n";
	iResult = send(ClientSocket, msg, strlen(msg) + sizeof(char), NULL);
	if (iResult == SOCKET_ERROR)
	{
		printf("send msg error.\n");
		closesocket(ClientSocket);
		WSACleanup();
		return ERROR;

	}

	do{//控制通信
		char buffer[DEFAULT_BUFLEN];
		int buflen = DEFAULT_BUFLEN;

		iResult = recv(ClientSocket, buffer, buflen, NULL);

		if (iResult > 0)
		{
			//printf("Bytes recv:  %s\n ", buffer);
			struct hdr pl;
			TurnCharToHdr(&pl, buffer);
			Checkout(pl, ClientSocket);
			mtutype_choice(pl.mtu_type, pl, ClientSocket);

		}
		else if (iResult == 0)
		{
			printf("Connect closed.\n");
		}
		else
		{
			printf("recv error.\n");
			closesocket(ClientSocket);
			WSACleanup();
			return ERROR;
		}

	} while (iResult > 0);
}