#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <fstream>
#include <Windows.h>
#include <direct.h>
#include <png.h>
#include <ft2build.h>
#include FT_FREETYPE_H

using namespace std;

#pragma pack(1)

typedef struct _CharBitmap
{
	DWORD bmp_width;  //位图宽
	DWORD bmp_height; //位图高
	DWORD bearingX;   //用于水平文本排列，这是从当前光标位置到字形图像最左边的边界的水平距离
	DWORD bearingY;   //用于水平文本排列，这是从当前光标位置（位于基线）到字形图像最上边的边界的水平距离。 
	DWORD Advance;    //用于水平文本排列，当字形作为字符串的一部分被绘制时，这用来增加笔位置的水平距离
	BYTE* bmpBuffer; //象素数据
}CharBitmap;

#pragma pack()

class FT_Make
{
public:
	FT_Make(string font_path, DWORD font_height);
	CharBitmap GetCharBitmap(WCHAR *wchar);
	~FT_Make();
private:
	FT_Library library;
	FT_Face face;
	FT_Error error;
	bool FT_Init(string font_path, DWORD font_height);
};