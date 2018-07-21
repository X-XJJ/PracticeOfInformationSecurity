
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


typedef struct hdr  //最大不超过1500字节
{
	/*unsigned short total_bytes_high;
	unsigned short total_bytes_low;  */
	unsigned short slice_file_len;   //       16
	unsigned char mtu_type;    //mtu &操作类型  8 
	unsigned char slice_num;        //第几个分片 8  
	unsigned char slice_sum;        //共几个分片  8 
	unsigned char ID;				//ID号 8
	unsigned short offset1;			//偏移量1 8
	unsigned short offset2;			//偏移量2 8
	char filedir[Maxf];				//目录名 不定长 最大2080位 260 byte
	char filename[Maxf];			//文件名 不定长
	char filecont[MaxM];			//文件内容不定长   最大916byte//1500

	//unsigned char slice_file_len;     //文件长度
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

char * createmes(struct hdr p1)			//拼接成要回复的报文函数
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
		s1[d] = p1.filecont[e];		//将filecont内容拼接到字符串中
		d++;
		e++;
	} while (e<p1.slice_file_len);//结尾处放\0

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

	return ret;	//创建报文流


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
void CrtDir(hdr pl, SOCKET s) //增目录
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

		if (r != 0){//返回操作成功信息 to client
			strcpy_s(ret, "增加目录成功！\n\0");
			
		}
		else{//返回操作错误信息 to client
			if (dwError == ERROR_ALREADY_EXISTS)
				strcpy_s(ret, "该目录名已经存在！\n\0");
			else if (dwError == ERROR_PATH_NOT_FOUND)
				strcpy_s(ret, "新建目录所在路径一个或多个不存在！\n\0");
		}

	b = create_struct(
		strlen(ret),
		0,
		0,
		pl.ID,
		ret);
	send(s, b, strlen(ret) + 8 + sizeof(char), NULL);
}
void DltDirTvs(TCHAR path1[]) //删目录调用的遍历函数
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
void DltDir(hdr pl, SOCKET s) //删目录
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

	if (strcmp(path, specify_dir2) == 0){//安全性检测
		strcpy_s(ret, "请求目录不合法!\n\0");
	}
	else{
		
		StringCchCopy(path2, MAX_PATH, path);
		DltDirTvs(path);
		r = RemoveDirectory(path2);

		if (r != 0)
		{//返回操作成功信息 to client
			strcpy_s(ret, "删除目录成功！\n\0");
		}
		else{//返回操作错误信息 to client
			strcpy_s(ret, "所删除路径一个或多个不存在！\n\0");
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
void  ListDir(hdr pl, SOCKET s)//列目录
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
		strcpy_s(ret, "目录长度不合法！\n\0");
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
		strcpy_s(ret, "该目录不存在，请检查目录输入是否正确！\n\0");
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
		strcpy_s(ret, "所列目录过长，不予显示！\n\0");
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
void writefile(hdr pl, SOCKET s)   //报文信息处理后传入该函数，将其分片内容写入 目标文件中并保存
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
	strcat_s(a, lenth1 + lenth2 + 1, pl.filename);//将文件目录、名字以及类型拼接在一起作为要保存文件的目标路径

	if (pl.slice_num == 1)//是否为文件的第一片（解决重名问题）
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

		if (INVALID_HANDLE_VALUE == hFind)//安全性检测，若没有文件则直接创建文件（空目录） 
		{
			if (fp=fopen(a, "a+")) //创建一个以追加形式（允许读写）文件。
			{
				flag = 0;
				fwrite(pl.filecont, strlen(pl.filecont), 1, fp);//将各个分片的内容以追加的形式写入文件中
			}
			fclose(fp);
		}

		do
		{
			if ((strcmp(ffd.cFileName, pl.filename))==0)
			{
				flag = 0;
				cont = "文件同名！\n";
				b = create_struct(strlen(cont), 0, 0, pl.ID, cont);
				send(s, b, strlen(cont) + 8 + sizeof(char), NULL);

				_tprintf(TEXT("\n文件同名！\n"));
				//send();// ack,错误，文件同名
			}
		} while (FindNextFile(hFind, &ffd) != 0);

		FindClose(hFind);

		if (flag)//没有重名时创建
		{
			if (fp=fopen(a, "a+")) //创建一个以追加形式（允许读写）文件。
			{
				fwrite(pl.filecont, strlen(pl.filecont), 1, fp);//将各个分片的内容以追加的形式写入文件中
			}

			fclose(fp);
		}
	}
	else
	{
		if (fp=fopen(a, "a+")) //创建一个以追加形式（允许读写）文件。
		{
			fwrite(pl.filecont, strlen(pl.filecont), 1, fp);//将各个分片的内容以追加的形式写入文件中
			if (pl.slice_sum == pl.slice_num)
			{
				cont = "文件上传成功！\n";
				b = create_struct(strlen(cont), 0, 0, pl.ID, cont);
				send(s, b, strlen(cont) + 8 + sizeof(char), NULL);

				_tprintf(TEXT("\n文件上传成功！\n"));
				//send();
			}
		}

		fclose(fp);

	}

}
void deletfile(hdr pl, SOCKET s)//删文件函数
{

	int lenth1, lenth2;
	char *a;
	char *b;
	char *cont;

	lenth1 = strlen(pl.filedir);
	lenth2 = strlen(pl.filename);
	a = new char(lenth1 + lenth2 + 1);

	strcpy_s(a, lenth1 + 1, pl.filedir);
	strcat_s(a, lenth1 + lenth2 + 1, pl.filename);//将文件目录、名字以及类型拼接在一起作为要保存文件的目标路径

	/*b = new TCHAR(strlen(a) + 1);
	strcpy_s(b, strlen(a) + 1, a);*/

	if (DeleteFile(a))
	{
		cont = "删除文件成功！\n";
		b = create_struct(strlen(cont), 0, 0, pl.ID, cont);
		send(s, b, strlen(cont) + 8 + sizeof(char), NULL);
		printf("删除文件成功！\n");
	}

	else{
		cont = "删除文件失败！\n";
		b = create_struct(strlen(cont), 0, 0, pl.ID, cont);
		send(s, b, strlen(cont) + 8 + sizeof(char), NULL);

		printf("删除文件成功！\n");
	}

	

}
int  filedownload(char *filedir, char *filename, char ID, SOCKET s) //下载文件分片
{
	long temp; //文件分片的大小
	char *b1 = { NULL };//创建的报文流1
	char *b2 = { NULL };//创建的报文流2

	char *dir1 = filedir;

	char *p = { 0 };//分片文件内容
	FILE *fp = NULL;
	long n = 0;//只有一片文件内容的内容字符数量

	int lenth1, lenth2;
	char *a;
	lenth1 = strlen(filedir);
	lenth2 = strlen(filename);
	a = new char(lenth1 + lenth2 + 1);
	strcpy_s(a, lenth1 + 1, filedir);
	strcat_s(a, lenth1 + lenth2 + 1, filename);//将文件目录、名字以及类型拼接在一起作为要保存文件的目标路径

	if (!(fp=fopen(a, "r")))
	{
		printf("文件打开失败.\n");
		fclose(fp);
		return 0;
	}

	fseek(fp, 0L, SEEK_END); /* 定位到文件末尾 */
	n = ftell(fp);   //返回指针偏离文件头的位置(即文件中字符个数)
	temp = 1500 - 8 - 1;
	int sum = n / temp;
	int i = 0;
	/* 根据文件大小动态分配内存空间 */
	fseek(fp, 0L, SEEK_SET);
	if (sum >= 1)
	{

		for (i = 0; i<sum; i++)
		{
			int num = i + 1;
			fseek(fp, 1L * (temp*i), SEEK_SET); /* 定位到文件开头 */
			p = (char *)malloc(temp + 1);
			fread(p, temp, 1, fp); /* 一次性读取全部文件内容 */
			p[temp] = 0; /* 字符串结束标志 */

			b1 = create_struct(temp, num, sum + 1, ID, p);
			send(s, b1, temp + 8 + sizeof(char), NULL);
			memset(p, 0, sizeof(p));
		}

		int num = i + 1;
		int wei = 0;
		fseek(fp, 1L * (temp*i), SEEK_SET); /* 定位到文件开头 */
		p = (char *)malloc(temp + 1);
		wei = n - temp*i;
		fread(p,wei, 1, fp); /* 一次性读取全部文件内容 */
		p[wei] = 0; /* 字符串结束标志 */

		b1 = create_struct(wei, num, sum + 1, ID, p);
		send(s, b1, wei + 8 + sizeof(char), NULL);
		memset(p, 0, sizeof(p));

		printf("下载成功！");

		return 1;
	}
	else if (sum<1)
	{
		char *y;
		y = (char *)malloc(n + 1); /* 根据文件大小动态分配内存空间 */
		if (y == NULL)
		{
			fclose(fp);
			return 0;
		}

		fseek(fp, 0L, SEEK_SET); /* 定位到文件开头 */
		fread(y, n, 1, fp); /* 一次性读取全部文件内容 */
		y[n] = 0; /* 字符串结束标志 */


		b2 = create_struct(n, 1, 1, ID, y);
		send(s, b2, n + 8 + sizeof(char), NULL);

		printf("下载成功！");
		return 1;
	}

	fclose(fp);
}

