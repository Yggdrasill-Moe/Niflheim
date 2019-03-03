#include "font.h"

FONT::FONT(string fontname)
{
	if (!FONT::ReadIndex(fontname))
		printf("read index fail!\n");
}

void FONT::GetFontCount(FILE *src)
{
	DWORD count = 0;
	do
	{
		fread(&count, 4, 1, src);
	} while (count == 0);
	FONT::count = count / 4;
	fseek(src, 0, SEEK_SET);
}

bool FONT::ReadIndex(string fontname)
{
	dirname = fontname.substr(0, fontname.find_last_of("."));
	fp = fopen(fontname.c_str(), "rb");
	FONT::GetFontCount(fp);
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
					findex.width = findex.size / findex.height;
				else
					findex.width = 0;
			}
			fseek(fp, savepos, SEEK_SET);
			findexs.push_back(findex);
		}
	}
	else
	{
		cout << "file read error.\n";
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
	printf("No.%08d offset:0x%X width:%d height:%d x:%d y:%d gmBlackBoxX:%d advance:%d size:0x%X\n",i, findexs[i].offset, findexs[i].width, findexs[i].height, findexs[i].x, findexs[i].y, findexs[i].gmBlackBoxX, findexs[i].advance, findexs[i].size);
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		printf("PNG create error!\n");
		exit(0);
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		printf("info create error!\n");
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		exit(0);
	}
	png_init_io(png_ptr, pngfile);
	png_set_IHDR(png_ptr, info_ptr, findexs[i].width, findexs[i].height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
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
	for (k = 0; k < findexs[i].height; k++)
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
	BYTE *lpBuf, *data;
	LOGFONT logfont;
	logfont.lfHeight = 35;
	logfont.lfWidth = 0;
	logfont.lfEscapement = 0;
	logfont.lfOrientation = 0;
	logfont.lfItalic = 0;
	logfont.lfUnderline = 0;
	logfont.lfStrikeOut = 0;
	logfont.lfCharSet = GB2312_CHARSET;
	logfont.lfOutPrecision = 0;
	logfont.lfClipPrecision = 0;
	logfont.lfQuality = CLEARTYPE_NATURAL_QUALITY;
	logfont.lfPitchAndFamily = 0;
	logfont.lfWeight = 500;
	wcscpy(logfont.lfFaceName, L"方正准圆_GBK");
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
	DWORD NeedSize = GetGlyphOutlineW(hDC, chText, GGO_GRAY8_BITMAP, &gm, 0, NULL, &mat2);
	if (NeedSize > 0 && NeedSize < 0xFFFF)
	{
		lpBuf = new BYTE[NeedSize];
		GetGlyphOutlineW(hDC, chText, GGO_GRAY8_BITMAP, &gm, NeedSize, lpBuf, &mat2);
		findexs[i].gmBlackBoxX = NeedSize / gm.gmBlackBoxY;
		findexs[i].height = gm.gmBlackBoxY;
		findexs[i].x = gm.gmptGlyphOrigin.x;
		findexs[i].y = gm.gmptGlyphOrigin.y;
		findexs[i].advance = gm.gmCellIncX;
		if (findexs[i].gmBlackBoxX % 4)//宽度要4字节对齐
			findexs[i].width = findexs[i].gmBlackBoxX + 4 - (findexs[i].gmBlackBoxX % 4);
		else
			findexs[i].width = findexs[i].gmBlackBoxX;
		findexs[i].size = findexs[i].width * gm.gmBlackBoxY;
		data = new BYTE[findexs[i].size];
		memset(data, 0, findexs[i].size);
		for (DWORD k = 0; k < findexs[i].height; k++)
			for (DWORD j = 0; j < findexs[i].gmBlackBoxX; j++)
				data[k * findexs[i].width + j] = lpBuf[k * findexs[i].gmBlackBoxX + j];
		printf("No.%08d offset::0x%X width:%d height:%d x:%d y:%d gmBlackBoxX:%d advance:%d size:0x%X\n", i, findexs[i].offset, findexs[i].width, findexs[i].height, findexs[i].x, findexs[i].y, findexs[i].gmBlackBoxX, findexs[i].advance, findexs[i].size);
		if (NeedSize % gm.gmBlackBoxY)
		{
			printf("No.%08d的gmBlackBoxY不可信任，建议切换成-fi模式。\n",i);
			system("pause");
		}
		delete[] lpBuf;
	}
	else
	{
		printf("所需大小错误！ size:0x%X index:%d\n", NeedSize, i);
		exit(0);
	}
	DeleteObject(hFont);
	DeleteDC(hDC);
	return data;
}

