/*
用于导出pb3图片
made by Darkness-TX
2018.08.16
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
typedef unsigned __int64 unit64;

unit32 FileNum = 0;//总文件数，初始计数为0

struct index
{
	WCHAR FileName[MAX_PATH];//文件名
	unit32 FileSize;//文件大小
}Index[50000];

struct jbp_header
{
	unit8 sign[4];//JBP1
	unit32 headsize;
	unit32 format;
	unit32 unk0;
	unit16 width;
	unit16 height;
	unit32 bpp;
	unit32 unk1;
	unit32 dc_bits;
	unit32 ac_bits;
	unit32 unk2;
	unit32 unk3;
}JBP_Header;

struct pb3_header
{
	unit8 sign[4];//PB3B
	unit32 filesize;//JBP1时会减去0x34
	unit32 unk0;
	unit32 headsize;//一般为0x34，JBP1时为0
	unit32 unk1;
	unit32 unk2;
	unit32 data_size;//v6用
	unit16 type;
	unit16 width;
	unit16 height;
	unit16 bpp;
	unit32 unk4;
	unit32 unk5;
	unit32 data_off;//v6、jbp、v1用,jbp时为alpha_off
	unit32 alpha_size;//jbp时用,v1的可以当成data_off2
}PB3_Header;

unit8 frame[0x800];

unit32 process_dir(char *dname)
{
	long Handle;
	struct _wfinddata64i32_t FileInfo;
	_chdir(dname);//跳转路径
	if ((Handle = _wfindfirst(L"*.pb3", &FileInfo)) == -1L)
	{
		printf("没有找到匹配的项目，请将后缀命名为.pb3\n");
		system("pause");
		exit(0);
	}
	do
	{
		if (FileInfo.name[0] == L'.')  //过滤本级目录和父目录
			continue;
		wsprintf(Index[FileNum].FileName, FileInfo.name);
		Index[FileNum].FileSize = FileInfo.size;
		FileNum++;
	} while (_wfindnext(Handle, &FileInfo) == 0);
	return FileNum;
}

void decrypt_header(FILE *src, struct pb3_header *PB3)
{
	unit16 key = 0, i = 0;
	unit8 *p = (unit8 *)PB3, *key_data = NULL;
	unit32 filesize = 0;
	fseek(src, 0, SEEK_END);
	filesize = ftell(src);
	fseek(src, filesize - 3, SEEK_SET);
	fread(&key, 2, 1, src);
	for (i = 8; i < 0x34; i += 2)
		*(unit16 *)&p[i] ^= key;
	fseek(src, filesize - 0x2F, SEEK_SET);
	key_data = malloc(0x34 - 8);
	fread(key_data, 0x34 - 8, 1, src);
	for (i = 8; i < 0x34; i++)
		p[i] -= key_data[i - 8];
	free(key_data);
	fseek(src, PB3->headsize, SEEK_SET);
	p = NULL;
}

void ReadIndex(FILE *src, struct pb3_header *PB3)
{
	fread(PB3, sizeof(PB3_Header), 1, src);
	if (strncmp(PB3->sign, "PB3B", 4))
	{
		wprintf(L"不支持的文件类型，文件头不是PB3B\n");
		system("pause");
		exit(0);
	}
	decrypt_header(src, PB3);
}

void WritePng(FILE *Pngname, unit32 Width, unit32 Height, unit32 Bpp, unit8* data)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unit32 i = 0;
	unit8 buff = 0;
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
	if (Bpp == 24)
	{
		png_set_IHDR(png_ptr, info_ptr, Width, Height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png_ptr, info_ptr);
		for (i = 0; i < Width * Height; i++)
		{
			buff = data[i * 3 + 0];
			data[i * 3 + 0] = data[i * 3 + 2];
			data[i * 3 + 2] = buff;
		}
		for (i = 0; i < Height; i++)
			png_write_row(png_ptr, data + i*Width * 3);
	}
	else if (Bpp == 32)
	{
		png_set_IHDR(png_ptr, info_ptr, Width, Height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png_ptr, info_ptr);
		for (i = 0; i < Width * Height; i++)
		{
			buff = data[i * 4 + 0];
			data[i * 4 + 0] = data[i * 4 + 2];
			data[i * 4 + 2] = buff;
		}
		for (i = 0; i < Height; i++)
			png_write_row(png_ptr, data + i*Width * 4);
	}
	else
	{
		printf("不支持的bpp模式!");
		system("pause");
		exit(0);
	}
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
}

unit32 HuffmanTree(unit32 *freq, unit32 *Nodes)
{
	unit32 depth = 0x10, MaxFreq = 0x7D2B7500, Root = 0;
	for (;;)
	{
		unit32 l = -1;
		unit32 min = MaxFreq - 1;
		for (unit32 i = 0; i < depth; ++i)
		{
			if (freq[i] < min)
			{
				min = freq[i];
				l = i;
			}
		}
		unit32 r = -1;
		min = MaxFreq - 1;
		for (unit32 i = 0; i < depth; ++i)
		{
			if ((i != l) && (freq[i] < min))
			{
				min = freq[i];
				r = i;
			}
		}
		if ((int)l < 0 || (int)r < 0)
			break;
		Nodes[depth] = l;
		Nodes[depth + 0x200] = r;
		freq[depth++] = freq[l] + freq[r];
		freq[l] = MaxFreq;
		freq[r] = MaxFreq;
	}
	Root = depth - 1;
	return Root;
}

void Dct(unit32 *dct_table, unit16 *quant)
{
	int a, b, c, d;
	int w, x, y, z;
	int s, t, u, v, n;
	int p = 0;
	int q = 0;
	for (int i = 0; i < 8; ++i)
	{
		if (dct_table[p + 0x08] == 0 && dct_table[p + 0x10] == 0 &&
			dct_table[p + 0x18] == 0 && dct_table[p + 0x20] == 0 &&
			dct_table[p + 0x28] == 0 && dct_table[p + 0x30] == 0 &&
			dct_table[p + 0x38] == 0)
		{
			dct_table[p] = dct_table[p + 0x08] =
				dct_table[p + 0x10] = dct_table[p + 0x18] =
				dct_table[p + 0x20] = dct_table[p + 0x28] =
				dct_table[p + 0x30] = dct_table[p + 0x38] = (dct_table[p] * quant[q]);
		}
		else
		{
			c = quant[q + 0x10] * dct_table[p + 0x10];
			d = quant[q + 0x30] * dct_table[p + 0x30];
			x = ((c + d) * 35467) >> 16;
			c = ((c * 50159) >> 16) + x;
			d = ((d * -121094) >> 16) + x;
			a = dct_table[p + 0x00] * quant[q + 0x00];
			b = dct_table[p + 0x20] * quant[q + 0x20];
			w = a + b + c;
			x = a + b - c;
			y = a - b + d;
			z = a - b - d;
			c = dct_table[p + 0x38] * quant[q + 0x38];
			d = dct_table[p + 0x28] * quant[q + 0x28];
			a = dct_table[p + 0x18] * quant[q + 0x18];
			b = dct_table[p + 0x08] * quant[q + 0x08];
			n = ((a + b + c + d) * 77062) >> 16;
			u = n + ((c * 19571) >> 16) + (((c + a) * -128553) >> 16) + (((c + b) * -58980) >> 16);
			v = n + ((d * 134553) >> 16) + (((d + b) * -25570) >> 16) + (((d + a) * -167963) >> 16);
			t = n + ((b * 98390) >> 16) + (((d + b) * -25570) >> 16) + (((c + b) * -58980) >> 16);
			s = n + ((a * 201373) >> 16) + (((c + a) * -128553) >> 16) + (((d + a) * -167963) >> 16);
			dct_table[p] = (short)(w + t);
			dct_table[p + 0x38] = (short)(w - t);
			dct_table[p + 0x08] = (short)(y + s);
			dct_table[p + 0x30] = (short)(y - s);
			dct_table[p + 0x10] = (short)(z + v);
			dct_table[p + 0x28] = (short)(z - v);
			dct_table[p + 0x18] = (short)(x + u);
			dct_table[p + 0x20] = (short)(x - u);
		}
		p++;
		q++;
	}
	p = 0;
	for (int i = 0; i < 8; ++i)
	{
		a = dct_table[p];
		c = dct_table[p + 2];
		b = dct_table[p + 4];
		d = dct_table[p + 6];
		x = ((c + d) * 35467) >> 16;
		c = ((c * 50159) >> 16) + x;
		d = ((d * -121094) >> 16) + x;
		w = a + b + c;
		x = a + b - c;
		y = a - b + d;
		z = a - b - d;
		d = dct_table[p + 5];
		b = dct_table[p + 1];
		c = dct_table[p + 7];
		a = dct_table[p + 3];
		n = ((a + b + c + d) * 77062) >> 16;
		s = n + ((a * 201373) >> 16) + (((a + c) * -128553) >> 16) + (((a + d) * -167963) >> 16);
		t = n + ((b * 98390) >> 16) + (((b + c) * -58980) >> 16) + (((b + d) * -25570) >> 16);
		u = n + ((c * 19571) >> 16) + (((b + c) * -58980) >> 16) + (((a + c) * -128553) >> 16);
		v = n + ((d * 134553) >> 16) + (((b + d) * -25570) >> 16) + (((a + d) * -167963) >> 16);
		dct_table[p] = (short)((w + t) >> 3);
		dct_table[p + 7] = (short)((w - t) >> 3);
		dct_table[p + 1] = (short)((y + s) >> 3);
		dct_table[p + 6] = (short)((y - s) >> 3);
		dct_table[p + 2] = (short)((z + v) >> 3);
		dct_table[p + 5] = (short)((z - v) >> 3);
		dct_table[p + 3] = (short)((x + u) >> 3);
		dct_table[p + 4] = (short)((x - u) >> 3);
		p += 8;
	}
}

void Ycc2Rgb(unit32 dc, unit32 ac, unit32 *dct_y, unit32 *dct_cb, unit32 *dct_cr, unit32 cbcr_src, unit32 stride, unit8 *data)
{
	unit32 y_src = 0, n = 0;
	unit8 Table[0x300];
	for (n = 0; n < 0x100; n++)
		Table[n] = 0;
	for (n = 0; n < 0x100; n++)
		Table[n + 0x100] = n;
	for (n = 0; n < 0x100; n++)
		Table[n + 0x200] = 0xFF;
	for (unit32 y = 0; y < 4; ++y)
	{
		for (unit32 x = 0; x < 4; ++x)
		{
			unit32 cb = dct_cb[cbcr_src];
			unit32 cr = dct_cr[cbcr_src];
			unit16 r = ((cr * 0x166F0) >> 16);
			unit16 g = ((cb * 0x5810) >> 16) + ((cr * 0xB6C0) >> 16);
			unit16 b = ((cb * 0x1C590) >> 16);
			unit16 c0 = dct_y[y_src] + 0x180;
			unit16 c1 = dct_y[y_src + 1] + 0x180;
			unit16 c8 = dct_y[y_src + 8] + 0x180;
			unit16 c9 = dct_y[y_src + 9] + 0x180;
			data[dc] = Table[(unit16)(c0 + b)];
			data[ac + 1 - stride] = Table[(unit16)(c0 - g)];
			data[ac + 2 - stride] = Table[(unit16)(c0 + r)];
			data[ac + 4 - stride] = Table[(unit16)(c1 + b)];
			data[ac + 5 - stride] = Table[(unit16)(c1 - g)];
			data[ac + 6 - stride] = Table[(unit16)(c1 + r)];
			data[ac] = Table[(unit16)(c8 + b)];
			data[ac + 1] = Table[(unit16)(c8 - g)];
			data[ac + 2] = Table[(unit16)(c8 + r)];
			data[ac + 4] = Table[(unit16)(c9 + b)];
			data[ac + 5] = Table[(unit16)(c9 - g)];
			data[ac + 6] = Table[(unit16)(c9 + r)];
			y_src += 2;
			dc += 8;
			ac += 8;
			cbcr_src++;
		}
		dc += stride * 2 - 32;
		ac += stride * 2 - 32;
		y_src += 8;
		cbcr_src += 4;
	}
}

unit32 ReverseByteBits(unit32 x)
{
	x = (x & 0xAA) >> 1 | (x & 0x55) << 1;
	x = (x & 0xCC) >> 2 | (x & 0x33) << 2;
	return (x >> 4 | x << 4) & 0xFF;
}

unit32 GetNextBit(unit8 *bits, unit32 *cached_bit, unit32 *bit, unit32 *pos, unit32 count)
{
	while (*cached_bit < count)
	{
		*bit = (*bit << 8) | ReverseByteBits(bits[(*pos)++]);
		*cached_bit += 8;
	}
	unit32 mask = (1 << count) - 1;
	*cached_bit -= count;
	return (*bit >> *cached_bit) & mask;
}

unit32 NodesRead(unit32 tree_root, unit32 *Nodes, unit8 *bits, unit32 *cached_bit, unit32 *bit, unit32 *pos)
{
	unit32 v = tree_root;
	while (v >= 0x10)
		v = Nodes[v + (GetNextBit(bits, cached_bit, bit, pos, 1) << 9)];
	return v;
}

void LzDecomp(unit32 bit_src, unit32 data_src, unit8* cdata, unit8 *udata, unit32 size)
{
	unit32 dst = 0;
	unit32 bit_mask = 0x80;
	unit32 frame_offset = 0x7DE;
	while (dst < size)
	{
		if (0 == bit_mask)
		{
			bit_mask = 0x80;
			++bit_src;
		}
		if (0 != (bit_mask & cdata[bit_src]))
		{
			unit32 v = *(unit16 *)&cdata[data_src];
			data_src += 2;
			unit32 count = (v & 0x1F) + 3;
			unit32 offset = v >> 5;
			for (unit32 i = 0; i < count; ++i)
			{
				unit8 b = frame[(i + offset) & 0x7FF];
				udata[dst++] = b;
				frame[frame_offset++] = b;
				frame_offset &= 0x7FF;
			}
		}
		else
		{
			unit8 b = cdata[data_src++];
			udata[dst++] = b;
			frame[frame_offset++] = b;
			frame_offset &= 0x7FF;
		}
		bit_mask >>= 1;
	}
}

unit8* DecodeV1(FILE *src)
{
	unit8 *cdata = NULL, *udata = NULL, *plane = NULL;
	unit32 x_blocks = 0, y_blocks = 0, plane_size = 0, channels = 0, i = 0, stride = 0;
	stride = PB3_Header.width * 4;
	cdata = malloc(PB3_Header.filesize);
	udata = malloc(stride * PB3_Header.height);
	x_blocks = PB3_Header.width >> 4;
	if (0 != (PB3_Header.width & 0xF))
		++x_blocks;
	y_blocks = PB3_Header.height >> 4;
	if (0 != (PB3_Header.height & 0xF))
		++y_blocks;
	plane_size = PB3_Header.width * PB3_Header.height;
	plane = malloc(plane_size);
	channels = PB3_Header.bpp / 8;
	fseek(src, 0, SEEK_SET);
	fread(cdata, PB3_Header.filesize, 1, src);
	for (unit32 channel = 0; channel < channels; channel++)
	{
		unit32 channel_offset = 4 * channels;
		for (i = 0; i < channel; ++i)
			channel_offset += *(unit32 *)&cdata[PB3_Header.data_off + i * 4];
		unit32 v21 = PB3_Header.data_off + channel_offset;
		unit32 bit_src = v21 + 12 + *(unit32 *)&cdata[v21] + *(unit32 *)&cdata[v21 + 4];
		unit32 channel_size = *(unit32 *)&cdata[v21 + 8];
		channel_offset = 4 * channels;
		for (i = 0; i < channel; ++i)
			channel_offset += *(unit32 *)&cdata[PB3_Header.alpha_size + i * 4];
		unit32 data_src = PB3_Header.alpha_size + channel_offset;
		memset(frame, 0, 0x7DE);
		LzDecomp(bit_src, data_src, cdata, plane, channel_size);
		if (0 == y_blocks || 0 == x_blocks)
			continue;
		unit32 plane_src = 0;
		bit_src = v21 + 12;
		unit32 bit_mask = 128;
		data_src = bit_src + *(unit32 *)&cdata[v21];
		unit32 v68 = 16;
		for (unit32 y = 0; y < y_blocks; ++y)
		{
			unit32 row = 16 * y;
			unit32 v66 = 16;
			unit32 dst_origin = stride * row + channel;
			for (unit32 x = 0; x < x_blocks; ++x)
			{
				unit32 dst = dst_origin;
				unit32 block_width = v66 > PB3_Header.width ? PB3_Header.width - 16 * x : 16;
				unit32 block_height = v68 > PB3_Header.height ? PB3_Header.height - row : 16;
				if (0 == bit_mask)
				{
					++bit_src;
					bit_mask = 128;
				}
				if (0 != (bit_mask & cdata[bit_src]))
				{
					unit8 b = cdata[data_src++];
					for (unit32 j = 0; j < block_height; ++j)
					{
						unit32 v49 = dst;
						for (unit32 i = 0; i < block_width; ++i)
						{
							udata[v49] = b;
							v49 += 4;
						}
						dst += stride;
					}
				}
				else
				{
					for (unit32 j = 0; j < block_height; ++j)
					{
						unit32 v49 = dst;
						for (unit32 i = 0; i < block_width; ++i)
						{
							udata[v49] = plane[plane_src++];
							v49 += 4;
						}
						dst += stride;
					}
				}
				bit_mask >>= 1;
				v66 += 16;
				dst_origin += 64;
			}
			v68 += 16;
		}
	}
	free(plane);
	free(cdata);
	if (PB3_Header.bpp == 32)
		return udata;
	else if (PB3_Header.bpp == 24)
	{
		cdata = malloc(PB3_Header.width * PB3_Header.height * 24);
		for (i = 0; i < (unit32)PB3_Header.width * (unit32)PB3_Header.height; i++)
		{
			cdata[i * 3] = udata[i * 4];
			cdata[i * 3 + 1] = udata[i * 4 + 1];
			cdata[i * 3 + 2] = udata[i * 4 + 2];
		}
		free(udata);
		return cdata;
	}
	else
	{
		wprintf(L"不支持的bpp类型！\n");
		system("pause");
		free(udata);
		return NULL;
	}
}

unit8* DecodeJBP(FILE *src)
{
	//JBP,大概就是jpeg相关的玩意，看吐了只能大段大段的抄，有兴趣的可以研究下jpeg的算法
	//JBP类型的时候PB3_Header的filesize是减去了0x34的长度,而且原有的headsize位置为0,所以先fseek到0x34
	unit32 aligned_width = 0, aligned_height = 0, blocks_x = 0, blocks_y = 0, stride = 0, tree_pos = 0, i = 0, bits_offset = 0, freq[0x20];
	unit16 quant_y[0x40], quant_c[0x40];
	unit8 *udata = NULL, tree_data[0x10];
	fseek(src, 0x34, SEEK_SET);
	fread(&JBP_Header, sizeof(JBP_Header), 1, src);
	if (strncmp(JBP_Header.sign, "JBP1", 4))
	{
		wprintf(L"不支持的type:3文件类型，文件头不是JBP1\n");
		system("pause");
		return NULL;
	}
	switch ((JBP_Header.format >> 28) & 3)
	{
	case 0:
		aligned_width = (JBP_Header.width + 7) & ~7;
		aligned_height = (JBP_Header.height + 7) & ~7;
		break;
	case 1:
		aligned_width = (JBP_Header.width + 0xF) & ~0xF;
		aligned_height = (JBP_Header.height + 0xF) & ~0xF;
		break;
	case 2:
		aligned_width = (JBP_Header.width + 0x1F) & ~0x1F;
		aligned_height = (JBP_Header.height + 0x0F) & ~0x0F;
		break;
	default:
		wprintf(L"未知的format:%d", (JBP_Header.format >> 28) & 3);
		system("pause");
		return NULL;
	}
	blocks_x = aligned_width >> 4;
	blocks_y = aligned_height >> 4;
	stride = 4 * aligned_width;
	udata = malloc(stride * aligned_height);
	tree_pos = 0x34 + JBP_Header.headsize + 0x80;
	fseek(src, tree_pos, SEEK_SET);
	for (i = 0; i < 0x10; i++)
	{
		fread(&tree_data[i], 1, 1, src);
		tree_data[i] += 1;
	}
	unit32 tree_dc[0x400], tree_ac[0x400], tree_dc_root = 0, tree_ac_root = 0;
	fseek(src, 0x34 + JBP_Header.headsize, SEEK_SET);
	fread(freq, 0x40, 1, src);
	tree_dc_root = HuffmanTree(freq, tree_dc);
	fread(freq, 0x40, 1, src);
	tree_ac_root = HuffmanTree(freq, tree_ac);
	memset(quant_c, 0, 0x40 * 2);
	memset(quant_y, 0, 0x40 * 2);
	if (0 != (JBP_Header.format & 0x8000000))
	{
		for (i = 0; i < 0x40; ++i)
		{
			fseek(src, tree_pos + 0x10 + i, SEEK_SET);
			fread(&quant_y[i], 1, 1, src);
			fseek(src, tree_pos + 0x10 + i + 0x40, SEEK_SET);
			fread(&quant_c[i], 1, 1, src);
		}
	}
	bits_offset = tree_pos + 0x10 + 0x80;
	unit8 *bits_dc = NULL, *bits_ac = NULL;
	bits_dc = malloc(JBP_Header.dc_bits);
	bits_ac = malloc(JBP_Header.ac_bits);
	fseek(src, bits_offset, SEEK_SET);
	fread(bits_dc, JBP_Header.dc_bits, 1, src);
	fread(bits_ac, JBP_Header.ac_bits, 1, src);
	unit8 ZigzagOrder[] =
	{
		1,  8,  16, 9,  2,  3, 10, 17,
		24, 32, 25, 18, 11,  4,  5, 12,
		19, 26, 33, 40, 48, 41, 34, 27,
		20, 13,  6,  7, 14, 21, 28, 35,
		42, 49, 56, 57, 50, 43, 36, 29,
		22, 15, 23, 30, 37, 44, 51, 58,
		59, 52, 45, 38, 31, 39, 46, 53,
		60, 61, 54, 47, 55, 62, 63,  0
	};
	unit32 total_blocks = blocks_x * blocks_y, prev_v = 0, pos = 0, bit = 0, cached_bit = 0;
	unit32 *blocks = malloc(total_blocks * 6 * 4);
	for (i = 0; i < total_blocks; i++)
	{
		for (unit32 j = 0; j < 6; j++)
		{
			unit32 bit_count = NodesRead(tree_dc_root, tree_dc, bits_dc, &cached_bit, &bit, &pos);
			unit32 v = (unit32)GetNextBit(bits_dc, &cached_bit, &bit, &pos, bit_count);
			if (v < (1u << (bit_count - 1)))
				v -= (1u << bit_count) - 1;
			prev_v += v;
			blocks[i * 6 + j] = prev_v;
		}
	}
	cached_bit = 0;
	bit = 0;
	pos = 0;
	unit32 dct_table[6];
	for (i = 0; i < 6; i++)
		dct_table[i] = (unit32)malloc(64 * 4);
	for (unit32 y = 0; y < blocks_y; ++y)
	{
		unit32 dst1 = y * stride * 16;
		unit32 dst2 = dst1 + stride * 9;
		for (unit32 x = 0; x < blocks_x; ++x)
		{
			for (unit32 j = 0; j < 6; ++j)
				for (unit32 k = 0; k < 64; ++k)
					((unit32 *)dct_table[j])[k] = 0;
			for (unit32 n = 0; n < 6; ++n)
			{
				((unit32 *)dct_table[n])[0] = blocks[(y * blocks_x + x) * 6 + n];
				for (i = 0; i < 63;)
				{
					unit32 bit_count = NodesRead(tree_ac_root, tree_ac, bits_ac, &cached_bit, &bit, &pos);
					if (15 == bit_count)
						break;
					if (0 == bit_count)
					{
						unit32 node_idx = 0;
						while (0 != (unit32)GetNextBit(bits_ac, &cached_bit, &bit, &pos, 1))
							node_idx++;
						i += tree_data[node_idx];
					}
					else
					{
						unit32 v = (unit32)GetNextBit(bits_ac, &cached_bit, &bit, &pos, bit_count);
						if (v < (1u << (bit_count - 1)))
							v -= (1u << bit_count) - 1;
						((unit32 *)dct_table[n])[ZigzagOrder[i]] = v;
						i++;
					}
				}
			}
			Dct((unit32 *)dct_table[0], quant_y);
			Dct((unit32 *)dct_table[1], quant_y);
			Dct((unit32 *)dct_table[2], quant_y);
			Dct((unit32 *)dct_table[3], quant_y);
			Dct((unit32 *)dct_table[4], quant_c);
			Dct((unit32 *)dct_table[5], quant_c);
			Ycc2Rgb(dst1, dst1 + stride, (unit32 *)dct_table[0], (unit32 *)dct_table[4], (unit32 *)dct_table[5], 0, stride, udata);
			Ycc2Rgb(dst1 + 32, dst1 + stride + 32, (unit32 *)dct_table[1], (unit32 *)dct_table[4], (unit32 *)dct_table[5], 4, stride, udata);
			Ycc2Rgb(dst2 - stride, dst2, (unit32 *)dct_table[2], (unit32 *)dct_table[4], (unit32 *)dct_table[5], 32, stride, udata);
			Ycc2Rgb(dst2 - stride + 32, dst2 + 32, (unit32 *)dct_table[3], (unit32 *)dct_table[4], (unit32 *)dct_table[5], 36, stride, udata);
			dst1 += 64;
			dst2 += 64;
		}
	}
	free(bits_ac);
	free(bits_dc);
	free(blocks);
	for (i = 0; i < 6; i++)
		free((unit32 *)dct_table[i]);
	if (PB3_Header.bpp == 24)
	{
		unit8 *data = malloc(PB3_Header.width * PB3_Header.height * 3);
		for (i = 0; i < (unit32)PB3_Header.width * (unit32)PB3_Header.height; i++)
		{
			data[i * 3] = udata[i * 4];
			data[i * 3 + 1] = udata[i * 4 + 1];
			data[i * 3 + 2] = udata[i * 4 + 2];
		}
		free(udata);
		return data;
	}
	else if (PB3_Header.bpp == 32)
	{
		if (PB3_Header.data_off == 0 || PB3_Header.alpha_size == 0)
		{
			wprintf(L"发生错误！alpha_off和alpha_size不能为0！alpha_off:0x%X alpha_size:0x%X\n", PB3_Header.data_off, PB3_Header.alpha_size);
			system("pause");
			free(udata);
			return NULL;
		}
		fseek(src, PB3_Header.data_off, SEEK_SET);
		unit8 *data = malloc(PB3_Header.alpha_size);
		fread(data, PB3_Header.alpha_size, 1, src);
		unit32 out = 3;
		for (i = 0; i < PB3_Header.alpha_size; i++)
		{
			if (0 != data[i] && 0xFF != data[i])
			{
				udata[out] = data[i];
				out += 4;
			}
			else
			{
				unit8 alpha = data[i++];
				unit32 count = data[i];
				while (count--> 0)
				{
					udata[out] = alpha;
					out += 4;
				}
			}
		}
		free(data);
		return udata;
	}
	else
	{
		wprintf(L"不支持的bpp类型！\n");
		system("pause");
		free(udata);
		return NULL;
	}
}

unit8* DecodeV5(FILE *src, unit32 width, unit32 height, unit32 bpp, unit32 headsize, unit32 filesize)
{
	unit8 *cdata = NULL, *udata = NULL;
	unit32 bit_src = 0, data_src = 0, i = 0, length = width * height * bpp / 8;
	udata = malloc(length);
	memset(frame, 0, 0x800);
	fseek(src, headsize + 0x20, SEEK_SET);
	cdata = malloc(filesize - headsize - 0x20);
	fread(cdata, filesize - headsize - 0x20, 1, src);
	for (i = 0; i < 4; i++)
	{
		fseek(src, headsize + i * 8, SEEK_SET);
		fread(&bit_src, 4, 1, src);
		fread(&data_src, 4, 1, src);
		memset(frame, 0, 0x7DE);
		unit32 frame_offset = 0x7DE;
		unit8 accum = 0;
		unit32 bit_mask = 0x80;
		unit32 dst = i;
		while (dst < length)
		{
			if (0 == bit_mask)
			{
				++bit_src;
				bit_mask = 0x80;
			}
			if (0 != (bit_mask & cdata[bit_src]))
			{
				unit32 v = *(unit16 *)&cdata[data_src];
				data_src += 2;
				unit32 count = (v & 0x1F) + 3;
				unit32 offset = v >> 5;
				for (unit32 k = 0; k < count; ++k)
				{
					unit8 b = frame[(k + offset) & 0x7FF];
					frame[frame_offset++] = b;
					accum += b;
					udata[dst] = accum;
					dst += 4;
					frame_offset &= 0x7FF;
				}
			}
			else
			{
				unit8 b = cdata[data_src++];
				frame[frame_offset++] = b;
				accum += b;
				udata[dst] = accum;
				dst += 4;
				frame_offset &= 0x7FF;
			}
			bit_mask >>= 1;
		}
	}
	free(cdata);
	return udata;
}

unit8* DecodeV6(FILE *src)
{
	unit8 name[0x20], key[] = { 0xA6, 0x75, 0xF3, 0x9C, 0xC5, 0x69, 0x78, 0xA3, 0x3E, 0xA5, 0x4F, 0x79, 0x59, 0xFE, 0x3A, 0xC7 };
	WCHAR wname[0x30];
	unit8 *base_data = NULL, *cdata = NULL, *udata = NULL;
	unit32 bit_src = 0, data_src = 0, bit_mask = 0, x_blocks = 0, y_blocks = 0, h = 0, dst_origin = 0, stride = PB3_Header.width * PB3_Header.bpp / 8;
	fread(name, 0x20, 1, src);
	for (unit32 i = 0; i < 0x20; i++)
		name[i] ^= key[i & 0x0F];
	MultiByteToWideChar(932, 0, name, 0x20, wname, 0x20);
	wcscat(wname, L".pb3");
	wprintf(L"basepic:%ls ", wname);
	if (_waccess(wname, 0) == -1)
	{
		wprintf(L"basepic不存在！请将文件放在同一个目录下！\n");
		system("pause");
	}
	else
	{
		FILE *base = _wfopen(wname, L"rb");
		struct pb3_header base_header;
		ReadIndex(base, &base_header);
		switch (base_header.type)
		{
		case 1:
			//DecodeV1(pbtSrc, dwSrcSize, lWidth, lHeight, wBpp);
			break;
		case 2:
		case 3:
			base_data = DecodeJBP(src);
			break;
		case 4:
			//DecodeV4(pbtSrc, dwSrcSize, lWidth, lHeight, wBpp);
			break;
		case 5:
			base_data = DecodeV5(base, base_header.width, base_header.height, base_header.bpp, base_header.headsize, base_header.filesize);
			break;
		case 6:
			wprintf(L"basepic的图片类型怎么可能是6？怕是有严重错误\n");
			base_data = NULL;
			system("pause");
			break;
		default:
			wprintf(L"basepic为不支持的图片类型:%d。\n", base_header.type);
			base_data = NULL;
			system("pause");
			break;
		}
		fclose(base);
		if (base_data)
		{
			fseek(src, PB3_Header.headsize + 0x20, SEEK_SET);
			cdata = malloc(PB3_Header.filesize - PB3_Header.headsize - 0x20);
			fread(cdata, PB3_Header.filesize - PB3_Header.headsize - 0x20, 1, src);
			udata = malloc(PB3_Header.data_size);
			LzDecomp(0, PB3_Header.data_off, cdata, udata, PB3_Header.data_size);
			free(cdata);
			bit_src = 8;
			data_src = bit_src + *(unit32 *)udata;
			bit_mask = 0x80;
			x_blocks = PB3_Header.width >> 3;
			if (0 != (PB3_Header.width & 7))
				++x_blocks;
			y_blocks = PB3_Header.height >> 3;
			if (0 != (PB3_Header.height & 7))
				++y_blocks;
			while (y_blocks > 0)
			{
				unit32 w = 0;
				for (unit32 x = 0; x < x_blocks; ++x)
				{
					if (0 == bit_mask)
					{
						++bit_src;
						bit_mask = 0x80;
					}
					if (0 == (bit_mask & udata[bit_src]))
					{
						unit32 dst = 8 * (dst_origin + 4 * x);
						unit32 x_count = 8 < PB3_Header.width - w ? 8 : PB3_Header.width - w;
						unit32 y_count = 8 < PB3_Header.height - h ? 8 : PB3_Header.height - h;
						for (unit32 v30 = y_count; v30 > 0; --v30)
						{
							unit32 count = 4 * x_count;
							memcpy(&base_data[dst], &udata[data_src], count);
							data_src += count;
							dst += stride;
						}
					}
					bit_mask >>= 1;
					w += 8;
				}
				dst_origin += stride;
				h += 8;
				--y_blocks;
			}
			free(udata);
		}
	}
	return base_data;
}

void WritePngFile()
{
	FILE *src = NULL, *dst = NULL;
	unit32 i = 0;
	unit8 *data = NULL;
	WCHAR dstname[MAX_PATH];
	for (i = 0; i < FileNum; i++)
	{
		src = _wfopen(Index[i].FileName, L"rb");
		wprintf(L"name:%ls ", Index[i].FileName);
		ReadIndex(src, &PB3_Header);
		wprintf(L"width:%d height:%d bpp:%d type:%d ", PB3_Header.width, PB3_Header.height, PB3_Header.bpp, PB3_Header.type);
		wsprintfW(dstname, L"%ls.png", Index[i].FileName);
		data = NULL;
		switch (PB3_Header.type)
		{
		case 1://主要类型
			data = DecodeV1(src);
			break;
		case 2://2的就是32位的JBP
		case 3://JBP似乎多用于CG、BG
			data = DecodeJBP(src);
			break;
		case 4://没见过
			wprintf(L"还不支持v4类型的图片。");
			system("pause");
			break;
		case 5://主要类型
			data = DecodeV5(src, PB3_Header.width, PB3_Header.height, PB3_Header.bpp, PB3_Header.headsize, PB3_Header.filesize);
			break;
		case 6://差分图，多数用于立绘
			data = DecodeV6(src);
			break;
		default:
			wprintf(L"不支持的图片类型。");
			data = NULL;
			system("pause");
			break;
		}
		wprintf(L"\n");
		if (data)
		{
			dst = _wfopen(dstname, L"wb");
			//fwrite(data, _msize(data), 1, dst);
			WritePng(dst, PB3_Header.width, PB3_Header.height, PB3_Header.bpp, data);
			free(data);
			fclose(dst);
		}
		fclose(src);
	}
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-cmvs\n用于导出pb3图片。\n将文件夹拖到程序上。\nby Darkness-TX 2018.08.16\n\n");
	process_dir(argv[1]);
	WritePngFile();
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}