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
#define Incre_cata "mkdir" //增目录
#define Dele_cata "rmdir" //删目录
#define List_cata "ls" //列目录
#define Upload_file "upload" //上传文件
#define Download_file "down" //下载文件
#define Dele_file "delfile"	//删除文件

#pragma comment (lib, "ws2_32")         //socket库文件




int main()
{

	Sleep(2000);                        //沉睡2秒再连接server

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);//本函数必须是应用程序或DLL调用的第一个Windows Sockets函数。
	//它允许应用程序或DLL指明Windows Sockets API的版本号及获得特定Windows Sockets实现的细节。
	//应用程序或DLL只能在一次成功的WSAStartup()调用之后才能调用进一步的Windows Sockets API函数

	SOCKET s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in sockaddr;
	sockaddr.sin_family = PF_INET;
	sockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//172.27.247.1
	sockaddr.sin_port = htons(9000);

	connect(s, (SOCKADDR*)&sockaddr, sizeof(SOCKADDR));

	char buffer[1500];
	recv(s, buffer, 1500, NULL);
	printf_s("**********Final Version**********\n");
	printf_s("**********≧ε ≦≧ 3≦**********\n");
	printf_s("***SERVER***\n%s", buffer);

	
		char* mymsg;
		char Op_Type[20]={0};
	
	for(int i=0;strcmp(Op_Type,"quit")!=0;i++)//循环输入命令，直到用户输入quit；
	{
		printf_s("<Client>");
		gets_s(Op_Type);//输入指令；
		if(strcmp(Op_Type,Upload_file)==0)//4输入上传文件的命令
		{
			
			char File_Dir[260]={0};//定义服务器文件路径
			char C_File_Dir[260]={0};//定义本地文件路径
			printf_s("<Client>(enter server file dir)");
			gets_s(File_Dir);
			printf_s("<Client>(enter target file dir)");
			gets_s(C_File_Dir);
			ReturnMem1(File_Dir,4,C_File_Dir,s);//将两个路径传入处理函数
			recv(s, buffer, 1500, NULL);//接收服务器返回结果
			struct hdr pl;
			Receive_Commen(&pl,buffer,IDtt-1);//对返回结果进行解构
			
		}
		else if(strcmp(Op_Type,Incre_cata)==0)//输入增加目录命令
		{
			char File_Dir[260]={0};//定义服务器路径
			char Dir_Name[260]={0};//定义目标目录名
			printf_s("<Client>(enter server file dir)");
			gets_s(File_Dir);
			printf_s("<Client>(enter dir name)");
			gets_s(Dir_Name);
			mymsg = ReturnMem(File_Dir, 1, Dir_Name);//讲两个参数传入处理函数
			send(s, mymsg,1500, NULL);//发送报文
			recv(s, buffer, 1500, NULL);//接收服务器返回结果
			struct hdr pl;
			Receive_Commen(&pl,buffer,IDtt-1);//解构ACK内容
		}
		else if(strcmp(Op_Type,Dele_cata)==0)//输入删除目录命令
		{
			char File_Dir[260]={0};//定义服务器路径
			char Dir_Name[260]={0};//定义目标目录
			printf_s("<Client>(enter server file dir)");
			gets_s(File_Dir);
			printf_s("<Client>(enter dir name)");
			gets_s(Dir_Name);
			mymsg=ReturnMem(File_Dir,2,Dir_Name);//将参数传入处理函数
			send(s, mymsg, 1500, NULL);//发送报文
			recv(s, buffer, 1500, NULL);//接收服务器返回结果
			struct hdr pl;
			Receive_Commen(&pl,buffer,IDtt-1);//解构ACK
		}
		else if(strcmp(Op_Type,List_cata)==0)//输入列目录命令
		{
			char File_Dir[260]={0};//定义目标目录
			printf_s("<Client>(enter file dir)");
			gets_s(File_Dir);
			mymsg=ReturnMem(File_Dir,3,NULL);//将参数传入处理函数
			send(s, mymsg, 1500, NULL);//发送报文
			recv(s, buffer, 1500, NULL);//接收服务器返回结果
			struct hdr pl;
			Receive_Commen(&pl,buffer,IDtt-1);//解构ACK
		}
		else if(strcmp(Op_Type,Download_file)==0)//输入下载文件命令
		{
			char File_Dir[260]={0};//定义服务器路径
			char Dir_Name[260]={0};//定义目标文件名
			printf_s("<Client>(enter server file dir)");
			gets_s(File_Dir);
			printf_s("<Client>(enter file name)");
			gets_s(Dir_Name);
			mymsg=ReturnMem(File_Dir,5,Dir_Name);//将参数传入处理函数
			send(s, mymsg,1500, NULL);//发送报文
			do{
				recv(s, buffer, 1500, NULL);
				Receive_Download(buffer, IDtt - 1, Dir_Name);

			} while (buffer[3] < buffer[4]);
		   
		}
		else if(strcmp(Op_Type,Dele_file)==0)//输入删除文件命令
		{
			char File_Dir[260]={0};//定义服务器文件路径
			char Dir_Name[260]={0};//定义目标文件名
			printf_s("<Client>(enter server file dir)");
			gets_s(File_Dir);
			printf_s("<Client>(enter file name)");
			gets_s(Dir_Name);
			mymsg=ReturnMem(File_Dir,6,Dir_Name);//将参数传入处理函数
			send(s, mymsg, 1500, NULL);//发送报文
			recv(s, buffer, 1500, NULL);//接收服务器返回信息
			struct hdr pl;
			Receive_Commen(&pl,buffer,IDtt-1);//解构ACK
		}
		else 
			printf_s("no such command!!!!\n");
	}

	closesocket(s);

	WSACleanup();

	getchar();

	exit(0);
}













