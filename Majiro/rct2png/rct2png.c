#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <Windows.h>
#include <locale.h>
#include <io.h>
#include <png.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

struct rct_header
{
	unit32 signature;//0x9A925A98
	unit8 magic[4];//前两字节是TS代表加密的，需要密钥，TC没有加密可以直接解，后两个字节为version，00或01，01版本的图片多为人物表情之类的，需要和00版本的图片组合
	unit32 width;
	unit32 height;
	unit32 size;
}RCT_Header;

struct rc8_header
{
	unit32 signature;//0x9A925A98
	unit8 magic[4];//8_00
	unit32 width;
	unit32 height;
	unit32 size;
}RC8_Header;

unit32 rct_decompress(unit8 *uncompr, unit32 uncomprLen, unit8 *compr, unit32 comprLen, unit32 width)
{
	unit32 i = 0;
	unit32 act_uncomprLen = 0;
	unit32 curByte = 0;
	unit32 pos[32];
	pos[0] = -3;
	pos[1] = -6;
	pos[2] = -9;
	pos[3] = -12;
	pos[4] = -15;
	pos[5] = -18;
	pos[6] = (3 - width) * 3;
	pos[7] = (2 - width) * 3;
	pos[8] = (1 - width) * 3;
	pos[9] = (0 - width) * 3;
	pos[10] = (-1 - width) * 3;
	pos[11] = (-2 - width) * 3;
	pos[12] = (-3 - width) * 3;
	pos[13] = 9 - ((width * 3) << 1);
	pos[14] = 6 - ((width * 3) << 1);
	pos[15] = 3 - ((width * 3) << 1);
	pos[16] = 0 - ((width * 3) << 1);
	pos[17] = -3 - ((width * 3) << 1);
	pos[18] = -6 - ((width * 3) << 1);
	pos[19] = -9 - ((width * 3) << 1);
	pos[20] = 9 - width * 9;
	pos[21] = 6 - width * 9;
	pos[22] = 3 - width * 9;
	pos[23] = 0 - width * 9;
	pos[24] = -3 - width * 9;
	pos[25] = -6 - width * 9;
	pos[26] = -9 - width * 9;
	pos[27] = 6 - ((width * 3) << 2);
	pos[28] = 3 - ((width * 3) << 2);
	pos[29] = 0 - ((width * 3) << 2);
	pos[30] = -3 - ((width * 3) << 2);
	pos[31] = -6 - ((width * 3) << 2);
	uncompr[act_uncomprLen++] = compr[curByte++];
	uncompr[act_uncomprLen++] = compr[curByte++];
	uncompr[act_uncomprLen++] = compr[curByte++];
	while (1)
	{
		unit8 flag;
		unit32 copy_bytes, copy_pos;
		if (curByte >= comprLen)
			break;
		flag = compr[curByte++];
		if (!(flag & 0x80))
		{
			if (flag != 0x7F)
				copy_bytes = flag * 3 + 3;
			else
			{
				if (curByte + 1 >= comprLen)
					break;
				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0x80;
				copy_bytes *= 3;
			}
			if (curByte + copy_bytes - 1 >= comprLen)
				break;
			if (act_uncomprLen + copy_bytes - 1 >= uncomprLen)
				break;
			memcpy(&uncompr[act_uncomprLen], &compr[curByte], copy_bytes);
			act_uncomprLen += copy_bytes;
			curByte += copy_bytes;
		}
		else
		{
			copy_bytes = flag & 3;
			copy_pos = (flag >> 2) & 0x1F;
			if (copy_bytes != 3)
				copy_bytes = copy_bytes * 3 + 3;
			else
			{
				if (curByte + 1 >= comprLen)
					break;
				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 4;
				copy_bytes *= 3;
			}
			for (i = 0; i < copy_bytes; i++)
			{
				if (act_uncomprLen >= uncomprLen)
					if (curByte != comprLen)
					{
						printf("compr miss-match %d VS %d\n", curByte, comprLen);
						system("pause");
					}
				uncompr[act_uncomprLen] = uncompr[act_uncomprLen + pos[copy_pos]];
				act_uncomprLen++;
			}
		}
	}
	return act_uncomprLen;
}

