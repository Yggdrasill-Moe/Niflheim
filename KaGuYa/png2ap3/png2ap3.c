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

unit8* ReadPng(FILE* pngfile)
{
	png_structp png_ptr;
	png_infop info_ptr, end_ptr;
	png_bytep* rows;
	int i = 0, width = 0, height = 0, bpp = 0, format = 0, j = 0;
	unit8 buff = 0, * data = NULL;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		printf("PNG信息创建失败!\n");
		exit(0);
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		printf("info信息创建失败!\n");
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		exit(0);
	}
	end_ptr = png_create_info_struct(png_ptr);
	if (end_ptr == NULL)
	{
		printf("end信息创建失败!\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		exit(0);
	}
	png_init_io(png_ptr, pngfile);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)&width, (png_uint_32*)&height, &bpp, &format, NULL, NULL, NULL);
	if (format != PNG_COLOR_TYPE_RGB_ALPHA)
	{
		printf("不支持非32位图，请转换！format:%d\n", format);
		system("pause");
		exit(0);
	}
	Prs_Header.width = width;
	Prs_Header.height = height;
	Prs_Header.bpp = 0x18;//24
	data = malloc(Prs_Header.height * Prs_Header.width * 4);
	rows = (png_bytep*)malloc(Prs_Header.height * sizeof(char*));
	for (i = Prs_Header.height - 1; i >= 0; i--)//上下颠倒
		rows[j++] = (png_bytep)(data + Prs_Header.width * i * 4);
	png_read_image(png_ptr, rows);
	free(rows);
	for (i = 0; i < width * height; i++)
	{
		buff = data[i * 4 + 0];
		data[i * 4 + 0] = data[i * 4 + 2];
		data[i * 4 + 2] = buff;
	}
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
	return data;
}

void ReadPngFile()
{
	FILE* src = NULL, * dst = NULL, * png = NULL;
	unit32 i = 0, j = 0, x = 0, y = 0;
	unit8* cdata = NULL, * ddata = NULL;
	WCHAR dstname[MAX_PATH], pngname[MAX_PATH];
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
		wsprintf(dstname, L"%ls.new", Index[i].FileName);
		dst = _wfopen(dstname, L"wb");
		fwrite(&Ap3_Header.magic, 4, 1, dst);
		fwrite(&Ap3_Header.version, 1, 1, dst);
		fwrite(&Ap3_Header.count, 4, 1, dst);
		for (j = 0; j < Ap3_Header.count; j++)
		{
			unit32 buff;
			fread(&buff, 4, 1, src);
			fwrite(&buff, 4, 1, dst);
			unit8 namesize;
			fread(&namesize, 1, 1, src);
			fwrite(&namesize, 1, 1, dst);
			unit8 namebuff[MAX_PATH];
			fread(namebuff, namesize, 1, src);
			fwrite(namebuff, namesize, 1, dst);
			if (namesize == 0)
			{
				fread(&buff, 4, 1, src);
				fwrite(&buff, 4, 1, dst);
				fread(&buff, 4, 1, src);
				fwrite(&buff, 4, 1, dst);
				fread(&buff, 4, 1, src);
				fwrite(&buff, 4, 1, dst);
				fread(&buff, 4, 1, src);
				fwrite(&buff, 4, 1, dst);
			}
			else
			{
				fread(&x, 4, 1, src);
				fwrite(&x, 4, 1, dst);
				fread(&y, 4, 1, src);
				fwrite(&y, 4, 1, dst);
				fread(&Ap3_Header.width, 4, 1, src);
				fwrite(&Ap3_Header.width, 4, 1, dst);
				fread(&Ap3_Header.height, 4, 1, src);
				fwrite(&Ap3_Header.height, 4, 1, dst);
			}
			fread(&buff, 4, 1, src);
			fwrite(&buff, 4, 1, dst);
			fread(&buff, 4, 1, src);
			fwrite(&buff, 4, 1, dst);
			fread(&buff, 4, 1, src);
			fwrite(&buff, 4, 1, dst);
		}
		wprintf(L"%ls width:%d height:%d x:%d y:%d\n", Index[i].FileName, Ap3_Header.width, Ap3_Header.height, x, y);
		unit32 buffsize = 0, dsize = 0;
		unit16 type = 0;
		wsprintf(pngname, L"%ls.png", Index[i].FileName);
		png = _wfopen(pngname, L"rb");
		ddata = ReadPng(png);
		fclose(png);
		dsize = Prs_Header.width * Prs_Header.height * 4 + 0xC;
		buffsize = dsize + 4 + 2;
		fwrite(&buffsize, 4, 1, dst);
		fwrite(&type, 2, 1, dst);
		fwrite(&dsize, 4, 1, dst);
		strncpy(Prs_Header.magic, "AP", 2);
		fwrite(Prs_Header.magic, 2, 1, dst);
		fwrite(&Prs_Header.width, 4, 1, dst);
		fwrite(&Prs_Header.height, 4, 1, dst);
		fwrite(&Prs_Header.bpp, 2, 1, dst);
		fwrite(ddata, dsize - 0xC, 1, dst);
		fclose(dst);
		fclose(src);
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-KaGuYa\n用于导入prs图片。\n将文件夹拖到程序上。\nby Darkness-TX 2022.09.10\n\n");
	process_dir(argv[1]);
	ReadPngFile();
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}