/*
用于生成fnt(描边类型)用的png
made by Darkness-TX
2017.01.06
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
	dst = malloc(width*height * 4);
	src = malloc(width*height);
	odata = malloc(width*height);
	udata = malloc(width*height);
	ddata = malloc(width*height);
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
		memcpy(ddata + i * width + 2, odata + i * width +1, width - 2);
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
	free(odata);
	free(udata);
	free(ddata);
	free(dst);
}

GLYPHMETRICS FontGlyph(wchar_t chText, unit32 i)
{
	unit8 dstname[200];
	LOGFONT logfont;
	logfont.lfHeight = 24;
	logfont.lfWidth = 0;
	logfont.lfEscapement = 0;
	logfont.lfOrientation = 0;
	logfont.lfItalic = 0;
	logfont.lfUnderline = 0;
	logfont.lfStrikeOut = 0;
	logfont.lfCharSet = GB2312_CHARSET;
	logfont.lfOutPrecision = 0;
	logfont.lfClipPrecision = 0;
	logfont.lfQuality = 0;
	logfont.lfPitchAndFamily = 0;
	wcscpy(logfont.lfFaceName, L"黑体");
	HFONT hFont = CreateFontIndirect(&logfont);
	HDC hDC = CreateCompatibleDC(NULL);;
	SelectObject(hDC, hFont);
	MAT2 mat2;
	mat2.eM11.value = 1;
	mat2.eM11.fract = 0;
	mat2.eM12.value = 0;
	mat2.eM12.fract = 0;
	mat2.eM21.value = 0;
	mat2.eM21.fract = 0;
	mat2.eM22.value = 1;
	mat2.eM22.fract = 0;
	GLYPHMETRICS gm;
	unit32 NeedSize = GetGlyphOutline(hDC, chText, GGO_GRAY8_BITMAP, &gm, 0, NULL, &mat2);
	if (NeedSize > 0 && NeedSize < 0xFFFF)
	{
		unit8* lpBuf = malloc(NeedSize);
		if (lpBuf)
		{
			GetGlyphOutline(hDC, chText, GGO_GRAY8_BITMAP, &gm, NeedSize, lpBuf, &mat2);
			sprintf(dstname, "%08d.png", i);
			FILE *fp = fopen(dstname, "wb");
			wprintf(L"ch:%lc size:%d width:%d height:%d x:%d y:%d cell:%d\n", chText, NeedSize, NeedSize / gm.gmBlackBoxY, gm.gmBlackBoxY, gm.gmptGlyphOrigin.x, 24 - gm.gmptGlyphOrigin.y - 2, gm.gmCellIncX + 2);
			for (unit32 j = 0; j < NeedSize; j++)
				if (lpBuf[j] == 0)
					lpBuf[j] = 0;
				else if (lpBuf[j] == 0x40)
					lpBuf[j] = 0xFF;
				else
					lpBuf[j] <<= 2;
			WritePng(fp, NeedSize / gm.gmBlackBoxY, gm.gmBlackBoxY, lpBuf);
			//fwrite(lpBuf, 1, NeedSize, fp);
			fclose(fp);
			free(lpBuf);
		}
	}
	else
		wprintf(L"所需大小错误！ size:0x%X\n", NeedSize);
	DeleteObject(hFont);
	DeleteDC(hDC);
	return gm;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-BALDR HEART\n用于生成fnt(描边类型)用的png。\n将码表txt文件拖到程序上。\nby Darkness-TX 2017.01.06\n\n");
	unit32 slen, k = 0, i = 1577;
	wchar_t tbl, data[256], *find;
	FILE *tbl_xy = fopen("tbl_xy.txt", "wt,ccs=UNICODE");
	FILE *tbl_cell = fopen("tbl_cell.txt", "wt,ccs=UNICODE");
	FILE *tbl_txt = fopen(argv[1], "rt,ccs=UNICODE");
	GLYPHMETRICS gm;
	_mkdir("tbl_fnt");
	_chdir("tbl_fnt");
	while (fgetws(data, 256, tbl_txt) != NULL)
	{
		slen = wcslen(data);
		find = wcschr(data, L'=');
		tbl = data[find - data + 1];
		gm = FontGlyph(tbl, i);
		fwprintf(tbl_xy, L"%d %d\n", gm.gmptGlyphOrigin.x, 24 - gm.gmptGlyphOrigin.y - 2);
		fwprintf(tbl_cell, L"%d\n", gm.gmCellIncX + 2);
		i++;
	}
	fclose(tbl_txt);
	fclose(tbl_xy);
	fclose(tbl_cell);
	system("pause");
	return 0;
}