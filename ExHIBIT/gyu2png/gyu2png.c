/*
用于将gyu文件转换成png
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
#include "mt19937int.h"
#include "lzss.h"
#include "gyu_new_uncomp.h"

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

void WritePng(FILE *pngfile, unit32 width, unit32 height, unit32 bpp, unit8* data)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unit32 i = 0;
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
	png_init_io(png_ptr, pngfile);
	if (bpp == 8 || bpp == 32)
		png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	else if (bpp == 24)
		png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	if (bpp == 8 || bpp == 32)
		for (i = 0; i < height; i++)
			png_write_row(png_ptr, data + i*width * 4);
	else if (bpp == 24)
		for (i = 0; i < height; i++)
			png_write_row(png_ptr, data + i*width * 3);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
}

void Gyu_WriteFile(char *fname)
{
	FILE *src = fopen(fname, "rb");
	unit32 i = 0, j = 0;
	unit8 *pal_data = NULL, *alphasrc_data = NULL;
	fread(&gyu_header, 1, sizeof(gyu_header), src);
	if (gyu_header.magic != 0x1A555947)
	{
		printf("文件头不是GYU\\x1A！\n");
		system("pause");
		exit(0);
	}
	printf("name:%s flag:0x%X mode:0x%X key:0x%X bpp:%d width:%d height:%d data_size:0x%X alpha_size:0x%X pal_num:%d\n", fname, gyu_header.flag, gyu_header.mode, gyu_header.key, gyu_header.bpp, gyu_header.width, gyu_header.height, gyu_header.data_size, gyu_header.alpha_size, gyu_header.pal_num);
	//如果是8位索引图
	if (gyu_header.pal_num != 0)
	{
		pal_data = malloc(gyu_header.pal_num * 4);
		fread(pal_data, 1, gyu_header.pal_num * 4, src);
	}
	//位图数据
	unit8* src_data = malloc(gyu_header.data_size);
	fread(src_data, 1, gyu_header.data_size, src);
	unit8* dst_data = malloc(gyu_header.height * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3));//每行数据大小需要4字节对齐
	if (gyu_header.key != 0xFFFFFFFF)
	{
		sgenrand(gyu_header.key);
		for (i = 0; i < 10; i++)
		{
			unit8 tmp;
			unit32 rand0, rand1;
			rand0 = genrand() % gyu_header.data_size;
			rand1 = genrand() % gyu_header.data_size;
			tmp = src_data[rand1];
			src_data[rand1] = src_data[rand0];
			src_data[rand0] = tmp;
		}
	}
	if (gyu_header.mode != 0x800)
		lzss_decompress(dst_data, gyu_header.height * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3), src_data, gyu_header.data_size);
	else
		gyu_new_uncompress(dst_data, src_data);
	free(src_data);
	//alpha数据
	if (gyu_header.flag & 1)
	{
		alphasrc_data = malloc(gyu_header.alpha_size);
		fread(alphasrc_data, 1, gyu_header.alpha_size, src);
	}
	unit8* alphadst_data = malloc(gyu_header.height * ((gyu_header.width + 3) & ~3));//宽度4字节对齐
	if (gyu_header.alpha_size != gyu_header.height * ((gyu_header.width + 3) & ~3) && gyu_header.alpha_size != 0)
	{
		lzss_decompress(alphadst_data, gyu_header.height * ((gyu_header.width + 3) & ~3), alphasrc_data, gyu_header.alpha_size);
		free(alphasrc_data);
	}
	else
	{
		memcpy(alphadst_data, alphasrc_data, gyu_header.alpha_size);
		free(alphasrc_data);
	}
	if (!(gyu_header.flag & 2))
		for (i = 0; i < gyu_header.height * ((gyu_header.width + 3)&~3); i++)
			alphadst_data[i] = alphadst_data[i] >= 0x10 ? 0xFF : alphadst_data[i] * 0x10;
	fclose(src);
	sprintf(fname, "%s.png", fname);
	//从png转成gyu的8位索引图不好搞，所以不如8位索引图全部当成32位图处理
	if (gyu_header.bpp == 8)
	{
		unit8 *png_data_down = malloc(gyu_header.height * gyu_header.width * 4);
		unit8 *png_data_up = malloc(gyu_header.height * gyu_header.width * 4);
		memset(png_data_down, 0, gyu_header.height * gyu_header.width * 4);
		memset(png_data_up, 0, gyu_header.height * gyu_header.width * 4);
		for (j = 0; j < gyu_header.height; j++)
		{
			for (i = 0; i < gyu_header.width; i++)
			{
				png_data_down[j * gyu_header.width * 4 + i * 4] = pal_data[dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i] * 4 + 2];
				png_data_down[j * gyu_header.width * 4 + i * 4 + 1] = pal_data[dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i] * 4 + 1];
				png_data_down[j * gyu_header.width * 4 + i * 4 + 2] = pal_data[dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i] * 4];
				png_data_down[j * gyu_header.width * 4 + i * 4 + 3] = alphadst_data[j * ((gyu_header.width + 3) & ~3) + i];
			}
		}
		free(pal_data);
		for (i = 0, j = gyu_header.height - 1; i < gyu_header.height; i++, j--)
			memcpy(&png_data_up[j * gyu_header.width * 4], &png_data_down[i * gyu_header.width * 4], gyu_header.width * 4);
		free(png_data_down);
		FILE *dst = fopen(fname, "wb");
		WritePng(dst, gyu_header.width, gyu_header.height, gyu_header.bpp, png_data_up);
		free(png_data_up);
		fclose(dst);
	}
	else if (gyu_header.bpp == 24 && gyu_header.alpha_size != 0)
	{
		unit8 *png_data_down = malloc(gyu_header.height * gyu_header.width * 4);
		unit8 *png_data_up = malloc(gyu_header.height * gyu_header.width * 4);
		memset(png_data_down, 0, gyu_header.height * gyu_header.width * 4);
		memset(png_data_up, 0, gyu_header.height * gyu_header.width * 4);
		for (j = 0; j < gyu_header.height; j++)
		{
			for (i = 0; i < gyu_header.width; i++)
			{
				png_data_down[j * gyu_header.width * 4 + i * 4] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 3 + 2];
				png_data_down[j * gyu_header.width * 4 + i * 4 + 1] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 3 + 1];
				png_data_down[j * gyu_header.width * 4 + i * 4 + 2] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 3];
				png_data_down[j * gyu_header.width * 4 + i * 4 + 3] = alphadst_data[j * ((gyu_header.width + 3) & ~3) + i];
			}
		}
		for (i = 0, j = gyu_header.height - 1; i < gyu_header.height; i++, j--)
			memcpy(&png_data_up[j * gyu_header.width * 4], &png_data_down[i * gyu_header.width * 4], gyu_header.width * 4);
		free(png_data_down);
		FILE *dst = fopen(fname, "wb");
		WritePng(dst, gyu_header.width, gyu_header.height, 32, png_data_up);
		free(png_data_up);
		fclose(dst);
	}
	else if (gyu_header.bpp == 32 && gyu_header.alpha_size != 0)//没见过，预防性分支
	{
		unit8* png_data_down = malloc(gyu_header.height * gyu_header.width * 4);
		unit8* png_data_up = malloc(gyu_header.height * gyu_header.width * 4);
		memset(png_data_down, 0, gyu_header.height * gyu_header.width * 4);
		memset(png_data_up, 0, gyu_header.height * gyu_header.width * 4);
		for (j = 0; j < gyu_header.height; j++)
		{
			//应该是BGRA而不是ABGR？
			for (i = 0; i < gyu_header.width; i++)
			{
				png_data_down[j * gyu_header.width * 4 + i * 4] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 4 + 2];
				png_data_down[j * gyu_header.width * 4 + i * 4 + 1] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 4 + 1];
				png_data_down[j * gyu_header.width * 4 + i * 4 + 2] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 4];
				png_data_down[j * gyu_header.width * 4 + i * 4 + 3] = alphadst_data[j * ((gyu_header.width + 3) & ~3) + i];
			}
		}
		for (i = 0, j = gyu_header.height - 1; i < gyu_header.height; i++, j--)
			memcpy(&png_data_up[j * gyu_header.width * 4], &png_data_down[i * gyu_header.width * 4], gyu_header.width * 4);
		free(png_data_down);
		FILE* dst = fopen(fname, "wb");
		WritePng(dst, gyu_header.width, gyu_header.height, 32, png_data_up);
		free(png_data_up);
		fclose(dst);
	}
	else if (gyu_header.bpp == 32)//没见过，预防性分支
	{
		unit8* png_data_down = malloc(gyu_header.height * gyu_header.width * 4);
		unit8* png_data_up = malloc(gyu_header.height * gyu_header.width * 4);
		memset(png_data_down, 0, gyu_header.height * gyu_header.width * 4);
		memset(png_data_up, 0, gyu_header.height * gyu_header.width * 4);
		for (j = 0; j < gyu_header.height; j++)
		{
			//应该是BGRA而不是ABGR？
			for (i = 0; i < gyu_header.width; i++)
			{
				png_data_down[j * gyu_header.width * 4 + i * 4] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 4 + 2];
				png_data_down[j * gyu_header.width * 4 + i * 4 + 1] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 4 + 1];
				png_data_down[j * gyu_header.width * 4 + i * 4 + 2] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 4];
				png_data_down[j * gyu_header.width * 4 + i * 4 + 3] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 4 + 3];
			}
		}
		for (i = 0, j = gyu_header.height - 1; i < gyu_header.height; i++, j--)
			memcpy(&png_data_up[j * gyu_header.width * 4], &png_data_down[i * gyu_header.width * 4], gyu_header.width * 4);
		free(png_data_down);
		FILE* dst = fopen(fname, "wb");
		WritePng(dst, gyu_header.width, gyu_header.height, 32, png_data_up);
		free(png_data_up);
		fclose(dst);
	}
	else if (gyu_header.bpp == 24)
	{
		unit8 *png_data_down = malloc(gyu_header.height * gyu_header.width * 3);
		unit8 *png_data_up = malloc(gyu_header.height * gyu_header.width * 3);
		memset(png_data_down, 0, gyu_header.height * gyu_header.width * 3);
		memset(png_data_up, 0, gyu_header.height * gyu_header.width * 3);
		for (j = 0; j < gyu_header.height; j++)
		{
			for (i = 0; i < gyu_header.width; i++)
			{
				png_data_down[j * gyu_header.width * 3 + i * 3] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 3 + 2];
				png_data_down[j * gyu_header.width * 3 + i * 3 + 1] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 3 + 1];
				png_data_down[j * gyu_header.width * 3 + i * 3 + 2] = dst_data[j * ((gyu_header.width * gyu_header.bpp / 8 + 3) & ~3) + i * 3];
			}
		}
		for (i = 0, j = gyu_header.height - 1; i < gyu_header.height; i++, j--)
			memcpy(&png_data_up[j * gyu_header.width * 3], &png_data_down[i * gyu_header.width * 3], gyu_header.width * 3);
		free(png_data_down);
		FILE *dst = fopen(fname, "wb");
		WritePng(dst, gyu_header.width, gyu_header.height, gyu_header.bpp, png_data_up);
		free(png_data_up);
		fclose(dst);
	}
	else
	{
		printf("未知的gyu类型！\n");
		system("pause");
		exit(0);
	}
	free(dst_data);
	free(alphadst_data);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-ExHIBIT\n用于将gyu文件转换成png。\n将gyu文件拖到程序上。\nby Darkness-TX 2017.11.14\n\n");
	Gyu_WriteFile(argv[1]);
	system("pause");
	return 0;
}