unit32 rc8_decompress(unit8 *uncompr, unit32 uncomprLen, unit8 *compr, unit32 comprLen, unit32 width)
{
	unit32 act_uncomprLen = 0;
	unit32 curByte = 0;
	unit32 pos[16];
	pos[0] = -1;
	pos[1] = -2;
	pos[2] = -3;
	pos[3] = -4;
	pos[4] = 3 - width;
	pos[5] = 2 - width;
	pos[6] = 1 - width;
	pos[7] = 0 - width;
	pos[8] = -1 - width;
	pos[9] = -2 - width;
	pos[10] = -3 - width;
	pos[11] = 2 - (width * 2);
	pos[12] = 1 - (width * 2);
	pos[13] = (0 - width) << 1;
	pos[14] = -1 - (width * 2);
	pos[15] = (-1 - width) * 2;
	uncompr[act_uncomprLen++] = compr[curByte++];
	while (1)
	{
		unit8 flag;
		unit32 copy_bytes, copy_pos;
		if (curByte >= comprLen)
			break;
		flag = compr[curByte++];
		if (!(flag & 0x80))
		{
			if (flag != 0x7F)
				copy_bytes = flag + 1;
			else
			{
				if (curByte + 1 >= comprLen)
					break;
				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0x80;
			}
			if (curByte + copy_bytes - 1 >= comprLen)
				break;
			if (act_uncomprLen + copy_bytes - 1 >= uncomprLen)
				break;
			memcpy(&uncompr[act_uncomprLen], &compr[curByte], copy_bytes);
			act_uncomprLen += copy_bytes;
			curByte += copy_bytes;
		}
		else
		{
			copy_bytes = flag & 7;
			copy_pos = (flag >> 3) & 0xF;
			if (copy_bytes != 7)
				copy_bytes += 3;
			else
			{
				if (curByte + 1 >= comprLen)
					break;
				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0xA;
			}
			for (unsigned int i = 0; i < copy_bytes; i++)
			{
				if (act_uncomprLen >= uncomprLen)
					break;
				uncompr[act_uncomprLen] = uncompr[act_uncomprLen + pos[copy_pos]];
				act_uncomprLen++;
			}
		}
	}
	return act_uncomprLen;
}

void WritePng(FILE *pngfile, unit32 width, unit32 height, unit32 bpp, unit8 *bitmapdata, unit8 *alphadata)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unit32 i = 0;
	unit8 *data = NULL;
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
	if (bpp == 32)
	{
		png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		data = malloc(width * height * 4);
		for (i = 0; i < width * height; i++)
		{
			data[i * 4] = bitmapdata[i * 3];
			data[i * 4 + 1] = bitmapdata[i * 3 + 1];
			data[i * 4 + 2] = bitmapdata[i * 3 + 2];
			//试了下，取反就能正常显示
			data[i * 4 + 3] = ~alphadata[i * 3];
		}
	}
	else if (bpp == 24)
	{
		png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		data = malloc(width * height * 3);
		memcpy(data, bitmapdata, width * height * 3);
	}
	png_write_info(png_ptr, info_ptr);
	if (bpp == 32)
		for (i = 0; i < height; i++)
			png_write_row(png_ptr, data + i*width * 4);
	else if (bpp == 24)
		for (i = 0; i < height; i++)
			png_write_row(png_ptr, data + i*width * 3);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	free(data);
}

BOOL RC8_Check(FILE *src)
{
	fread(&RC8_Header, sizeof(RC8_Header), 1, src);
	if (RC8_Header.signature != 0x9A925A98)
	{
		printf("rc8文件头不是0x9A925A98！\n");
		system("pause");
		return FALSE;
	}
	if (RC8_Header.height == 0 || RCT_Header.width == 0 || RCT_Header.size == 0)
	{
		printf("rc8文件头数据有零值！\n");
		system("pause");
		return FALSE;
	}
	if (strncmp(RC8_Header.magic, "8_00", 4))
	{
		printf("未知的rc8文件magic!\n");
		system("pause");
		return FALSE;
	}
	return TRUE;
}

