/*
用于导出ap3图片
made by Darkness-TX
2022.09.10
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

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

unit32 FileNum = 0;//总文件数，初始计数为0

struct ap3_header
{
	unit32 magic;//0x53504104
	unit8 version;//'3'
	unit32 count;
	unit32 width;
	unit32 height;
}Ap3_Header;

struct prs_header
{
	unit8 magic[2];
	unit32 width;
	unit32 height;
	unit16 bpp;
}Prs_Header;

struct index
{
	WCHAR FileName[MAX_PATH];//文件名
	unit32 FileSize;//文件大小
}Index[50000];

unit32 process_dir(char* dname)
{
	long Handle;
	struct _wfinddata64i32_t FileInfo;
	_chdir(dname);//跳转路径
	if ((Handle = _wfindfirst(L"*.ap3", &FileInfo)) == -1L)
	{
		printf("没有找到匹配的项目，请将后缀命名为.ap3\n");
		system("pause");
		exit(0);
	}
	do
	{
		if (FileInfo.name[0] == L'.')  //过滤本级目录和父目录
			continue;
		wsprintf(Index[FileNum].FileName, FileInfo.name);
		Index[FileNum].FileSize = FileInfo.size;
		FileNum++;
	} while (_wfindnext(Handle, &FileInfo) == 0);
	return FileNum;
}

void WritePng(FILE* Pngname, unit32 Width, unit32 Height, unit32 Bpp, unit8* data)
{
	png_structp png_ptr;
	png_infop info_ptr;
	int i = 0;
	unit8 buff = 0;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		printf("PNG信息创建失败!\n");
		exit(0);
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		printf("info信息创建失败!\n");
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		exit(0);
	}
	png_init_io(png_ptr, Pngname);
	if (Bpp == 24)//虽然是24，但是却是BGRA
	{
		png_set_IHDR(png_ptr, info_ptr, Width, Height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png_ptr, info_ptr);
		for (i = 0; i < Width * Height; i++)
		{
			buff = data[i * 4 + 0];
			data[i * 4 + 0] = data[i * 4 + 2];
			data[i * 4 + 2] = buff;
		}
		for (i = Height - 1; i >= 0; i--)//上下颠倒
			png_write_row(png_ptr, data + i * Width * 4);
	}
	else
	{
		printf("不支持的bpp模式!");
		system("pause");
		exit(0);
	}
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
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

void WritePngFile()
{
	FILE* src = NULL, * dst = NULL;
	unit32 i = 0, j = 0, x = 0, y = 0;
	unit8* cdata = NULL, * ddata = NULL;
	WCHAR dstname[MAX_PATH];
	for (i = 0; i < FileNum; i++)
	{
		src = _wfopen(Index[i].FileName, L"rb");
		fread(&Ap3_Header.magic, 4, 1, src);
		if (Ap3_Header.magic != 0x53504104)//\x04APS
		{
			printf("文件头不是\\x04APS！\n");
			system("pause");
			exit(0);
		}
		fread(&Ap3_Header.version, 1, 1, src);
		if (Ap3_Header.version != '3')
		{
			printf("版本不是3！\n");
			system("pause");
			exit(0);
		}
		fread(&Ap3_Header.count, 4, 1, src);
		for (j = 0; j < Ap3_Header.count; j++)
		{
			fseek(src, 4, SEEK_CUR);
			unit8 namesize;
			fread(&namesize, 1, 1, src);
			fseek(src, namesize, SEEK_CUR);
			if (namesize == 0)
				fseek(src, 0x10, SEEK_CUR);
			else
			{
				fread(&x, 4, 1, src);
				fread(&y, 4, 1, src);
				fread(&Ap3_Header.width, 4, 1, src);
				fread(&Ap3_Header.height, 4, 1, src);
			}
			fseek(src, 0xC, SEEK_CUR);
		}
		wprintf(L"%ls width:%d height:%d x:%d y:%d\n", Index[i].FileName, Ap3_Header.width, Ap3_Header.height, x, y);
		unit32 buffsize = 0, csize = 0, dsize = 0;
		unit16 type = 0;
		fread(&buffsize, 4, 1, src);
		fread(&type, 2, 1, src);
		fread(&csize, 4, 1, src);
		if (type == 1)
		{
			fread(&dsize, 4, 1, src);
			cdata = malloc(csize);
			fread(cdata, csize, 1, src);
			ddata = malloc(dsize);
			lz_uncompress(ddata, dsize, cdata, csize);
			free(cdata);
		}
		else
		{
			dsize = csize;
			ddata = malloc(dsize);
			fread(ddata, dsize, 1, src);
		}
		//之后直接就是prs文件格式
		if (strncmp(ddata, "AP", 2) != 0)
		{
			printf("数据标识不是AP！\n");
			system("pause");
			exit(0);
		}
		memcpy(&Prs_Header.width, ddata + 2, 4);
		memcpy(&Prs_Header.height, ddata + 6, 4);
		memcpy(&Prs_Header.bpp, ddata + 10, 4);
		wprintf(L"data:\twidth:%d height:%d bpp:%d\n", Prs_Header.width, Prs_Header.height, Prs_Header.bpp);
		wsprintf(dstname, L"%ls.png", Index[i].FileName);
		dst = _wfopen(dstname, L"wb");
		WritePng(dst, Prs_Header.width, Prs_Header.height, Prs_Header.bpp, ddata + 0xC);
		//fwrite(ddata, dsize, 1, dst);
		free(ddata);
		fclose(dst);
		fclose(src);
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-KaGuYa\n用于导出ap3图片。\n将文件夹拖到程序上。\nby Darkness-TX 2022.09.10\n\n");
	process_dir(argv[1]);
	WritePngFile();
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}