
//#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <strsafe.h>
#include <conio.h>
#include <fstream>
#include <io.h>
#include <locale.h>


//#include <winsock2.h>
//#include <ws2tcpip.h>

#define MAX_THREADS 3
#define BUF_SIZE 255


#define dir_len 12
#define len 8
#pragma comment (lib, "Ws2_32.lib")


#define DEFAULT_BUFLEN 1500
//#define Maxf 260		//���к�MAX_PATH ��Ϊ260
#define Filecont_Max 974	
#define ExceptFile 6	//2 + 4 ��ȥfiledir��filename��filecont�Ķ���
#define SliceFileLen 65535

//mtu_type��8λ��2-4λ���ڱ�ʾ��ͬtype,
#define TypeACK 240		//240 = 1111 0000��ʾACK��
#define TypeUpload_file0 48		//0011 0000 ��λΪ0 1���������
#define TypeUpload_file1 176	//1011 0000
#define TypeDownload_file0 64	//0100 0000
#define TypeDownload_file1 192	//1100 0000


typedef struct _Header  //��󲻳���1500�ֽ�(B) = 2 + 4 + 260 + 260 + 974
{
	unsigned short slice_file_len;	//��Ƭ���ĳ���	2Byte = 16bit ���2^16-1
	unsigned char mtu_type;			//mtu &�������� 1B 8b 
	unsigned char slice_num;        //�ڼ�����Ƭ	1B 8b  
	unsigned char slice_sum;        //��������Ƭ	1B 8b 
	unsigned char ID;				//ID��			1B 8b

	char filedir[MAX_PATH];			//Ŀ¼��������	 ���2080λ 260byte
	char filename[MAX_PATH];		//�ļ���������	 ���2080λ 260byte
	char filecont[Filecont_Max];	//�ļ����ݲ����� ���974byte 

}hdr, HDR, *PHDR;


//����ACK������
char * Create_struct(
	char type,
	char slice_num_,
	char slice_sum_,
	char ID,
	char *filedir,
	char *filename,
	char *filecont,
	int fileContLength)
{
	//�����ڴ�
	PHDR phdr = (PHDR)malloc(sizeof(HDR));

	memset(phdr, 0, sizeof(HDR));

	phdr->mtu_type = type;
	phdr->ID = ID;
	phdr->slice_num = slice_num_;
	phdr->slice_sum = slice_sum_;

	if (filedir != NULL)
		strcpy_s(phdr->filedir, MAX_PATH, filedir);

	if (filename != NULL)
		strcpy_s(phdr->filename, MAX_PATH, filename);

	if (filecont != NULL)
		memcpy(phdr->filecont, filecont, fileContLength);

	//Ŀ¼���ļ���Ϊ�ַ������ɣ�����ʹ��strcpy
	//�ļ�������һ��Ϊ�ַ�������ͼƬ��soʹ��memcpy()�����ڴ濽����fileContLength���ֽڣ�

	phdr->slice_file_len = fileContLength + MAX_PATH + MAX_PATH + ExceptFile;

	return (char*)phdr;
}

int Check_out(PHDR pl, SOCKET sock)
{
	char*msg;
	char*msg_error;

	if (pl->slice_file_len < 0 || pl->slice_file_len > SliceFileLen)
	{
		msg = "slice_file_len error!\n";
		msg_error = Create_struct(pl->mtu_type,0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);  //strlen()���Ȳ�����'\0'

		send(sock, msg_error, strlen(msg) + len + sizeof(char), NULL);

		return ERROR;
	}
	else if (pl->slice_num > pl->slice_sum || pl->slice_num < 0 || pl->slice_sum < 0)
	{
		msg = "slice_num or slice_sum error!\n";
		msg_error = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);

		send(sock, msg_error, strlen(msg) + len + sizeof(char), NULL);

		return ERROR;
	}

	return 1;
}

