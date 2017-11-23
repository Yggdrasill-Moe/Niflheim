/*
用于解压解密scb文件
made by Darkness-TX
2017.11.23
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
#include "lzss.h"

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

struct Header
{
	unit8 magic[4];//GLPK
	unit32 uncomplen;
	unit32 version;
}Scb_header;

void Scb_WriteFile(char *fname)
{
	FILE *src = fopen(fname, "rb");
	unit8 dstname[260];
	unit32 filesize, i;
	fread(&Scb_header, 1, sizeof(Scb_header), src);
	if (strncmp(Scb_header.magic, "GLPK", 3) != 0)
	{
		printf("文件头不是GLPK!\n");
		fclose(src);
	}
	else
	{
		printf("name:%s uncomplen:0x%X version:%d\n", fname, Scb_header.uncomplen, Scb_header.version);
		fseek(src, 0, SEEK_END);
		filesize = ftell(src);
		fseek(src, 0xC, SEEK_SET);
		unit8 *srcdata = malloc(filesize - 0xC);
		fread(srcdata, 1, filesize - 0xC, src);
		fclose(src);
		for (i = 0; i < filesize - 0xC; i++)
			srcdata[i] = ~srcdata[i];
		unit8 *dstdata = malloc(Scb_header.uncomplen);
		Scb_header.uncomplen = lzss_decompress(dstdata, Scb_header.uncomplen, srcdata, filesize - 0xC);

		free(srcdata);
		sprintf(dstname, "%s.bin", fname);
		FILE *dst = fopen(dstname, "wb");
		fwrite(dstdata, 1, Scb_header.uncomplen, dst);
		free(dstdata);
		fclose(dst);
	}
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-GPK2\n用于解压解密scb文件。\n将scb文件拖到程序上。\nby Darkness-TX 2017.11.23\n\n");
	Scb_WriteFile(argv[1]);
	system("pause");
	return 0;
}