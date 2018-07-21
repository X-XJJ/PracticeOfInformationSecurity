using   namespace   std;

#define DEFAULT_BUFLEN 1500//mtu长度
//#define Maxf 260		//已有宏MAX_PATH 即为260
#define Filecont_Max 974	
#define ExceptFile 6	//2 + 4 除去filedir、filename、filecont的定长
#define SliceFileLen 65535

//mtu_type的8位，2-4位用于表示不同type,首位0、1两种情况均视为同一type
#define TypeACK 240				//240 = 1111 0000 表示ACK类
#define TypeUpload_file0 48		//0011 0000 首位为0 1，两种情况
#define TypeUpload_file1 176	//1011 0000
#define TypeDownload_file0 64	//0100 0000
#define TypeDownload_file1 192	//1100 0000


#define upload_file 4 //上传文件
#define download_file 5 //下载文件
#define ACK 7		//ACK协议

#define OK 1
#define ERROR 0

typedef struct _Header  //最大不超过1500字节
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

int IDtt = 1;

//p为功能选择
char* choose_func(int func_choice, int mtu)
{

	char mtu_type1 = TypeACK;	

	//定义成宏
	switch (func_choice)
	{
	case upload_file:
		if (mtu == 0)
			mtu_type1 = TypeUpload_file0;		
		else
			mtu_type1 = TypeUpload_file1;	
		break;
	case download_file:
		if (mtu == 0)
			mtu_type1 = TypeDownload_file0;
		else
			mtu_type1 = TypeDownload_file1;
		break;
	case ACK:
		mtu_type1 = TypeACK;		//ACK协议
		break;
	default:
		printf("\n invalid choice");
	}
	return &mtu_type1;
}


//创建报文流char *
char* create_struct(
	int func_choice,		//功能选择
	int mtu,		//是否mtu
	char slice_num_,
	char slice_sum_,
	char ID,
	char *filedir,
	char *filename,
	char *filecont,
	int fileContLength)			

{
	PHDR phdr = (PHDR)malloc(sizeof(HDR));
	memset(phdr, 0, sizeof(HDR));

	phdr->mtu_type = *choose_func(func_choice, mtu);
	phdr->ID = ID;
	phdr->slice_num = slice_num_;
	phdr->slice_sum = slice_sum_;

	if (filedir != NULL)
		strcpy_s(phdr->filedir, MAX_PATH, filedir);

	if (filename != NULL)
		strcpy_s(phdr->filename, MAX_PATH, filename);

	if (filecont != NULL)
		memcpy(phdr->filecont, filecont, fileContLength);

	phdr->slice_file_len = fileContLength + MAX_PATH + MAX_PATH + ExceptFile;

	return (char*)phdr;

}

char* GetFilename(char* fullpathname)
{
	char* save_name, *pos;
	int name_len;
	name_len = strlen(fullpathname);
	pos = fullpathname + name_len;

	while (*pos != '\\' && pos != fullpathname)
		pos--;
	
	if (pos == fullpathname)
	{
		save_name = fullpathname + 1;
		return save_name;
	}
	
	name_len = name_len - (pos - fullpathname);
	save_name = (char*)malloc(name_len + 1);
	memcpy(save_name, pos + 1, name_len);
	
	return save_name;
}

//处理上传文件（将文件分片，并发送到对面）
int  Uploadfile(char *filedir, char *C_File_Dir, SOCKET sock)
{
	char *namezz;//定义文件名
	long temp;//定义filecont的长度
	
	namezz = GetFilename(C_File_Dir);//根据本地路径获取文件名字，即报文变量

	//打开文件
	FILE *fp = NULL;//定义文件指针
	if (fopen_s(&fp, C_File_Dir, "rb") != NULL)
	{
		printf("文件打开失败.\n");
		return ERROR;
	}

	//定位到文件末尾
	fseek(fp, 0L, SEEK_END);

	long file_len = 0;//定义文件长度
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

	//定位到文件开头
	fseek(fp, 0L, SEEK_SET);

	char *s_filecount = NULL;//分片文件内容
	s_filecount = (char *)malloc(Filecont_Max);

	int offset = 0;	//文件总长度？？

	//传输第i片报文
	for (int i = 0;i < send_num;i++)
	{
		int currentLen = 0;	//当前片长
		int mtu = 0;	//初始化mtu flag

		memset(s_filecount, 0, Filecont_Max);

		if (i == send_num - 1)
		{
			mtu = 1;
			currentLen = last_len;	//当前为最后一片
		}
		else
		{
			currentLen = Filecont_Max;	//除last片，其他片都是满的
			mtu = 0;
		}

		//fread()读文件，内部指针会自动移动
		fread(s_filecount, 1, currentLen, fp);

		char *dir1 = filedir;//初始化服务器路径
		char* a = { 0 };
		a = create_struct(upload_file, mtu, i, send_num, IDtt, dir1, namezz, s_filecount, currentLen);

		int iResult;
		iResult = send(sock, a, ((PHDR)a)->slice_file_len, NULL);
		
		if (iResult != SOCKET_ERROR)
			offset += currentLen;
		else 
		{
			printf("网络错误，文件传输失败\n");
			return ERROR;
		}
	}

	fclose(fp);

	return OK;
	
}

//创建结构体 char*
char* Downloadfile(char *filedir, char *C_File_Dir)
{
	char* a = { 0 };
	
	a = create_struct(download_file, 0, 1, 1, IDtt, filedir, C_File_Dir, NULL, 0);

	IDtt = IDtt + 1;

	return a;

}