void TC00_Export(FILE *src, wchar_t *fname)
{
	FILE *dst = NULL, *alpha = NULL;
	unit8 *cdata = NULL, *udata = NULL, *rc8_data = NULL, *rc8_alpha = NULL, *alphadata = NULL;
	unit32 uncomprLen = 0,i = 0;
	int pos = 0;
	wchar_t dstname[MAX_PATH], buff[MAX_PATH];
	cdata = malloc(RCT_Header.size);
	uncomprLen = RCT_Header.height * RCT_Header.width * 3;
	udata = malloc(uncomprLen);
	fread(cdata, RCT_Header.size, 1, src);
	rct_decompress(udata, uncomprLen, cdata, RCT_Header.size, RCT_Header.width);
	free(cdata);
	pos = wcsrchr(fname, L'.') - fname;
	wcsncpy(buff, fname, pos);
	wsprintfW(dstname, L"%ls_.rc8", buff);
	if (_waccess(dstname, 0) == 0)
	{
		alpha = _wfopen(dstname, L"rb");
		if (!RC8_Check(alpha))
			exit(0);
		else
		{
			printf("bpp:32\n");
			cdata = malloc(RC8_Header.size);
			uncomprLen = RC8_Header.height * RC8_Header.width;
			rc8_data = malloc(uncomprLen);
			rc8_alpha = malloc(0x300);
			fread(rc8_alpha, 0x300, 1, alpha);
			fread(cdata, RC8_Header.size, 1, alpha);
			rc8_decompress(rc8_data, uncomprLen, cdata, RC8_Header.size, RC8_Header.width);
			free(cdata);
			fclose(alpha);
			alphadata = malloc(uncomprLen * 3);
			//这色板的3个字节好像是一模一样的，没懂在搞啥，好像很高端的样子
			for (i = 0; i < uncomprLen; i++)
			{
				alphadata[i * 3] = rc8_alpha[rc8_data[i]];
				alphadata[i * 3 + 1] = rc8_alpha[rc8_data[i] + 1];
				alphadata[i * 3 + 2] = rc8_alpha[rc8_data[i] + 2];
			}
			free(rc8_data);
			free(rc8_alpha);
			wsprintfW(dstname, L"%ls.png", buff);
			dst = _wfopen(dstname, L"wb");
			WritePng(dst, RCT_Header.width, RCT_Header.height, 32, udata, alphadata);
			free(alphadata);
			fclose(dst);
		}
	}
	else
	{
		printf("bpp:24\n");
		wsprintfW(dstname, L"%ls.png", buff);
		dst = _wfopen(dstname, L"wb");
		WritePng(dst, RCT_Header.width, RCT_Header.height, 24, udata, NULL);
		fclose(dst);
	}
	free(udata);
}

void RCT_WriteFile(wchar_t *fname)
{
	FILE *src = _wfopen(fname, L"rb");
	fread(&RCT_Header, sizeof(RCT_Header), 1, src);
	if (RCT_Header.signature != 0x9A925A98)
	{
		printf("rct文件头不是0x9A925A98！\n");
		fclose(src);
		system("pause");
		exit(0);
	}
	if (RCT_Header.height == 0 || RCT_Header.width == 0 || RCT_Header.size == 0)
	{
		printf("rct文件头数据有零值！\n");
		fclose(src);
		system("pause");
		exit(0);
	}
	printf("magic:%s width:%d height:%d size:0x%X ", RCT_Header.magic, RCT_Header.width, RCT_Header.height, RCT_Header.size);
	if (strncmp(RCT_Header.magic, "TC00", 4) == 0)
	{
		TC00_Export(src, fname);
	}
	else if (strncmp(RCT_Header.magic, "TC01", 4) == 0)
	{

	}
	else if (strncmp(RCT_Header.magic, "TS00", 4) == 0)
	{

	}
	else if (strncmp(RCT_Header.magic, "TS01", 4) == 0)
	{

	}
	else
	{
		printf("未知的magic!\n");
		fclose(src);
		system("pause");
		exit(0);
	}
	fclose(src);
}

int wmain(int argc, wchar_t *argv[])
{
	//setlocale(LC_ALL, "chs");
	printf("project：Niflheim-Majiro\n用于转换rct文件至png。\n将rct文件拖到程序上。\nby Darkness-TX 2018.07.09\n\n");
	RCT_WriteFile(argv[1]);
	printf("已完成，总文件数%d\n", 1);
	system("pause");
	return 0;
}