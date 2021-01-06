//base on crass:NeXAS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <direct.h>
#include <windows.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

struct bits 
{
	unsigned long curbits;
	unsigned long curbyte;
	unsigned char cache;
	unsigned char *stream;
	unsigned long stream_length;
};

void bits_init(struct bits *bits, unsigned char *stream, unsigned long stream_length)
{
	memset(bits, 0, sizeof(*bits));
	bits->stream = stream;
	bits->stream_length = stream_length;
}

int bits_get_high(struct bits *bits, unsigned int req_bits, unsigned int *retval)
{
	unsigned int bits_value = 0;

	while (req_bits > 0)
	{
		unsigned int req;

		if (!bits->curbits)
		{
			if (bits->curbyte >= bits->stream_length)
				return -1;
			bits->cache = bits->stream[bits->curbyte++];
			bits->curbits = 8;
		}

		if (bits->curbits < req_bits)
			req = bits->curbits;
		else
			req = req_bits;

		bits_value <<= req;
		bits_value |= bits->cache >> (bits->curbits - req);
		bits->cache &= (1 << (bits->curbits - req)) - 1;
		req_bits -= req;
		bits->curbits -= req;
	}
	*retval = bits_value;
	return 0;
}

int bit_get_high(struct bits *bits, void *retval)
{
	return bits_get_high(bits, 1, (unsigned int *)retval);
}

int huffman_tree_create(struct bits *bits, unit16 children[2][255], unsigned int *index, unit16 *retval)
{
	unsigned int bitval;
	unit16 child;

	if (bit_get_high(bits, &bitval))
		return -1;
	
	if (bitval)
	{
		unsigned int parent;

		parent = *index;
		*index = parent + 1;
					
		if (huffman_tree_create(bits, children, index, &child))
			return -1;
		children[0][parent - 256] = child;
		
		if (huffman_tree_create(bits, children, index, &child))
			return -1;
		children[1][parent - 256] = child;

		child = parent;
	}
	else
	{
		unsigned int byteval;
		
		if (bits_get_high(bits, 8, &byteval))
			return -1;
		
		child = byteval;			
	}
	*retval = child;
	
	return 0;
}

int huffman_uncompress(unsigned char *uncompr, unsigned long *uncomprlen, unsigned char *compr, unsigned long comprlen)
{
	struct bits bits;
	unit16 children[2][255];
	unsigned int index = 256;
	unsigned long max_uncomprlen;
	unsigned long act_uncomprlen;
	unsigned int bitval;
	unit16 retval;	
	
	bits_init(&bits, compr, comprlen);
	if (huffman_tree_create(&bits, children, &index, &retval))
		return -1;
	if (retval != 256)
		return -1;

	index = 0;	/* 从根结点开始遍历 */
	act_uncomprlen = 0;
	max_uncomprlen = *uncomprlen;
	while (!bit_get_high(&bits, &bitval))
	{
		if (bitval)
			retval = children[1][index];
		else
			retval = children[0][index];
	
		if (retval >= 256)
			index = retval - 256;
		else {
			if (act_uncomprlen >= max_uncomprlen)
				break;
			uncompr[act_uncomprlen++] = (unit8)retval;
			index = 0;	/* 返回到根结点 */
		}
	}
	*uncomprlen = act_uncomprlen;

	return 0;
}