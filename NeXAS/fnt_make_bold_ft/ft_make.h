#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <fstream>
#include <locale>
#include <Windows.h>
#include <direct.h>
#include <io.h>
#include <png.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_BITMAP_H

using namespace std;

#pragma pack(1)

typedef struct _CharBitmap
{
	DWORD bmp_width;
	DWORD bmp_height;
	DWORD bearingX;
	DWORD bearingY;
	DWORD Advance;
	BYTE* bmpBuffer;
}CharBitmap;

#pragma pack()

class FT_Make
{
public:
	FT_Make(string font_path, DWORD font_height, DWORD font_width);
	CharBitmap GetCharBitmap(WCHAR wchar);
	~FT_Make();
private:
	FT_Library library;
	FT_Face face;
	FT_Error error;
	bool FT_Init(string font_path, DWORD font_height, DWORD font_width);
};
BYTE* BuildOutline(DWORD width, DWORD height, BYTE* data, bool do_delete);
void WritePng(FILE *pngfile, DWORD width, DWORD height, DWORD p_count, DWORD interval, DWORD gradient, DWORD fill, BYTE* data);