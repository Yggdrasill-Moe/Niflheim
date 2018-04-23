/*
用于导出PMS图片
made by Darkness-TX
2018.03.12
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

struct PMS_Header{
	unit8 magic[2];//PM
	unit16 version;//\x01\x00
	unit16 head_size;
	unit8 bpp;
	//0x10开始
	unit32 offsetX;
	unit32 offsetY;
	unit32 width;
	unit32 height;
	unit32 bitmapoffset;
	unit32 alphaoffset;
} pms_header;

struct index
{
	char FileName[260];//文件名
	unit32 FileSize;//文件大小
}Index[5000];

unit32 process_dir(char *dname)
{
	long Handle;
	unit32 i = 0;
	struct _finddata64i32_t FileInfo;
	_chdir(dname);//跳转路径
	if ((Handle = _findfirst("*.PMS", &FileInfo)) == -1L)
	{
		printf("没有找到匹配的项目，请将后缀命名为.PMS\n");
		system("pause");
		exit(0);
	}
	do
	{
		if (FileInfo.name[0] == '.')  //过滤本级目录和父目录
			continue;
		for (i = 0; i <= 260; i++)
			Index[FileNum].FileName[i] = FileInfo.name[i];
		Index[FileNum].FileSize = FileInfo.size;
		FileNum++;
	} while (_findnext(Handle, &FileInfo) == 0);
	return FileNum;
}

unit16* Decomp_16bpp(FILE *input,unit32 width,unit32 height)
{
	unit16* output = malloc(width * height * 2);
	unit32 stride = width, i = 0;

	for (unit32 y = 0; y < height; ++y)
	{
		for (unit32 x = 0; x < width; )
		{
			unit32 dst = y * stride + x;
			unit32 count = 1;
			unit8 ctl;
			fread(&ctl, 1, 1, input);
			if (ctl < 0xF8)
			{
				unit8 px;
				fread(&px, 1, 1, input);
				output[dst] = (unit16)(ctl | (px << 8));
			}
			else if (ctl == 0xF8)
			{
				fread(&output[dst], 2, 1, input);
			}
			else if (ctl == 0xF9)
			{
				fread(&count, 1, 1, input);
				count += 1;
				unit32 p0;
				fread(&p0, 1, 1, input);
				unit32 p1;
				fread(&p1, 1, 1, input);
				p0 = ((p0 & 0xE0) << 8) | ((p0 & 0x18) << 6) | ((p0 & 7) << 2);
				p1 = ((p1 & 0xC0) << 5) | ((p1 & 0x3C) << 3) | (p1 & 3);
				output[dst] = (unit16)(p0 | p1);
				for (i = 1; i < count; i++)
				{
					fread(&p1, 1, 1, input);
					p1 = ((p1 & 0xC0) << 5) | ((p1 & 0x3C) << 3) | (p1 & 3);
					output[dst + i] = (unit16)(p0 | p1);
				}
			}
			else if (ctl == 0xFA)
			{
				output[dst] = output[dst - stride + 1];
			}
			else if (ctl == 0xFB)
			{
				output[dst] = output[dst - stride - 1];
			}
			else if (ctl == 0xFC)
			{
				fread(&count, 1, 1, input);
				count = (count + 2) * 2;
				unit16 px0;
				fread(&px0, 2, 1, input);
				unit16 px1;
				fread(&px1, 2, 1, input);
				for (i = 0; i < count; i += 2)
				{
					output[dst + i] = px0;
					output[dst + i + 1] = px1;
				}
			}
			else if (ctl == 0xFD)
			{
				fread(&count, 1, 1, input);
				count += 3;
				unit16 px;
				fread(&px, 2, 1, input);
				for (i = 0; i < count; i++)
				{
					output[dst + i] = px;
				}
			}
			else if (ctl == 0xFE)
			{
				fread(&count, 1, 1, input);
				count += 2;
				unit32 src = dst - stride * 2;
				for (i = 0; i < count; ++i)
				{
					output[dst + i] = output[src + i];
				}
			}
			else // ctl == 0xFF
			{
				fread(&count, 1, 1, input);
				count += 2;
				unit32 src = dst - stride;
				for (i = 0; i < count; ++i)
				{
					output[dst + i] = output[src + i];
				}
			}
			x += count;
		}
	}
	return output;
}

unit8* Decomp_8bpp(FILE *input, unit32 width, unit32 height)
{
	unit8* output = malloc(width * height);
	unit32 stride = width, i = 0;

	for (unit32 y = 0; y < height; y++)
	{
		for (unit32 x = 0; x < width; )
		{
			unit32 dst = y * stride + x;
			unit32 count = 1;
			unit8 ctl;
			fread(&ctl, 1, 1, input);
			if (ctl < 0xF8)
			{
				output[dst] = ctl;
			}
			else if (ctl == 0xFF)
			{
				fread(&count, 1, 1, input);
				count += 3;
				unit32 src = dst - stride;
				for (i = 0; i < count; ++i)
				{
					output[dst + i] = output[src + i];
				}
			}
			else if (ctl == 0xFE)
			{
				fread(&count, 1, 1, input);
				count += 3;
				unit32 src = dst - stride * 2;
				for (i = 0; i < count; ++i)
				{
					output[dst + i] = output[src + i];
				}
			}
			else if (ctl == 0xFD)
			{
				fread(&count, 1, 1, input);
				count += 4;
				unit8 px;
				fread(&px, 1, 1, input);
				for (i = 0; i < count; ++i)
				{
					output[dst + i] = px;
				}
			}
			else if (ctl == 0xFC)
			{
				fread(&count, 1, 1, input);
				count = (count + 3) * 2;
				unit8 px0;
				fread(&px0, 1, 1, input);
				unit8 px1;
				fread(&px1, 1, 1, input);
				for (i = 0; i < count; i += 2)
				{
					output[dst + i] = px0;
					output[dst + i + 1] = px1;
				}
			}
			else // >= 0xF8 < 0xFC
			{
				fread(&output[dst], 1, 1, input);
			}
			x += count;
		}
	}
	return output;
}

void WritePng(FILE *Pngname, unit32 Width, unit32 Height, unit32 Bpp, unit8* PixelData, unit8* BitmapData)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_colorp pcolor;
	unit8 *png_alpha;
	unit32 i = 0;
	unit8 *data;
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
	if (Bpp == 16)
	{
		if (PixelData != NULL)
		{
			png_set_IHDR(png_ptr, info_ptr, Width, Height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			png_write_info(png_ptr, info_ptr);
			unit8 *p = BitmapData;
			data = malloc(Width * Height * 4);
			for (i = 0; i < Width * Height; i++)
			{
				data[i * 4 + 0] = (p[1] & 0xf8);
				data[i * 4 + 1] = ((p[1] & 0x07) << 5) | (p[0] & 0xe0) >> 3;
				data[i * 4 + 2] = (p[0] & 0x1f) << 3;
				data[i * 4 + 3] = PixelData[i];
				p += 2;
			}
			for (i = 0; i < Height; i++)
				png_write_row(png_ptr, data + i*Width * 4);
		}
		else
		{
			png_set_IHDR(png_ptr, info_ptr, Width, Height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			png_write_info(png_ptr, info_ptr);
			unit8 *p = BitmapData;
			data = malloc(Width * Height * 3);
			for (i = 0; i < Width * Height; i++)
			{
				data[i * 3 + 0] = (p[1] & 0xf8);
				data[i * 3 + 1] = ((p[1] & 0x07) << 5) | (p[0] & 0xe0) >> 3;
				data[i * 3 + 2] = (p[0] & 0x1f) << 3;
				p += 2;
			}
			for (i = 0; i < Height; i++)
				png_write_row(png_ptr, data + i*Width * 3);
		}
	}
	else if (Bpp == 8)
	{
		png_set_IHDR(png_ptr, info_ptr, Width, Height, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		pcolor = (png_colorp)malloc(256 * sizeof(png_color));
		png_alpha = (unit8 *)malloc(256 * sizeof(unit8));
		for (i = 0; i < 256; i++)
		{
			pcolor[i].red = PixelData[i * 3 + 0];
			pcolor[i].green = PixelData[i * 3 + 1];
			pcolor[i].blue = PixelData[i * 3 + 2];
			png_alpha[i] = 0xFF;
		}
		png_set_PLTE(png_ptr, info_ptr, pcolor, 256);
		png_set_tRNS(png_ptr, info_ptr, (png_bytep)png_alpha, 256, (png_color_16p)0);
		free(pcolor);
		free(png_alpha);
		png_write_info(png_ptr, info_ptr);
		for (i = 0; i < Height; i++)
			png_write_row(png_ptr, BitmapData + i*Width);
	}
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
}

void WritePngFile()
{
	FILE *src, *dst;
	unit8 *bitmap = NULL, *alpha= NULL;
	unit8 dstname[MAX_PATH];
	unit32 i = 0;
	for (i = 0; i < FileNum; i++)
	{
		src = fopen(Index[i].FileName, "rb");
		fread(pms_header.magic, 2, 1, src);
		if (strncmp(pms_header.magic, "PM", 2))
		{
			printf("不支持的文件类型，文件头不是PM\n");
			system("pause");
			exit(0);
		}
		fread(&pms_header.version, 2, 1, src);
		if (pms_header.version != 1)
		{
			printf("不支持的文件类型，文件版本不是1\n");
			system("pause");
			exit(0);
		}
		fread(&pms_header.head_size, 2, 1, src);
		fread(&pms_header.bpp, 1, 1, src);
		fseek(src, 0x10, SEEK_SET);
		fread(&pms_header.offsetX, 4, 1, src);
		fread(&pms_header.offsetY, 4, 1, src);
		fread(&pms_header.width, 4, 1, src);
		fread(&pms_header.height, 4, 1, src);
		fread(&pms_header.bitmapoffset, 4, 1, src);
		fread(&pms_header.alphaoffset, 4, 1, src);
		printf("filename: %s headsize:0x%X bpp:%d X:%d Y:%d width:%d height:%d bitmap_offset:0x%X alpha_offset:0x%X\n",
			Index[i].FileName, pms_header.head_size, pms_header.bpp, pms_header.offsetX, pms_header.offsetY,
			pms_header.width, pms_header.height, pms_header.bitmapoffset, pms_header.alphaoffset);
		if (pms_header.bpp == 16)
		{
			fseek(src, pms_header.bitmapoffset, SEEK_SET);
			bitmap = (unit8 *)Decomp_16bpp(src, pms_header.width, pms_header.height);
			if (pms_header.alphaoffset != 0)
			{
				fseek(src, pms_header.alphaoffset, SEEK_SET);
				alpha = Decomp_8bpp(src, pms_header.width, pms_header.height);
			}
		}
		else if (pms_header.bpp == 8)
		{
			fseek(src, pms_header.bitmapoffset, SEEK_SET);
			bitmap = Decomp_8bpp(src, pms_header.width, pms_header.height);
			fseek(src, pms_header.alphaoffset, SEEK_SET);
			alpha = malloc(0x100 * 3);
			fread(alpha, 0x100 * 3, 1, src);
		}
		else
		{
			printf("不支持的bpp类型！\n");
			system("pause");
			exit(0);
		}
		sprintf(dstname, "%s.png", Index[i].FileName);
		dst = fopen(dstname, "rb");
		if (pms_header.alphaoffset != 0)
		{
			WritePng(dst, pms_header.width, pms_header.height, pms_header.bpp, alpha, bitmap);
			free(alpha);
		}
		else
			WritePng(dst, pms_header.width, pms_header.height, pms_header.bpp, NULL, bitmap);

		free(bitmap);
		fclose(src);
		fclose(dst);
	}
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-AliceSoft\n用于导出PMS图片。\n将文件夹拖到程序上。\nby Darkness-TX 2018.03.12\n\n");
	process_dir(argv[1]);
	WritePngFile();
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}