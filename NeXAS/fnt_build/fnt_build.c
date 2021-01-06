/*
用于将png合成fnt
made by Darkness-TX
2016.12.20
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

unit8* ReadIndex(char *fname,unit8* fntname,unit32 *savepos)
{
	FILE *src;
	unit8 *cdata, *udata;
	unit32 i, fntnamesize;
	src = fopen(fname, "rb");
	fread(fnt_header.magic, 1, 4, src);
	if (strncmp(fnt_header.magic, "FNT\0", 4) != 0)
	{
		printf("文件头不是FNT\0!");
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
			exit(0);
		}
		else if (fnt_header.flag != 1)
		{
			printf("flag不为1!");
			exit(0);
		}
		if (fnt_header.fontflag == 0x103)
			while (fgetc(src) != '\0');
		fntnamesize = ftell(src) - 0x11;
		fseek(src, 0x11, SEEK_SET);
		fread(fntname, 1, fntnamesize, src);
	}
	fread(&fnt_header.width, 1, 4, src);
	fread(&fnt_header.height, 1, 4, src);
	*savepos = ftell(src);
	fread(&fnt_header.decompsize, 1, 4, src);
	if ((fnt_header.decompsize & 0xFF00) == 0xFF00)
	{
		fnt_header.seekflag = fnt_header.decompsize;
		fseek(src, 9, SEEK_CUR);
		fread(&fnt_header.decompsize, 1, 4, src);
	}
	fread(&fnt_header.compsize, 1, 4, src);
	cdata = malloc(fnt_header.compsize);
	udata = malloc(fnt_header.decompsize);
	fread(cdata, 1, fnt_header.compsize, src);
	uncompress(udata, &fnt_header.decompsize, cdata, fnt_header.compsize);
	free(cdata);
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
	return udata;
}

 unit8* ReadPng(FILE *OpenPng, unit32 *width, unit32 *height)
{
	png_structp png_ptr;
	png_infop info_ptr, end_ptr;
	png_bytep *rows;
	unit32 i = 0, bpp = 0, format = 0, llen;
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
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)width, (png_uint_32*)height, &bpp, &format, NULL, NULL, NULL);
	rows = (png_bytep*)malloc(*height * sizeof(char*));
	TexData = malloc(*height * *width * 4);
	llen = png_get_rowbytes(png_ptr, info_ptr);
	for (i = 0; i < *height; i++)
		rows[i] = (png_bytep)(TexData + llen*i);
	png_read_image(png_ptr, rows);
	free(rows);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
	return TexData;
}

void WriteFntFile(char *fname)
{
	FILE *src, *dst, *tbl_xy, *tbl_cell;
	unit8 *udata, *cdata, *pdata, *bdata, dstname[200], fntname[32];
	unit32 savepos, width, height, i, k, offset = 0;
	wchar_t data[256], tbl_x[5], tbl_y[5];
	udata = ReadIndex(fname, fntname, &savepos);
	sprintf(dstname, "%s_new", fname);
	dst = fopen(dstname, "wb");
	tbl_xy = fopen("tbl_xy.txt", "rt,ccs=UNICODE");
	tbl_cell = fopen("tbl_cell.txt", "rt,ccs=UNICODE");
	sprintf(dstname, "%s_unpack", fname);
	_chdir(dstname);
	for (i = 0; i < font_count; i++)
	{
		sprintf(dstname, "%08d.png", i);
		src = fopen(dstname, "rb");
		fseek(src, 0, SEEK_END);
		if (ftell(src) == 0)
		{
			font_info[i].width = 0;
			font_info[i].height = 0;
			font_info[i].offset = 0;
		}
		else
		{
			fseek(src, 0, SEEK_SET);
			pdata = ReadPng(src, &width, &height);
			bdata = malloc(width*height * 2);
			font_info[i].width = width;
			font_info[i].height = height;
			font_info[i].offset = offset;
			for (k = 0; k < width*height; k++)
			{
				bdata[k * 2] = pdata[k * 4];
				bdata[k * 2 + 1] = pdata[k * 4 + 3];
			}
			free(pdata);
			fwrite(bdata, 1, width*height * 2, dst);
			offset += width*height * 2;
			free(bdata);
		}
		fclose(src);
	}
	fclose(dst);
	_chdir("..");
	sprintf(dstname, "%s_new", fname);
	dst = fopen(dstname, "rb");
	bdata = malloc(offset);
	fread(bdata, 1, offset, dst);
	fclose(dst);
	dst = fopen(dstname, "wb");
	fwrite(fnt_header.magic, 1, 4, dst);
	if (strncmp(fname, "systemascii", 11) != 0 && strncmp(fname, "systemtutorial", 14) != 0 && strncmp(fname, "system10b", 9))
	{
		fwrite(fnt_header.magic2, 1, 9, dst);
		fwrite(&fnt_header.flag, 1, 2, dst);
		fwrite(&fnt_header.fontflag, 1, 2, dst);
	}
	if (fnt_header.fontflag == 0x103)
	{
		fprintf(dst, "%s", fntname);
		fputc(0, dst);
	}
	fwrite(&fnt_header.width, 1, 4, dst);
	fwrite(&fnt_header.height, 1, 4, dst);
	if ((fnt_header.seekflag & 0xFF00) == 0xFF00)
	{
		fwrite(&fnt_header.seekflag, 1, 4, dst);
		fseek(dst, 9, SEEK_CUR);
	}
	if (fnt_header.fontflag == 0x103)
	{
		for (i = 0; i < font_count; i++)
		{
			memcpy(&udata[i * 0x10 + 0xC], &font_info[i].offset, 4);
			memcpy(&udata[i * 0x10 + 0x4], &font_info[i].width, 2);
			memcpy(&udata[i * 0x10 + 0x6], &font_info[i].height, 2);
			if (i >= 1577)
			{
				if(tbl_xy != NULL)
					if (fgetws(data, 256, tbl_xy) != NULL)
					{
						memset(tbl_x, 0, 5 * 2);
						memset(tbl_y, 0, 5 * 2);
						wcsncpy(tbl_x, data, wcschr(data, L' ') - data);
						wcscpy(tbl_y, wcschr(data, L' ') + 1);
						font_info[i].x = (short)_wtoi(tbl_x);
						font_info[i].y = (short)_wtoi(tbl_y);
					}
				if(tbl_cell != NULL)
					if (fgetws(data, 256, tbl_cell) != NULL)
						font_info[i].cell = (short)_wtoi(data);
			}
			memcpy(&udata[i * 0x10], &font_info[i].x, 2);
			memcpy(&udata[i * 0x10 + 0x2], &font_info[i].y, 2);
			memcpy(&udata[i * 0x10 + 0x8], &font_info[i].cell, 2);
			printf("fntnum:%d width:%d height:%d fntoffset:0x%X x:%d y:%d cell:%d\n", i, font_info[i].width, font_info[i].height, font_info[i].offset, font_info[i].x, font_info[i].y, font_info[i].cell);
		}
	}
	else
	{
		for (i = 0; i < font_count; i++)
		{
			if (i >= 1577)
			{
				if (tbl_xy != NULL)
					if (fgetws(data, 256, tbl_xy) != NULL)
					{
						memset(tbl_x, 0, 5 * 2);
						memset(tbl_y, 0, 5 * 2);
						wcsncpy(tbl_x, data, wcschr(data, L' ') - data);
						wcscpy(tbl_y, wcschr(data, L' ') + 1);
						font_info[i].x = (short)_wtoi(tbl_x);
						font_info[i].y = (short)_wtoi(tbl_y);
					}
			}
			memcpy(&udata[i * 0xC], &font_info[i], 0xC);
			printf("fntnum:%d width:%d height:%d fntoffset:0x%X\n", i, font_info[i].width, font_info[i].height, font_info[i].offset);
		}
	}
	cdata = malloc(fnt_header.decompsize * 2);
	compress2(cdata, &fnt_header.compsize, udata, fnt_header.decompsize, Z_DEFAULT_COMPRESSION);
	sprintf(dstname, "%s_new_INDEX", fname);
	src = fopen(dstname, "wb");
	fwrite(udata, 1, fnt_header.decompsize, src);
	fclose(src);
	fwrite(&fnt_header.decompsize, 1, 4, dst);
	fwrite(&fnt_header.compsize, 1, 4, dst);
	fwrite(cdata, 1, fnt_header.compsize, dst);
	free(cdata);
	free(udata);
	fwrite(bdata, 1, offset, dst);
	free(bdata);
	fclose(dst);
	
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-BALDR HEART\n用于将png合成fnt。\n将fnt文件拖到程序上。\nby Darkness-TX 2016.12.20\n\n");
	WriteFntFile(argv[1]);
#ifdef DEBUG
	system("pause");
#endif // DEBUG
	return 0;
}