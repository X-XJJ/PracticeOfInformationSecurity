
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


#include <winsock2.h>
#include <ws2tcpip.h>

#define MAX_THREADS 3
#define BUF_SIZE 255
#define specify_dir1 "C:\\\0"
#define specify_dir2 "C:\\\0"
#define specify_dir_len strlen(specify_dir1)
#define dir_len 12

#pragma comment (lib, "Ws2_32.lib")


#define DEFAULT_BUFLEN 1500


#define NUM 100
#define Maxf 260
#define Max 916	
#define MaxM 1500
#define N 1500
#define MAX_LEN 1000


typedef struct hdr  //��󲻳���1500�ֽ�
{
	/*unsigned short total_bytes_high;
	unsigned short total_bytes_low;  */
	unsigned short slice_file_len;   //       16
	unsigned char mtu_type;    //mtu &��������  8 
	unsigned char slice_num;        //�ڼ�����Ƭ 8  
	unsigned char slice_sum;        //��������Ƭ  8 
	unsigned char ID;				//ID�� 8
	unsigned short offset1;			//ƫ����1 8
	unsigned short offset2;			//ƫ����2 8
	char filedir[Maxf];				//Ŀ¼�� ������ ���2080λ 260 byte
	char filename[Maxf];			//�ļ��� ������
	char filecont[MaxM];			//�ļ����ݲ�����   ���916byte//1500

	//unsigned char slice_file_len;     //�ļ�����
	//char *filedir=new char[Max];

}hdr;

void  TurnCharToHdr(struct hdr *pl, char *str)
{
	int x, b;

	//pl->total_bytes_high = str[0];
	//pl->total_bytes_low = str[1];
	pl->slice_file_len = (short)(((int)str[0] - 128) * 256 + (int)str[1]);
	pl->mtu_type = str[2];
	pl->slice_num = str[3];
	pl->slice_sum = str[4];
	pl->ID = str[5];
	pl->offset1 = str[6];
	pl->offset2 = str[7];
	x = pl->offset1 - 2;
	for (b = 0; b < x; b++)
	{
		pl->filedir[b] = str[b + 8];
	}
	pl->filedir[b] = '\0';

	x = pl->offset2 - pl->offset1;

	for (b = 0; b < x; b++)
	{
		pl->filename[b] = str[6 + pl->offset1 + b];
	}
	pl->filename[b] = '\0';

	x = 6 + pl->offset2;

	for (b = 0; str[x] != '\0'; x++)
	{
		pl->filecont[b] = str[x];
		b++;
	}

	pl->filecont[b] = '\0';

}

