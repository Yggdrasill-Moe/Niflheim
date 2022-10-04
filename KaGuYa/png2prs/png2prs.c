/*
用于导入prs图片
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
	if ((Handle = _wfindfirst(L"*.prs", &FileInfo)) == -1L)
	{
		printf("没有找到匹配的项目，请将后缀命名为.prs\n");
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
	if (Prs_Header.bpp != 24)
	{
		printf("不支持的bpp模式!");
		system("pause");
		exit(0);
	}
	if (format != PNG_COLOR_TYPE_RGB_ALPHA)
	{
		printf("原始prs文件为32位rgba图，而png文件不是32位rgba图，请转换！format:%d\n", format);
		system("pause");
		exit(0);
	}
	if (width != Prs_Header.width || height != Prs_Header.height)
	{
		printf("图片的长宽与原图不符！\n");
		system("pause");
		exit(0);
	}
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
	unit32 i = 0;
	unit8* data = NULL;
	WCHAR dstname[MAX_PATH];
	for (i = 0; i < FileNum; i++)
	{
		src = _wfopen(Index[i].FileName, L"rb");
		fread(Prs_Header.magic, 2, 1, src);
		fread(&Prs_Header.width, 4, 1, src);
		fread(&Prs_Header.height, 4, 1, src);
		fread(&Prs_Header.bpp, 2, 1, src);
		wprintf(L"%ls width:%d height:%d bpp:%d\n", Index[i].FileName, Prs_Header.width, Prs_Header.height, Prs_Header.bpp);
		if (strncmp(Prs_Header.magic, "AP", 2) != 0)
		{
			printf("文件头不是AP！\n");
			system("pause");
			exit(0);
		}
		wsprintf(dstname, L"%ls.png", Index[i].FileName);
		png = _wfopen(dstname, L"rb");
		data = ReadPng(png);
		fclose(png);
		wsprintf(dstname, L"%ls.new", Index[i].FileName);
		dst = _wfopen(dstname, L"wb");
		fwrite(Prs_Header.magic, 2, 1, dst);
		fwrite(&Prs_Header.width, 4, 1, dst);
		fwrite(&Prs_Header.height, 4, 1, dst);
		fwrite(&Prs_Header.bpp, 2, 1, dst);
		fwrite(data, Index[i].FileSize - 0xC, 1, dst);
		free(data);
		fclose(dst);
		fclose(src);
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-KaGuYa\n用于导入ap3图片。\n将文件夹拖到程序上。\nby Darkness-TX 2022.09.10\n\n");
	process_dir(argv[1]);
	ReadPngFile();
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}