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
	unit8 *dst, *src, *odata, *udata, *ddata;
	height += 2;
	width += 2;
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
	src = new unit8[width*height];
	odata = new unit8[width*height];
	udata = new unit8[width*height];
	ddata = new unit8[width*height];
	memset(src, 0, width*height);
	memset(odata, 0, width*height);
	memset(udata, 0, width*height);
	memset(ddata, 0, width*height);
	for (i = 0; i < height - 2; i++)
	{
		memcpy(odata + width + i*width + 1, data + i*(width - 2), width - 2);
		memcpy(src + width + i*width + 1, data + i*(width - 2), width - 2);
	}
	memcpy(udata, odata + width, width*height - width);
	memcpy(ddata + width, odata, width*height - width);
	for (i = 0; i < width*height; i++)
	{
		if (udata[i] != 0xFF)
		{
			if (ddata[i] != 0xFF)
			{
				if (udata[i] + ddata[i] >= 0xFF)
					udata[i] = 0xFF;
				else
					udata[i] += ddata[i];
			}
			else
				udata[i] = 0xFF;
		}
	}
	for (i = 0; i < width*height; i++)
	{
		if (odata[i] != 0xFF)
		{
			if (udata[i] != 0xFF)
			{
				if (odata[i] + udata[i] >= 0xFF)
					odata[i] = 0xFF;
				else
					odata[i] += udata[i];
			}
			else
				odata[i] = 0xFF;
		}
	}
	memset(udata, 0, width*height);
	memset(ddata, 0, width*height);
	for (i = 0; i < height; i++)
	{
		memcpy(udata + i * width, odata + i * width + 1, width - 2);
		memcpy(ddata + i * width + 2, odata + i * width + 1, width - 2);
	}
	for (i = 0; i < width*height; i++)
	{
		if (udata[i] != 0xFF)
		{
			if (ddata[i] != 0xFF)
			{
				if (udata[i] + ddata[i] >= 0xFF)
					udata[i] = 0xFF;
				else
					udata[i] += ddata[i];
			}
			else
				udata[i] = 0xFF;
		}
	}
	for (i = 0; i < width*height; i++)
	{
		if (odata[i] != 0xFF)
		{
			if (udata[i] != 0xFF)
			{
				if (odata[i] + udata[i] >= 0xFF)
					odata[i] = 0xFF;
				else
					odata[i] += udata[i];
			}
			else
				odata[i] = 0xFF;
		}
	}
	for (i = 0; i < width*height; i++)
	{
		dst[i * 4] = src[i];
		dst[i * 4 + 1] = src[i];
		dst[i * 4 + 2] = src[i];
		dst[i * 4 + 3] = odata[i];
	}
	for (i = 0; i < height; i++)
		png_write_row(png_ptr, dst + i*width * 4);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	delete[] odata;
	delete[] udata;
	delete[] ddata;
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
			wprintf(L"ch:%lc size:%d width:%d height:%d x:%d y:%d cell:%d\n", tbl, (cb.bmp_width + 2)*(cb.bmp_height + 2), cb.bmp_width + 2, cb.bmp_height + 2, cb.bearingX, 26 - cb.bearingY - 1, cb.Advance + 2);
			fwprintf(tbl_xy, L"%d %d\n", cb.bearingX, 26 - cb.bearingY - 1);
			fwprintf(tbl_cell, L"%d\n", cb.Advance + 2);
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