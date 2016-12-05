#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <Windows.h>
#include <zlib.h>
#include <locale.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

struct header
{
	unit8 magic[4];//PAC\0
	unit8 magic2[9];//DATA VER-
	unit16 flag;//为1
	unit16 fontflag;//0101没字体名,0301有字体名
	unit32 height;
	unit32 width;
	unit32 compsize;
	unit32 decompsize;
}fnt_header;

void ReadIndex(char *fname)
{
	FILE *src, *dst;
	unit8 *cdata, *udata, dstname[200];
	sprintf(dstname, "%s_INDEX", fname);
	src = fopen(fname, "rb");
	fread(fnt_header.magic, 1, 4, src);
	fread(fnt_header.magic2, 1, 9, src);
	fread(&fnt_header.flag, 1, 2, src);
	fread(&fnt_header.fontflag, 1, 2, src);
	if (strncmp(fnt_header.magic, "FNT\0", 4) != 0)
	{
		printf("文件头不是FNT\0!");
		exit(0);
	}
	else if (strncmp(fnt_header.magic2, "DATA VER-", 9) != 0)
	{
		printf("文件头无DATA VER-!");
		exit(0);
	}
	else if (fnt_header.flag != 1)
	{
		printf("flag不为1!");
		exit(0);
	}
	if (fnt_header.fontflag == 0x101)
	{
		fread(&fnt_header.width, 1, 4, src);
		fread(&fnt_header.height, 1, 4, src);
		fread(&fnt_header.decompsize, 1, 4, src);
		fread(&fnt_header.compsize, 1, 4, src);
		printf("width:%d height%d decompsize:0x%X compsize:0x%X\n", fnt_header.width, fnt_header.height, fnt_header.decompsize, fnt_header.compsize);
		cdata = malloc(fnt_header.compsize);
		udata = malloc(fnt_header.decompsize);
		fread(cdata, 1, fnt_header.compsize, src);
		uncompress(udata, &fnt_header.decompsize, cdata, fnt_header.compsize);
		dst = fopen(dstname, "wb");
		fwrite(udata, 1, fnt_header.decompsize, dst);
		free(udata);
		free(cdata);
		fclose(dst);
	}
	fclose(src);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-BALDR HEART\n用于解压BH的fnt字体文件索引。\n将fnt文件拖到程序上。\nby Darkness-TX 2016.12.05\n\n");
	ReadIndex(argv[1]);
	system("pause");
	return 0;
}