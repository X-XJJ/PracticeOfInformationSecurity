
//1、纯ACK，只需将filecontent打印出来即可
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

//2、文件下载。拼接文件内内容，写入计算机内存
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

	if (fp = fopen(PATHH, "ab+")) //创建一个以追加形式（允许读写）文件。
	{
		//将各个分片的内容以追加的形式写入文件中
		fwrite(pl->filecont, len - MAX_PATH * 2 - 4 - 2, 1, fp);
	}

	fclose(fp);
}

