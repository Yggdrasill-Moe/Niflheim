/*
用于生成fnt用的png
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
	dst = malloc(width*height * 4);
	src = data;
	for (i = 0, k = 0; i < width*height; i++, k++)
	{
		if (src[i] == 0)
		{
			dst[k * 4 + 0] = 0xFF;
			dst[k * 4 + 1] = 0xFF;
			dst[k * 4 + 2] = 0xFF;
			dst[k * 4 + 3] = src[i];
		}
		else
		{
			dst[k * 4 + 0] = 0xFF;
			dst[k * 4 + 1] = 0xFF;
			dst[k * 4 + 2] = 0xFF;
			dst[k * 4 + 3] = src[i]+0xB0;
		}
	}
	for (i = 0; i < height; i++)
		png_write_row(png_ptr, dst + i*width * 4);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	free(dst);
}

void FontGlyph(wchar_t chText, unit32 i)
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
			wprintf(L"char:%lc width:%d height:%d\n", chText, NeedSize / gm.gmBlackBoxY, gm.gmBlackBoxY);
			WritePng(fp, NeedSize / gm.gmBlackBoxY, gm.gmBlackBoxY, lpBuf);
			fclose(fp);
			free(lpBuf);
		}
	}
	else
		wprintf(L"所需大小错误！ size:0x%X\n", NeedSize);
	DeleteObject(hFont);
	DeleteDC(hDC);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-BALDR HEART\n用于将字模导出成png。\n将txt文件拖到程序上。\nby Darkness-TX 2016.12.20\n\n");
	unit32 slen, k = 0, i = 1577;
	wchar_t tbl, data[256], *find;
	FILE *Tbl = fopen(argv[1], "rt,ccs=UNICODE");
	_mkdir("fnt");
	_chdir("fnt");
	while (fgetws(data, 256, Tbl) != NULL)
	{
		slen = wcslen(data);
		find = wcschr(data, L'=');
		tbl = data[find - data + 1];
		FontGlyph(tbl, i);
		i++;
	}
	system("pause");
	return 0;
}