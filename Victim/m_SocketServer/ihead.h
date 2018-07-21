
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
//#define Maxf 260		//已有宏MAX_PATH 即为260
#define Filecont_Max 974	
#define ExceptFile 6	//2 + 4 除去filedir、filename、filecont的定长
#define SliceFileLen 65535

//mtu_type的8位，2-4位用于表示不同type,
#define TypeACK 240		//240 = 1111 0000表示ACK类
#define TypeUpload_file0 48		//0011 0000 首位为0 1，两种情况
#define TypeUpload_file1 176	//1011 0000
#define TypeDownload_file0 64	//0100 0000
#define TypeDownload_file1 192	//1100 0000


typedef struct _Header  //最大不超过1500字节(B) = 2 + 4 + 260 + 260 + 974
{
	unsigned short slice_file_len;	//该片报文长度	2Byte = 16bit 最大2^16-1
	unsigned char mtu_type;			//mtu &操作类型 1B 8b 
	unsigned char slice_num;        //第几个分片	1B 8b  
	unsigned char slice_sum;        //共几个分片	1B 8b 
	unsigned char ID;				//ID号			1B 8b

	char filedir[MAX_PATH];			//目录名不定长	 最大2080位 260byte
	char filename[MAX_PATH];		//文件名不定长	 最大2080位 260byte
	char filecont[Filecont_Max];	//文件内容不定长 最大974byte 

}hdr, HDR, *PHDR;


//创建ACK报文流
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
	//分配内存
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

	//目录和文件名为字符串无疑，可以使用strcpy
	//文件内容则不一定为字符串，如图片，so使用memcpy()进行内存拷贝（fileContLength个字节）

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
		msg_error = Create_struct(pl->mtu_type,0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);  //strlen()长度不包含'\0'

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

//报文信息处理后传入该函数，将其分片内容写入 目标文件中并保存
void Write_file(PHDR pl, SOCKET sock)
{

	FILE* fp;
	int flag = 1;
	char *file_path;
	char *msg_long;
	char *msg;


	file_path = (char *)malloc(MAX_PATH);
	memset(file_path, 0, MAX_PATH);

	//将文件目录、名字以及类型拼接在一起作为要保存文件的目标路径
	strcpy_s(file_path, MAX_PATH, pl->filedir);
	strcat_s(file_path, MAX_PATH, pl->filename);	//strcat连接两个字符串

	if (pl->slice_num == 0)
	{//是否为文件的第一片（解决重名问题）

		//计算字符串str长度，不包括结束符NULL，最大为maxlen
		size_t filedir_len;
		filedir_len = strnlen_s(pl->filedir, MAX_PATH);
		if (filedir_len > (MAX_PATH - 3))
		{
			msg = "Directory path is too long!\n";
			msg_long = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);
			send(sock, msg_long, ((PHDR)msg_long)->slice_file_len, NULL);

			//_tprintf(TEXT("\nDirectory path is too long.\n"));
		}

		strcat_s(pl->filedir, MAX_PATH, "*");	//？？？

		WIN32_FIND_DATA ffd = { 0 };
		HANDLE hFind = INVALID_HANDLE_VALUE;

		//安全性检测，若没有文件则直接创建文件（空目录）
		hFind = FindFirstFile(pl->filedir, &ffd);

		if (hFind == INVALID_HANDLE_VALUE)
		{
			//创建一个以追加形式（允许读写）文件 a+ ab???
			if (fp = fopen(file_path, "ab"))
			{
				flag = 0;

				//将各个分片的内容以追加的形式写入文件中
				fwrite(pl->filecont, strlen(pl->filecont), 1, fp);
			}
			fclose(fp);
		}

		do
		{//遍历 除FindFirstFile找到的第一个文件外的 目录下文件，检查是否同名
			if ((strcmp(ffd.cFileName, pl->filename)) == 0)
			{
				flag = 0;

				msg = "文件同名！\n";
				msg_long = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);
				send(sock, msg_long, ((PHDR)msg_long)->slice_file_len, NULL);

				//_tprintf(TEXT("\n文件同名！\n"));
			}

		} while (FindNextFile(hFind, &ffd) != 0);

		FindClose(hFind);

		//没有重名且没有创建时，创建
		if (flag)
		{
			//创建一个以追加形式（允许读写）文件 a+ wb?
			if (fp = fopen(file_path, "wb")) 
			{
				//将各个分片的内容以追加的形式写入文件中
				//第二个参数strlen(pl->filecont) ???
				fwrite(pl->filecont, pl->slice_file_len - 260 * 2 - 4 - 2, 1, fp);

				if (pl->slice_sum - 1 == pl->slice_num)
				{
					msg = "文件传输成功1！\n";
					msg_long = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);
					send(sock, msg_long, ((PHDR)msg_long)->slice_file_len, NULL);

					//_tprintf(TEXT("\n文件传输成功！\n"));
				}
			}

			fclose(fp);
		}
	}
	else
	{
		//非首片报文，a+ ab?? 以追加形式（允许读写）文件
		if (fp = fopen(file_path, "ab"))
		{
			//将各个分片的内容以追加的形式写入文件中
			fwrite(pl->filecont, strlen(pl->filecont) - 260 * 2 - 4 - 2, 1, fp);

			if (pl->slice_sum == pl->slice_num)
			{
				msg = "文件上传成功2！\n";
				msg_long = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);
				send(sock, msg_long, ((PHDR)msg_long)->slice_file_len, NULL);

				//_tprintf(TEXT("\n文件上传成功！\n"));
			}
			fclose(fp);
		}
	}
}

