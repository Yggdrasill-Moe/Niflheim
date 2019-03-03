#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <direct.h>
#include <png.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_BITMAP_H

using namespace std;

#pragma pack(1)

typedef struct font_s
{
	//其实就是GetGlyphOutline，GGO_GRAY8_BITMAP
	DWORD offset;		//字模所在地址
	DWORD gmBlackBoxX;	//字宽
	DWORD height;		//图片高
	DWORD x;			//gmptGlyphOrigin.x
	DWORD y;			//gmptGlyphOrigin.y，从下往上
	DWORD advance;		//未知，暂定方框宽，有点类似Freetype中FT_Bitmap中的advance概念？
	DWORD size;			//字模占用字节
	DWORD width;		//用size / gmBlackBoxY得到，转成图片后的宽
}font_t;

#pragma pack()

class FONT
{
public:
	FONT(string fontname);
	bool font2png(DWORD i);
	BYTE* makefont(WCHAR chText, DWORD i);
	bool FT_Init(string font_path, DWORD font_height);
	BYTE* makefont_ft(WCHAR chText, DWORD i);
	~FONT();
	FILE *fp;
	string dirname;
	DWORD count;
	DWORD tbl_start = 2199;
	vector<font_t> findexs;
private:
	FT_Library library;
	FT_Face face;
	FT_Error error;
	bool ReadIndex(string fontname);
	void GetFontCount(FILE *src);
};