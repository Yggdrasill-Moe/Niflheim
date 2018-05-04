#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <direct.h>
#include <png.h>

using namespace std;

#pragma pack (1)

typedef struct {
	char magic[4];
	DWORD filesize;
}fSG_header_t;

typedef struct {
	char magic[4];//cJPG
	DWORD size;//当前cJPG块的长度
	DWORD head_size;//cJPG头的长度
	DWORD data_length;//当前块的数据长度
	USHORT x_limit;//该图所处的坐标空间的最大宽度
	USHORT y_limit;//该图所处的坐标空间的最大高度
	USHORT x;//该图原点x的位置
	USHORT y;//该图原点y的位置
	USHORT width;
	USHORT height;
	USHORT unknown0;//0x48
	USHORT unknown1;//0x00
	DWORD seed;//解密用
} cJPG_header_t;

typedef struct {
	char magic[4];//cRGB
	DWORD size;//当前cRGB块的长度
	DWORD head_size;//cRGB头的长度
	DWORD data_length;//当前块的数据长度
	USHORT mode;//0, 1(bg), 2(with palette), 3(立绘), 4, etc
	USHORT unknown2;//type2: 0x8808
	USHORT x_limit;//该图所处的坐标空间的最大宽度
	USHORT y_limit;//该图所处的坐标空间的最大高度
	USHORT x;//该图原点x的位置
	USHORT y;//该图原点y的位置
	USHORT width;
	USHORT height;
	USHORT unknown4;//0x48
	USHORT bpp;//< 24就是cRGB块 0x18(with palette) or 0x20
	BYTE unknown[8];//0x08 0x18 0x08 0x10 0x08 0x08 0x08 0x00
} cRGB_header_t;

#pragma pack()

class sg
{
public:
	sg(string sgname);
	void sg_decode();
	~sg();

private:
	bool ReadHeader(string sgname);
	bool ReadSecHeader();
	void _cRGB_decode(BYTE *srcdata, BYTE *dstdata);
	bool _cRGB2png(FILE *file, BYTE *data);
	void _cJPG_decode(BYTE *srcdata, BYTE *dstdata);
	FILE *fp;
	fSG_header_t fSG_header;
	cJPG_header_t cJPG_header;
	cRGB_header_t cRGB_header;
	string filename;
	char sec_head[4];
	bool Header_OK;
};