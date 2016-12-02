/**
 * @file		HuffmanEncoder.c
 * @brief		The implementation of the huffman encoder (NeXaS flavor).
 * @copyright	Covered by 2-clause BSD, please refer to license.txt.
 * @author		CloudiDust
 * @date		2010.02
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <direct.h>
#include <windows.h>
#include <stdbool.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

struct TreeNode {
	unit16 parent;
	unit16 isrchild;
	unit16 lchild;
	unit16 rchild;
	unit32 weight;
};
typedef struct TreeNode TreeNode;

struct HeapNode {
	unit32 elem;
	unit32 weight;
};
typedef struct HeapNode HeapNode;

struct MinHeap {
	HeapNode* data;
	unit32 maxSize;
	unit32 currSize;
};
typedef struct MinHeap MinHeap;

struct BitStream {
	byte* data;
	unit32 dataLen;
	byte currBitIndex;
	unit32 currByteIndex;
};
typedef struct BitStream BitStream;

struct ByteArray {
	byte* data;
	unit32 length;
};
typedef struct ByteArray ByteArray;

bool heapInsert(MinHeap* heap, unit32 elem, unit32 weight) {
	if (heap->currSize == heap->maxSize) return false;

	heap->data[heap->currSize].elem = elem;
	heap->data[heap->currSize].weight = weight;

	unit32 index = (heap->currSize)++;
	bubbleUp(heap, index);
	return true;
}

MinHeap* newMinHeap(unit32 size) {
	MinHeap* heap = malloc(sizeof(MinHeap));
	heap->data = malloc(sizeof(HeapNode) * size);
	heap->maxSize = size;
	heap->currSize = 0;
	return heap;
}

unit32 heapElementCount(const MinHeap* heap) {
	return heap->currSize;
}

void deleteMinHeap(MinHeap* heap) {
	if (heap == NULL) return;
	if (heap->data != NULL) free(heap->data);
	free(heap);
}

bool heapPopMin(MinHeap* heap, unit32* elem, unit32* weight) {
	if (heap->currSize == 0) return false;

	*elem = heap->data[0].elem;
	*weight = heap->data[0].weight;
	swap(heap, 0, heap->currSize - 1);

	--(heap->currSize);
	bubbleDown(heap, 0);
	return true;
}

bool bsSetNextBit(BitStream* bs, byte data) {
	if (bs->currByteIndex == bs->dataLen) return false;
	setBit(&(bs->data[bs->currByteIndex]), (bs->currBitIndex)++, data);
	if (bs->currBitIndex == 8) {
		bs->currBitIndex = 0;
		++(bs->currByteIndex);
	}
	return true;
}

bool bsSetNextByte(BitStream* bs, byte data) {
	if (bs->currBitIndex > 0 && bs->currByteIndex == bs->dataLen - 1)
		return false;

	if (bs->currBitIndex == 0) {
		bs->data[(bs->currByteIndex)++] = data;
		return true;
	}
	byte offset = bs->currBitIndex;
	byte part1 = data >> offset;
	byte part2 = data << (8 - offset);
	bs->data[bs->currByteIndex] = ((bs->data[bs->currByteIndex] >> (8 - offset)) << (8 - offset)) | part1;
	bs->data[bs->currByteIndex + 1] = ((bs->data[bs->currByteIndex + 1] << offset) >> offset) | part2;
	++bs->currByteIndex;
	return true;
}

static unit32 buildTree(const wchar_t* treeName, TreeNode tree [], const byte* originalData, unit32 originalLen)
{
	/// The first 256 nodes are leaves that represent byte values.
	for (unit32 i = 0; i < originalLen; ++i) {
		++(tree[originalData[i]].weight);
	}

	/// Generate the other nodes.
	MinHeap* heap = newMinHeap(256);
	for (unit16 i = 0; i < 256; ++i) {
		/// Byte values that don't appear in the data are ignored.
		if (tree[i].weight > 0) {
			heapInsert(heap, i, tree[i].weight);
		}
	}

	unit16 index = 256;
	while (heapElementCount(heap) > 1) {
		unit32 ia, ib, wa, wb;
		heapPopMin(heap, &ia, &wa);
		heapPopMin(heap, &ib, &wb);
		tree[index].lchild = ia;
		tree[index].rchild = ib;
		tree[index].weight = wa + wb;
		tree[ia].parent = index;
		tree[ia].isrchild = false;
		tree[ib].parent = index;
		tree[ib].isrchild = true;
		heapInsert(heap, index, wa + wb);
		++index;
	}
	unit32 rootIndex, rootWeight;
	heapPopMin(heap, &rootIndex, &rootWeight);
	deleteMinHeap(heap);
	return rootIndex;
}

static void subtreeEncodingWorker(const wchar_t* treeName, TreeNode tree[], unit32 rootIndex, BitStream* bs) {
	if (rootIndex < 256) {
		bsSetNextBit(bs, 0);
		bsSetNextByte(bs, rootIndex);
	} else {
		bsSetNextBit(bs, 1);
		subtreeEncodingWorker(treeName, tree, tree[rootIndex].lchild, bs);
		subtreeEncodingWorker(treeName, tree, tree[rootIndex].rchild, bs);
	}
}

static void encodeTree(const wchar_t* treeName, TreeNode tree[], unit32 rootIndex, BitStream* bs) {
	subtreeEncodingWorker(treeName, tree, rootIndex, bs);
}

static void encodeData(const wchar_t* treeName, TreeNode tree[], unit32 rootIndex, const byte* data, unit32 oriLen, BitStream* bs) {
	unit8 code[256][256];
	unit8 codeLen[256];
	memset(code, 0, 256 * 256);
	memset(codeLen, 0, 256);

	for (unit16 i = 0; i < 256; ++i) {
		if (tree[i].weight == 0) continue;
		unit16 index = i;
		while (index != rootIndex) {
			code[i][codeLen[i]++] = tree[index].isrchild;
			index = tree[index].parent;
		}
	}

	for (unit32 i = 0; i < oriLen; ++i) {
		unit8* thisCode = code[data[i]];
		for (short j = codeLen[data[i]] - 1; j >= 0; --j) {
			bsSetNextBit(bs, thisCode[j]);
		}
	}
}

ByteArray* huffmanEncode(const wchar_t* treeName, const byte* originalData, unit32 originalLen) {
	/// At most 256 leaves, and 255 internal nodes, but 512 just looks nicer. ;)
	TreeNode tree[512];
	memset(tree, 0, sizeof(tree));
	unit32 rootIndex = buildTree(treeName, tree, originalData, originalLen);
	ByteArray* encodedData = newByteArray(2 * originalLen);
	BitStream* encodedStream = newBitStream(baData(encodedData), 2 * originalLen);
	encodeTree(treeName, tree, rootIndex, encodedStream);
	encodeData(treeName, tree, rootIndex, originalData, originalLen, encodedStream);
	unit32 encodedLen = bsGetCurrentByteIndex(encodedStream) + 1;
	ByteArray* resultData = newByteArray(encodedLen);
	memcpy(baData(resultData), baData(encodedData), encodedLen);
	deleteBitStream(encodedStream);
	deleteByteArray(encodedData);
	return resultData;
}
