#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <direct.h>
#include <png.h>

using namespace std;

#pragma pack(1)

typedef struct font_s
{
	//其实就是GetGlyphOutline，GGO_GRAY8_BITMAP
	DWORD offset;		//字模所在地址
	DWORD gmBlackBoxX;	//字模宽
	DWORD gmBlackBoxY;	//字模高
	DWORD x;			//gmptGlyphOrigin.x
	DWORD y;			//gmptGlyphOrigin.y，从下往上
	DWORD gmCellIncX;	//方框宽
	DWORD size;			//字模占用字节
	DWORD width;		//用size / gmBlackBoxY得到，转成图片后的宽
}font_t;

struct char_s
{
	//不知是啥，猜的
	DWORD height;
	DWORD width;
	DWORD font_type;
};

#pragma pack()

class FONT
{
public:
	FONT(string fontname);
	bool font2png(DWORD i);
	BYTE* makefont(WCHAR chText, DWORD i);
	~FONT();
	FILE *fp;
	string dirname;
	DWORD count = 0x6D92;
	DWORD tbl_start = 2199;
	vector<font_t> findexs;
private:
	bool ReadIndex(string fontname);
};