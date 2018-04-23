/*
用于导入PMS图片
made by Darkness-TX
2018.03.13
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

struct PMS_Header {
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

unit32 GetMax(unit32* data, unit32* index, unit32 size)
{
	*index = 0;
	unit32 maxvalue = data[*index];
	for (unit32 i = 0; i < size; i++)
	{
		if (data[i] > maxvalue)
		{
			maxvalue = data[i];
			*index = i;
		}
	}
	return maxvalue;
}

int MeasureRleRun_8Bit(unit8 *pic, int i, int width)
{
	int maxLength = 259;
	int x = i % width;
	if (maxLength > width - x)
	{
		maxLength = width - x;
	}

	byte b = pic[i];
	int i0 = i;
	i++;
	while (i < sizeof(pic) && pic[i] == b)
	{
		if (i - i0 >= maxLength) break;
		i++;
	}
	return i - i0;
}

int MeasureRleRun2_8Bit(unit8 *pic, int i, int width)
{
	int maxLength = 516;
	int x = i % width;
	if (maxLength > width - x)
	{
		maxLength = width - x;
	}

	int i0 = i;
	if (i + 1 >= sizeof(pic)) return 0;
	if (i + 2 == sizeof(pic)) return 2;
	if (maxLength < 6) return 0;
	byte b1 = pic[i];
	byte b2 = pic[i + 1];
	i += 2;
	while (i + 1 < sizeof(pic) && pic[i] == b1 && pic[i + 1] == b2)
	{
		if (i - i0 + 1 >= maxLength) break;
		i += 2;
	}
	return i - i0;
}

int MeasurePreviousScanlineRun_8Bit(unit8 *pic, int i, int width, int offset)
{
	int maxLength = 258;
	int x = i % width;
	if (maxLength > width - x)
	{
		maxLength = width - x;
	}

	int i0 = i;
	if (i - offset < 0)
	{
		return 0;
	}
	while (i < sizeof(pic) && pic[i] == pic[i - offset])
	{
		if (i - i0 >= maxLength) break;
		i++;
	}
	return i - i0;
}

unit32 SaveImageData8Bit(unit8* data, unit8* pic, int width, int height)
{
	unit32 *sizes = malloc(4 * 4);
	unit8 *stream = data;
	unit32 stream_size = 0;
	for (int i = 0; i < sizeof(pic); i++)
	{
		//find one of these: RLE run, previous scanline run, two scanlines ago run, alternating RLE run
		int rleLength1 = MeasureRleRun_8Bit(pic, i, width);
		int rleLength2 = MeasureRleRun2_8Bit(pic, i, width);
		int prevScanlineLength = MeasurePreviousScanlineRun_8Bit(pic, i, width, width);
		int prevScanlineLength2 = MeasurePreviousScanlineRun_8Bit(pic, i, width, width * 2);

		if (rleLength1 < 4) rleLength1 = -1;
		if (rleLength2 < 6) rleLength2 = -1;
		if (prevScanlineLength < 3) prevScanlineLength = -1;
		if (prevScanlineLength2 < 3) prevScanlineLength2 = -1;

		sizes[0] = rleLength1;
		sizes[1] = rleLength2;
		sizes[2] = prevScanlineLength;
		sizes[3] = prevScanlineLength2;

		int maxIndex = 0;
		int max = GetMax(sizes, &maxIndex, 4);

		if (max < 3)
		{
			maxIndex = -1;
		}

		unit8 b = pic[i];

		switch (maxIndex)
		{
		case 0:
		{
			*stream++ = 0xFD;
			*stream++ = (unit8)(max - 4);
			*stream++ = b;
			stream_size += 3;
			i += max;
		}
		break;
		case 1:
		{
			*stream++ = 0xFC;
			*stream++ = (unit8)((max - 6) / 2);
			*stream++ = b;
			*stream++ = pic[i + 1];
			stream_size += 4;
			i += max;
		}
		break;
		case 2:
		{
			*stream++ = 0xFF;
			*stream++ = (unit8)(max - 3);
			stream_size += 2;
			i += max;
		}
		break;
		case 3:
		{
			*stream++ = 0xFE;
			*stream++ = (unit8)(max - 3);
			stream_size += 2;
			i += max;
		}
		break;
		default:
		{
			if (b <= 0xF7)
			{
				*stream++ = b;
				stream_size++;
			}
			else
			{
				*stream++ = 0xF8;
				*stream++ = b;
				stream_size += 2;
			}
			i++;
		}
		break;
		}
		i--;
	}
	return stream_size;
}

int MeasureRleRun_16Bit(unit16 *pic, int i, int width)
{
	int maxLength = 258;
	int x = i % width;
	if (maxLength > width - x)
	{
		maxLength = width - x;
	}

	unit16 b = pic[i];
	int i0 = i;
	i++;
	while (i < sizeof(pic) && pic[i] == b)
	{
		if (i - i0 >= maxLength) break;
		i++;
	}
	return i - i0;
}

int MeasureRleRun2_16Bit(unit16 *pic, int i, int width)
{
	int maxLength = 514;
	int x = i % width;
	if (maxLength > width - x)
	{
		maxLength = width - x;
	}

	int i0 = i;
	if (i + 1 >= sizeof(pic)) return 0;
	if (i + 2 == sizeof(pic)) return 2;
	if (maxLength < 6) return 0;
	unit16 b1 = pic[i];
	unit16 b2 = pic[i + 1];
	i += 2;
	while (i + 1 < sizeof(pic) && pic[i] == b1 && pic[i + 1] == b2)
	{
		if (i - i0 + 1 >= maxLength) break;
		i += 2;
	}
	return i - i0;
}

int MeasurePreviousScanlineRun_16Bit(unit16 *pic, int i, int width, int offset)
{
	int maxLength = 257;
	int x = i % width;
	if (maxLength > width - x)
	{
		maxLength = width - x;
	}

	int i0 = i;
	if (i - offset < 0)
	{
		return 0;
	}
	while (i < sizeof(pic) && pic[i] == pic[i - offset])
	{
		if (i - i0 >= maxLength) break;
		i++;
	}
	return i - i0;
}

int GetCommonCount(unit16 *pic, int i, int width)
{
	const int mask = 0xE61C;
	int maxLength = 256;
	int x = i % width;
	if (maxLength > width - x)
	{
		maxLength = width - x;
	}
	int i0 = i;

	int first = pic[i] & mask;
	while (i < sizeof(pic) && ((pic[i] & mask) == first))
	{
		if (i - i0 >= maxLength) break;
		i++;
	}
	return i - i0;
}

/*
unit32 SaveImageData16Bit(unit8 *stream, unit16 *pic, int width, int height)
{
	//<F7 = raw byte
	//FF: length = b+2, from previous scanline
	//FE: length = b+2, from two scanlines ago
	//FD: length = b+3, RLE run
	//FC: length = (b+2)*2, alternating RLE run
	//FB: one from previous scanline - 1
	//FA: one from previous scanline + 1
	//F9: length = b+1, 3 high bits for B, 2 high bits for G, 3 high bits for R,
	//                  read 2 low bits for B, 4 low bits for G, 3 low bits for R
	//F8: literal

	unit32* sizes = malloc(4 * 4);

	//first pass: encode anything that isn't a run as F8
	unit8 *ms = malloc(width * height);
	unit8 *msp = *ms;
	int i = 0;
	for (i = 0; i < sizeof(pic); i++)
	{
		//find one of these: RLE run, previous scanline run, two scanlines ago run, alternating RLE run
		int rleLength1 = MeasureRleRun(pic, i, width);
		int rleLength2 = MeasureRleRun2(pic, i, width);
		int prevScanlineLength = MeasurePreviousScanlineRun(pic, i, width, width);
		int prevScanlineLength2 = MeasurePreviousScanlineRun(pic, i, width, width * 2);

		if (rleLength1 < 3) rleLength1 = -1;
		if (rleLength2 < 4) rleLength2 = -1;
		if (prevScanlineLength < 2) prevScanlineLength = -1;
		if (prevScanlineLength2 < 2) prevScanlineLength2 = -1;

		sizes[0] = rleLength1;
		sizes[1] = rleLength2;
		sizes[2] = prevScanlineLength;
		sizes[3] = prevScanlineLength2;

		int maxIndex;
		int max = GetMax(sizes, &maxIndex, 4);

		if (max < 3)
		{
			maxIndex = -1;
		}

		unit16 b = pic[i];

		switch (maxIndex)
		{
		case 0:
		{
			//RLE
			*ms++ = 0xFD;
			*ms++ = (byte)(max - 3);
			//stream.WriteByte(b);
			i += max;
		}
		break;
		case 1:
		{
			//Alternating RLE
			*ms++ = 0xFC;
			*ms++ = (byte)((max - 4) / 2);
			//stream.WriteByte(b);
			//stream.WriteByte(pic[i + 1]);
			i += max;
		}
		break;
		case 2:
		{
			*ms++ = 0xFF;
			*ms++ = (byte)(max - 2);
			i += max;
		}
		break;
		case 3:
		{
			*ms++ = 0xFE;
			*ms++ = (byte)(max - 2);
			i += max;
		}
		break;
		default:
		{
			*ms++ = 0xF8;
			i++;
		}
		break;
		}
		i--;
	}
	BinaryWriter bw = new BinaryWriter(stream);

	////JUNK PASS: all literals
	//for (i = 0; i < pic.Length; i++)
	//{
	//    bw.Write((byte)0xF8);
	//    bw.Write((ushort)pic[i]);
	//}
	//return;


	//pass #2
	ms = msp;
	i = 0;
	while (ms.Position < sizeof(ms))
	{
		int literalCount = 0;
		int b = ms.PeekByte();
		if (b == 0xF8)
		{
			while (TRUE)
			{
				literalCount++;
				b = ms.ReadByte();
				b = ms.PeekByte();
				if (b != 0xF8)
				{
					break;
				}
			}
			//process a bunch of literals
			int literalsRemaining = literalCount;
			while (literalsRemaining > 0)
			{
				int commonCount = GetCommonCount(pic, i, width);
				if (commonCount > literalsRemaining) commonCount = literalsRemaining;
				if (commonCount >= 3)
				{
					bw.Write((byte)0xF9);
					bw.Write((byte)(commonCount - 1));

					//const int mask2 = 0x19E3;
					int p, r, g;
					p = pic[i];
					b = (p >> 0) & 0x1F;
					g = (p >> 5) & 0x3F;
					r = (p >> 11) & 0x1F;

					int highRGB = ((b >> 2) << 0) |
						((g >> 4) << 3) |
						((r >> 2) << 5);

					bw.Write((byte)highRGB);

					for (int c = 0; c < commonCount; c++)
					{
						p = pic[i];
						b = (p >> 0) & 0x1F;
						g = (p >> 5) & 0x3F;
						r = (p >> 11) & 0x1F;
						int lowRGB = ((b & 0x03) << 0) |
							((g & 0x0F) << 2) |
							((r & 0x03) << 6);
						bw.Write((byte)lowRGB);

						i++;
					}

					literalsRemaining -= commonCount;
				}
				else
				{
					int p = pic[i];
					int x = i % width;
					if (x > 0 && i - width - 1 >= 0 && pic[i - width - 1] == p)
					{
						bw.Write((byte)0xFB);
					}
					else if (x < width && i - width + 1 >= 0 && pic[i - width + 1] == p)
					{
						bw.Write((byte)0xFA);
					}
					else
					{
						if ((p & 0xFF) < 0xF8)
						{
							bw.Write((unit16)p);
						}
						else
						{
							bw.Write((byte)0xF8);
							bw.Write((unit16)p);
						}
					}
					i++;
					literalsRemaining--;
				}
			}
		}
		else
		{
			b = ms.ReadByte();
			//process the tag
			int lengthByte = ms.ReadByte();
			switch (b)
			{
			case 0xFD:
				//RLE
				bw.Write((byte)b);
				bw.Write((byte)lengthByte);
				bw.Write((unit16)pic[i]);
				i += lengthByte + 3;
				break;
			case 0xFC:
				//Alternating RLE
				bw.Write((byte)b);
				bw.Write((byte)lengthByte);
				bw.Write((unit16)pic[i]);
				bw.Write((unit16)pic[i + 1]);
				i += lengthByte * 2 + 4;
				break;
			case 0xFF:
				//From previous scanline
				bw.Write((byte)b);
				bw.Write((byte)lengthByte);
				i += lengthByte + 2;
				break;
			case 0xFE:
				//from two scanlines ago
				bw.Write((byte)b);
				bw.Write((byte)lengthByte);
				i += lengthByte + 2;
				break;
			}
		}
	}
}
*/