bool  mtutype_choice(unsigned char mtu_type, hdr pl, SOCKET sockect_n)//主函数功能选择
{
	char ret[Max];
	char *b;
	char a[dir_len] = { 0 };

	strncpy(a, pl.filedir + 0, specify_dir_len);
	if (strcmp(a, specify_dir1) != 0){//安全性检测
		strcpy_s(ret, "请求目录不合法!\n\0");
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
		if (mtu_type == 0 || mtu_type == 128)//增目录
		{
			CrtDir(pl, sockect_n);
			return true;
		}
		else if (mtu_type == 16 || mtu_type == 144)//删目录
		{
			DltDir(pl, sockect_n);
			return true;
		}
		else if (mtu_type == 32 || mtu_type == 160)//列目录
		{
			ListDir(pl, sockect_n);
			return true;
		}
		else if (mtu_type == 48 || mtu_type == 176)// 上传
		{

			writefile(pl, sockect_n);

			return true;
		}
		else if (mtu_type == 64 || mtu_type == 192)//下载
		{
			filedownload(pl.filedir, pl.filename, pl.ID, sockect_n);//Q：id?socket_n
			return true;
		}
		else if (mtu_type == 80 || mtu_type == 208)//删文件
		{
			deletfile(pl, sockect_n);
			return true;
		}
		else{//无效请求
			strcpy_s(ret, "该请求为无效请求！\0");
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


