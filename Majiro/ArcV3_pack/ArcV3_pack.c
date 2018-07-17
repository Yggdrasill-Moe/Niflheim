/*
用于封包MajiroArcV3文件
made by Darkness-TX
V1:2018.05.10
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
#define __PAIR__(high, low) (((unsigned __int64)(high) << sizeof(high) * 8) | low)

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
	WCHAR name[64];
}file_index[50000];

struct index_v2
{
	unit64 hash;
	unit32 offset;
	unit32 size;
	WCHAR name[64];
}file_index_v2[50000];

int comp(const void*a, const void*b)
{
	return *(unit32*)a - *(unit32*)b;
}

int compV2(const void*a, const void*b)
{
	return *(unit64*)a > *(unit64*)b ? 1 : -1;
}

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
		if (FileInfo.name[0] == '.')//过滤本级目录和父目录
			continue;
		swprintf(file_index[FileNum].name, 260, FileInfo.name);
		file_index[FileNum].size = FileInfo.size;
		file_index[FileNum].hash = 0;
		file_index[FileNum].key = 0;
		FileNum++;
	} while (_wfindnext(Handle, &FileInfo) == 0);
	_chdir("..");
	return FileNum;
}

unit32 process_dirV2(char *dname)
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
		if (FileInfo.name[0] == '.')//过滤本级目录和父目录
			continue;
		swprintf(file_index_v2[FileNum].name, 260, FileInfo.name);
		file_index_v2[FileNum].size = FileInfo.size;
		FileNum++;
	} while (_wfindnext(Handle, &FileInfo) == 0);
	_chdir("..");
	return FileNum;
}

void BuildHash_Key()
{
	FILE *txt = fopen("hash_key.txt", "rt,ccs=UNICODE");
	WCHAR buff[300], value[300];
	int pos = 0;
	unit32 i = 0;
	unit8 dstname[MAX_PATH];
	strcpy(arc_header.magic, "MajiroArcV3.000\0");
	while (fgetws(buff, 300, txt) != NULL)
	{
		pos = wcschr(buff, L'|') - buff;
		wcsncpy(value, buff, pos);
		value[pos] = L'\0';
		for (i = 0; i < FileNum; i++)
		{
			if (file_index[i].key == 0)
			{
				if (wcsncmp(file_index[i].name, value, pos) == 0)
				{
					if (wcschr(&buff[pos + 1], L'|') - buff < 0)
					{
						wprintf(L"此行有错：%ls", buff);
						system("pause");
						exit(0);
					}
					wcsncpy(value, &buff[pos + 1], wcschr(&buff[pos + 1], L'|') - &buff[pos]);
					value[wcschr(&buff[pos + 1], L'|') - &buff[pos] - 1] = L'\0';
					file_index[i].hash = wcstoul(value, NULL, 16);
					pos = wcschr(&buff[pos + 1], L'|') - buff;
					wcsncpy(value, &buff[pos + 1], wcslen(buff) - pos);
					value[wcslen(buff) - pos] = L'\0';
					file_index[i].key = wcstoul(value, NULL, 16);
					break;
				}
			}
		}
	}
	arc_header.filename_off = 0x1C + FileNum * 0x10;
	arc_header.data_off = arc_header.filename_off;
	for (i = 0; i < FileNum; i++)
	{
		if (file_index[i].key == 0 || file_index[i].hash == 0)
		{
			wprintf(L"%ls未读取到key或hash，请检查hash_key.txt中是否有对应数据\n", file_index[i].name);
			system("pause");
			exit(0);
		}
		WideCharToMultiByte(932, 0, file_index[i].name, 64, dstname, MAX_PATH, NULL, FALSE);
		arc_header.data_off += strlen(dstname) + 1;
	}
	fclose(txt);
}

/*
取自游戏：あの晴れわたる空より高く
int __cdecl sub_460CF0(const char *a1)
{
	signed int v1; // ebx
	int v2; // ebp
	int *v3; // edi
	unsigned __int64 v4; // rax
	signed int i; // esi
	int v6; // edi
	unsigned __int64 v7; // rax
	signed int v9; // [esp+Ch] [ebp-10h]
	signed int v10; // [esp+10h] [ebp-Ch]
	int v11; // [esp+18h] [ebp-4h]

	v1 = -1;
	v11 = -1;
	v10 = strlen(a1);
	if (!dword_5C2FFC)
	{
		v2 = 0;
		v3 = dword_5C27F8;
		do
		{
			v9 = 8;
			v4 = v2;
			do
			{
				if (v4 & 1)
				{
					LODWORD(v4) = v4 ^ 0x53D46D27;
					HIDWORD(v4) ^= 0x85E1C3D7;
				}
				v4 >>= 1;
				--v9;
			} while (v9);
			*(_QWORD *)v3 = v4;
			v3 += 2;
			++v2;
		} while ((signed int)v3 < (signed int)&dword_5C2FF8);
		dword_5C2FFC = 1;
	}
	for (i = 0; i < v10; v11 = HIDWORD(v7) ^ dword_5C27FC[2 * v6])
	{
		v6 = (unsigned __int8)v1 ^ (unsigned __int8)a1[i];
		v7 = __PAIR__((unsigned int)v11, v1) >> 8;
		v1 = (__PAIR__((unsigned int)v11, v1) >> 8) ^ dword_5C27F8[2 * v6];
		++i;
	}
	return ~v1;
}
*/

