/*
用于解包KaGuYa社游戏的arc文件
made by Darkness-TX
2022.09.09
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
	FILE *ari = NULL;
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

static inline unsigned char getbit_be(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << (7 - pos)));
}

static DWORD lz_uncompress(BYTE* uncompr, DWORD uncomprLen, BYTE* compr, DWORD comprLen)
{
	DWORD act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	DWORD curbit = 0;
	/* compr中的当前扫描字节 */
	DWORD curbyte = 0;
	DWORD nCurWindowByte = 1;
	BYTE window[4096];

	memset(window, 0, sizeof(window));
	while (1) {
		DWORD i;

		if (curbit == 8) {
			curbit = 0;
			curbyte++;
		}
		/* 如果为1, 表示接下来的1个字节原样输出 */
		if (getbit_be(compr[curbyte], curbit++)) {
			BYTE data = 0;

			for (i = 0; i < 8; i++) {
				if (curbit == 8) {
					curbit = 0;
					curbyte++;
				}
				data |= getbit_be(compr[curbyte], curbit++) << (7 - i);
			}
			/* 输出1字节非压缩数据 */
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			window[nCurWindowByte++] = data;
			nCurWindowByte &= sizeof(window) - 1;
		}
		else {
			DWORD copy_bytes, win_offset = 0;

			/* 该循环次数由窗口大小决定 */
			/* 得到窗口内压缩数据的索引 */
			for (i = 0; i < 12; i++) {
				if (curbit == 8) {
					curbit = 0;
					curbyte++;
				}
				win_offset |= getbit_be(compr[curbyte], curbit++) << (11 - i);
			}
			if (!win_offset)
				goto out;

			copy_bytes = 0;
			for (i = 0; i < 4; i++) {
				if (curbit == 8) {
					curbit = 0;
					curbyte++;
				}
				copy_bytes |= getbit_be(compr[curbyte], curbit++) << (3 - i);
			}
			copy_bytes += 2;
			for (i = 0; i < copy_bytes; i++) {
				BYTE data;

				data = window[(win_offset + i) & (sizeof(window) - 1)];
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				window[nCurWindowByte++] = data;
				nCurWindowByte &= sizeof(window) - 1;
			}
		}
	}
out:
	return act_uncomprlen;
}

void Unpack(char* fname)
{
	FILE *src = NULL, *dst = NULL;
	unit32 i = 0,filesize = 0;
	unit8 *data = NULL, *ddata = NULL, dirname[MAX_PATH];
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
	sprintf(dirname, "%s_unpack", fname);
	_mkdir(dirname);
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
		{
			fread(&Header.desize, 4, 1, src);
			wprintf(L"%ls type:%d size:0x%X desize:0x%X\n", dstname, Header.type, Header.size, Header.desize);
		}
		else
			wprintf(L"%ls type:%d size:0x%X\n", dstname, Header.type, Header.size);
		for (WCHAR* tag = (WCHAR*)(dstname+1);*tag;*tag++)
		{
			if (*tag == L'\\')
			{
				WCHAR buf[MAX_PATH], path[MAX_PATH];
				wcscpy(buf, dstname + 1);
				buf[wcslen(dstname + 1) - wcslen(tag) + 1] = L'\0';
				wcscpy(path, buf);
				if (_waccess(path, 0) == -1)
					_wmkdir(path);
			}
		}
		data = malloc(Header.size);
		fread(data, Header.size, 1, src);
		dst = _wfopen(dstname + 1, L"wb");
		if (Header.type == 1)
		{
			ddata = malloc(Header.desize);
			lz_uncompress(ddata, Header.desize, data, Header.size);
			fwrite(ddata, Header.desize, 1, dst);
			free(ddata);
		}
		else
			fwrite(data, Header.size, 1, dst);
		free(data);
		fclose(dst);
		i++;
	}
	fclose(src);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-KaGuYa\n用于解包KaGuYa社游戏的arc文件。\n将arc文件拖到程序上。\nby Darkness-TX 2022.09.09\n\n");
	ReadIndex(argv[1]);
	Unpack(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}