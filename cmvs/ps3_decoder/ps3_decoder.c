/*
用于解密并导出文件头为PS2A的ps3文件的文本
made by Darkness-TX
2018.04.20
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

struct ps3_header
{
	unit8 magic[4];//PS2A
	unit32 header_length;// 0x30
	unit32 unknown0;//各有不同，不明意义
	unit32 key;
	unit32 unknown1_count;// 每项4字节
	unit32 unknown2_length;//代码段
	unit32 unknown3_length;
	unit32 name_index_length;//文本块长度
	unit32 unknown4;// 0
	unit32 comprlen;
	unit32 uncomprlen;
	unit32 unknown5;// 0
	//下面为自制
	wchar_t FileName[260];//文件名
	unit32 FileSize;//文件大小
} PS3_Header[500];

//改了几个参数的lzss
void lzss_ps3_uncompress(unit8 *uncompr, unit32 uncomprlen, unit8 *compr, unit32 comprlen)
{
	unit32 curbyte = 0;
	unit32 flag = 0;
	unit32 cur_winpos = 0x7df;
	unit8 window[2048];
	memset(window, 0, cur_winpos);
	while (1)
	{
		flag >>= 1;
		if (!(flag & 0x0100))
		{
			if (curbyte >= comprlen)
				break;
			flag = compr[curbyte++] | 0xff00;
		}
		if (flag & 1)
		{
			unit8 data;
			if (curbyte >= comprlen)
				break;
			data = compr[curbyte++];
			window[cur_winpos++] = data;
			*uncompr++ = data;
			cur_winpos &= 0x7ff;
		}
		else
		{
			unit32 offset, count;
			if (curbyte >= comprlen)
				break;
			offset = compr[curbyte++];
			if (curbyte >= comprlen)
				break;
			count = compr[curbyte++];
			offset |= (count & 0xe0) << 3;
			count = (count & 0x1f) + 2;
			for (unit32 i = 0; i < count; ++i)
			{
				unit8 data;
				data = window[(offset + i) & 0x7ff];
				*uncompr++ = data;
				window[cur_winpos++] = data;
				cur_winpos &= 0x7ff;
			}
		}
	}
}

unit32 process_dir(char *dname)
{
	long Handle;
	struct _wfinddata64i32_t FileInfo;
	_chdir(dname);//跳转路径
	if ((Handle = _wfindfirst(L"*.ps3", &FileInfo)) == -1L)
	{
		printf("没有找到匹配的项目，请将后缀命名为.ps3\n");
		system("pause");
		return -1;
	}
	do
	{
		if (FileInfo.name[0] == L'.')  //过滤本级目录和父目录
			continue;
		wsprintfW(PS3_Header[FileNum].FileName , FileInfo.name);
		PS3_Header[FileNum].FileSize = FileInfo.size;
		FileNum++;
	} while (_wfindnext(Handle, &FileInfo) == 0);
	return FileNum;
}

BOOL ReadIndex(FILE *src, unit32 i)
{
	wprintf(L"name:%ls ", PS3_Header[i].FileName);
	fread(&PS3_Header[i], 0x30, 1, src);
	if (strncmp(PS3_Header[i].magic, "PS2A", 4))
	{
		printf("\n不支持的文件类型，文件头不是PS2A\n");
		system("pause");
		return FALSE;
	}
	if (PS3_Header[i].header_length != 0x30)
	{
		printf("\n不支持的文件类型，文件头长度不是0x30\n");
		system("pause");
		return FALSE;
	}
	wprintf(L"complen:0x%X decomplen:0x%X key:0x%X ", PS3_Header[i].comprlen, PS3_Header[i].uncomprlen, PS3_Header[i].key);
	return TRUE;
}

void FileDecode(char *fname)
{
	unit32 i = 0, k = 0;
	FILE *src, *dst;
	unit8 *cdata, *udata;
	wchar_t dstname[MAX_PATH];
	_chdir(fname);
	for (i = 0; i < FileNum; i++)
	{
		src = _wfopen(PS3_Header[i].FileName, L"rb");
		if (ReadIndex(src, i))
		{
			fseek(src, PS3_Header[i].header_length, SEEK_SET);
			cdata = malloc(PS3_Header[i].comprlen);
			fread(cdata, PS3_Header[i].comprlen, 1, src);
			unit8 xor = (unit8)((PS3_Header[i].key >> 24) + (PS3_Header[i].key >> 3));
			unit8 shifts = ((PS3_Header[i].key >> 20) % 5) + 1;
			for (k = 0; k < PS3_Header[i].comprlen; k++)
			{
				unit8 tmp = (cdata[k] - 0x7c) ^ xor;
				cdata[k] = (tmp >> shifts) | (tmp << (8 - shifts));
			}
			udata = malloc(PS3_Header[i].uncomprlen);
			lzss_ps3_uncompress(udata, PS3_Header[i].uncomprlen, cdata, PS3_Header[i].comprlen);
			free(cdata);
			wsprintfW(dstname, L"%ls.dec", PS3_Header[i].FileName);
			dst = _wfopen(dstname, L"wb");
			fwrite(&PS3_Header[i], PS3_Header[i].header_length, 1, dst);
			fwrite(udata, PS3_Header[i].uncomprlen, 1, dst);
			fclose(dst);
			unit32 str_offset = PS3_Header[i].unknown2_length + PS3_Header[i].unknown1_count * 4;
			wprintf(L"str_offset:0x%X str_length:0x%X\n", str_offset, PS3_Header[i].name_index_length);
			free(udata);
		}
		fclose(src);
	}
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-cmvs\n用于解密并导出文件头为PS2A的ps3文件的文本。\n将目录拖到程序上。\nby Darkness-TX 2018.04.20\n\n");
	process_dir(argv[1]);
	FileDecode(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}