char * createmes(struct hdr p1)			//ƴ�ӳ�Ҫ�ظ��ı��ĺ���
{

	char s1[1500] = { '\0' };

	short p = p1.slice_file_len;
	char buf1[8];
	int a = p / 256;
	int b = p % 256;
	short q1 = p1.offset1;
	short q2 = p1.offset2;
	buf1[0] = a;
	buf1[1] = b;
	buf1[2] = p1.mtu_type;
	buf1[3] = p1.slice_num;
	buf1[4] = p1.slice_sum;
	buf1[5] = p1.ID;
	buf1[6] = q1;
	buf1[7] = q2;

	for (int i = 0; i < 8; i++)
	{
		s1[i] = buf1[i];
	}
	memset(buf1, 0, sizeof(buf1));

	int d = 8, e = 0;
	do{
		s1[d] = p1.filecont[e];		//��filecont����ƴ�ӵ��ַ�����
		d++;
		e++;
	} while (e<p1.slice_file_len);//��β����\0

	if (d<1500)
		s1[d] = '\0';

	else if (d >= 1500)
		s1[1499] = '\0';

	return s1;
}
char * create_struct(

	unsigned short file_lenth,
	char slice_num_,
	char slice_sum_,
	char ID,
	char *filecont
	){

	char *ret = NULL;
	//ret = (char*)malloc(1500);
	unsigned char mtu_type_ = 240;
	struct hdr p1;
	int i = 0;

	p1.slice_file_len = file_lenth;
	p1.mtu_type = mtu_type_;
	p1.slice_num = slice_num_;
	p1.slice_sum = slice_sum_;
	p1.ID = ID;
	p1.offset1 = 0;
	p1.offset2 = 0;
	strcpy_s(p1.filecont, filecont);

	ret = createmes(p1);

	return ret;	//����������


}
int Checkout(hdr pl, SOCKET s){
	char*msg;
	char*msg_error;
	if (pl.slice_file_len < 0 || pl.slice_file_len> 65535){
		msg = "slice_file_len error!\n";
		msg_error = create_struct(strlen(msg), 0, 0, pl.ID, msg);
		send(s, msg_error, strlen(msg) + 8 + sizeof(char), NULL);
		return ERROR;
	}
	else if (pl.slice_num>pl.slice_sum || pl.slice_num<0 || pl.slice_sum<0){
		msg = "slice_num or slice_sum error!\n";
		msg_error = create_struct(strlen(msg), 0, 0, pl.ID, msg);
		send(s, msg_error, strlen(msg) + 8 + sizeof(char), NULL);
		return ERROR;
	}
	else if (pl.offset1 <= 2 || pl.offset1>pl.offset2){
		msg = "offset error!\n";
		msg_error = create_struct(strlen(msg), 0, 0, pl.ID, msg);
		send(s, msg_error, strlen(msg) + 8 + sizeof(char), NULL);
		return ERROR;
	}
	return 1;

}
void CrtDir(hdr pl, SOCKET s) //��Ŀ¼
{
	char path[MAX_PATH];
	char ret[Max];
	char *b; 
	memset(path, NULL, MAX_PATH);
	memset(ret, NULL, Max);
	DWORD dwError = 0;
	int r = 0;

		strcpy_s(path, pl.filedir);
	    strcat_s(path, "\\");
	    strcat_s(path, pl.filename);
		r = CreateDirectory(path, NULL);
		dwError = GetLastError();

		if (r != 0){//���ز����ɹ���Ϣ to client
			strcpy_s(ret, "����Ŀ¼�ɹ���\n\0");
			
		}
		else{//���ز���������Ϣ to client
			if (dwError == ERROR_ALREADY_EXISTS)
				strcpy_s(ret, "��Ŀ¼���Ѿ����ڣ�\n\0");
			else if (dwError == ERROR_PATH_NOT_FOUND)
				strcpy_s(ret, "�½�Ŀ¼����·��һ�����������ڣ�\n\0");
		}

	b = create_struct(
		strlen(ret),
		0,
		0,
		pl.ID,
		ret);
	send(s, b, strlen(ret) + 8 + sizeof(char), NULL);
}
void DltDirTvs(TCHAR path1[]) //ɾĿ¼���õı�������
{

	TCHAR path2[MAX_PATH];
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	TCHAR szDire[MAX_PATH];
	TCHAR szDirec[MAX_PATH];


	StringCchCopy(szDirec, MAX_PATH, path1);
	StringCchCat(path1, MAX_PATH, TEXT("\\*"));

	hFind = FindFirstFile(path1, &ffd);

	do
	{
		StringCchCopy(szDire, MAX_PATH, szDirec);
		StringCchCat(szDire, MAX_PATH, TEXT("\\"));
		StringCchCat(szDire, MAX_PATH, ffd.cFileName);
		StringCchCopy(path2, MAX_PATH, szDire);

		if (ffd.cFileName[0] != _T('.')){

			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				DltDirTvs(szDire);
				RemoveDirectory(path2);
			}
			else
			{
				DeleteFile(path2);
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);
	return;
}
void DltDir(hdr pl, SOCKET s) //ɾĿ¼
{
	char *b;
	char ret[Max];
	TCHAR path[MAX_PATH];
	TCHAR path2[MAX_PATH];
	int r = 0;
	memset(path, NULL, MAX_PATH);
	memset(path, NULL, MAX_PATH);
	memset(ret, NULL, Max);

	strcpy_s(path, pl.filedir);
	StringCchCat(path, MAX_PATH, TEXT("\\"));
	StringCchCat(path, MAX_PATH, pl.filename);

	if (strcmp(path, specify_dir2) == 0){//��ȫ�Լ��
		strcpy_s(ret, "����Ŀ¼���Ϸ�!\n\0");
	}
	else{
		
		StringCchCopy(path2, MAX_PATH, path);
		DltDirTvs(path);
		r = RemoveDirectory(path2);

		if (r != 0)
		{//���ز����ɹ���Ϣ to client
			strcpy_s(ret, "ɾ��Ŀ¼�ɹ���\n\0");
		}
		else{//���ز���������Ϣ to client
			strcpy_s(ret, "��ɾ��·��һ�����������ڣ�\n\0");
		}
	}
	b = create_struct(
		strlen(ret),
		0,
		0,
		pl.ID,
		ret);
	send(s, b, strlen(ret) + 8 + sizeof(char), NULL);
}
void  ListDir(hdr pl, SOCKET s)//��Ŀ¼
{
	TCHAR path[MAX_PATH];
	char ret[Max];
	memset(path, NULL, MAX_PATH);
	memset(ret, NULL, Max);
	char *b;

	WIN32_FIND_DATA ffd;
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	strcpy_s(path, pl.filedir);

	StringCchLength(path, MAX_PATH, &length_of_arg);
	if (length_of_arg > (MAX_PATH - 3))
	{
		strcpy_s(ret, "Ŀ¼���Ȳ��Ϸ���\n\0");
		b = create_struct(
			strlen(ret),
			0,
			0,
			pl.ID,
			ret);
		send(s, b, strlen(ret) + 8 + sizeof(char), NULL);
		return;
	}

	StringCchCat(path, MAX_PATH, TEXT("\\*"));
	hFind = FindFirstFile(path, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		strcpy_s(ret, "��Ŀ¼�����ڣ�����Ŀ¼�����Ƿ���ȷ��\n\0");
		b = create_struct(
			strlen(ret),
			0,
			0,
			pl.ID,
			ret);
		send(s, b, strlen(ret)+8 + sizeof(char), NULL);

		return;
	}

	do
	{
		StringCchCat(ret, Max, ffd.cFileName);
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			StringCchCat(ret, Max, TEXT("<DIR>"));
		}
		StringCchCat(ret, Max, TEXT("\n"));

	} while (FindNextFile(hFind, &ffd) != 0);
	StringCchCat(ret, Max, TEXT("\0"));
	if (strlen(ret) - 9 > 1500)
	{
		strcpy_s(ret, "����Ŀ¼������������ʾ��\n\0");
	}
	b = create_struct(
		strlen(ret),
		0,
		0,
		pl.ID,
		ret);
	send(s, b, strlen(ret) + 8 + sizeof(char), NULL);

	FindClose(hFind);
	return;
}
void writefile(hdr pl, SOCKET s)   //������Ϣ�������ú����������Ƭ����д�� Ŀ���ļ��в�����
{

	FILE* fp;
	int lenth1, lenth2, flag = 1;
	char *a;
	char *b;
	char *cont;


	lenth1 = strlen(pl.filedir);
	lenth2 = strlen(pl.filename);
	a = new char(lenth1 + lenth2 + 1);

	strcpy_s(a, lenth1 + 1, pl.filedir);
	strcat_s(a, lenth1 + lenth2 + 1, pl.filename);//���ļ�Ŀ¼�������Լ�����ƴ����һ����ΪҪ�����ļ���Ŀ��·��

	if (pl.slice_num == 1)//�Ƿ�Ϊ�ļ��ĵ�һƬ������������⣩
	{

		size_t length_of_arg;
		WIN32_FIND_DATA ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;

		StringCchLength(pl.filedir, MAX_PATH, &length_of_arg);

		if (length_of_arg > (MAX_PATH - 3))
		{
			cont = "Directory path is too long!\n";
			b = create_struct(strlen(cont), 0, 0, pl.ID, cont);
			send(s, b, strlen(cont) + 8 + sizeof(char), NULL);

			_tprintf(TEXT("\nDirectory path is too long.\n"));
		}

		StringCchCat(pl.filedir, MAX_PATH, TEXT("\\*"));
		hFind = FindFirstFile(pl.filedir, &ffd);

		if (INVALID_HANDLE_VALUE == hFind)//��ȫ�Լ�⣬��û���ļ���ֱ�Ӵ����ļ�����Ŀ¼�� 
		{
			if (fp=fopen(a, "a+")) //����һ����׷����ʽ�������д���ļ���
			{
				flag = 0;
				fwrite(pl.filecont, strlen(pl.filecont), 1, fp);//��������Ƭ��������׷�ӵ���ʽд���ļ���
			}
			fclose(fp);
		}

		do
		{
			if ((strcmp(ffd.cFileName, pl.filename))==0)
			{
				flag = 0;
				cont = "�ļ�ͬ����\n";
				b = create_struct(strlen(cont), 0, 0, pl.ID, cont);
				send(s, b, strlen(cont) + 8 + sizeof(char), NULL);

				_tprintf(TEXT("\n�ļ�ͬ����\n"));
				//send();// ack,�����ļ�ͬ��
			}
		} while (FindNextFile(hFind, &ffd) != 0);

		FindClose(hFind);

		if (flag)//û������ʱ����
		{
			if (fp=fopen(a, "a+")) //����һ����׷����ʽ�������д���ļ���
			{
				fwrite(pl.filecont, strlen(pl.filecont), 1, fp);//��������Ƭ��������׷�ӵ���ʽд���ļ���
			}

			fclose(fp);
		}
	}
	else
	{
		if (fp=fopen(a, "a+")) //����һ����׷����ʽ�������д���ļ���
		{
			fwrite(pl.filecont, strlen(pl.filecont), 1, fp);//��������Ƭ��������׷�ӵ���ʽд���ļ���
			if (pl.slice_sum == pl.slice_num)
			{
				cont = "�ļ��ϴ��ɹ���\n";
				b = create_struct(strlen(cont), 0, 0, pl.ID, cont);
				send(s, b, strlen(cont) + 8 + sizeof(char), NULL);

				_tprintf(TEXT("\n�ļ��ϴ��ɹ���\n"));
				//send();
			}
		}

		fclose(fp);

	}

}
void deletfile(hdr pl, SOCKET s)//ɾ�ļ�����
{

	int lenth1, lenth2;
	char *a;
	char *b;
	char *cont;

	lenth1 = strlen(pl.filedir);
	lenth2 = strlen(pl.filename);
	a = new char(lenth1 + lenth2 + 1);

	strcpy_s(a, lenth1 + 1, pl.filedir);
	strcat_s(a, lenth1 + lenth2 + 1, pl.filename);//���ļ�Ŀ¼�������Լ�����ƴ����һ����ΪҪ�����ļ���Ŀ��·��

	/*b = new TCHAR(strlen(a) + 1);
	strcpy_s(b, strlen(a) + 1, a);*/

	if (DeleteFile(a))
	{
		cont = "ɾ���ļ��ɹ���\n";
		b = create_struct(strlen(cont), 0, 0, pl.ID, cont);
		send(s, b, strlen(cont) + 8 + sizeof(char), NULL);
		printf("ɾ���ļ��ɹ���\n");
	}

	else{
		cont = "ɾ���ļ�ʧ�ܣ�\n";
		b = create_struct(strlen(cont), 0, 0, pl.ID, cont);
		send(s, b, strlen(cont) + 8 + sizeof(char), NULL);

		printf("ɾ���ļ��ɹ���\n");
	}

	

}
int  filedownload(char *filedir, char *filename, char ID, SOCKET s) //�����ļ���Ƭ
{
	long temp; //�ļ���Ƭ�Ĵ�С
	char *b1 = { NULL };//�����ı�����1
	char *b2 = { NULL };//�����ı�����2

	char *dir1 = filedir;

	char *p = { 0 };//��Ƭ�ļ�����
	FILE *fp = NULL;
	long n = 0;//ֻ��һƬ�ļ����ݵ������ַ�����

	int lenth1, lenth2;
	char *a;
	lenth1 = strlen(filedir);
	lenth2 = strlen(filename);
	a = new char(lenth1 + lenth2 + 1);
	strcpy_s(a, lenth1 + 1, filedir);
	strcat_s(a, lenth1 + lenth2 + 1, filename);//���ļ�Ŀ¼�������Լ�����ƴ����һ����ΪҪ�����ļ���Ŀ��·��

	if (!(fp=fopen(a, "r")))
	{
		printf("�ļ���ʧ��.\n");
		fclose(fp);
		return 0;
	}

	fseek(fp, 0L, SEEK_END); /* ��λ���ļ�ĩβ */
	n = ftell(fp);   //����ָ��ƫ���ļ�ͷ��λ��(���ļ����ַ�����)
	temp = 1500 - 8 - 1;
	int sum = n / temp;
	int i = 0;
	/* �����ļ���С��̬�����ڴ�ռ� */
	fseek(fp, 0L, SEEK_SET);
	if (sum >= 1)
	{

		for (i = 0; i<sum; i++)
		{
			int num = i + 1;
			fseek(fp, 1L * (temp*i), SEEK_SET); /* ��λ���ļ���ͷ */
			p = (char *)malloc(temp + 1);
			fread(p, temp, 1, fp); /* һ���Զ�ȡȫ���ļ����� */
			p[temp] = 0; /* �ַ���������־ */

			b1 = create_struct(temp, num, sum + 1, ID, p);
			send(s, b1, temp + 8 + sizeof(char), NULL);
			memset(p, 0, sizeof(p));
		}

		int num = i + 1;
		int wei = 0;
		fseek(fp, 1L * (temp*i), SEEK_SET); /* ��λ���ļ���ͷ */
		p = (char *)malloc(temp + 1);
		wei = n - temp*i;
		fread(p,wei, 1, fp); /* һ���Զ�ȡȫ���ļ����� */
		p[wei] = 0; /* �ַ���������־ */

		b1 = create_struct(wei, num, sum + 1, ID, p);
		send(s, b1, wei + 8 + sizeof(char), NULL);
		memset(p, 0, sizeof(p));

		printf("���سɹ���");

		return 1;
	}
	else if (sum<1)
	{
		char *y;
		y = (char *)malloc(n + 1); /* �����ļ���С��̬�����ڴ�ռ� */
		if (y == NULL)
		{
			fclose(fp);
			return 0;
		}

		fseek(fp, 0L, SEEK_SET); /* ��λ���ļ���ͷ */
		fread(y, n, 1, fp); /* һ���Զ�ȡȫ���ļ����� */
		y[n] = 0; /* �ַ���������־ */


		b2 = create_struct(n, 1, 1, ID, y);
		send(s, b2, n + 8 + sizeof(char), NULL);

		printf("���سɹ���");
		return 1;
	}

	fclose(fp);
}

