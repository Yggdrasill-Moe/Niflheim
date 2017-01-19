#include "ft_make.h"
#include <stdio.h>
#include <direct.h>
#include <locale.h>
#include <png.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

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
	dst = new unit8[width*height * 4];
	src = data;
	for (i = 0, k = 0; i < width*height; i++, k++)
	{
		if (src[i] == 0)
		{
			dst[k * 4 + 0] = 0;
			dst[k * 4 + 1] = 0;
			dst[k * 4 + 2] = 0;
		}
		else
		{
			dst[k * 4 + 0] = 0xFF;
			dst[k * 4 + 1] = 0xFF;
			dst[k * 4 + 2] = 0xFF;
		}
		dst[k * 4 + 3] = src[i];
	}
	for (i = 0; i < height; i++)
		png_write_row(png_ptr, dst + i*width * 4);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	delete[] dst;
}

int main(int agrc, char* agrv[])
{
	setlocale(LC_ALL, "chs");
	wprintf(L"project：Niflheim-BALDR HEART\n用于生成fnt用的png。\n将码表txt文件拖到程序上。\nby Destinyの火狐 2017.01.19\n");
	if (agrc != 2)
		wprintf(L"\nUsage:fnt_make_bold_ft txtfile\n");
	else
	{
		unit32 slen, k = 0, i = 1577;
		char dstname[200];
		wchar_t tbl, data[256], *find;
		FILE *tbl_xy = fopen("tbl_xy.txt", "wt,ccs=UNICODE");
		FILE *tbl_cell = fopen("tbl_cell.txt", "wt,ccs=UNICODE");
		FILE *tbl_txt = fopen(agrv[1], "rt,ccs=UNICODE");
		FILE *pngfile;
		_mkdir("tbl_fnt");
		FT_Make ft("simhei.ttf", 24);
		_chdir("tbl_fnt");
		CharBitmap cb;
		while (fgetws(data, 256, tbl_txt) != NULL)
		{
			slen = wcslen(data);
			find = wcschr(data, L'=');
			tbl = data[find - data + 1];
			cb = ft.GetCharBitmap(tbl);
			sprintf(dstname, "%08d.png", i);
			pngfile = fopen(dstname, "wb");
			WritePng(pngfile, cb.bmp_width, cb.bmp_height, cb.bmpBuffer);
			wprintf(L"ch:%lc size:%d width:%d height:%d x:%d y:%d cell:%d\n", tbl, cb.bmp_width*cb.bmp_height, cb.bmp_width, cb.bmp_height, cb.bearingX, 24 - cb.bearingY, cb.Advance);
			fwprintf(tbl_xy, L"%d %d\n", cb.bearingX, 24 - cb.bearingY);
			fwprintf(tbl_cell, L"%d\n", cb.Advance);
			i++;
			fclose(pngfile);
		}
		fclose(tbl_txt);
		fclose(tbl_xy);
		fclose(tbl_cell);
		system("pause");
		return 0;
	}
}