unit64 Hash_Build(WCHAR *filename)
{
	unit64 hash_table[256], flag = 0;
	unit32 i = 0, j = 0, strsize = 0;
	int hash_index = 0;
	unit8 dstname[MAX_PATH];
	WideCharToMultiByte(932, 0, filename, 64, dstname, MAX_PATH, NULL, FALSE);
	strsize = strlen(dstname);
	for (i = 0; i < 256; i++)
	{
		flag = i;
		for (j = 0; j < 8; j++)
		{
			if (flag & 1)
				flag ^= 0x85E1C3D753D46D27;
			flag = flag >> 1;
		}
		hash_table[i] = flag;
	}
	unit32 v1 = -1, v11 = -1;
	for (i = 0; i < strsize; i++)
	{
		hash_index = (int)((unit8)v1 ^ (unit8)dstname[i]);
		flag = ((__PAIR__(v11, v1)) >> 8);
		v1 = (unit32)(flag ^ hash_table[hash_index]);
		v11 = (flag ^ hash_table[hash_index]) >> 32;
	}
	flag = ~(__PAIR__(v11, v1));
	return flag;
}

void BuildHashV2()
{
	unit32 i = 0;
	unit8 dstname[MAX_PATH];
	strcpy(arc_header.magic, "MajiroArcV3.000\0");
	arc_header.filename_off = 0x1C + FileNum * 0x10;
	arc_header.data_off = arc_header.filename_off;
	for (i = 0; i < FileNum; i++)
	{
		file_index_v2[i].hash = Hash_Build(file_index_v2[i].name);
		WideCharToMultiByte(932, 0, file_index_v2[i].name, 64, dstname, MAX_PATH, NULL, FALSE);
		arc_header.data_off += strlen(dstname) + 1;
	}
}

void PackFile(char *fname)
{
	FILE *src, *dst;
	unit32 i = 0, savepos = 0, k = 0;
	unit8 dstname[MAX_PATH], *data;
	unit32 *key_sort = malloc(FileNum * 4);
	sprintf(dstname, "%s.arc", fname);
	dst = fopen(dstname, "wb");
	_chdir(fname);
	fseek(dst, arc_header.data_off, SEEK_SET);
	for (i = 0; i < FileNum; i++)
	{
		key_sort[i] = file_index[i].key;
		file_index[i].offset = ftell(dst);
		src = _wfopen(file_index[i].name, L"rb");
		data = malloc(file_index[i].size);
		fread(data, file_index[i].size, 1, src);
		fwrite(data, file_index[i].size, 1, dst);
		fclose(src);
		free(data);
	}
	fseek(dst, 0, SEEK_SET);
	fwrite(arc_header.magic, 16, 1, dst);
	fwrite(&arc_header.num, 12, 1, dst);
	qsort(key_sort, FileNum, sizeof(unit32), comp);
	for (k = 0; k < FileNum; k++)
	{
		for (i = 0; i < FileNum; i++)
		{
			if (key_sort[k] == file_index[i].key)
			{
				wprintf(L"name:%ls offset:0x%X size:0x%X hash:0x%X key:0x%X\n", file_index[i].name, file_index[i].offset, file_index[i].size, file_index[i].hash, file_index[i].key);
				fseek(dst, 0x1C + k * 0x10, SEEK_SET);
				fwrite(&file_index[i].hash, 4, 4, dst);
				fseek(dst, arc_header.filename_off + savepos, SEEK_SET);
				WideCharToMultiByte(932, 0, file_index[i].name, 64, dstname, MAX_PATH, NULL, FALSE);
				savepos += strlen(dstname) + 1;
				fwrite(dstname, strlen(dstname) + 1, 1, dst);
			}
		}
	}
	fclose(dst);
}

void PackFileV2(char *fname)
{
	FILE *src, *dst;
	unit32 i = 0, savepos = 0, k = 0;
	unit8 dstname[MAX_PATH], *data;
	unit64 *hash_sort = malloc(FileNum * 8);
	sprintf(dstname, "%s.arc", fname);
	dst = fopen(dstname, "wb");
	_chdir(fname);
	fseek(dst, arc_header.data_off, SEEK_SET);
	for (i = 0; i < FileNum; i++)
	{
		hash_sort[i] = file_index_v2[i].hash;
		file_index_v2[i].offset = ftell(dst);
		src = _wfopen(file_index_v2[i].name, L"rb");
		data = malloc(file_index_v2[i].size);
		fread(data, file_index_v2[i].size, 1, src);
		fwrite(data, file_index_v2[i].size, 1, dst);
		fclose(src);
		free(data);
	}
	fseek(dst, 0, SEEK_SET);
	fwrite(arc_header.magic, 16, 1, dst);
	fwrite(&arc_header.num, 12, 1, dst);
	qsort(hash_sort, FileNum, sizeof(unit64), compV2);
	for (k = 0; k < FileNum; k++)
	{
		for (i = 0; i < FileNum; i++)
		{
			if (hash_sort[k] == file_index_v2[i].hash)
			{
				wprintf(L"name:%ls offset:0x%X size:0x%X hash:0x%llX\n", file_index_v2[i].name, file_index_v2[i].offset, file_index_v2[i].size, file_index_v2[i].hash);
				fseek(dst, 0x1C + k * 0x10, SEEK_SET);
				fwrite(&file_index_v2[i].hash, 4, 4, dst);
				fseek(dst, arc_header.filename_off + savepos, SEEK_SET);
				WideCharToMultiByte(932, 0, file_index_v2[i].name, 64, dstname, MAX_PATH, NULL, FALSE);
				savepos += strlen(dstname) + 1;
				fwrite(dstname, strlen(dstname) + 1, 1, dst);
			}
		}
	}
	fclose(dst);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-Majiro\n用于封包MajiroArcV3文件。\n将文件夹拖到程序上。\nby Darkness-TX 2018.07.09\n\n");
	//arc_header.num = process_dir(argv[1]);
	//BuildHash_Key();
	//PackFile(argv[1]);
	arc_header.num = process_dirV2(argv[1]);
	BuildHashV2();
	PackFileV2(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}