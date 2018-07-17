/*
用于解包AliceSoft的ALD文件
made by Darkness-TX
2018.03.12
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <Windows.h>
#include <locale.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

unit32 FileNum = 0;//总文件数，初始计数为0

struct ALD_Header
{
	unit8 magic[3];//NL
	unit8 unk1[6];
	unit32 file_num;
	unit8 unk2[3];
}ald_header;

struct ALD_Index
{
	unit32 offset;
	unit32 header_size;
	unit32 filesize;
	unit32 unk1;
	unit32 unk2;
	unit8 filename[32];
}ald_index[50000];

unit32 ReadIndex(FILE *src)
{
	unit32 i = 0, offset_size = 0;
	fseek(src, -0x10, SEEK_END);
	fread(&ald_header.magic, 3, 1, src);
	fread(&ald_header.unk1, 6, 1, src);
	fread(&ald_header.file_num, 4, 1, src);
	fread(&ald_header.unk2, 3, 1, src);
	if (strncmp(ald_header.magic, "NL\x01", 3))
	{
		printf("不支持的文件类型，请检查文件尾有无NL\\x01\n");
		system("pause");
		exit(0);
	}
	fseek(src, 0, SEEK_SET);
	fread(&offset_size, 3, 1, src);
	offset_size <<= 8;
	printf("filenum:%d offset_size:0x%X\n", ald_header.file_num, offset_size);
	for (i = 0; i < ald_header.file_num; i++)
	{
		fread(&ald_index[i].offset, 3, 1, src);
		ald_index[i].offset <<= 8;
	}
	for (i = 0; i < ald_header.file_num; i++)
	{
		fseek(src, ald_index[i].offset, SEEK_SET);
		fread(&ald_index[i].header_size, 4, 1, src);
		fread(&ald_index[i].filesize, 4, 1, src);
		fread(&ald_index[i].unk1, 4, 1, src);
		fread(&ald_index[i].unk2, 4, 1, src);
		fread(&ald_index[i].filename, ald_index[i].header_size - 0x10, 1, src);
	}
	return ald_header.file_num;
}

void UnpackFile(char *fname)
{
	FILE *src, *dst;
	unit8 dstname[MAX_PATH], *data;
	unit32 i = 0;
	wchar_t namebuff[MAX_PATH];
	src = fopen(fname, "rb");
	FileNum = ReadIndex(src);
	sprintf(dstname, "%s_unpack", fname);
	_mkdir(dstname);
	_chdir(dstname);
	for (i = 0; i < FileNum; i++)
	{
		printf("\tfilename:%s offset:0x%X headersize:0x%X size:0x%X\n", ald_index[i].filename, ald_index[i].offset, ald_index[i].header_size, ald_index[i].filesize);
		data = malloc(ald_index[i].filesize);
		fseek(src, ald_index[i].offset + ald_index[i].header_size, SEEK_SET);
		fread(data, ald_index[i].filesize, 1, src);
		MultiByteToWideChar(932, 0, ald_index[i].filename, 32, namebuff, 32);
		dst = _wfopen(namebuff, L"wb");
		fwrite(data, ald_index[i].filesize, 1, dst);
		fclose(dst);
		free(data);
	}
	fclose(src);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-AliceSoft\n用于解包AliceSoft的ALD文件。\n将ALD文件拖到程序上。\nby Darkness-TX 2018.03.12\n\n");
	UnpackFile(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}