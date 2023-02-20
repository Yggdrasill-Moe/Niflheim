//base on GARbro:Cmvs/HuffmanDecoder.cs
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

struct bits
{
	unit8 *m_input;
	unit8 *m_output;

	unit32 m_src;
	unit32 m_bits;
	unit32 m_bit_count;

	unit16 lhs[512];
	unit16 rhs[512];
	unit16 token;
}h_bits;

int GetBits(int count)
{
	int bits = 0;
	while (count--> 0)
	{
		if (0 == h_bits.m_bit_count)
		{
			memcpy(&h_bits.m_bits, h_bits.m_input + h_bits.m_src, 4);
			h_bits.m_src += 4;
			h_bits.m_bit_count = 32;
		}
		bits = bits << 1 | (h_bits.m_bits & 1);
		h_bits.m_bits >>= 1;
		--h_bits.m_bit_count;
	}
	return bits;
}

unit16 CreateTree()
{
	if (0 != GetBits(1))
	{
		unit16 v = h_bits.token++;
		if (v >= 511)
		{
			printf("HuffmanÊý¾ÝÓÐÎó£¡\n");
			system("pause");
			exit(0);
		}
		h_bits.lhs[v] = CreateTree();
		h_bits.rhs[v] = CreateTree();
		return v;
	}
	else
		return (unit16)GetBits(8);
}

void Unpack(unit32 dst_length)
{
	unit32 dst = 0;
	h_bits.token = 256;
	unit16 root = CreateTree();
	while (dst < dst_length)
	{
		unit16 symbol = root;
		while (symbol >= 0x100)
		{
			if (0 != GetBits(1))
				symbol = h_bits.rhs[symbol];
			else
				symbol = h_bits.lhs[symbol];
		}
		h_bits.m_output[dst++] = (unit8)symbol;
	}
}

void HuffmanDecoder(unit8 *src, unit32 index, unit32 length,unit32 dst_length, unit8 *dst)
{
	h_bits.m_input = src;
	h_bits.m_output = dst;

	h_bits.m_src = index;
	h_bits.m_bit_count = 0;

	h_bits.token = 256;
	
	Unpack(dst_length);
}