//要传到对面的文件分片
int File_download(PHDR pl, SOCKET sock)
{
	long temp; //文件分片的大小

	char *dir1 = pl->filedir;

	char *p = { 0 };//分片文件内容
	FILE *fp = NULL;

	int lenth1, lenth2, flag = 1;
	char *file_path;
	char *msg_long;
	char *msg;

	file_path = (char *)malloc(MAX_PATH);
	memset(file_path, 0, MAX_PATH);

	//将文件目录、名字以及类型拼接在一起作为要保存文件的目标路径
	strcpy_s(file_path, MAX_PATH, pl->filedir);
	strcat_s(file_path, MAX_PATH, pl->filename);	//strcat连接两个字符串

	lenth1 = strlen(pl->filedir);
	lenth2 = strlen(pl->filename);

	
	if (!(fp = fopen(file_path, "rb")))
	{
		printf("文件打开失败.\n");
		fclose(fp);
		return 0;
	}

	//定位到文件末尾
	fseek(fp, 0L, SEEK_END); 
	
	long file_len = 0;
	file_len = ftell(fp);   //返回指针偏离文件头的位置(即文件中字符个数)
	//printf("文件中字符个数：%d\n", file_len);

	
	//计算分片数
	int send_num = 0;
	if (file_len % Filecont_Max == 0)
	{
		//分片刚好分完
		send_num = file_len / Filecont_Max;
	}
	else
	{
		//无法整数分片，最后一片充不满但还是要有
		send_num = file_len / Filecont_Max + 1;
	}

	//定义最后一片的长度 = 文件长 - 分满的片长
	int last_len = file_len - (send_num - 1)*Filecont_Max;
	
	//定位到文件开头 feek()函数设置文件指针stream的位置
	fseek(fp, 0L, SEEK_SET);	

	//根据文件大小动态分配内存空间
	char *s_filecount = { 0 };//分片文件内容
	s_filecount = (char *)malloc(Filecont_Max);

	int offset = 0;	//文件总长度？？

	//传输第i片报文
	for (int i = 0;i < send_num;i++)
	{
		int currentLen = 0;	//当前片的长度
		memset(s_filecount, 0, Filecont_Max);

		if (i == send_num - 1)
		{
			currentLen = last_len;	//当前为最后一片
		}
		else
		{
			currentLen = Filecont_Max;	//其他片都是满的
		}

		//fread()读文件，内部指针会自动移动
		fread(s_filecount, 1, currentLen, fp);


		char *b = { NULL };//创建的报文流
		b = Create_struct(pl->mtu_type, i, send_num, pl->ID, NULL, NULL, s_filecount, currentLen);

		int iResult = send(sock, b, ((PHDR)b)->slice_file_len, NULL);	
		if (iResult != SOCKET_ERROR)
		{
			offset += currentLen;
		}
		else
		{
			flag = 0;
			printf("网络错误，文件传输失败\n");
		}

	}
	
	if (flag)
	{
		msg = "下载成功！\n";
		msg_long = Create_struct(pl->mtu_type, 0, 0, pl->ID, NULL, NULL, msg, strlen(msg) + 1);
		send(sock, msg_long, ((PHDR)msg_long)->slice_file_len, NULL);
	}

	return 1;

}

//主函数功能选择
bool mtutype_choice(unsigned char mtu_type, PHDR pl, SOCKET sockect_n)
{
	char ret[Filecont_Max];
	char *b;

	if (mtu_type == TypeUpload_file0 || mtu_type == TypeUpload_file1)
	{
		// 上传
		Write_file(pl, sockect_n);
		return true;
	}
	else if (mtu_type == TypeDownload_file0 || mtu_type == TypeDownload_file1)
	{
		//下载
		File_download(pl, sockect_n);
		return true;
	}
	else
	{
		//无效请求
		strcpy_s(ret, "该请求为无效请求！\0");
		b = Create_struct(TypeACK, 0, 0, pl->ID, NULL, NULL, ret, strlen(ret) + 1);
		send(sockect_n, b, ((PHDR)b)->slice_file_len, NULL);
		return false;
	}

}