//������Ϣ�������ú����������Ƭ����д�� Ŀ���ļ��в�����
void Write_file(PHDR pl, SOCKET sock)
{

	FILE* fp;
	int flag = 1;
	char *file_path;
	char *msg_long;
	char *msg;


	file_path = (char *)malloc(MAX_PATH);
	memset(file_path, 0, MAX_PATH);

	//���ļ�Ŀ¼�������Լ�����ƴ����һ����ΪҪ�����ļ���Ŀ��·��
	strcpy_s(file_path, MAX_PATH, pl->filedir);
	strcat_s(file_path, MAX_PATH, pl->filename);	//strcat���������ַ���

	if (pl->slice_num == 0)
	{//�Ƿ�Ϊ�ļ��ĵ�һƬ������������⣩

		//�����ַ���str���ȣ�������������NULL�����Ϊmaxlen
		size_t filedir_len;
		filedir_len = strnlen_s(pl->filedir, MAX_PATH);
		if (filedir_len > (MAX_PATH - 3))
		{
			msg = "Directory path is too long!\n";
			msg_long = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);
			send(sock, msg_long, ((PHDR)msg_long)->slice_file_len, NULL);

			//_tprintf(TEXT("\nDirectory path is too long.\n"));
		}

		strcat_s(pl->filedir, MAX_PATH, "*");	//������

		WIN32_FIND_DATA ffd = { 0 };
		HANDLE hFind = INVALID_HANDLE_VALUE;

		//��ȫ�Լ�⣬��û���ļ���ֱ�Ӵ����ļ�����Ŀ¼��
		hFind = FindFirstFile(pl->filedir, &ffd);

		if (hFind == INVALID_HANDLE_VALUE)
		{
			//����һ����׷����ʽ�������д���ļ� a+ ab???
			if (fp = fopen(file_path, "ab"))
			{
				flag = 0;

				//��������Ƭ��������׷�ӵ���ʽд���ļ���
				fwrite(pl->filecont, strlen(pl->filecont), 1, fp);
			}
			fclose(fp);
		}

		do
		{//���� ��FindFirstFile�ҵ��ĵ�һ���ļ���� Ŀ¼���ļ�������Ƿ�ͬ��
			if ((strcmp(ffd.cFileName, pl->filename)) == 0)
			{
				flag = 0;

				msg = "�ļ�ͬ����\n";
				msg_long = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);
				send(sock, msg_long, ((PHDR)msg_long)->slice_file_len, NULL);

				//_tprintf(TEXT("\n�ļ�ͬ����\n"));
			}

		} while (FindNextFile(hFind, &ffd) != 0);

		FindClose(hFind);

		//û��������û�д���ʱ������
		if (flag)
		{
			//����һ����׷����ʽ�������д���ļ� a+ wb?
			if (fp = fopen(file_path, "wb")) 
			{
				//��������Ƭ��������׷�ӵ���ʽд���ļ���
				//�ڶ�������strlen(pl->filecont) ???
				fwrite(pl->filecont, pl->slice_file_len - 260 * 2 - 4 - 2, 1, fp);

				if (pl->slice_sum - 1 == pl->slice_num)
				{
					msg = "�ļ�����ɹ�1��\n";
					msg_long = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);
					send(sock, msg_long, ((PHDR)msg_long)->slice_file_len, NULL);

					//_tprintf(TEXT("\n�ļ�����ɹ���\n"));
				}
			}

			fclose(fp);
		}
	}
	else
	{
		//����Ƭ���ģ�a+ ab?? ��׷����ʽ�������д���ļ�
		if (fp = fopen(file_path, "ab"))
		{
			//��������Ƭ��������׷�ӵ���ʽд���ļ���
			fwrite(pl->filecont, strlen(pl->filecont) - 260 * 2 - 4 - 2, 1, fp);

			if (pl->slice_sum == pl->slice_num)
			{
				msg = "�ļ��ϴ��ɹ�2��\n";
				msg_long = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);
				send(sock, msg_long, ((PHDR)msg_long)->slice_file_len, NULL);

				//_tprintf(TEXT("\n�ļ��ϴ��ɹ���\n"));
			}
			fclose(fp);
		}
	}
}

