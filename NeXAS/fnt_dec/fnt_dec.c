/*
用于解开fnt转换成png
made by Darkness-TX
2016.12.05
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <Windows.h>
#include <zlib.h>
#include <locale.h>
#include <png.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

struct header
{
	unit8 magic[4];//PAC\0
	unit8 magic2[9];//DATA VER-
	unit16 flag;//为1
	unit16 fontflag;//0101没字体名，字模0xC一组；0301有字体名，字模0x10一组
	unit32 seekflag;//0xFFFF00右移9字节
	unit32 height;
	unit32 width;
	unit32 compsize;
	unit32 decompsize;
}fnt_header;

struct Font_Info
{
	short x;
	short y;
	unit16 width;
	unit16 height;
	unit32 offset;
	unit16 cell;
}font_info[10000];

unit32 font_count = 0;

void WritePng(FILE *pngfile, unit32 width, unit32 height, unit8* data)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unit32 i = 0, k = 0;
	unit8 *dst, *src;
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
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	dst = malloc(width*height * 4);
	src = data;
	for (i = 0, k = 0; i < width*height * 2; i += 2, k++)
	{
		dst[k * 4 + 0] = src[i];
		dst[k * 4 + 1] = src[i];
		dst[k * 4 + 2] = src[i];
		dst[k * 4 + 3] = src[i + 1];
	}
	for (i = 0; i < height; i++)
		png_write_row(png_ptr, dst + i*width * 4);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	free(dst);
}

void ReadIndex(FILE *src, char *fname)
{
	FILE *dst;
	unit8 *cdata, *udata, dstname[200];
	unit32 i;
	sprintf(dstname, "%s_INDEX", fname);
	fread(fnt_header.magic, 1, 4, src);
	if (strncmp(fnt_header.magic, "FNT\0", 4) != 0)
	{
		printf("文件头不是FNT\0!");
		system("pause");
		exit(0);
	}
	if (strncmp(fname, "systemascii", 11) != 0 && strncmp(fname, "systemtutorial", 14) != 0 && strncmp(fname, "system10b", 9))
	{
		fread(fnt_header.magic2, 1, 9, src);
		fread(&fnt_header.flag, 1, 2, src);
		fread(&fnt_header.fontflag, 1, 2, src);
		if (strncmp(fnt_header.magic2, "DATA VER-", 9) != 0)
		{
			printf("文件头无DATA VER-!");
			system("pause");
			exit(0);
		}
		else if (fnt_header.flag != 1)
		{
			printf("flag不为1!");
			system("pause");
			exit(0);
		}
		if (fnt_header.fontflag == 0x103)
			while (fgetc(src) != '\0');
	}
	fread(&fnt_header.width, 1, 4, src);
	fread(&fnt_header.height, 1, 4, src);
	fread(&fnt_header.decompsize, 1, 4, src);
	if ((fnt_header.decompsize & 0xFF00) == 0xFF00)
	{
		fnt_header.seekflag = fnt_header.decompsize;
		fseek(src, 9, SEEK_CUR);
		fread(&fnt_header.decompsize, 1, 4, src);
	}
	fread(&fnt_header.compsize, 1, 4, src);
	printf("Index:\n\twidth:%d height%d decompsize:0x%X compsize:0x%X\n", fnt_header.width, fnt_header.height, fnt_header.decompsize, fnt_header.compsize);
	cdata = malloc(fnt_header.compsize);
	udata = malloc(fnt_header.decompsize);
	fread(cdata, 1, fnt_header.compsize, src);
	uncompress(udata, &fnt_header.decompsize, cdata, fnt_header.compsize);
	dst = fopen(dstname, "wb");
	fwrite(udata, 1, fnt_header.decompsize, dst);
	free(cdata);
	fclose(dst);
	if (fnt_header.fontflag == 0x103)
	{
		font_count = fnt_header.decompsize / 0x10;
		for (i = 0; i < font_count; i++)
		{
			memcpy(&font_info[i].offset, &udata[i * 0x10 + 0xC], 4);
			memcpy(&font_info[i].width, &udata[i * 0x10 + 0x4], 2);
			memcpy(&font_info[i].height, &udata[i * 0x10 + 0x6], 2);
			memcpy(&font_info[i].x, &udata[i * 0x10], 2);
			memcpy(&font_info[i].y, &udata[i * 0x10 + 0x2], 2);
			memcpy(&font_info[i].cell, &udata[i * 0x10 + 0x8], 2);
		}
	}
	else
	{
		font_count = fnt_header.decompsize / 0xC;
		for (i = 0; i < font_count; i++)
			memcpy(&font_info[i], &udata[i * 0xC], 0xC);
	}
	printf("font_count:%d\n", font_count);
#ifdef DEBUG
	system("pause");
#endif // DEBUG
	free(udata);
}

void WritePngFile(char *fname)
{
	FILE *src, *dst;
	unit32 i, savepos, k;
	unit8 *cdata, *bdata, dstname[200];
	src = fopen(fname, "rb");
	ReadIndex(src, fname);
	savepos = ftell(src);
	sprintf(dstname, "%s_unpack", fname);
	_mkdir(dstname);
	_chdir(dstname);
	for (i = 0; i < font_count; i++)
	{
		if (fnt_header.fontflag == 0x103)
			printf("\t%08d.png x:%d y:%d cell:%d width:%d height:%d offset:0x%X\n", i, font_info[i].x, font_info[i].y,font_info[i].cell, font_info[i].width, font_info[i].height, font_info[i].offset);
		else
			printf("\t%08d.png x:%d y:%d width:%d height:%d offset:0x%X\n", i, font_info[i].x, font_info[i].y, font_info[i].width, font_info[i].height, font_info[i].offset);
		if (font_info[i].width != 0 && font_info[i].height != 0)
		{
			fseek(src, font_info[i].offset + savepos, SEEK_SET);
			cdata = malloc(font_info[i].width*font_info[i].height * 2);
			fread(cdata, 1, font_info[i].width*font_info[i].height * 2, src);
			sprintf(dstname, "%08d.png", i);
			dst = fopen(dstname, "wb");
			WritePng(dst, font_info[i].width, font_info[i].height, cdata);
			fclose(dst);
			bdata = malloc(font_info[i].width*font_info[i].height);
			for (k = 0; k < font_info[i].width*font_info[i].height; k++)
				bdata[k] = cdata[k * 2];
			free(cdata);
			sprintf(dstname, "%08d.bin", i);
			dst = fopen(dstname, "wb");
			fwrite(bdata, 1, font_info[i].width*font_info[i].height, dst);
			fclose(dst);
			free(bdata);
		}
		else
		{
			sprintf(dstname, "%08d.png", i);
			dst = fopen(dstname, "wb");
			fclose(dst);
			sprintf(dstname, "%08d.bin", i);
			dst = fopen(dstname, "wb");
			fclose(dst);
		}
	}
	fclose(src);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-BALDR HEART\n用于解压BH的fnt字体文件索引并将字模导出成png。\n将fnt文件拖到程序上。\nby Darkness-TX 2016.12.05\n\n");
	WritePngFile(argv[1]);
#ifdef DEBUG
	system("pause");
#endif // DEBUG
	return 0;
}