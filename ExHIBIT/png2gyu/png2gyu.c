/*
用于将png文件转换成gyu
made by Darkness-TX
2017.11.14
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
#include "lzss.h"

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

struct Header
{
	unit32 magic;//GYU\x1A
	unit16 flag;
	unit16 mode;
	unit32 key;
	unit32 bpp;
	unit32 width;
	unit32 height;
	unit32 data_size;
	unit32 alpha_size;
	unit32 pal_num;
}gyu_header;

unit8* ReadPng(FILE *OpenPng, unit32 width, unit32 height, unit32 mode)
{
	png_structp png_ptr;
	png_infop info_ptr, end_ptr;
	png_bytep *rows;
	unit32 i = 0, bpp = 0, format = 0, llen;
	unit32 pwidth = 0, pheight = 0;
	unit8 *TexData;
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
	png_init_io(png_ptr, OpenPng);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)&pwidth, (png_uint_32*)&pheight, &bpp, &format, NULL, NULL, NULL);
	if (width != pwidth || height != pheight)
	{
		printf("gyu与png的宽高不一致！\ngyu:%d * %d\npng:%d * %d\n", width, height, pwidth, pheight);
		system("pause");
		exit(0);
	}
	rows = (png_bytep*)malloc(height * sizeof(char*));
	if (mode == 32)
		TexData = malloc(height * width * 4);
	else
		TexData = malloc(height * width * 3);
	llen = png_get_rowbytes(png_ptr, info_ptr);
	for (i = 0; i < height; i++)
		rows[i] = (png_bytep)(TexData + llen*i);
	png_read_image(png_ptr, rows);
	free(rows);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
	return TexData;
}

void Gyu_WriteFile(char *fname)
{
	FILE *src = fopen(fname, "rb");
	unit32 i = 0, j = 0;
	unit8 dstname[260], *png_data_up = NULL, *png_data_down = NULL;
	fread(&gyu_header, 1, sizeof(gyu_header), src);
	fclose(src);
	if (gyu_header.magic != 0x1A555947)
	{
		printf("文件头不是GYU\\x1A！\n");
		system("pause");
		exit(0);
	}
	if (gyu_header.bpp == 8)
		gyu_header.bpp = 24;
	else if (gyu_header.bpp == 32)
	{
		gyu_header.bpp = 24;
		gyu_header.alpha_size = 1;//防止gyu_header.bpp == 32 && gyu_header.alpha_size == 0的情况
	}
	sprintf(dstname, "%s_new", fname);
	sprintf(fname, "%s.png", fname);
	src = fopen(fname, "rb");
	if (gyu_header.bpp == 24 && gyu_header.alpha_size == 0)
	{
		png_data_up = ReadPng(src, gyu_header.width, gyu_header.height, 24);
		png_data_down = malloc(gyu_header.width * gyu_header.height * 3);
		for (i = 0, j = gyu_header.height - 1; i < gyu_header.height; i++, j--)
			memcpy(&png_data_down[j * gyu_header.width * 3], &png_data_up[i * gyu_header.width * 3], gyu_header.width * 3);
	}
	else
	{
		png_data_up = ReadPng(src, gyu_header.width, gyu_header.height, 32);
		png_data_down = malloc(gyu_header.width * gyu_header.height * 4);
		for (i = 0, j = gyu_header.height - 1; i < gyu_header.height; i++, j--)
			memcpy(&png_data_down[j * gyu_header.width * 4], &png_data_up[i * gyu_header.width * 4], gyu_header.width * 4);
	}
	free(png_data_up);
	fclose(src);
	unit8 *src_data = malloc(gyu_header.height * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3));//每行数据大小需要4字节对齐
	memset(src_data, 0, gyu_header.height * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3));
	if (gyu_header.bpp == 24 && gyu_header.alpha_size == 0)
	{
		for (j = 0; j < gyu_header.height; j++)
		{
			for (i = 0; i < gyu_header.width; i++)
			{
				src_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 3 + 2] = png_data_down[j * gyu_header.width * 3 + i * 3];
				src_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 3 + 1] = png_data_down[j * gyu_header.width * 3 + i * 3 + 1];
				src_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 3] = png_data_down[j * gyu_header.width * 3 + i * 3 + 2];
			}
		}
		free(png_data_down);
		src = fopen("intmp.bin", "wb");
		fwrite(src_data, 1, ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) * gyu_header.height, src);
		fclose(src);
		free(src_data);
		infile = fopen("intmp.bin", "rb");
		outfile = fopen("outtmp.bin", "wb");
		Encode();
		fclose(infile);
		fclose(outfile);
		remove("intmp.bin");
		src = fopen("outtmp.bin", "rb");
		src_data = malloc(codesize);
		fread(src_data, 1, codesize, src);
		fclose(src);
		remove("outtmp.bin");
		FILE *dst = fopen(dstname, "wb");
		gyu_header.data_size = codesize;
		gyu_header.key = 0xFFFFFFFF;
		gyu_header.mode = 0x400;
		fwrite(&gyu_header, 1, sizeof(gyu_header), dst);
		fwrite(src_data, 1, gyu_header.data_size, dst);
		free(src_data);
		fclose(dst);
	}
	else
	{
		unit8* alphasrc_data = malloc(((gyu_header.width + 3) & ~3) * gyu_header.height);
		memset(alphasrc_data, 0, ((gyu_header.width + 3) & ~3) * gyu_header.height);
		for (j = 0; j < gyu_header.height; j++)
		{
			for (i = 0; i < gyu_header.width; i++)
			{
				src_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 3 + 2] = png_data_down[j * gyu_header.width * 4 + i * 4];
				src_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 3 + 1] = png_data_down[j * gyu_header.width * 4 + i * 4 + 1];
				src_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 3] = png_data_down[j * gyu_header.width * 4 + i * 4 + 2];
				alphasrc_data[j * ((gyu_header.width + 3) & ~3) + i] = png_data_down[j * gyu_header.width * 4 + i * 4 + 3];
			}
		}
		src = fopen("intmp.bin", "wb");
		fwrite(src_data, 1, ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) * gyu_header.height, src);
		fclose(src);
		free(src_data);
		infile = fopen("intmp.bin", "rb");
		outfile = fopen("outtmp.bin", "wb");
		Encode();
		fclose(infile);
		fclose(outfile);
		remove("intmp.bin");
		src = fopen("outtmp.bin", "rb");
		src_data = malloc(codesize);
		fread(src_data, 1, codesize, src);
		fclose(src);
		remove("outtmp.bin");
		gyu_header.data_size = codesize;
		gyu_header.key = 0xFFFFFFFF;
		gyu_header.mode = 0x400;
		gyu_header.flag = 3;
		gyu_header.pal_num = 0;
		gyu_header.bpp = 24;
		src = fopen("intmp.bin", "wb");
		fwrite(alphasrc_data, 1, ((gyu_header.width + 3) & ~3)* gyu_header.height, src);
		fclose(src);
		free(alphasrc_data);
		infile = fopen("intmp.bin", "rb");
		outfile = fopen("outtmp.bin", "wb");
		textsize = 0;
		codesize = 0;
		printcount = 0;
		Encode();
		fclose(infile);
		fclose(outfile);
		remove("intmp.bin");
		src = fopen("outtmp.bin", "rb");
		alphasrc_data = malloc(codesize);
		fread(alphasrc_data, 1, codesize, src);
		fclose(src);
		remove("outtmp.bin");
		gyu_header.alpha_size = codesize;
		FILE *dst = fopen(dstname, "wb");
		fwrite(&gyu_header, 1, sizeof(gyu_header), dst);
		fwrite(src_data, 1, gyu_header.data_size, dst);
		fwrite(alphasrc_data, 1, gyu_header.alpha_size, dst);
		free(src_data);
		free(alphasrc_data);
		fclose(dst);
	}
	printf("name:%s flag:0x%X mode:0x%X key:0x%X bpp:%d width:%d height:%d data_size:0x%X alpha_size:0x%X pal_num:%d\n", fname, gyu_header.flag, gyu_header.mode, gyu_header.key, gyu_header.bpp, gyu_header.width, gyu_header.height, gyu_header.data_size, gyu_header.alpha_size, gyu_header.pal_num);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-ExHIBIT\n用于将png文件转换成gyu。\n将原始gyu文件拖到程序上。\nby Darkness-TX 2017.11.14\n\n");
	Gyu_WriteFile(argv[1]);
	system("pause");
	return 0;
}