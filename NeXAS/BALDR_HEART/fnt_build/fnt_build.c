/*
用于将png合成fnt
made by Darkness-TX
2016.12.20
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <Windows.h>
#include <zlib.h>
#include <locale.h>
#include <png.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

struct header
{
	unit8 magic[4];//PAC\0
	unit8 magic2[9];//DATA VER-
	unit16 flag;//为1
	unit16 fontflag;//0101没字体名，字模0xC一组；0301有字体名，字模0x10一组
	unit32 seekflag;//0xFFFF00右移9字节
	unit32 height;
	unit32 width;
	unit32 compsize;
	unit32 decompsize;
}fnt_header;

struct Font_Info
{
	unit32 unk;
	unit16 width;
	unit16 height;
	unit32 offset;
}font_info[10000];

unit32 font_count = 0;

unit32 ReadIndex(char *fname)
{
	FILE *src;
	unit8 *cdata, *udata, dstname[200];
	unit32 i, savepos;
	src = fopen(fname, "rb");
	fread(fnt_header.magic, 1, 4, src);
	if (strncmp(fnt_header.magic, "FNT\0", 4) != 0)
	{
		printf("文件头不是FNT\0!");
		exit(0);
	}
	if (strncmp(fname, "systemascii", 11) != 0)
	{
		fread(fnt_header.magic2, 1, 9, src);
		fread(&fnt_header.flag, 1, 2, src);
		fread(&fnt_header.fontflag, 1, 2, src);
		if (strncmp(fnt_header.magic2, "DATA VER-", 9) != 0)
		{
			printf("文件头无DATA VER-!");
			exit(0);
		}
		else if (fnt_header.flag != 1)
		{
			printf("flag不为1!");
			exit(0);
		}
		if (fnt_header.fontflag == 0x103)
			while (fgetc(src) != '\0');
	}
	fread(&fnt_header.width, 1, 4, src);
	fread(&fnt_header.height, 1, 4, src);
	savepos = ftell(src);
	fread(&fnt_header.decompsize, 1, 4, src);
	if (fnt_header.decompsize == 0xFFFF00 || fnt_header.decompsize == 0xFFFF01)
	{
		fnt_header.seekflag = fnt_header.decompsize;
		fseek(src, 9, SEEK_CUR);
		fread(&fnt_header.decompsize, 1, 4, src);
	}
	fread(&fnt_header.compsize, 1, 4, src);
	cdata = malloc(fnt_header.compsize);
	udata = malloc(fnt_header.decompsize);
	fread(cdata, 1, fnt_header.compsize, src);
	uncompress(udata, &fnt_header.decompsize, cdata, fnt_header.compsize);
	free(cdata);
	if (fnt_header.fontflag == 0x103)
	{
		font_count = fnt_header.decompsize / 0x10;
		for (i = 0; i < font_count; i++)
		{
			memcpy(&font_info[i].offset, &udata[i * 0x10 + 0xC], 4);
			memcpy(&font_info[i].width, &udata[i * 0x10 + 0x4], 2);
			memcpy(&font_info[i].height, &udata[i * 0x10 + 0x6], 2);
			memcpy(&font_info[i].unk, &udata[i * 0x10], 4);
		}
	}
	else
	{
		font_count = fnt_header.decompsize / 0xC;
		for (i = 0; i < font_count; i++)
			memcpy(&font_info[i], &udata[i * 0xC], 0xC);
	}
	printf("font_count:%d\n", font_count);
	system("pause");
	return savepos;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-BALDR HEART\n用于将png合成fnt。\n将fnt文件拖到程序上。\nby Darkness-TX 2016.12.20\n\n");
	WriteFntFile(argv[1]);
	system("pause");
	return 0;
}