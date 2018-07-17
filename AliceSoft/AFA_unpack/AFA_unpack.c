/*
用于解包AliceSoft的AFA文件
made by Darkness-TX
2018.07.14
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <Windows.h>
#include <locale.h>
#include <zlib.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

unit32 FileNum = 0;//总文件数，初始计数为0

struct afa_header {
	unit8 sign[4];//AFAH
	unit32 header_length;
	unit8 sign2[8];//AlicArch
	unit32 version;//未知
	unit32 version2;//未知
	unit32 data_offset;
	unit8 sign3[4];//INFO
	unit32 index_complen;
	unit32 index_decomplen;
	unit32 filenum;
}AFA_Header;

struct index
{
	unit32 name_length;
	unit32 name_step;
	unit32 unk;
	unit32 unk2;
	unit32 unt3;
	unit32 offset;
	unit32 size;
	WCHAR name[MAX_PATH];
}Index[50000];

unit32 ReadIndex(FILE *src)
{
	unit8 *cdata = NULL, *udata = NULL, *p = NULL, srcname[MAX_PATH];
	unit32 i = 0;
	fread(&AFA_Header, sizeof(AFA_Header), 1, src);
	if (strncmp(AFA_Header.sign, "AFAH", 4) || strncmp(AFA_Header.sign2, "AlicArch", 8) || strncmp(AFA_Header.sign3, "INFO", 4))
	{
		printf("不支持的文件类型，请检查文件头有无AFAH和AliceArch和INFO\n");
		system("pause");
		exit(0);
	}
	if (AFA_Header.version != 1 || AFA_Header.version2 != 1)
	{
		printf("不支持的文件类型，请检查文件version是否为1\n");
		system("pause");
		exit(0);
	}
	printf("filenum:%d data_offset:0x%X index_complen:0x%X index_decomplen:0x%X\n", AFA_Header.filenum, AFA_Header.data_offset, AFA_Header.index_complen, AFA_Header.index_decomplen);
	cdata = malloc(AFA_Header.index_complen);
	udata = malloc(AFA_Header.index_decomplen);
	fread(cdata, AFA_Header.index_complen, 1, src);
	uncompress(udata, &AFA_Header.index_decomplen, cdata, AFA_Header.index_complen);
	free(cdata);
	for (i = 0, p = udata; i < AFA_Header.filenum; i++)
	{
		memcpy(&Index[i], p, 8);
		p += 8;
		memcpy(srcname, p, Index[i].name_step);
		MultiByteToWideChar(932, 0, srcname, Index[i].name_step, Index[i].name, Index[i].name_step);
		p += Index[i].name_step;
		memcpy(&Index[i].unk, p, 20);
		p += 20;
	}
	free(udata);
	p = NULL;
	return AFA_Header.filenum;
}

void UnpackFile(char *fname)
{
	FILE *src = NULL, *dst = NULL;
	unit32 i = 0;
	unit8 dstname[MAX_PATH], *data = NULL;
	src = fopen(fname,"rb");
	sprintf(dstname, "%s_unpack", fname);
	FileNum = ReadIndex(src);
	_mkdir(dstname);
	_chdir(dstname);
	for (i = 0; i < FileNum; i++)
	{
		wprintf(L"\tname:%ls offset:0x%X size:0x%X\n", Index[i].name, Index[i].offset, Index[i].size);
		data = malloc(Index[i].size);
		fseek(src, Index[i].offset + AFA_Header.data_offset, SEEK_SET);
		fread(data, Index[i].size, 1, src);
		dst = _wfopen(Index[i].name, L"wb");
		fwrite(data, Index[i].size, 1, dst);
		fclose(dst);
		free(data);
	}
	fclose(src);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-AliceSoft\n用于解包AliceSoft的AFA文件。\n将ALD文件拖到程序上。\nby Darkness-TX 2018.07.14\n\n");
	UnpackFile(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}