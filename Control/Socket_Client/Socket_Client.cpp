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
#define Incre_cata "mkdir" //��Ŀ¼
#define Dele_cata "rmdir" //ɾĿ¼
#define List_cata "ls" //��Ŀ¼
#define Upload_file "upload" //�ϴ��ļ�
#define Download_file "down" //�����ļ�
#define Dele_file "delfile"	//ɾ���ļ�

#pragma comment (lib, "ws2_32")         //socket���ļ�




int main()
{

	Sleep(2000);                        //��˯2��������server

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);//������������Ӧ�ó����DLL���õĵ�һ��Windows Sockets������
	//������Ӧ�ó����DLLָ��Windows Sockets API�İ汾�ż�����ض�Windows Socketsʵ�ֵ�ϸ�ڡ�
	//Ӧ�ó����DLLֻ����һ�γɹ���WSAStartup()����֮����ܵ��ý�һ����Windows Sockets API����

	SOCKET s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in sockaddr;
	sockaddr.sin_family = PF_INET;
	sockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//172.27.247.1
	sockaddr.sin_port = htons(9000);

	connect(s, (SOCKADDR*)&sockaddr, sizeof(SOCKADDR));

	char buffer[1500];
	recv(s, buffer, 1500, NULL);
	printf_s("**********Final Version**********\n");
	printf_s("**********�R�� �Q�R 3�Q**********\n");
	printf_s("***SERVER***\n%s", buffer);

	
		char* mymsg;
		char Op_Type[20]={0};
	
	for(int i=0;strcmp(Op_Type,"quit")!=0;i++)//ѭ���������ֱ���û�����quit��
	{
		printf_s("<Client>");
		gets_s(Op_Type);//����ָ�
		if(strcmp(Op_Type,Upload_file)==0)//4�����ϴ��ļ�������
		{
			
			char File_Dir[260]={0};//����������ļ�·��
			char C_File_Dir[260]={0};//���屾���ļ�·��
			printf_s("<Client>(enter server file dir)");
			gets_s(File_Dir);
			printf_s("<Client>(enter target file dir)");
			gets_s(C_File_Dir);
			ReturnMem1(File_Dir,4,C_File_Dir,s);//������·�����봦����
			recv(s, buffer, 1500, NULL);//���շ��������ؽ��
			struct hdr pl;
			Receive_Commen(&pl,buffer,IDtt-1);//�Է��ؽ�����н⹹
			
		}
		else if(strcmp(Op_Type,Incre_cata)==0)//��������Ŀ¼����
		{
			char File_Dir[260]={0};//���������·��
			char Dir_Name[260]={0};//����Ŀ��Ŀ¼��
			printf_s("<Client>(enter server file dir)");
			gets_s(File_Dir);
			printf_s("<Client>(enter dir name)");
			gets_s(Dir_Name);
			mymsg = ReturnMem(File_Dir, 1, Dir_Name);//�������������봦����
			send(s, mymsg,1500, NULL);//���ͱ���
			recv(s, buffer, 1500, NULL);//���շ��������ؽ��
			struct hdr pl;
			Receive_Commen(&pl,buffer,IDtt-1);//�⹹ACK����
		}
		else if(strcmp(Op_Type,Dele_cata)==0)//����ɾ��Ŀ¼����
		{
			char File_Dir[260]={0};//���������·��
			char Dir_Name[260]={0};//����Ŀ��Ŀ¼
			printf_s("<Client>(enter server file dir)");
			gets_s(File_Dir);
			printf_s("<Client>(enter dir name)");
			gets_s(Dir_Name);
			mymsg=ReturnMem(File_Dir,2,Dir_Name);//���������봦����
			send(s, mymsg, 1500, NULL);//���ͱ���
			recv(s, buffer, 1500, NULL);//���շ��������ؽ��
			struct hdr pl;
			Receive_Commen(&pl,buffer,IDtt-1);//�⹹ACK
		}
		else if(strcmp(Op_Type,List_cata)==0)//������Ŀ¼����
		{
			char File_Dir[260]={0};//����Ŀ��Ŀ¼
			printf_s("<Client>(enter file dir)");
			gets_s(File_Dir);
			mymsg=ReturnMem(File_Dir,3,NULL);//���������봦����
			send(s, mymsg, 1500, NULL);//���ͱ���
			recv(s, buffer, 1500, NULL);//���շ��������ؽ��
			struct hdr pl;
			Receive_Commen(&pl,buffer,IDtt-1);//�⹹ACK
		}
		else if(strcmp(Op_Type,Download_file)==0)//���������ļ�����
		{
			char File_Dir[260]={0};//���������·��
			char Dir_Name[260]={0};//����Ŀ���ļ���
			printf_s("<Client>(enter server file dir)");
			gets_s(File_Dir);
			printf_s("<Client>(enter file name)");
			gets_s(Dir_Name);
			mymsg=ReturnMem(File_Dir,5,Dir_Name);//���������봦����
			send(s, mymsg,1500, NULL);//���ͱ���
			do{
				recv(s, buffer, 1500, NULL);
				Receive_Download(buffer, IDtt - 1, Dir_Name);

			} while (buffer[3] < buffer[4]);
		   
		}
		else if(strcmp(Op_Type,Dele_file)==0)//����ɾ���ļ�����
		{
			char File_Dir[260]={0};//����������ļ�·��
			char Dir_Name[260]={0};//����Ŀ���ļ���
			printf_s("<Client>(enter server file dir)");
			gets_s(File_Dir);
			printf_s("<Client>(enter file name)");
			gets_s(Dir_Name);
			mymsg=ReturnMem(File_Dir,6,Dir_Name);//���������봦����
			send(s, mymsg, 1500, NULL);//���ͱ���
			recv(s, buffer, 1500, NULL);//���շ�����������Ϣ
			struct hdr pl;
			Receive_Commen(&pl,buffer,IDtt-1);//�⹹ACK
		}
		else 
			printf_s("no such command!!!!\n");
	}

	closesocket(s);

	WSACleanup();

	getchar();

	exit(0);
}













