/*
用于解包BH的pac文件
made by Darkness-TX
2016.12.01
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
#include "BH_huffman_comp.h"
//#include "../pac_unpack/BH_huffman_dec.h"

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

unit32 FileNum = 0;//总文件数，初始计数为0

struct header
{
	unit8 magic[4];//PAC\0
	unit32 num;
	unit32 mode;//BH中是4
}pac_header;

struct index
{
	unit8 name[64];//文件名
	unit32 Offset;//文件偏移
	unit32 FileSize;//解压大小
	unit32 ComSize;//未解压大小
}Index[7000];
/*
void ReadIndex(char *fname)
{
	FILE *src, *dst;
	unit32 ComSize = 0, UncomSize = 0, i = 0;
	unit8 *cdata, *udata, dstname[200];
	src = fopen(fname, "rb");
	sprintf(dstname, "%s_INDEX", fname);
	fread(pac_header.magic, 4, 1, src);
	if (strncmp(pac_header.magic, "PAC\0", 4) != 0)
	{
		printf("文件头不是PAC\0!\n");
		fclose(src);
		system("pause");
		exit(0);
	}
	else
	{
		fread(&pac_header.num, 4, 1, src);
		fread(&pac_header.mode, 4, 1, src);
		printf("%s filenum:%d mode:%d\n\n", fname, pac_header.num, pac_header.mode);
		if (pac_header.mode != 4)
		{
			printf("不是模式4！\n");
			system("pause");
			exit(0);
		}
		else
		{
			fseek(src, -4, SEEK_END);
			fread(&ComSize, 4, 1, src);
			fseek(src, -4 - ComSize, SEEK_END);
			cdata = malloc(ComSize);
			fread(cdata, ComSize, 1, src);
			for (i = 0; i < ComSize; i++)
				cdata[i] = ~cdata[i];
			UncomSize = 76 * pac_header.num;
			udata = malloc(UncomSize);
			huffman_uncompress(udata, &UncomSize, cdata, ComSize);
			dst = fopen(dstname, "wb");
			fwrite(udata, UncomSize, 1, dst);
			free(cdata);
			fclose(dst);
			fclose(src);
			for (i = 0; i < pac_header.num; i++)
				memcpy(&Index[i], &udata[i * 76], 76);
			free(udata);
		}
	}
}

void packFileNoIndex(char *fname)
{
	FILE *src, *dst;
	unit8 *cdata, *udata, dirname[200];
	unit32 compsize, i;
	WCHAR srcname[200];
	sprintf(dirname, "%s_new", fname);
	dst = fopen(dirname, "wb");
	sprintf(dirname, "%s_unpack", fname);
	_chdir(dirname);
	fwrite(pac_header.magic, 1, 4, dst);
	fwrite(&pac_header.num, 1, 4, dst);
	fwrite(&pac_header.mode, 1, 4, dst);
	for (i = 0; i < pac_header.num; i++)
	{
		MultiByteToWideChar(932, 0, Index[i].name, -1, srcname, 64);
		src = _wfopen(srcname, L"rb");
		Index[i].ComSize = Index[i].FileSize * 4;
		cdata = malloc(Index[i].ComSize);
		udata = malloc(Index[i].FileSize);
		fread(udata, 1, Index[i].FileSize, src);
		compress2(cdata, &Index[i].ComSize, udata, Index[i].FileSize, Z_DEFAULT_COMPRESSION);
		Index[i].Offset = ftell(dst);
		fwrite(cdata, Index[i].ComSize, 1, dst);
		fclose(src);
		free(cdata);
		free(udata);
		FileNum += 1;
		wprintf(L"%ls offset:0x%X filesize:0x%X comsize:0x%X\n", srcname, Index[i].Offset, Index[i].FileSize, Index[i].ComSize);
	}
	udata = malloc(pac_header.num * 76);
	compsize = pac_header.num * 76 * 4;
	cdata = malloc(compsize);
	for (i = 0; i < pac_header.num; i++)
		memcpy(&udata[i * 76], &Index[i], 76);
	//huffman_compress(cdata, &compsize, udata, compsize / 4);
	//fwrite(cdata, 1, compsize, dst);
	//fwrite(&compsize, 1, 4, dst);
	//free(udata);
	//free(cdata);
	//fclose(dst);
	//printf("IndexSize:0x%X\n", compsize);
}
*/
int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-BALDR HEART\n用于封包BH的pac文件。\n将pac文件拖到程序上。\nby Darkness-TX 2016.12.02\n\n");
	//ReadIndex(argv[1]);
	//packFileNoIndex(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}