unit8* PseudoComp_8bpp(unit8 *bitmap, unit32 width, unit32 height)
{
	unit8 *data = malloc(width * height * 2);
	for (unit32 i = 0; i < width * height; i++)
	{
		data[i * 2 + 0] = 0xF8;
		data[i * 2 + 1] = bitmap[i];
	}
	free(bitmap);
	return data;
}

unit8* PseudoComp_16bpp(unit8 *bitmap, unit32 width, unit32 height)
{
	unit8 *data = malloc(width * height * 3);
	for (unit32 i = 0; i < width * height; i++)
	{
		data[i * 3 + 0] = 0xF8;
		data[i * 3 + 1] = bitmap[i * 2 + 0];
		data[i * 3 + 2] = bitmap[i * 2 + 1];
	}
	free(bitmap);
	return data;
}

void ReadPng(FILE *pngfile, unit8 *bitmapdata, unit8 *alphadata)
{
	png_structp png_ptr;
	png_infop info_ptr, end_ptr;
	png_colorp pcolor;
	png_bytep *rows;
	unit32 i, width = 0, height = 0, bpp = 0, format = 0, num_palette = 0;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		printf("PNG信息创建失败!\n");
		exit(0);
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		printf("info信息创建失败!\n");
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		exit(0);
	}
	end_ptr = png_create_info_struct(png_ptr);
	if (end_ptr == NULL)
	{
		printf("end信息创建失败!\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		exit(0);
	}
	png_init_io(png_ptr, pngfile);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)&width, (png_uint_32*)&height, &bpp, &format, NULL, NULL, NULL);
	if (pms_header.bpp == 16)
	{
		if (pms_header.alphaoffset != 0 && format != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			printf("原始PMS文件为32位rgba图，而png文件不是32位rgba图，请转换！format:%d bpp:%d\n", format, bpp);
			system("pause");
			exit(0);
		}
		else if (pms_header.alphaoffset == 0 && format != PNG_COLOR_TYPE_RGB)
		{
			printf("原始PMS文件为24位rgb图，而png文件不是24位rgb图，请转换！format:%d bpp:%d\n", format, bpp);
			system("pause");
			exit(0);
		}
	}
	else if (pms_header.bpp == 8)
	{
		if (format != PNG_COLOR_TYPE_PALETTE)
		{
			printf("原始PMS文件为8位索引图，而png文件不是8位索引图，请转换！format:%d bpp:%d\n", format, bpp);
			system("pause");
			exit(0);
		}
	}
	if (width != pms_header.width || height != pms_header.height)
	{
		printf("图片的长宽与原图不符！\n");
		system("pause");
		exit(0);
	}
	if (format == PNG_COLOR_TYPE_PALETTE)
	{
		png_get_PLTE(png_ptr, info_ptr, &pcolor, &num_palette);
		for (i = 0; i < 256; i++)
		{
			alphadata[i * 3 + 0] = pcolor[i].red;
			alphadata[i * 3 + 1] = pcolor[i].green;
			alphadata[i * 3 + 2] = pcolor[i].blue;
		}
		rows = (png_bytep*)malloc(pms_header.height * sizeof(char*));
		for (i = 0; i < pms_header.height; i++)
			rows[i] = (png_bytep)(bitmapdata + pms_header.width*i);
		png_read_image(png_ptr, rows);
		free(rows);
		png_read_end(png_ptr, info_ptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
	}
	else if (format == PNG_COLOR_TYPE_RGB)
	{
		unit8 *data = malloc(pms_header.height * pms_header.width * 3);
		rows = (png_bytep*)malloc(pms_header.height * sizeof(char*));
		for (i = 0; i < pms_header.height; i++)
			rows[i] = (png_bytep)(data + pms_header.width*i * 3);
		png_read_image(png_ptr, rows);
		free(rows);
		png_read_end(png_ptr, info_ptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
		unit8 *p = bitmapdata;
		for (i = 0; i < pms_header.height*pms_header.width; i++)
		{
			*p++ = (data[i * 3 + 2] >> 3) | ((data[i * 3 + 1] << 3) & 0xe0);
			*p++ = ((data[i * 3 + 1] >> 5) & 0x07) | (data[i * 3 + 0] & 0xf8);
		}
	}
	else if (format == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		unit8 *data = malloc(pms_header.height * pms_header.width * 4);
		rows = (png_bytep*)malloc(pms_header.height * sizeof(char*));
		for (i = 0; i < pms_header.height; i++)
			rows[i] = (png_bytep)(data + pms_header.width*i * 4);
		png_read_image(png_ptr, rows);
		free(rows);
		png_read_end(png_ptr, info_ptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
		unit8 *p = bitmapdata;
		for (i = 0; i < pms_header.height*pms_header.width; i++)
		{
			*p++ = (data[i * 4 + 2] >> 3) | ((data[i * 4 + 1] << 3) & 0xe0);
			*p++ = ((data[i * 4 + 1] >> 5) & 0x07) | (data[i * 4 + 0] & 0xf8);
			alphadata[i] = data[i * 4 + 3];
		}
	}
}

void WritePMSFile()
{
	FILE *src, *dst, *fsrc;
	unit8 *bitmap = NULL, *alpha = NULL, *head = NULL;
	unit8 dstname[MAX_PATH];
	unit32 i = 0;
	for (i = 0; i < FileNum; i++)
	{
		bitmap = NULL;
		alpha = NULL;
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
		fseek(src, 0, SEEK_SET);
		head = malloc(pms_header.head_size);
		fread(head, pms_header.head_size, 1, src);
		fclose(src);
		printf("filename: %s ", Index[i].FileName);
		sprintf(dstname, "%s.png", Index[i].FileName);
		fsrc = fopen(dstname, "rb");
		if (pms_header.bpp == 16)
		{
			bitmap = malloc(pms_header.width * pms_header.height * 2);
			if (pms_header.alphaoffset != 0)
				alpha = malloc(pms_header.width * pms_header.height);
		}
		else if (pms_header.bpp == 8)
		{
			bitmap = malloc(pms_header.width * pms_header.height);
			alpha = malloc(0x300);
		}
		ReadPng(fsrc, bitmap, alpha);
		fclose(fsrc);
		if (pms_header.bpp == 8)
			bitmap = PseudoComp_8bpp(bitmap, pms_header.width, pms_header.height);
		else if (pms_header.bpp == 16)
		{
			bitmap = PseudoComp_16bpp(bitmap, pms_header.width, pms_header.height);
			if (pms_header.alphaoffset != 0)
				alpha = PseudoComp_8bpp(alpha, pms_header.width, pms_header.height);
		}
		Index[i].FileName[sizeof(Index[i].FileName) - 4] = '\0';
		sprintf(dstname, "%s_NEW", Index[i].FileName);
		dst = fopen(dstname, "wb");
		fseek(dst, pms_header.head_size, SEEK_SET);
		if (pms_header.alphaoffset == 0)
		{
			fwrite(bitmap, pms_header.width*pms_header.height * 3, 1, dst);
		}
		else
		{
			if (pms_header.bpp == 16)
			{
				if (pms_header.bitmapoffset < pms_header.alphaoffset)
				{
					fwrite(bitmap, pms_header.width*pms_header.height * 3, 1, dst);
					pms_header.alphaoffset = ftell(dst);
					fwrite(alpha, pms_header.width*pms_header.height * 2, 1, dst);
				}
				else
				{
					fwrite(alpha, pms_header.width*pms_header.height * 2, 1, dst);
					pms_header.bitmapoffset = ftell(dst);
					fwrite(bitmap, pms_header.width*pms_header.height * 3, 1, dst);
				}
			}
			else if (pms_header.bpp == 8)
			{
				if (pms_header.bitmapoffset < pms_header.alphaoffset)
				{
					fwrite(bitmap, pms_header.width*pms_header.height * 2, 1, dst);
					pms_header.alphaoffset = ftell(dst);
					fwrite(alpha, 0x300, 1, dst);
				}
				else
				{
					fwrite(alpha, 0x300, 1, dst);
					pms_header.bitmapoffset = ftell(dst);
					fwrite(bitmap, pms_header.width*pms_header.height * 2, 1, dst);
				}
			}
		}
		free(bitmap);
		if (alpha != NULL)
			free(alpha);
		fseek(dst, 0, SEEK_SET);
		fwrite(head, pms_header.head_size, 1, dst);
		free(head);
		fseek(dst, 0, SEEK_SET);
		fwrite(pms_header.magic, 2, 1, dst);
		fwrite(&pms_header.version, 2, 1, dst);
		fwrite(&pms_header.head_size, 2, 1, dst);
		fwrite(&pms_header.bpp, 1, 1, dst);
		fseek(dst, 0x10, SEEK_SET);
		fwrite(&pms_header.offsetX, 4, 1, dst);
		fwrite(&pms_header.offsetY, 4, 1, dst);
		fwrite(&pms_header.width, 4, 1, dst);
		fwrite(&pms_header.height, 4, 1, dst);
		fwrite(&pms_header.bitmapoffset, 4, 1, dst);
		fwrite(&pms_header.alphaoffset, 4, 1, dst);
		fclose(dst);
		printf("headsize:0x%X bpp:%d X:%d Y:%d width:%d height:%d bitmap_offset:0x%X alpha_offset:0x%X\n",
			pms_header.head_size, pms_header.bpp, pms_header.offsetX, pms_header.offsetY,
			pms_header.width, pms_header.height, pms_header.bitmapoffset, pms_header.alphaoffset);
	}
}


int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-AliceSoft\n用于导入PMS图片。\n将文件夹拖到程序上。\nby Darkness-TX 2018.03.13\n\n");
	process_dir(argv[1]);
	WritePMSFile();
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}