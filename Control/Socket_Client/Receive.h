
//1����ACK��ֻ�轫filecontent��ӡ��������
void Receive_Commen(PHDR pl, char * str, int ID)
{
	//char Content[1500] = { 0 };
	//int x, b;
	//pl->slice_file_len = *(short *)str;
	////pl->slice_file_len = (short)(((int)str[0]) * 256 + (int)str[1]);
	//pl->mtu_type = str[2];
	//pl->slice_num = str[3];
	//pl->slice_sum = str[4];
	//pl->ID = str[5];
	//
	//x = 8;

	//for (b = 0; str[x] != '\0'; x++)
	//{
	//	pl->filecont[b] = str[x];
	//	b++;
	//}

	pl = (PHDR)str;
	//pl->filecont[b] = '\0';
	printf("%s\n", pl->filecont);

}

//2���ļ����ء�ƴ���ļ������ݣ�д�������ڴ�
void Receive_Download(PHDR pl, int ID, char * NAMEM)
{
	FILE* fp;
	char*PATHH;
	int lenth1, lenth2, len;
	char *a;

	a = "F:\\ctl\\";

	lenth1 = strlen(a);
	lenth2 = strlen(NAMEM);
	
	PATHH = new char(lenth1 + lenth2 + 1);

	strcpy_s(PATHH, lenth1 + 1, a);
	strcat_s(PATHH, lenth1 + lenth2 + 1, NAMEM);

	len = pl->slice_file_len;

	if (fp = fopen(PATHH, "ab+")) //����һ����׷����ʽ�������д���ļ���
	{
		//��������Ƭ��������׷�ӵ���ʽд���ļ���
		fwrite(pl->filecont, len - MAX_PATH * 2 - 4 - 2, 1, fp);
	}

	fclose(fp);
}

