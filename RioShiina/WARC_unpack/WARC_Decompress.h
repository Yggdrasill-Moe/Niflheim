/*
WARC 1.7中用到的解压算法
made by Darkness-TX
2018.05.04
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <Windows.h>
#include <locale.h>
#include <math.h>
#include <zlib.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

struct huffman_state {
	unit8 *in;
	unit32 curbits;
	unit32 cache;
	unit32 index;
	unit32 left[511];
	unit32 right[511];
};

unit32 huffman_get_bits(struct huffman_state *hstat, DWORD req_bits)
{
	unit32 ret_val = 0;
	if (req_bits > hstat->curbits)
	{
		do
		{
			req_bits -= hstat->curbits;
			ret_val |= (hstat->cache & ((1 << hstat->curbits) - 1)) << req_bits;
			hstat->cache = *(unit32 *)hstat->in;
			hstat->in += 4;
			hstat->curbits = 32;
		} while (req_bits > 32);
	}
	hstat->curbits -= req_bits;
	return ret_val | ((1 << req_bits) - 1) & (hstat->cache >> hstat->curbits);
}

unit32 huffman_create_tree(struct huffman_state *hstat)
{
	unit32 not_leaf;
	if (hstat->curbits-- < 1)
	{
		hstat->curbits = 31;
		hstat->cache = *(unit32 *)hstat->in;
		hstat->in += 4;
		not_leaf = hstat->cache >> 31;
	}
	else
		not_leaf = (hstat->cache >> hstat->curbits) & 1;
	unit32 index;
	if (not_leaf)
	{
		index = hstat->index++;
		hstat->left[index] = huffman_create_tree(hstat);
		hstat->right[index] = huffman_create_tree(hstat);
	}
	else//返回字符
		index = huffman_get_bits(hstat, 8);
	return index;
}

void huffman_decompress(unit8 *out, unit32 out_len, unit8 *in, unit32 in_len)
{
	struct huffman_state hstat;
	hstat.in = in;
	hstat.curbits = 0;
	hstat.index = 256;
	memset(hstat.left, 0, sizeof(hstat.left));
	memset(hstat.right, 0, sizeof(hstat.right));
	unit32 index = huffman_create_tree(&hstat);

	for (unit32 i = 0; i < out_len; i++)
	{
		unit32 idx = index;
		while (idx >= 256)
		{
			unit32 is_right;
			if ((int)--hstat.curbits < 0)
			{
				hstat.curbits = 31;
				hstat.cache = *(unit32 *)hstat.in;
				hstat.in += 4;
				is_right = hstat.cache >> 31;
			}
			else
				is_right = (hstat.cache >> hstat.curbits) & 1;
			if (is_right)
				idx = hstat.right[idx];
			else
				idx = hstat.left[idx];
		}
		*out++ = (unit8)idx;
	}
}

void YPK_decompress(unit8 *src, unit8 *dst, unit32 comprlen, unit32 uncomprlen, unit32 sig)
{
	unit32 i = 0;
	if ((sig & 0xFF000000) != 0)
	{
		unit32 key = ~0x4B4D4B4D;//KMKM
		unit32 *enc = (unit32 *)src;
		for (i = 0; i < comprlen / 4; i++)
			enc[i] ^= key;
		for (i *= 4; i < comprlen; i++)
			src[i] ^= (unit8)key;
	}
	if (src[0] != 0x78)
	{
		printf("此YPK文件疑似未经过ZLIB压缩，请检查！src[0]:0x%X\n", src[0]);
		system("pause");
		exit(0);
	}
	uncompress(dst, &uncomprlen, src, comprlen);
}

void YH1_decompress(unit8 *src, unit8 *dst, unit32 comprlen, unit32 uncomprlen, unit32 sig)
{
	unit32 i = 0;
	if ((sig & 0xFF000000) != 0)
	{
		unit32 key = 0x6393528E ^ 0x4B4D;//山田 ^ MK
		unit32 *enc = (unit32 *)src;
		for (i = 0; i < comprlen / 4; i++)
			enc[i] ^= key;
	}
	huffman_decompress(dst, uncomprlen, src, comprlen);
}