bool  mtutype_choice(unsigned char mtu_type, hdr pl, SOCKET sockect_n)//����������ѡ��
{
	char ret[Max];
	char *b;
	char a[dir_len] = { 0 };

	strncpy(a, pl.filedir + 0, specify_dir_len);
	if (strcmp(a, specify_dir1) != 0){//��ȫ�Լ��
		strcpy_s(ret, "����Ŀ¼���Ϸ�!\n\0");
		b = create_struct(
			strlen(ret),
			0,
			0,
			pl.ID,
			ret);
		send(sockect_n, b, strlen(ret) + 8 + sizeof(char), NULL);
	}
	else
	{
		if (mtu_type == 0 || mtu_type == 128)//��Ŀ¼
		{
			CrtDir(pl, sockect_n);
			return true;
		}
		else if (mtu_type == 16 || mtu_type == 144)//ɾĿ¼
		{
			DltDir(pl, sockect_n);
			return true;
		}
		else if (mtu_type == 32 || mtu_type == 160)//��Ŀ¼
		{
			ListDir(pl, sockect_n);
			return true;
		}
		else if (mtu_type == 48 || mtu_type == 176)// �ϴ�
		{

			writefile(pl, sockect_n);

			return true;
		}
		else if (mtu_type == 64 || mtu_type == 192)//����
		{
			filedownload(pl.filedir, pl.filename, pl.ID, sockect_n);//Q��id?socket_n
			return true;
		}
		else if (mtu_type == 80 || mtu_type == 208)//ɾ�ļ�
		{
			deletfile(pl, sockect_n);
			return true;
		}
		else{//��Ч����
			strcpy_s(ret, "������Ϊ��Ч����\0");
			b = create_struct(
				strlen(ret),
				0,
				0,
				pl.ID,
				ret);
			send(sockect_n, b, strlen(ret) + 8 + sizeof(char), NULL);
			return false;
		}
	}
	
}


