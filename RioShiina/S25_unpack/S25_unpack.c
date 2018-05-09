/*
用于解包文件头为S25的S25文件。
made by Darkness-TX
2018.04.16
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

unit32 FileNum = 0;//总文件数，初始计数为0

struct header
{
	unit8 magic[4];//S25\0
	unit32 index_num;
}S25_header;

struct index
{
	char filename[150];
	unit32 size;
	unit32 offset;
	unit32 width;
	unit32 height;
	unit32 x;
	unit32 y;
	unit32 unk;//0
}S25_Index[10000];

void S25_decompress(BYTE *out_o, BYTE *compr_line_o, DWORD width)
{
	BYTE b, g, r, a;
	DWORD cnt;
	BYTE *out = out_o;
	BYTE *compr_line = compr_line_o;
	for (DWORD x = width; (int)x > 0; )
	{
		compr_line = (BYTE *)((unit32)(compr_line + 1) & ~1);//2字节对齐
		cnt = *(unit16 *)compr_line;
		compr_line += 2;
		DWORD flag = cnt >> 13;
		compr_line += (cnt & 0x1800) >> 11;
		cnt &= 0x7ff;
		if (x < cnt)
		{
			cnt = x;
			x = 0;
		}
		else
			x -= cnt;
		if (flag == 2)
		{
			for (DWORD i = 0; i < cnt; ++i)
			{
				*out++ = *compr_line++;
				*out++ = *compr_line++;
				*out++ = *compr_line++;
				*out++ = 0xff;
			}
		}
		else if (flag == 3)
		{
			b = *compr_line++;
			g = *compr_line++;
			r = *compr_line++;
			for (DWORD i = 0; i < cnt; ++i)
			{
				*out++ = b;
				*out++ = g;
				*out++ = r;
				*out++ = 0xff;
			}
		}
		else if (flag == 4)
		{
			for (DWORD i = 0; i < cnt; ++i)
			{
				a = *compr_line++;
				*out++ = *compr_line++;
				*out++ = *compr_line++;
				*out++ = *compr_line++;
				*out++ = a;
			}
		}
		else if (flag == 5)
		{
			unit32 rgba = *(unit32 *)compr_line;
			compr_line += 4;
			r = (rgba >> 24) & 0xff;
			g = (rgba >> 16) & 0xff;
			b = (rgba >> 8) & 0xff;
			a = rgba & 0xff;
			for (DWORD i = 0; i < cnt; ++i)
			{
				*out++ = b;
				*out++ = g;
				*out++ = r;
				*out++ = a;
			}
		}
		else
		{
			out += cnt * 4;
		}
	}
}

void WritePng(FILE *Pngname, unit32 Width, unit32 Height, unit8* BitmapData)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unit32 i = 0;
	unit8 buff;
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
	png_init_io(png_ptr, Pngname);
	png_set_IHDR(png_ptr, info_ptr, Width, Height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	for (i = 0; i < Width * Height; i++)
	{
		buff = BitmapData[i * 4 + 0];
		BitmapData[i * 4 + 0] = BitmapData[i * 4 + 2];
		BitmapData[i * 4 + 2] = buff;
	}
	for (i = 0; i < Height; i++)
		png_write_row(png_ptr, BitmapData + i*Width * 4);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
}

void ReadIndex(FILE *src, char *fname)
{
	unit32 i = 0;
	int p = -1;
	fread(S25_header.magic, 1, 4, src);
	fread(&S25_header.index_num, 1, 4, src);
	if (strncmp(S25_header.magic, "S25\0", 4) != 0)
	{
		printf("不支持的文件类型，请确认文件头为S25\n");
		system("pause");
		exit(0);
	}
	for (i = 0; i < S25_header.index_num; i++)
	{
		sprintf(S25_Index[i].filename, "%s_%04d.png", fname, i);
		fread(&S25_Index[i].offset, 4, 1, src);
		if (S25_Index[i].offset == 0)
			S25_Index[i].size = 0;
		else
		{
			fseek(src, S25_Index[i].offset, SEEK_SET);
			fread(&S25_Index[i].width, 4, 1, src);
			fread(&S25_Index[i].height, 4, 1, src);
			fread(&S25_Index[i].x, 4, 1, src);
			fread(&S25_Index[i].y, 4, 1, src);
			fread(&S25_Index[i].unk, 4, 1, src);
			fseek(src, i * 4 + 4 + 8, SEEK_SET);
			if (S25_Index[i].height == 0 || S25_Index[i].width == 0)
			{
				printf("宽或高为0！height:%d width:%d\n", S25_Index[i].height, S25_Index[i].width);
				system("pause");
			}
			if (S25_Index[i].unk != 0)
			{
				printf("unk字段不等于0！unk:0%X\n", S25_Index[i].unk);
				system("pause");
			}
			if (p != -1)
				S25_Index[p].size = S25_Index[i].offset - S25_Index[p].offset;
			p = i;
			FileNum++;
		}
	}
	if (p != -1)
	{
		fseek(src, 0, SEEK_END);
		S25_Index[p].size = ftell(src) - S25_Index[p].offset;
	}
}

void Unpack(char *fname)
{
	FILE *src, *dst;
	unit8 dirname[200], *cdata, *udata, *adata;
	unit32 i = 0, k = 0, savepos, line_offset = 0;
	unit16 line_size = 0;
	src = fopen(fname, "rb");
	ReadIndex(src, fname);
	sprintf(dirname, "%s_unpack", fname);
	_mkdir(dirname);
	_chdir(dirname);
	for (i = 0; i < S25_header.index_num; i++)
	{
		if (S25_Index[i].size != 0)
		{
			printf("name:%s offset:0x%X size:0x%X width:%d height:%d x:%d y:%d\n", S25_Index[i].filename, S25_Index[i].offset, S25_Index[i].size, S25_Index[i].width, S25_Index[i].height, S25_Index[i].x, S25_Index[i].y);
			dst = fopen(S25_Index[i].filename, "wb");
			fseek(src, S25_Index[i].offset + 0x14, SEEK_SET);
			adata = malloc(S25_Index[i].width * S25_Index[i].height * 4);
			for (k = 0; k < S25_Index[i].height; k++)
			{
				fread(&line_offset, 4, 1, src);
				savepos = ftell(src);
				fseek(src, line_offset, SEEK_SET);
				fread(&line_size, 2, 1, src);
				if ((ftell(src) & 1) != 0)
				{
					fseek(src, 1, SEEK_CUR);
					line_size -= 1;
				}
				cdata = malloc(line_size);
				udata = malloc(S25_Index[i].width * 4);
				memset(udata, 0, S25_Index[i].width * 4);
				fread(cdata, line_size, 1, src);
				S25_decompress(udata, cdata, S25_Index[i].width);
				free(cdata);
				memcpy(adata + S25_Index[i].width * 4 * k, udata, S25_Index[i].width * 4);
				free(udata);
				fseek(src, savepos, SEEK_SET);
			}
			//fwrite(adata, S25_Index[i].width * 4 * S25_Index[i].height, 1, dst);
			WritePng(dst, S25_Index[i].width, S25_Index[i].height, adata);
			free(adata);
			fclose(dst);
		}
	}
	fclose(src);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-BLOODY RONDO\n用于解包文件头为S25的S25文件。\n将S25文件拖到程序上。\nby Darkness-TX 2018.04.16\n\n");
	Unpack(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}