//Ҫ����������ļ���Ƭ
int File_download(PHDR pl, SOCKET sock)
{
	long temp; //�ļ���Ƭ�Ĵ�С

	char *dir1 = pl->filedir;

	char *p = { 0 };//��Ƭ�ļ�����
	FILE *fp = NULL;

	int lenth1, lenth2, flag = 1;
	char *file_path;
	char *msg_long;
	char *msg;

	file_path = (char *)malloc(MAX_PATH);
	memset(file_path, 0, MAX_PATH);

	//���ļ�Ŀ¼�������Լ�����ƴ����һ����ΪҪ�����ļ���Ŀ��·��
	strcpy_s(file_path, MAX_PATH, pl->filedir);
	strcat_s(file_path, MAX_PATH, pl->filename);	//strcat���������ַ���

	lenth1 = strlen(pl->filedir);
	lenth2 = strlen(pl->filename);

	
	if (!(fp = fopen(file_path, "rb")))
	{
		printf("�ļ���ʧ��.\n");
		fclose(fp);
		return 0;
	}

	//��λ���ļ�ĩβ
	fseek(fp, 0L, SEEK_END); 
	
	long file_len = 0;
	file_len = ftell(fp);   //����ָ��ƫ���ļ�ͷ��λ��(���ļ����ַ�����)
	//printf("�ļ����ַ�������%d\n", file_len);

	
	//�����Ƭ��
	int send_num = 0;
	if (file_len % Filecont_Max == 0)
	{
		//��Ƭ�պ÷���
		send_num = file_len / Filecont_Max;
	}
	else
	{
		//�޷�������Ƭ�����һƬ�䲻��������Ҫ��
		send_num = file_len / Filecont_Max + 1;
	}

	//�������һƬ�ĳ��� = �ļ��� - ������Ƭ��
	int last_len = file_len - (send_num - 1)*Filecont_Max;
	
	//��λ���ļ���ͷ feek()���������ļ�ָ��stream��λ��
	fseek(fp, 0L, SEEK_SET);	

	//�����ļ���С��̬�����ڴ�ռ�
	char *s_filecount = { 0 };//��Ƭ�ļ�����
	s_filecount = (char *)malloc(Filecont_Max);

	int offset = 0;	//�ļ��ܳ��ȣ���

	//�����iƬ����
	for (int i = 0;i < send_num;i++)
	{
		int currentLen = 0;	//��ǰƬ�ĳ���
		memset(s_filecount, 0, Filecont_Max);

		if (i == send_num - 1)
		{
			currentLen = last_len;	//��ǰΪ���һƬ
		}
		else
		{
			currentLen = Filecont_Max;	//����Ƭ��������
		}

		//fread()���ļ����ڲ�ָ����Զ��ƶ�
		fread(s_filecount, 1, currentLen, fp);


		char *b = { NULL };//�����ı�����
		b = Create_struct(pl->mtu_type, i, send_num, pl->ID, NULL, NULL, s_filecount, currentLen);

		int iResult = send(sock, b, ((PHDR)b)->slice_file_len, NULL);	
		if (iResult != SOCKET_ERROR)
		{
			offset += currentLen;
		}
		else
		{
			flag = 0;
			printf("��������ļ�����ʧ��\n");
		}

	}
	
	if (flag)
	{
		msg = "���سɹ���\n";
		msg_long = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);
		send(sock, msg_long, ((PHDR)msg_long)->slice_file_len, NULL);
	}

	return 1;

}

//����������ѡ��
bool mtutype_choice(unsigned char mtu_type, PHDR pl, SOCKET sockect_n)
{
	char ret[Filecont_Max];
	char *b;

	if (mtu_type == TypeUpload_file0 || mtu_type == TypeUpload_file1)
	{
		// �ϴ�
		Write_file(pl, sockect_n);
		return true;
	}
	else if (mtu_type == TypeDownload_file0 || mtu_type == TypeDownload_file1)
	{
		//����
		File_download(pl, sockect_n);
		return true;
	}
	else
	{
		//��Ч����
		strcpy_s(ret, "������Ϊ��Ч����\0");
		b = Create_struct(TypeACK, 0, 0, pl->ID, NULL, NULL, ret, strlen(ret) + 1);
		send(sockect_n, b, ((PHDR)b)->slice_file_len, NULL);
		return false;
	}

}

