/*
用于遍历RES2文件中的信息
made by Darkness-TX
2020.07.14
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

typedef struct fileinfo
{
	unit32 offset;
	char arc_name[MAX_PATH];
	unit32 arc_index;
	char arc_type[MAX_PATH];
	char arc_path[MAX_PATH];
	char name[MAX_PATH];
	char type[MAX_PATH];
	struct fileinfo *next;
}NodeFileInfo, *LinkFileInfo;
LinkFileInfo FileInfo = NULL;

unit32 ReadNumber(FILE *src, unit32 length_code)
{
	unit32 count = 0, i = 0, n = 0, rank = 0, b = 0;
	count = (length_code & 7) + 1;
	if (count > 4)
	{
		printf("count大于4！pos:0x%X count:0x%X\n", ftell(src), count);
		system("pause");
		exit(0);
	}
	for (i = 0; i < count; ++i)
	{
		fread(&b, 1, 1, src);
		n |= b << rank;
		rank += 8;
	}
	if (count <= 3)
	{
		unit32 sign = n & (1 << (8 * count - 1));
		if (sign != 0)
			n -= sign << 1;
	}
	return n;
}

unit32 ReadInteger(FILE *src)
{
	unit32 opcode = 0;
	fread(&opcode, 1, 1, src);
	if ((opcode & 0xE0) != 0)
	{
		if ((opcode & 0xF8) != 0x80)
		{
			printf("opcode不为0x80！pos:0x%X opcode:0x%X\n", ftell(src) - 1, opcode);
			system("pause");
			exit(0);
		}
		opcode = ReadNumber(src, opcode);
	}
	else
		opcode = (opcode & 0x0F) - (opcode & 0x10);
	return opcode;
}

void SkipObject(FILE *src)
{
	unit32 opcode = 0;
	fread(&opcode, 1, 1, src);
	if ((opcode & 0xE0) != 0)
		ReadNumber(src, opcode);
}

void ReadString(FILE *src, char *dstname)
{
	unit32 opcode = 0, offset = 4, name_length = 0, savepos = 0;
	char name_str[MAX_PATH];
	fread(&opcode, 1, 1, src);
	if ((opcode & 0xF8) != 0x90)
	{
		printf("opcode不等于0x90！pos:0x%X opcode:0x%X\n", ftell(src) - 1, opcode);
		system("pause");
		exit(0);
	}
	offset += ReadNumber(src, opcode);
	savepos = ftell(src);
	fseek(src, offset, SEEK_SET);
	fread(&name_length, 4, 1, src);
	fread(name_str, name_length, 1, src);
	name_str[name_length] = '\0';
	sprintf(dstname, "%s", name_str);
	fseek(src, savepos, SEEK_SET);
}

void ReadIndex(char *fname)
{
	unit32 opcode_off = 0, name_count = 0, param_count = 0, i = 0, j = 0;
	char param_name[MAX_PATH];
	FileInfo = malloc(sizeof(NodeFileInfo));
	FileInfo->next = NULL;
	LinkFileInfo p = FileInfo;
	FILE *src = fopen("SEC5/RES2", "rb");
	if (src == NULL)
	{
		printf("RES2打开错误，请检查SEC5文件夹下是否有RES2\n");
		system("pause");
		exit(0);
	}
	fread(&opcode_off, 4, 1, src);
	opcode_off += 4;
	fseek(src, opcode_off, SEEK_SET);
	fread(&name_count, 4, 1, src);
	for (i = 0; i < name_count; i++)
	{
		p->next = malloc(sizeof(NodeFileInfo));
		p->next->offset = ftell(src);
		p->next->arc_path[0] = '\0';
		ReadString(src, p->next->name);
		ReadString(src, p->next->type);
		ReadString(src, p->next->arc_type);
		param_count = ReadInteger(src);
		for (j = 0; j < param_count; j++)
		{
			ReadString(src, param_name);
			if (strncmp("path", param_name, 4) == 0)
				ReadString(src, p->next->arc_name);
			else if (strncmp("arc-index", param_name, 9) == 0)
				p->next->arc_index = ReadInteger(src);
			else if (strncmp("arc-path", param_name, 8) == 0)
				ReadString(src, p->next->arc_path);
			else
			{
				printf("发现未知的参数名：%s offset:0x%X\n", param_name, ftell(src));
				system("pause");
				SkipObject(src);
			}
		}
		//printf("arc_name:%s arc_index:%d arc_type:%s file_name:%s file_type:%s arc_path:%s\n", p->next->arc_name, p->next->arc_index, p->next->arc_type, p->next->name, p->next->type, p->next->arc_path);
		p = p->next;
		p->next = NULL;
		FileNum++;
	}
}

void DumpInfo()
{
	FILE *dst = NULL;
	unit32 i = 0;
	LinkFileInfo p = FileInfo;
	dst = fopen("RES2_info.txt", "wt");
	fputs("offset,arc_name,arc_index,arc_type,file_name,file_type,arc_path\n", dst);
	for (i = 0; i < FileNum; i++)
	{
		p = p->next;
		fprintf(dst, "0x%08X,%s,%d,%s,%s,%s,%s\n", p->offset, p->arc_name, p->arc_index, p->arc_type, p->name, p->type, p->arc_path);
	}
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-StudioSeldomAdventureSystem\n用于遍历RES2文件中的信息。\n双击运行程序。\nby Darkness-TX 2020.07.14\n\n");
	ReadIndex(argv[1]);
	DumpInfo();
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}