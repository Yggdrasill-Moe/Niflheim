/*
用于封包文件头为CPZ6的cpz文件
made by Darkness-TX
2023.01.02
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <Windows.h>
#include <locale.h>
#include "MD5.h"
#include "cmvs_md5.h"

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;
typedef unsigned __int64 unit64;

unit32 FileNum = 0;//总文件数，初始计数为0

struct cpz_header
{
	unit32 Magic;//CPZ6
	unit32 DirCount;
	unit32 DirIndexLength;
	unit32 FileIndexLength;
	unit32 IndexVerify[4];
	unit32 Md5Data[4];
	unit32 IndexKey;
	unit32 IsEncrypt;//1
	unit32 IndexSeed;
	unit32 HeaderCRC;
}CPZ_Header, CPZ_Header_new;

typedef struct cpz_file_index
{
	unit32 IndexLength;
	unit32 Offset;
	unit32 unk1;//0
	unit32 Length;
	unit32 CRC;
	unit32 FileKey;
	LPWSTR FileName;
	struct cpz_file_index* next;
}NodeCPZ_File_Index, *LinkCPZ_FIle_Index;

typedef struct cpz_dir_index
{
	unit32 IndexLength;
	unit32 FileCount;
	unit32 FileIndexOffset;
	unit32 DirKey;
	LPWSTR DirName;
	unit32 FileIndexLength;//自制
	struct cpz_file_index* file_index;
	struct cpz_dir_index* next;
}NodeCPZ_Dir_Index, *LinkCPZ_Dir_Index;
LinkCPZ_Dir_Index CPZ_Dir_Index;

unit8 ByteString[96] = {
	0x89, 0xF0, 0x90, 0xCD, 0x82, 0xB7, 0x82, 0xE9, 0x88, 0xAB, 0x82, 0xA2, 0x8E, 0x71, 0x82, 0xCD,
	0x83, 0x8A, 0x83, 0x52, 0x82, 0xAA, 0x82, 0xA8, 0x8E, 0x64, 0x92, 0x75, 0x82, 0xAB, 0x82, 0xB5,
	0x82, 0xBF, 0x82, 0xE1, 0x82, 0xA2, 0x82, 0xDC, 0x82, 0xB7, 0x81, 0x42, 0x8E, 0xF4, 0x82, 0xED,
	0x82, 0xEA, 0x82, 0xBF, 0x82, 0xE1, 0x82, 0xA2, 0x82, 0xDC, 0x82, 0xB7, 0x82, 0xE6, 0x81, 0x60,
	0x81, 0x41, 0x82, 0xC6, 0x82, 0xA2, 0x82, 0xA4, 0x82, 0xA9, 0x82, 0xE0, 0x82, 0xA4, 0x8E, 0xF4,
	0x82, 0xC1, 0x82, 0xBF, 0x82, 0xE1, 0x82, 0xA2, 0x82, 0xDC, 0x82, 0xB5, 0x82, 0xBD, 0x81, 0xF4
};

unit32 Ror(unit32 N, unit8 Bit)
{
	return (N << (32 - Bit)) + (N >> Bit);
}

unit32 Rol(unit32 N, unit8 Bit)
{
	return (N << Bit) + (N >> (32 - Bit));
}

unit32 CheckCRC(unit8* data, unit32 len, unit32 crc)
{
	unit32* buff = (unit32*)data;
	unit32 k = 0;
	unit32 i = 0;
	unit32 count = len / 4;
	for (k = 0; k < count; k++)
		crc += buff[k];
	for (i = 0; i < (len & 3); i++)
		crc += *((unit8*)&buff[k] + i);
	return crc;
}

void CPZHeaderDecrypt()
{
	CPZ_Header.DirCount ^= 0xFE3A53DA;
	CPZ_Header.DirIndexLength ^= 0x37F298E8;
	CPZ_Header.FileIndexLength ^= 0x7A6F3A2D;
	CPZ_Header.Md5Data[0] ^= 0x43DE7C1A;
	CPZ_Header.Md5Data[1] ^= 0xCC65F416;
	CPZ_Header.Md5Data[2] ^= 0xD016A93D;
	CPZ_Header.Md5Data[3] ^= 0x97A3BA9B;
	CPZ_Header.IndexKey ^= 0xAE7D39B7;
	CPZ_Header.IsEncrypt ^= 0xFB73A956;
	CPZ_Header.IndexSeed ^= 0x37ACF832;
	CPZ_Header.IndexSeed = Ror(CPZ_Header.IndexSeed, 5);
	CPZ_Header.IndexSeed *= 0x7DA8F173;
	CPZ_Header.IndexSeed += 0x13712765;

	cmvs_md5_ctx CTX;
	cmvs_md5(CPZ_Header.Md5Data, &CTX);
	CPZ_Header.Md5Data[0] ^= 0x45A76C2F;
	CPZ_Header.Md5Data[1] -= 0x5BA17FCB;
	CPZ_Header.Md5Data[2] ^= 0x79ABE8AD;
	CPZ_Header.Md5Data[3] -= 0x1C08561B;
}

void CPZHeaderEncrypt()
{
	CPZ_Header_new.DirCount = CPZ_Header.DirCount ^ 0xFE3A53DA;
	CPZ_Header_new.DirIndexLength = CPZ_Header.DirIndexLength ^ 0x37F298E8;
	CPZ_Header_new.FileIndexLength = CPZ_Header.FileIndexLength ^ 0x7A6F3A2D;
	CPZ_Header_new.IndexKey = CPZ_Header.IndexKey ^ 0xAE7D39B7;
	CPZ_Header_new.IsEncrypt = CPZ_Header.IsEncrypt ^ 0xFB73A956;
	unit32 InitCheckcrc = 0x923A564C;
	CPZ_Header_new.HeaderCRC = CheckCRC((unit8*)&CPZ_Header_new, 0x3C, InitCheckcrc);
}

BOOL IndexVerify(unit8* data, unit32 len)
{
	unit32 verify[4];
	MD5_CTX CTX;
	MD5Init(&CTX);
	MD5Update(&CTX, data, len);
	MD5Final((unit8*)verify, &CTX);
	if (CPZ_Header.IndexVerify[0] != verify[0] || CPZ_Header.IndexVerify[1] != verify[1] || CPZ_Header.IndexVerify[2] != verify[2] || CPZ_Header.IndexVerify[3] != verify[3])
		return FALSE;
	return TRUE;
}

void IndexMD5(unit8* data, unit32 len)
{
	unit32 verify[4];
	MD5_CTX CTX;
	MD5Init(&CTX);
	MD5Update(&CTX, data, len);
	MD5Final((unit8*)verify, &CTX);
	CPZ_Header_new.IndexVerify[0] = verify[0];
	CPZ_Header_new.IndexVerify[1] = verify[1];
	CPZ_Header_new.IndexVerify[2] = verify[2];
	CPZ_Header_new.IndexVerify[3] = verify[3];
}

unit32* GetIndexTable1(unit32 IndexKey)
{
	IndexKey ^= 0x3795b39a;
	unit32 DwordString[24];
	memcpy(DwordString, ByteString, 96);
	for (unit32 i = 0; i < 24; i++)
		DwordString[i] -= IndexKey;
	return DwordString;
}

unit8 GetIndexRorBit1(unit32 IndexKey)
{
	IndexKey ^= 0x3795b39a;
	unit32 Temp = IndexKey;
	Temp >>= 8;
	Temp ^= IndexKey;
	Temp >>= 8;
	Temp ^= IndexKey;
	Temp >>= 8;
	Temp ^= IndexKey;
	Temp ^= 0xfffffffb;
	Temp &= 0xf;
	Temp += 7;
	return Temp;
}

void CPZIndexDecrypt1(unit8* Buff, unit32 IndexLength, unit32 IndexKey)
{
	unit32* IndexTable1 = GetIndexTable1(IndexKey);
	unit8 RorBit = GetIndexRorBit1(IndexKey);
	unit32* IndexBuff = (unit32*)Buff;
	unit32 Flag = 5;
	for (unit32 i = 0; i < IndexLength / 4; i++)
	{
		IndexBuff[i] ^= IndexTable1[(5 + i) % 0x18];
		IndexBuff[i] += 0x784c5062;
		IndexBuff[i] = Ror(IndexBuff[i], RorBit);
		IndexBuff[i] += 0x1010101;
		Flag++;
		Flag %= 0x18;
	}
	for (unit32 i = IndexLength / 4 * 4; i < IndexLength; i++)
	{
		unit32 Temp = IndexTable1[Flag % 0x18];
		Temp >>= 4 * (i % 4);
		Buff[i] ^= (unit8)Temp;
		Buff[i] -= 0x7d;
		Flag++;
	}
}

void CPZIndexEncrypt1(unit8* Buff, unit32 IndexLength, unit32 IndexKey)
{
	unit32* IndexTable1 = GetIndexTable1(IndexKey);
	unit8 RorBit = GetIndexRorBit1(IndexKey);
	unit32* IndexBuff = (unit32*)Buff;
	unit32 Flag = 5;
	for (unit32 i = 0; i < IndexLength / 4; i++)
	{
		IndexBuff[i] -= 0x1010101;
		IndexBuff[i] = Rol(IndexBuff[i], RorBit);
		IndexBuff[i] -= 0x784c5062;
		IndexBuff[i] ^= IndexTable1[(5 + i) % 0x18];
		Flag++;
		Flag %= 0x18;
	}
	for (unit32 i = IndexLength / 4 * 4; i < IndexLength; i++)
	{
		unit32 Temp = IndexTable1[Flag % 0x18];
		Temp >>= 4 * (i % 4);
		Buff[i] += 0x7d;
		Buff[i] ^= (unit8)Temp;
		Flag++;
	}
}

unit8* GetByteTable2(unit32 Key, unit32 Seed)
{
	unit8 ByteTable[0x100];
	unit32 i = 0;
	for (i = 0; i < 0x100; i++)
		ByteTable[i] = i;
	unit8 temp;
	for (i = 0; i < 0x100; i++)
	{
		temp = ByteTable[(Key >> 0x10) & 0xff];
		ByteTable[(Key >> 0x10) & 0xff] = ByteTable[Key & 0xff];
		ByteTable[Key & 0xff] = temp;
		temp = ByteTable[(Key >> 0x8) & 0xff];
		ByteTable[(Key >> 0x8) & 0xff] = ByteTable[Key >> 0x18];
		ByteTable[Key >> 0x18] = temp;
		Key = Ror(Key, 2);
		Key *= 0x1a74f195;
		Key += Seed;
	}
	return ByteTable;
}

void CPZIndexDecrypt2(unit8* IndexBuff, unit32 IndexLength, unit32 IndexKey, unit32 Seed)
{
	unit8* ByteTable = GetByteTable2(IndexKey, Seed);
	for (unit32 i = 0; i < IndexLength; i++)
		IndexBuff[i] = ByteTable[IndexBuff[i] ^ 0x3a];
}

void CPZIndexEncrypt2(unit8* IndexBuff, unit32 IndexLength, unit32 IndexKey, unit32 Seed)
{
	unit8* ByteTable = GetByteTable2(IndexKey, Seed);
	unit32 j = 0;
	for (unit32 i = 0; i < IndexLength; i++)
	{
		for (j = 0; j < 0x100; j++)
			if (IndexBuff[i] == ByteTable[j])
				break;
		IndexBuff[i] = j ^ 0x3a;
	}
}

unit32* GetIndexKey3()
{
	unit32 Key[4];
	Key[0] = CPZ_Header.Md5Data[0] ^ (CPZ_Header.IndexKey + 0x76a3bf29);
	Key[1] = CPZ_Header.IndexKey ^ CPZ_Header.Md5Data[1];
	Key[2] = CPZ_Header.Md5Data[2] ^ (CPZ_Header.IndexKey + 0x10000000);
	Key[3] = CPZ_Header.IndexKey ^ CPZ_Header.Md5Data[3];
	return Key;
}

void CPZIndexDecrypt3(unit8* Buff, unit32 IndexLength, unit32* Key, unit32 Seed)
{
	unit32* IndexBuff = (unit32*)Buff;
	unit32 Flag = 0;
	for (unit32 i = 0; i < IndexLength / 4; i++)
	{
		IndexBuff[i] ^= Key[i & 3];
		IndexBuff[i] -= 0x4a91c262;
		IndexBuff[i] = Rol(IndexBuff[i], 3);
		IndexBuff[i] -= Seed;
		Seed += 0x10fb562a;
		Flag++; Flag &= 3;
	}
	for (unit32 i = IndexLength / 4 * 4; i < IndexLength; i++)
	{
		unit32 Temp = Key[Flag];
		Temp >>= 6;
		Buff[i] ^= (unit8)Temp;
		Buff[i] += 0x37;
		Flag++; Flag &= 3;
	}
}

void CPZIndexEncrypt3(unit8* Buff, unit32 IndexLength, unit32* Key, unit32 Seed)
{
	unit32* IndexBuff = (unit32*)Buff;
	unit32 Flag = 0;
	for (unit32 i = 0; i < IndexLength / 4; i++)
	{
		IndexBuff[i] += Seed;
		IndexBuff[i] = Ror(IndexBuff[i], 3);
		IndexBuff[i] += 0x4a91c262;
		IndexBuff[i] ^= Key[i & 3];
		Seed += 0x10fb562a;
		Flag++; Flag &= 3;
	}
	for (unit32 i = IndexLength / 4 * 4; i < IndexLength; i++)
	{
		unit32 Temp = Key[Flag];
		Temp >>= 6;
		Buff[i] -= 0x37;
		Buff[i] ^= (unit8)Temp;
		Flag++; Flag &= 3;
	}
}

unit32* GetFileIndexKey2(unit32 DirKey)
{
	unit32 Key[4];
	Key[0] = DirKey ^ CPZ_Header.Md5Data[0];
	Key[2] = DirKey ^ CPZ_Header.Md5Data[2];
	Key[1] = (DirKey + 0x11003322) ^ CPZ_Header.Md5Data[1];
	DirKey += 0x34216785;
	Key[3] = DirKey ^ CPZ_Header.Md5Data[3];
	return Key;
}

void CPZFileIndexDecrypt1(unit8* Buff, unit32 Length, unit32 Key, unit32 Seed)
{
	unit8* ByteTable = GetByteTable2(Key, Seed);
	for (unit32 i = 0; i < Length; i++)
		Buff[i] = ByteTable[Buff[i] ^ 0x7e];
}

void CPZFileIndexEncrypt1(unit8* Buff, unit32 Length, unit32 Key, unit32 Seed)
{
	unit8* ByteTable = GetByteTable2(Key, Seed);
	unit32 j = 0;
	for (unit32 i = 0; i < Length; i++)
	{
		for (j = 0; j < 0x100; j++)
			if (Buff[i] == ByteTable[j])
				break;
		Buff[i] = j ^ 0x7e;
	}
}

void CPZFileIndexDecrypt2(unit8* FileIndexBuff, unit32 Length, unit32 DirKey)
{
	unit32* FileIndexKey = GetFileIndexKey2(DirKey);
	unit32* Buff = (unit32*)FileIndexBuff;
	unit32 Seed = 0x2a65cb4f;
	unit32 Flag = 0;
	for (unit32 i = 0; i < Length / 4; i++)
	{
		Buff[i] ^= FileIndexKey[i & 3];
		Buff[i] -= Seed;
		Buff[i] = Rol(Buff[i], 2);
		Buff[i] += 0x37a19e8b;
		Seed -= 0x139fa9b;
		Flag++;
		Flag &= 3;
	}
	for (unit32 i = Length / 4 * 4; i < Length; i++)
	{
		unit32 Temp = FileIndexKey[Flag];
		Temp >>= 4;
		FileIndexBuff[i] ^= (unit8)Temp;
		FileIndexBuff[i] += 3;
		Flag++;
		Flag &= 3;
	}
}

void CPZFileIndexEncrypt2(unit8* FileIndexBuff, unit32 Length, unit32 DirKey)
{
	unit32* FileIndexKey = GetFileIndexKey2(DirKey);
	unit32* Buff = (unit32*)FileIndexBuff;
	unit32 Seed = 0x2a65cb4f;
	unit32 Flag = 0;
	for (unit32 i = 0; i < Length / 4; i++)
	{
		Buff[i] -= 0x37a19e8b;
		Buff[i] = Ror(Buff[i], 2);
		Buff[i] += Seed;
		Buff[i] ^= FileIndexKey[i & 3];
		Seed -= 0x139fa9b;
		Flag++;
		Flag &= 3;
	}
	for (unit32 i = Length / 4 * 4; i < Length; i++)
	{
		unit32 Temp = FileIndexKey[Flag];
		Temp >>= 4;
		FileIndexBuff[i] -= 3;
		FileIndexBuff[i] ^= (unit8)Temp;
		Flag++;
		Flag &= 3;
	}
}

void ReadDirIndex(unit8* data)
{
	unit32 i = 0;
	NodeCPZ_Dir_Index* q;
	q = malloc(sizeof(NodeCPZ_Dir_Index));
	q->file_index = NULL;
	q->next = NULL;
	CPZ_Dir_Index = q;
	while (i < CPZ_Header.DirIndexLength)
	{
		NodeCPZ_Dir_Index* row;
		row = malloc(sizeof(NodeCPZ_Dir_Index));
		row->next = NULL;
		row->file_index = NULL;
		memcpy(row, data + i, 0x10);
		row->DirName = malloc(MAX_PATH * 2);
		memset(row->DirName, 0, MAX_PATH * 2);
		MultiByteToWideChar(932, 0, data + i + 0x10, strlen(data + i + 0x10), row->DirName, MAX_PATH);
		i += row->IndexLength;
		q->next = row;
		q = q->next;
	}
	q = CPZ_Dir_Index;
	if (q->next)
	{
		q = q->next;
		for (i = 0; i < CPZ_Header.DirCount - 1; i++)
		{
			q->FileIndexLength = q->next->FileIndexOffset - q->FileIndexOffset;
			q = q->next;
		}
		q->FileIndexLength = CPZ_Header.FileIndexLength - q->FileIndexOffset;
	}
}

void ReadFileIndex(unit8* data)
{
	unit32 i = 0;
	NodeCPZ_Dir_Index* q;
	q = CPZ_Dir_Index;
	while (q->next)
	{
		i = 0;
		q = q->next;
		NodeCPZ_File_Index* p;
		p = malloc(sizeof(NodeCPZ_File_Index));
		p->next = NULL;
		if (q->FileIndexLength)
			q->file_index = p;
		CPZFileIndexDecrypt1(data + CPZ_Header.DirIndexLength + q->FileIndexOffset, q->FileIndexLength, CPZ_Header.IndexKey, CPZ_Header.Md5Data[2]);
		CPZFileIndexDecrypt2(data + CPZ_Header.DirIndexLength + q->FileIndexOffset, q->FileIndexLength, q->DirKey);
		while (i < q->FileIndexLength)
		{
			FileNum++;
			memcpy(p, data + q->FileIndexOffset + CPZ_Header.DirIndexLength + i, 0x18);
			p->FileName = malloc(100 * 2);
			memset(p->FileName, 0, 100 * 2);
			MultiByteToWideChar(932, 0, data + q->FileIndexOffset + CPZ_Header.DirIndexLength + i + 0x18, strlen(data + q->FileIndexOffset + CPZ_Header.DirIndexLength + i + 0x18), p->FileName, MAX_PATH);
			i += p->IndexLength;
			if (i < q->FileIndexLength)
			{
				p->next = malloc(sizeof(NodeCPZ_File_Index));
				p = p->next;
			}
		}
	}
}

unit8* ReadIndex(FILE* src)
{
	unit32 IndexSize = 0, i = 0;
	unit8* data;
	fread(&CPZ_Header, sizeof(CPZ_Header), 1, src);
	if (CPZ_Header.Magic != 0x365A5043)
	{
		fclose(src);
		printf("文件头不是CPZ6！\n");
		system("pause");
		exit(0);
	}
	unit32 InitCheckcrc = 0x923A564C;
	if (CPZ_Header.HeaderCRC != CheckCRC((unit8*)&CPZ_Header, 0x3C, InitCheckcrc))
	{
		printf("验证不通过，请检查文件头是否损坏或是不支持的文件类型。\n");
		system("pause");
		exit(0);
	}
	memcpy(&CPZ_Header_new, &CPZ_Header, sizeof(CPZ_Header));
	CPZHeaderDecrypt();
	IndexSize = CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength;
	data = malloc(IndexSize);
	fread(data, IndexSize, 1, src);
	if (!IndexVerify(data, IndexSize))
	{
		printf("验证不通过，请检查索引是否损坏或是不支持的文件类型。\n");
		system("pause");
		exit(0);
	};
	CPZIndexDecrypt1(data, CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength, CPZ_Header.IndexKey);
	CPZIndexDecrypt2(data, CPZ_Header.DirIndexLength, CPZ_Header.IndexKey, CPZ_Header.Md5Data[1]);
	unit32* Key = GetIndexKey3();
	CPZIndexDecrypt3(data, CPZ_Header.DirIndexLength, Key, 0x76548aef);
	unit8 version = 0;
	memcpy(&version, (unit8*)&CPZ_Header + 3, 1);
	printf("CPZHeader ver:%c dir_num:%d dir_index_len:0x%X file_index_len:0x%X header_crc:0x%X\n\n", version, CPZ_Header.DirCount, CPZ_Header.DirIndexLength, CPZ_Header.FileIndexLength, CPZ_Header.HeaderCRC);
	return data;
}

void CPZResourceEncrypt(unit8* FileBuff, unit32 Length, unit32 IndexKey, unit32* Md5Data, unit32 Seed)
{
	unit32 DecryptKey[32], j = 0;
	unit32* Buff = (unit32*)FileBuff;
	unit32 Key = Md5Data[1] >> 2;
	unit8* ByteTable = GetByteTable2(Md5Data[3], IndexKey);
	unit8* p = (unit8*)DecryptKey;
	for (unit32 i = 0; i < 96; i++)
		p[i] = (unit8)Key ^ ByteTable[ByteString[i] & 0xff];
	for (unit32 i = 0; i < 24; i++)
		DecryptKey[i] ^= Seed;
	Key = 0x2748c39e;
	unit32 Flag = 0x0a;
	for (unit32 i = Length / 4 * 4; i < Length; i++)
	{
		for (j = 0; j < 0x100; j++)
			if (FileBuff[i] == ByteTable[j])
				break;
		FileBuff[i] = j ^ 0xae;
	}
	for (unit32 i = 0; i < Length / 4; i++)
	{
		unit32 Temp = DecryptKey[Flag];
		Temp >>= 1;
		Temp ^= DecryptKey[(Key >> 6) & 0xf];
		unit32 Temp2 = Buff[i];
		Temp2 ^= Md5Data[Key & 3];
		Temp2 += Seed;
		Key = Key + Seed + Buff[i];
		Buff[i] = Temp ^ Temp2;
		Flag++;
		Flag &= 0xf;
	}
}

void PackFile(char* fname)
{
	FILE* src, * dst;
	unit32 i = 0;
	src = fopen(fname, "rb");
	unit8* data = NULL;
	unit8* indexdata = ReadIndex(src);
	ReadDirIndex(indexdata);
	ReadFileIndex(indexdata);
	fclose(src);
	unit8 dirname[MAX_PATH];
	wchar_t filename[MAX_PATH];
	sprintf(dirname, "%s_new", fname);
	dst = fopen(dirname, "wb");
	unit32 headsize = sizeof(CPZ_Header) + CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength;
	fseek(dst, headsize, SEEK_SET);
	sprintf(dirname, "%s_unpack", fname);
	_chdir(dirname);
	NodeCPZ_Dir_Index* q = CPZ_Dir_Index;
	while (q->next)
	{
		q = q->next;
		wprintf(L"dirname:%ls file_num:%d file_index_offset:0x%X file_index_len:0x%X dir_key:0x%X\n", q->DirName, q->FileCount, q->FileIndexOffset, q->FileIndexLength, q->DirKey);
		NodeCPZ_File_Index* p = q->file_index;
		while (p)
		{
			wsprintfW(filename, L"%ls/%ls", q->DirName, p->FileName);
			src = _wfopen(filename, L"rb");
			fseek(src, 0, SEEK_END);
			p->Length = ftell(src);
			fseek(src, 0, SEEK_SET);
			data = malloc(p->Length);
			fread(data, p->Length, 1, src);
			fclose(src);
			if (CPZ_Header.IsEncrypt)
				CPZResourceEncrypt(data, p->Length, CPZ_Header.IndexKey, CPZ_Header.Md5Data, CPZ_Header.IndexSeed ^ ((CPZ_Header.IndexKey ^ (q->DirKey + p->FileKey)) + CPZ_Header.DirCount + 0xa3d61785));
			p->CRC = CheckCRC(data, p->Length, 0x5A902B7C);//sub_455D90 in ChronoClock
			p->Offset = ftell(dst) - headsize;
			fwrite(data, p->Length, 1, dst);
			free(data);
			wprintf(L"\t%s offset:0x%X size:0x%X file_key:0x%X crc:0x%X\n", p->FileName, p->Offset, p->Length, p->FileKey, p->CRC);		
			p = p->next;
		}
	}
	q = CPZ_Dir_Index;
	while (q->next)
	{
		i = 0;
		q = q->next;
		if (q->FileIndexLength)
		{
			NodeCPZ_File_Index* p = q->file_index;
			while (i < q->FileIndexLength)
			{
				memcpy(indexdata + q->FileIndexOffset + CPZ_Header.DirIndexLength + i + 4, &p->Offset, 4);
				memcpy(indexdata + q->FileIndexOffset + CPZ_Header.DirIndexLength + i + 0x0C, &p->Length, 4);
				memcpy(indexdata + q->FileIndexOffset + CPZ_Header.DirIndexLength + i + 0x10, &p->CRC, 4);
				i += p->IndexLength;
				p = p->next;
			}
			CPZFileIndexEncrypt2(indexdata + CPZ_Header.DirIndexLength + q->FileIndexOffset, q->FileIndexLength, q->DirKey);
			CPZFileIndexEncrypt1(indexdata + CPZ_Header.DirIndexLength + q->FileIndexOffset, q->FileIndexLength, CPZ_Header.IndexKey, CPZ_Header.Md5Data[2]);
		}
	}
	unit32* Key = GetIndexKey3();
	CPZIndexEncrypt3(indexdata, CPZ_Header.DirIndexLength, Key, 0x76548aef);
	CPZIndexEncrypt2(indexdata, CPZ_Header.DirIndexLength, CPZ_Header.IndexKey, CPZ_Header.Md5Data[1]);
	CPZIndexEncrypt1(indexdata, CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength, CPZ_Header.IndexKey);
	IndexMD5(indexdata, CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength);
	fseek(dst, sizeof(CPZ_Header), SEEK_SET);
	fwrite(indexdata, CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength, 1, dst);
	free(indexdata);
	CPZHeaderEncrypt();
	fseek(dst, 0, SEEK_SET);
	fwrite(&CPZ_Header_new, sizeof(CPZ_Header), 1, dst);
	fclose(dst);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-cmvs\n用于封包文件头为CPZ6的cpz文件。\n将cpz文件拖到程序上。\nby Darkness-TX 2023.01.02\n\n");
	PackFile(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}