bool FONT::FT_Init(string font_path, DWORD font_height)
{
	FT_Error error;
	ifstream infile;
	infile.open(font_path, ios::in | ios::binary | ios::ate);
	if (!infile.is_open())
	{
		cout << font_path + " open fail!\n";
		exit(0);
	}
	DWORD filesize = (DWORD)infile.tellg();
	infile.seekg(0, ios::beg);
	char* buff = new char[filesize];
	infile.read(buff, filesize);
	infile.close();
	error = FT_Init_FreeType(&library);
	if (error)
	{
		cout << "Init_FreeType Error!\n";
		return false;
	}
	cout << "read ttf...\n";
	error = FT_New_Memory_Face(library, (BYTE *)buff, filesize, 0, &face);
	if (error)
	{
		cout << "New_Memory_Face Error!\n";
		return false;
	}
	error = FT_Set_Char_Size(face, font_height * 64, font_height * 64, 0, 0);
	if (error)
	{
		cout << "Set_Char_Size Error!\n";
		return false;
	}
	error = FT_Set_Pixel_Sizes(face, font_height, font_height);
	if (error)
	{
		cout << "Set_Pixel_Sizes Error!\n";
		return false;
	}
	return true;
}

BYTE* FONT::makefont_ft(WCHAR chText, DWORD i)
{
	FT_GlyphSlot slot = face->glyph;
	FT_Error error;
	FT_Bitmap bmp;
	//似乎加不加都一样，可能要FT_Outline_Embolden，不过现在效果足够了
	FT_Bitmap_Embolden(library, &face->glyph->bitmap, 48 << 6, 48 << 6);
	error = FT_Load_Char(face, chText, FT_LOAD_RENDER);
	if (error)
	{
		cout << "Load_Char Error!\n";
		exit(0);
	}
	bmp = slot->bitmap;
	findexs[i].gmBlackBoxX = bmp.width;
	findexs[i].height = bmp.rows;
	findexs[i].x = slot->bitmap_left;
	findexs[i].y = slot->bitmap_top;
	findexs[i].advance = slot->advance.x / 64;
	if (bmp.width % 4)//宽度要4字节对齐
		findexs[i].width = bmp.width + 4 - (bmp.width % 4);
	else
		findexs[i].width = bmp.width;
	findexs[i].size = findexs[i].width * bmp.rows;
	printf("No.%08d offset::0x%X width:%d height:%d x:%d y:%d gmBlackBoxX:%d advance:%d size:0x%X\n", i, findexs[i].offset, findexs[i].width, findexs[i].height, findexs[i].x, findexs[i].y, findexs[i].gmBlackBoxX, findexs[i].advance, findexs[i].size);
	BYTE *data = new BYTE[findexs[i].size];
	memset(data, 0, findexs[i].size);
	for (DWORD k = 0; k < bmp.rows; k++)
		for (DWORD j = 0; j < bmp.width; j++)
			data[k * findexs[i].width + j] = bmp.buffer[k * bmp.width + j] >> 2;
	return data;
}

FONT::~FONT()
{
	if (fp)
		fclose(fp);
	findexs.clear();
}