/*
用于解包FNL字库
made by Darkness-TX
2018.10.29
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <Windows.h>
#include <locale.h>
#include <png.h>
#include <zlib.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

unit32 FileNum = 0;//总文件数，初始计数为0

struct fna_header
{
	unit8 sign[4];//FNA
	unit32 version;
	unit32 file_size;
	unit32 info_size;
	unit32 group_count;//总块数
}FNA_Header;

struct fna_group
{
	unit32 num;//这个组中有多少块，每块就是下面这三个信息
	unit32 height;
	unit32 width;
	unit32 count;//索引数目
}FNA_Group;

//4色 2bpp
struct index
{
	unit16 h_revise;
	unit32 offset;
	unit32 size;
	unit32 width;//这个好像不是图片解压后的长，似乎是字的基线
	unit32 heigh;
}Index[100000];

unit32 ReadIndex(FILE *src)
{
	unit32 i = 0, j = 0, k = 0, count = 0;
	fread(&FNA_Header, sizeof(FNA_Header), 1, src);
	if (strncmp(FNA_Header.sign, "FNA\0", 4))
	{
		printf("不支持的文件类型，文件头不是FNA\\0\n");
		system("pause");
		exit(0);
	}
	if (FNA_Header.version != 0)
	{
		printf("不支持的文件类型，文件版本不是0\n");
		system("pause");
		exit(0);
	}
	printf("verson:%d file_size:0x%X info_size:0x%X group_count:%d\n", FNA_Header.version, FNA_Header.file_size, FNA_Header.info_size, FNA_Header.group_count);
	for (i = 0; i < FNA_Header.group_count; i++)
	{
		fread(&FNA_Group.num, 4, 1, src);
		for (j = 0; j < FNA_Group.num; j++)
		{
			fread(&FNA_Group.height, 0x0C, 1, src);
			for (k = 0; k < FNA_Group.count; k++, count++)
			{
				fread(&Index[count].h_revise, 2, 1, src);
				fread(&Index[count].offset, 4, 1, src);
				fread(&Index[count].size, 4, 1, src);
				Index[count].width = FNA_Group.width;
				Index[count].heigh = FNA_Group.height;
			}
		}
	}
	return count;
}

void UnpackFile(char *fname)
{
	FILE *src = NULL, *dst = NULL;
	unit32 i = 0, uncomplen = 0;
	unit8 dstname[MAX_PATH], *cdata = NULL, *udata = NULL;
	src = fopen(fname, "rb");
	FileNum = ReadIndex(src);
	sprintf(dstname, "%s_unpack", fname);
	_mkdir(dstname);
	_chdir(dstname);
	for (i = 0; i < FileNum; i++)
	{
		printf("offset:0x%X size:0x%X h_revise:0x%X width:%d height:%d num:%08d\n", Index[i].offset, Index[i].size, Index[i].h_revise, Index[i].width, Index[i].heigh, i);
		cdata = malloc(Index[i].size);
		fseek(src, Index[i].offset, SEEK_SET);
		fread(cdata, Index[i].size, 1, src);
		uncomplen = Index[i].size * 20;
		udata = malloc(uncomplen);
		uncompress(udata, &uncomplen, cdata, Index[i].size);
		free(cdata);
		sprintf(dstname, "%08d.png", i);
		dst = fopen(dstname, "wb");
		Index[i].width = uncomplen / Index[i].heigh;
		fwrite(udata, uncomplen, 1, dst);
		free(udata);
		fclose(dst);
	}
	fclose(src);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-AliceSoft\n用于解包AliceSoft的FNL字库文件。\n将FNL文件拖到程序上。\nby Darkness-TX 2018.10.29\n\n");
	UnpackFile(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}