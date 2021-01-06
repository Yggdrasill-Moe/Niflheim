/*
用于封包NeXAS引擎的pac文件
made by Darkness-TX
2016.12.01

添加新版NeXAS支持
upload by AyamiKaze
2020.03.18
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <Windows.h>
#include <zstd.h>
#include <zlib.h>
#include <locale.h>
#include "Huffman_comp.h"

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
	WCHAR name[64];//文件名
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

unit32 process_dir(char *dname)
{
	long Handle;
	unit32 i = 0;
	struct _wfinddata64i32_t FileInfo;
	_chdir(dname);//跳转路径
	if ((Handle = _wfindfirst(L"*.*", &FileInfo)) == -1L)
	{
		printf("没有找到匹配的项目\n");
		system("pause");
		return -1;
	}
	do
	{
		if (FileInfo.name[0] == '.')  //过滤本级目录和父目录
			continue;
		swprintf(Index[FileNum].name, 260, FileInfo.name);
		Index[FileNum].FileSize = FileInfo.size;
		Index[FileNum].ComSize = Index[FileNum].FileSize * 4;
		FileNum++;
	} while (_wfindnext(Handle, &FileInfo) == 0);
	return FileNum;
}

void PackFile(char *fname)
{
	FILE *src, *dst;
	unit32 i, compsize;
	unit8 *udata, *cdata, dstname[200];
	wchar_t bname[5];
	sprintf(pac_header.magic, "PAC\0");
	pac_header.num = FileNum;
	pac_header.mode = 4;
	pac_header.mode &= 0xFF;
	sprintf(dstname, "%s.pac", fname);
	_chdir("..");
	dst = fopen(dstname, "wb");
	fwrite(&pac_header, 1, sizeof(pac_header), dst);
	_chdir(fname);
	for (i = 0; i < pac_header.num; i++)
	{
		src = _wfopen(Index[i].name, L"rb");
		wcscpy(bname, wcschr(Index[i].name, L'.') + 1);
		udata = malloc(Index[i].FileSize);
		cdata = malloc(Index[i].ComSize);
		Index[i].Offset = ftell(dst);
		fread(udata, 1, Index[i].FileSize, src);
		//mode >= 4时，已知fnt、png文件不进行压缩，如果压缩了会无法读取
		if (wcscmp(bname, L"fnt") == 0 || wcscmp(bname, L"png") == 0)
		{
			fwrite(udata, 1, Index[i].FileSize, dst);
			Index[i].ComSize = Index[i].FileSize;
		}
		else
		{
			compress2(cdata, &Index[i].ComSize, udata, Index[i].FileSize, Z_DEFAULT_COMPRESSION);
			fwrite(cdata, 1, Index[i].ComSize, dst);
		}
		free(cdata);
		free(udata);
		fclose(src);
		wprintf(L"%ls offset:0x%X filesize:0x%X comsize:0x%X\n", Index[i].name, Index[i].Offset, Index[i].FileSize, Index[i].ComSize);
	}
	udata = malloc(76 * pac_header.num);
	compsize = 76 * pac_header.num * 4;
	cdata = malloc(compsize);
	memset(udata, 0, 76 * pac_header.num);
	for (i = 0; i < pac_header.num; i++)
	{
		WideCharToMultiByte(932, 0, Index[i].name, 64, dstname, 64, NULL, FALSE);
		memcpy(&udata[i * 76], dstname, 64);
		memcpy(&udata[i * 76 + 64], &Index[i].Offset, 4);
		memcpy(&udata[i * 76 + 68], &Index[i].FileSize, 4);
		memcpy(&udata[i * 76 + 72], &Index[i].ComSize, 4);
	}
	huffman_compress(cdata, &compsize, udata, 76 * pac_header.num);
	for (i = 0; i < compsize; i++)
		cdata[i] = ~cdata[i];
	fwrite(cdata, 1, compsize, dst);
	fwrite(&compsize, 1, 4, dst);
	free(cdata);
	free(udata);
	fclose(dst);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-NeXAS\n用于封包BH的pac文件。\n将文件夹拖到程序上。\nby Darkness-TX 2016.12.02\n\n添加新版NeXAS封包支持\nby AyamiKaze 2020.03.18\n\n");
	//ReadIndex(argv[1]);
	//packFileNoIndex(argv[1]);
	process_dir(argv[1]);
	PackFile(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}