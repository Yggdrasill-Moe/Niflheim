#include "font.h"

FONT::FONT(string fontname)
{
	if (!FONT::ReadIndex(fontname))
		printf("read index fail\n");
}

bool FONT::ReadIndex(string fontname)
{
	dirname = fontname.substr(0, fontname.find_last_of("."));
	fp = fopen(fontname.c_str(), "rb");
	DWORD savepos = 0;
	printf("read index...\n");
	if (fp)
	{
		for (DWORD i = 0; i < count; i++)
		{
			font_t findex;
			fread(&findex.offset, 1, 4, fp);
			savepos = ftell(fp);
			if (findex.offset == 0)
				memset(&findex.gmBlackBoxX, 0, sizeof(font_t) - 4);
			else
			{
				fseek(fp, findex.offset, SEEK_SET);
				fread(&findex.gmBlackBoxX, 1, sizeof(font_t) - 8, fp);
				if (findex.size != 0)
					findex.width = findex.size / findex.gmBlackBoxY;
				else
					findex.width = 0;
			}
			fseek(fp, savepos, SEEK_SET);
			findexs.push_back(findex);
		}
	}
	else
	{
		cout << "文件不存在";
		return false;
	}
	return true;
}

bool FONT::font2png(DWORD i)
{
	char dstname[200];
	DWORD k = 0;
	BYTE *dst, *src;
	if (findexs[i].size == 0)
		return true;
	sprintf(dstname, "%08d.png", i);
	FILE *pngfile = fopen(dstname, "wb");
	png_structp png_ptr;
	png_infop info_ptr;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	printf("offset:0x%X gmBlackBoxX:%d width:%d gmBlackBoxY:%d x:%d y:%d gmCellIncX:%d size:0x%X\n", findexs[i].offset, findexs[i].gmBlackBoxX, findexs[i].width, findexs[i].gmBlackBoxY, findexs[i].x, findexs[i].y, findexs[i].gmCellIncX, findexs[i].size);
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
	png_set_IHDR(png_ptr, info_ptr, findexs[i].width, findexs[i].gmBlackBoxY, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	dst = new BYTE[findexs[i].size * 4];
	src = new BYTE[findexs[i].size];
	fseek(fp, findexs[i].offset + sizeof(font_t) - 8, SEEK_SET);
	fread(src, 1, findexs[i].size, fp);

	for (k = 0; k < findexs[i].size; k++)
	{
		if (src[k] == 0)
		{
			dst[k * 4 + 0] = 0;
			dst[k * 4 + 1] = 0;
			dst[k * 4 + 2] = 0;
			dst[k * 4 + 3] = src[k];
		}
		else if (src[k] == 0x40)
		{
			dst[k * 4 + 0] = 0;
			dst[k * 4 + 1] = 0;
			dst[k * 4 + 2] = 0;
			dst[k * 4 + 3] = 0xFF;
		}
		else
		{
			dst[k * 4 + 0] = 0;
			dst[k * 4 + 1] = 0;
			dst[k * 4 + 2] = 0;
			dst[k * 4 + 3] = src[k] << 2;
		}
	}
	for (k = 0; k < findexs[i].gmBlackBoxY; k++)
		png_write_row(png_ptr, dst + k*findexs[i].width * 4);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	delete[] src;
	delete[] dst;
	fclose(pngfile);
	return true;
}

BYTE* FONT::makefont(WCHAR chText, DWORD i)
{
	BYTE* lpBuf;
	LOGFONT logfont;
	logfont.lfHeight = 26;
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
	//logfont.lfWeight = 700;
	wcscpy(logfont.lfFaceName, L"幼圆");
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
	DWORD NeedSize = GetGlyphOutline(hDC, chText, GGO_GRAY8_BITMAP, &gm, 0, NULL, &mat2);
	if (NeedSize > 0 && NeedSize < 0xFFFF)
	{
		lpBuf = new BYTE[NeedSize];
		GetGlyphOutline(hDC, chText, GGO_GRAY8_BITMAP, &gm, NeedSize, lpBuf, &mat2);
		findexs[i].gmBlackBoxX = gm.gmBlackBoxX;
		findexs[i].gmBlackBoxY = gm.gmBlackBoxY;
		findexs[i].gmCellIncX = gm.gmCellIncX;
		findexs[i].size = NeedSize;
		findexs[i].width = NeedSize / gm.gmBlackBoxY;
		findexs[i].x = gm.gmptGlyphOrigin.x;
		findexs[i].y = gm.gmptGlyphOrigin.y;
	}
	else
	{
		printf("所需大小错误！ size:0x%X\n index:%d", NeedSize, i);
		exit(0);
	}
	DeleteObject(hFont);
	DeleteDC(hDC);
	return lpBuf;
}

FONT::~FONT()
{
	if (fp)
		fclose(fp);
	findexs.clear();
}