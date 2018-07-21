using   namespace   std;

#define DEFAULT_BUFLEN 1500//mtu����
//#define Maxf 260		//���к�MAX_PATH ��Ϊ260
#define Filecont_Max 974	
#define ExceptFile 6	//2 + 4 ��ȥfiledir��filename��filecont�Ķ���
#define SliceFileLen 65535

//mtu_type��8λ��2-4λ���ڱ�ʾ��ͬtype,��λ0��1�����������Ϊͬһtype
#define TypeACK 240				//240 = 1111 0000 ��ʾACK��
#define TypeUpload_file0 48		//0011 0000 ��λΪ0 1���������
#define TypeUpload_file1 176	//1011 0000
#define TypeDownload_file0 64	//0100 0000
#define TypeDownload_file1 192	//1100 0000


#define upload_file 4 //�ϴ��ļ�
#define download_file 5 //�����ļ�
#define ACK 7		//ACKЭ��

#define OK 1
#define ERROR 0

typedef struct _Header  //��󲻳���1500�ֽ�
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

int IDtt = 1;

//pΪ����ѡ��
char* choose_func(int func_choice, int mtu)
{

	char mtu_type1 = TypeACK;	

	//����ɺ�
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
		mtu_type1 = TypeACK;		//ACKЭ��
		break;
	default:
		printf("\n invalid choice");
	}
	return &mtu_type1;
}


//����������char *
char* create_struct(
	int func_choice,		//����ѡ��
	int mtu,		//�Ƿ�mtu
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

//�����ϴ��ļ������ļ���Ƭ�������͵����棩
int  Uploadfile(char *filedir, char *C_File_Dir, SOCKET sock)
{
	char *namezz;//�����ļ���
	long temp;//����filecont�ĳ���
	
	namezz = GetFilename(C_File_Dir);//���ݱ���·����ȡ�ļ����֣������ı���

	//���ļ�
	FILE *fp = NULL;//�����ļ�ָ��
	if (fopen_s(&fp, C_File_Dir, "rb") != NULL)
	{
		printf("�ļ���ʧ��.\n");
		return ERROR;
	}

	//��λ���ļ�ĩβ
	fseek(fp, 0L, SEEK_END);

	long file_len = 0;//�����ļ�����
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

	//��λ���ļ���ͷ
	fseek(fp, 0L, SEEK_SET);

	char *s_filecount = NULL;//��Ƭ�ļ�����
	s_filecount = (char *)malloc(Filecont_Max);

	int offset = 0;	//�ļ��ܳ��ȣ���

	//�����iƬ����
	for (int i = 0;i < send_num;i++)
	{
		int currentLen = 0;	//��ǰƬ��
		int mtu = 0;	//��ʼ��mtu flag

		memset(s_filecount, 0, Filecont_Max);

		if (i == send_num - 1)
		{
			mtu = 1;
			currentLen = last_len;	//��ǰΪ���һƬ
		}
		else
		{
			currentLen = Filecont_Max;	//��lastƬ������Ƭ��������
			mtu = 0;
		}

		//fread()���ļ����ڲ�ָ����Զ��ƶ�
		fread(s_filecount, 1, currentLen, fp);

		char *dir1 = filedir;//��ʼ��������·��
		char* a = { 0 };
		a = create_struct(upload_file, mtu, i, send_num, IDtt, dir1, namezz, s_filecount, currentLen);

		int iResult;
		iResult = send(sock, a, ((PHDR)a)->slice_file_len, NULL);
		
		if (iResult != SOCKET_ERROR)
			offset += currentLen;
		else 
		{
			printf("��������ļ�����ʧ��\n");
			return ERROR;
		}
	}

	fclose(fp);

	return OK;
	
}

//�����ṹ�� char*
char* Downloadfile(char *filedir, char *C_File_Dir)
{
	char* a = { 0 };
	
	a = create_struct(download_file, 0, 1, 1, IDtt, filedir, C_File_Dir, NULL, 0);

	IDtt = IDtt + 1;

	return a;

}

