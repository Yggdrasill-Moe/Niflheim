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
}fAGS_fHKQ_header_t;

typedef struct {
	char magic[4];
	DWORD data_length;
	DWORD head_size;
	DWORD seed;
}cTEX_header_t;

typedef struct {
	char magic[4];
	DWORD data_length;
	DWORD head_size;
	DWORD seed;
}cFNM_header_t;

typedef struct {
	char magic[4];
	DWORD data_length;
	DWORD head_size;
	DWORD code_length;
	DWORD seed;
	DWORD mode;//?
	DWORD code_count;//code_length + count * 4 = data_length - head_size,地址是解密后的code_length+head_size处,表示含有文本的opcode组数（当然有几个文件会有例外）
}cCOD_header_t;

typedef struct {
	char magic[4];
	DWORD data_length;
	DWORD head_size;
	DWORD seed;
	DWORD code_count;
}cQZT_header_t;

#pragma pack()

class AGS
{
public:
	AGS(string agsname);
	void AGS_decode();
	~AGS();

private:
	bool ReadHeader(string agsname);
	void _cTEX_decode(BYTE *srcdata, BYTE *dstdata);
	void _cFNM_decode(BYTE *srcdata, BYTE *dstdata);
	void _cCOD_decode(BYTE *srcdata, BYTE *dstdata);
	void _cQZT_decode(BYTE *srcdata, BYTE *dstdata);
	FILE *fp;
	fAGS_fHKQ_header_t fAGS_fHKQ_header;
	cTEX_header_t cTEX_header;
	cFNM_header_t cFNM_header;
	cCOD_header_t cCOD_header;
	cQZT_header_t cQZT_header;
	string filename;
	bool Header_OK;
};