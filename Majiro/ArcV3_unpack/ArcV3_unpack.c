/*
用于解包MajiroArcV3文件
made by Darkness-TX
V1:2017.03.09
V2:2018.07.09
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
typedef unsigned __int64 unit64;

unit32 FileNum = 0;//总文件数，初始计数为0

struct header
{
	unit8 magic[16];//MajiroArcV3.000\0
	unit32 num;//文件数
	unit32 filename_off;//文件名列表地址
	unit32 data_off;//数据地址
}arc_header;

struct index
{
	unit32 hash;
	unit32 key;
	unit32 offset;
	unit32 size;
	unit8 name[64];
}file_index[50000];

struct index_v2
{
	unit64 hash;
	unit32 offset;
	unit32 size;
	unit8 name[64];
}file_index_v2[50000];

void ReadIndex(char *fname)
{
	FILE *fp;
	unit32 i,k;
	fp = fopen(fname, "rb");
	fread(arc_header.magic, 0x10, 1, fp);
	if (strncmp(arc_header.magic, "MajiroArcV3.000\0", 4) != 0)
	{
		printf("文件头不是MajiroArcV3.000\\0!\n");
		system("pause");
		exit(0);
	}
	fread(&arc_header.num, 4, 1, fp);
	fread(&arc_header.filename_off, 4, 1, fp);
	fread(&arc_header.data_off, 4, 1, fp);
	printf("%s filenum:%d filename_off:0x%X data_off:0x%X\n\n", fname, arc_header.num, arc_header.filename_off, arc_header.data_off);
	for (i = 0; i < arc_header.num; i++)
		fread(&file_index[i].hash, 16, 1, fp);
	fseek(fp, arc_header.filename_off, SEEK_SET);
	for (i = 0; i < arc_header.num; i++)
	{
		k = 0;
		while ((file_index[i].name[k] = fgetc(fp)) != 0)
			k++;
	}
	fclose(fp);
}

void ReadIndexV2(char *fname)
{
	FILE *fp;
	unit32 i, k;
	fp = fopen(fname, "rb");
	fread(arc_header.magic, 0x10, 1, fp);
	if (strncmp(arc_header.magic, "MajiroArcV3.000\0", 4) != 0)
	{
		printf("文件头不是MajiroArcV3.000\\0!\n");
		system("pause");
		exit(0);
	}
	fread(&arc_header.num, 4, 1, fp);
	fread(&arc_header.filename_off, 4, 1, fp);
	fread(&arc_header.data_off, 4, 1, fp);
	printf("%s filenum:%d filename_off:0x%X data_off:0x%X\n\n", fname, arc_header.num, arc_header.filename_off, arc_header.data_off);
	for (i = 0; i < arc_header.num; i++)
		fread(&file_index_v2[i].hash, 16, 1, fp);
	fseek(fp, arc_header.filename_off, SEEK_SET);
	for (i = 0; i < arc_header.num; i++)
	{
		k = 0;
		while ((file_index_v2[i].name[k] = fgetc(fp)) != 0)
			k++;
	}
	fclose(fp);
}

void UnpackFile(char *fname)
{
	FILE *src, *dst, *txt;
	unit32 i;
	unit8 dirname[200], *data;
	WCHAR dstname[200];
	sprintf(dirname, "%s_unpack", fname);
	src = fopen(fname, "rb");
	txt = fopen("hash_key.txt", "at+,ccs=UNICODE");
	_mkdir(dirname);
	_chdir(dirname);
	for (i = 0; i < arc_header.num; i++)
	{
		fseek(src, file_index[i].offset, SEEK_SET);
		data = malloc(file_index[i].size);
		fread(data, 1, file_index[i].size, src);
		MultiByteToWideChar(932, 0, file_index[i].name, -1, dstname, 64);
		fwprintf(txt, L"%ls|%08X|%08X\n", dstname, file_index[i].hash, file_index[i].key);
		dst = _wfopen(dstname, L"wb");
		fwrite(data, file_index[i].size, 1, dst);
		fclose(dst);
		free(data);
		wprintf(L"%ls offset:0x%X size:0x%X hash:0x%X key:0x%X\n", dstname, file_index[i].offset, file_index[i].size, file_index[i].hash, file_index[i].key);
		FileNum++;
	}
	fclose(txt);
	fclose(src);
}

void UnpackFileV2(char *fname)
{
	FILE *src, *dst;
	unit32 i;
	unit8 dirname[200], *data;
	WCHAR dstname[200];
	sprintf(dirname, "%s_unpack", fname);
	src = fopen(fname, "rb");
	_mkdir(dirname);
	_chdir(dirname);
	for (i = 0; i < arc_header.num; i++)
	{
		fseek(src, file_index_v2[i].offset, SEEK_SET);
		data = malloc(file_index_v2[i].size);
		fread(data, 1, file_index_v2[i].size, src);
		MultiByteToWideChar(932, 0, file_index_v2[i].name, -1, dstname, 64);
		dst = _wfopen(dstname, L"wb");
		fwrite(data, file_index_v2[i].size, 1, dst);
		fclose(dst);
		free(data);
		wprintf(L"%ls offset:0x%X size:0x%X hash:0x%llX\n", dstname, file_index_v2[i].offset, file_index_v2[i].size, file_index_v2[i].hash);
		FileNum++;
	}
	fclose(src);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-Majiro\n用于解包MajiroArcV3文件。\n将arc文件拖到程序上。\nby Darkness-TX 2018.07.09\n\n");
	//ReadIndex(argv[1]);
	ReadIndexV2(argv[1]);
	//UnpackFile(argv[1]);
	UnpackFileV2(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}