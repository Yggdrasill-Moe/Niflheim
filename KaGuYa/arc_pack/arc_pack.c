/*
用于封包KaGuYa社游戏的arc文件
made by Darkness-TX
2022.09.11
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

struct ari_header
{
	unit32 namelen;
	unit8 name[MAX_PATH];
	unit16 type;
	unit32 size;
	unit32 desize;//只在arc中type为1时使用
} Header, Ari_Header[30000];

struct arc_header
{
	unit8 magic[4];
}Arc_Header;

unit32 FileNum = 0;//总文件数，初始计数为0

void ReadIndex(char* fname)
{
	FILE* ari = NULL;
	unit8 ariname[MAX_PATH];
	unit32 i = 0, j = 0, filesize = 0;
	strcpy(ariname, fname);
	ariname[strlen(ariname) - 1] = 'i';//ari
	if (_access(ariname, 4) == -1)
	{
		printf("对应ari文件不存在！\n");
		system("pause");
		exit(0);
	}
	ari = fopen(ariname, "rb");
	fseek(ari, 0, SEEK_END);
	filesize = ftell(ari);
	fseek(ari, 0, SEEK_SET);
	while (ftell(ari) < filesize)
	{
		fread(&Ari_Header[i].namelen, 4, 1, ari);
		fread(Ari_Header[i].name, Ari_Header[i].namelen, 1, ari);
		Ari_Header[i].name[Ari_Header[i].namelen] = '\0';
		for (j = 0; j < Ari_Header[i].namelen; j++)
			Ari_Header[i].name[j] = ~Ari_Header[i].name[j];
		fread(&Ari_Header[i].type, 2, 1, ari);
		fread(&Ari_Header[i].size, 4, 1, ari);
		i++;
	}
	FileNum = i;
}

void Pack(char* fname)
{
	FILE* src = NULL, * dst = NULL, * base = NULL, * ari = NULL;
	unit32 i = 0, filesize = 0, basesize = 0;
	unit8* data = NULL, dirname[MAX_PATH];
	WCHAR dstname[MAX_PATH];
	src = fopen(fname, "rb");
	fseek(src, 0, SEEK_END);
	filesize = ftell(src);
	fseek(src, 0, SEEK_SET);
	fread(&Arc_Header.magic, 4, 1, src);
	if (strncmp(Arc_Header.magic, "WFL1", 4) != 0)
	{
		printf("文件头不是WFL1！\n");
		system("pause");
		exit(0);
	}
	sprintf(dirname, "%s.new", fname);
	dst = fopen(dirname, "wb");
	strcpy(dirname, fname);
	dirname[strlen(dirname) - 1] = 'i';//ari
	sprintf(dirname, "%s.new", dirname);
	ari = fopen(dirname, "wb");
	fwrite(Arc_Header.magic, 4, 1, dst);
	sprintf(dirname, "%s_unpack", fname);
	_chdir(dirname);
	while (ftell(src) < filesize)
	{
		fread(&Header.namelen, 4, 1, src);
		fread(Header.name, Header.namelen, 1, src);
		fread(&Header.type, 2, 1, src);
		fread(&Header.size, 4, 1, src);
		if (Header.namelen != Ari_Header[i].namelen || Header.type != Ari_Header[i].type || Header.size != Ari_Header[i].size)
		{
			printf("arc文件中的索引与ari文件中的索引不一致！num:%d\n", i);
			printf("arc_header: namelen:%d type:%d size:0x%X\n", Header.namelen, Header.type, Header.size);
			printf("ari_header: namelen:%d type:%d size:0x%X\n", Ari_Header[i].namelen, Ari_Header[i].type, Ari_Header[i].size);
			system("pause");
			exit(0);
		}
		MultiByteToWideChar(932, 0, Ari_Header[i].name, Ari_Header[i].namelen + 1, dstname, Ari_Header[i].namelen + 1);
		if (Header.type == 1)
			fseek(src, 4, SEEK_CUR);
		fseek(src, Header.size, SEEK_CUR);
		base = _wfopen(dstname + 1, L"rb");
		fseek(base, 0, SEEK_END);
		basesize = ftell(base);
		fseek(base, 0, SEEK_SET);
		data = malloc(basesize);
		fread(data, basesize, 1, base);
		fclose(base);
		Header.type = 0;
		Header.size = basesize;
		wprintf(L"%ls type:%d size:0x%X\n", dstname, Header.type, Header.size);
		//写ari
		fwrite(&Header.namelen, 4, 1, ari);
		fwrite(Header.name, Header.namelen, 1, ari);
		fwrite(&Header.type, 2, 1, ari);
		fwrite(&Header.size, 4, 1, ari);
		//写arc
		fwrite(&Header.namelen, 4, 1, dst);
		fwrite(Header.name, Header.namelen, 1, dst);
		fwrite(&Header.type, 2, 1, dst);
		fwrite(&Header.size, 4, 1, dst);
		fwrite(data, Header.size, 1, dst);
		free(data);
		i++;
	}
	fclose(dst);
	fclose(src);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-KaGuYa\n用于封包KaGuYa社游戏的arc文件。\n将arc文件拖到程序上。\nby Darkness-TX 2022.09.11\n\n");
	ReadIndex(argv[1]);
	Pack(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}