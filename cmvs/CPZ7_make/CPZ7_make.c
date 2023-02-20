/*
用于生成文件头为CPZ7的cpz文件
made by Darkness-TX
2023.02.17
*/
#define _CRT_SECURE_NO_WARNINGS
#define _USE_32BIT_TIME_T
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <time.h>
#include <Windows.h>
#include <locale.h>
#include "MD5.h"
#include "cmvs_md5.h"
#include "HuffmanDecoder.h"
/*
CPZ7加的这段哈夫曼解压后居然只是直接对索引异或，搞半天有效加密就TM只是一次异或！！！
妈个鸡，Purple在搞毛啊！！！
还不如学NeXAS直接对索引哈夫曼压缩都更有效果，哪怕对加密后的索引用哈夫曼会增大体积，
但你这段0x400压完也增大了啊，还不如直接对索引哈夫曼得了。
不过GIGA毕竟是正规软件厂的子公司，技术强点也正常，缅怀下即将死去的GIGA，
NeXAS没有用什么无效的加密，都是简单实用的，功能也强大，只是挖了不少坑，但是不搞游戏翻译那完全不会碰到。
可惜也要随着GIGA的死去成为绝唱了QAQ。
cmvs对着索引进行一大堆的操作，但是这一大堆的操作最后反馈到索引数据加密上的都是简单的一两步，也幸好不是对文件数据部分搞这些。
加密中的学问，还是应该好好学习下老前辈DES算法的思想。
简而言之，因为CPZ7加的这段加密弱智程度直逼RioShiina，根本没有反写压缩的意义，所以直接抄数据解决。

这段数据为4字节数据长度，4字节密钥，数据，然后在头部加上上面三个部分组合后算出来的MD5组成，解压后为一段0x400的密钥。
*/
#include "HuffmanData.h"

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;
typedef unsigned __int64 unit64;

unit32 FileNum = 0;//总文件数，初始计数为0
unit32 cmvsMd5Data[4];
unit32 IndexSeed;

struct cpz_header
{
	unit32 Magic;//CPZ7
	unit32 DirCount;
	unit32 DirIndexLength;
	unit32 FileIndexLength;
	unit32 IndexVerify[4];
	unit32 Md5Data[4];
	unit32 IndexKey;
	unit32 IsEncrypt;//1
	unit32 IndexSeed;
	unit32 unk;//似乎都是0xA7B09C16
	unit32 IndexKeySize;
	unit32 HeaderCRC;
}CPZ_Header;

typedef struct cpz_file_index
{
	unit32 IndexLength;
	//unit64是预留的，反正现在的版本中还是mov eax, dword ptr [esi+0xC]...........
	//unit64 Offset;
	unit32 Offset;
	unit32 unk1;//0
	//unit64 Length;
	unit32 Length;
	unit32 unk2;
	unit32 CRC;
	unit32 FileKey;
	unit8* FileNameSave;
	unit32 NameLength;
	LPWSTR FileName;
	struct cpz_file_index* next;
}NodeCPZ_File_Index, * LinkCPZ_FIle_Index;

typedef struct cpz_dir_index
{
	unit32 IndexLength;
	unit32 FileCount;
	unit32 FileIndexOffset;
	unit32 DirKey;
	unit8* DirNameSave;
	unit32 NameLength;
	LPWSTR DirName;
	unit32 FileIndexLength;//自制
	struct cpz_file_index* file_index;
	struct cpz_dir_index* next;
}NodeCPZ_Dir_Index, * LinkCPZ_Dir_Index;
LinkCPZ_Dir_Index CPZ_Dir_Index;

unit8 ByteString[96] = {
	0x89, 0xF0, 0x90, 0xCD, 0x82, 0xB7, 0x82, 0xE9, 0x88, 0xAB, 0x82, 0xA2, 0x8E, 0x71, 0x82, 0xCD,
	0x83, 0x8A, 0x83, 0x52, 0x82, 0xAA, 0x82, 0xA8, 0x8E, 0x64, 0x92, 0x75, 0x82, 0xAB, 0x82, 0xB5,
	0x82, 0xBF, 0x82, 0xE1, 0x82, 0xA2, 0x82, 0xDC, 0x82, 0xB7, 0x81, 0x42, 0x8E, 0xF4, 0x82, 0xED,
	0x82, 0xEA, 0x82, 0xBF, 0x82, 0xE1, 0x82, 0xA2, 0x82, 0xDC, 0x82, 0xB7, 0x82, 0xE6, 0x81, 0x60,
	0x81, 0x41, 0x82, 0xC6, 0x82, 0xA2, 0x82, 0xA4, 0x82, 0xA9, 0x82, 0xE0, 0x82, 0xA4, 0x8E, 0xF4,
	0x82, 0xC1, 0x82, 0xBF, 0x82, 0xE1, 0x82, 0xA2, 0x82, 0xDC, 0x82, 0xB5, 0x82, 0xBD, 0x81, 0xF4
};

unit32 process_dir(char* dname)
{
	long Handle;
	unit32 i = 0;
	struct _wfinddata_t FileInfo;
	_chdir(dname);//跳转路径
	LinkCPZ_Dir_Index q;
	q = malloc(sizeof(NodeCPZ_Dir_Index));
	q->file_index = NULL;
	q->next = NULL;
	CPZ_Dir_Index = q;
	if ((Handle = _wfindfirst(L"*.*", &FileInfo)) == -1L)
	{
		printf("没有找到匹配的项目\n");
		system("pause");
		return -1;
	}
	do
	{
		if (FileInfo.name[0] == '.')  //过滤本级目录和父目录
			continue;
		LinkCPZ_Dir_Index row;
		row = malloc(sizeof(NodeCPZ_Dir_Index));
		row->file_index = NULL;
		row->next = NULL;
		row->DirName = malloc(MAX_PATH * 2);
		memset(row->DirName, 0, MAX_PATH * 2);
		wsprintf(row->DirName, FileInfo.name);
		row->NameLength = WideCharToMultiByte(932, 0, row->DirName, -1, NULL, 0, NULL, NULL);
		row->DirNameSave = malloc(row->NameLength * 2);
		memset(row->DirNameSave, 0, row->NameLength * 2);
		WideCharToMultiByte(932, 0, row->DirName, row->NameLength, row->DirNameSave, row->NameLength, NULL, NULL);
		if (row->NameLength % 4 == 0)//四字节对齐
			row->IndexLength = 0x10 + row->NameLength;
		else
			row->IndexLength = 0x10 + row->NameLength + (4 - (row->NameLength % 4));
		row->DirKey = (unit32)FileInfo.time_create;//这里Key就选用创建时间来代替，保证是32位数就行
		row->FileCount = 0;
		q->next = row;
		q = q->next;
	} while (_wfindnext(Handle, &FileInfo) == 0);
	q = CPZ_Dir_Index;
	CPZ_Header.DirCount = 0;
	while (q->next)
	{
		q = q->next;
		_wchdir(q->DirName);
		if ((Handle = _wfindfirst(L"*.*", &FileInfo)) == -1L)
		{
			printf("没有找到匹配的项目\n");
			system("pause");
			return -1;
		}
		LinkCPZ_FIle_Index p = q->file_index;
		do
		{
			if (FileInfo.name[0] == '.')  //过滤本级目录和父目录
				continue;
			LinkCPZ_FIle_Index row;
			row = malloc(sizeof(NodeCPZ_File_Index));
			row->next = NULL;
			row->FileName = malloc(MAX_PATH * 2);
			memset(row->FileName, 0, MAX_PATH * 2);
			wsprintf(row->FileName, FileInfo.name);
			row->NameLength = WideCharToMultiByte(932, 0, row->FileName, -1, NULL, 0, NULL, NULL);
			row->FileNameSave = malloc(row->NameLength * 2);
			memset(row->FileNameSave, 0, row->NameLength * 2);
			WideCharToMultiByte(932, 0, row->FileName, row->NameLength, row->FileNameSave, row->NameLength, NULL, NULL);
			if (row->NameLength % 4 == 0)//四字节对齐
				row->IndexLength = 0x1C + row->NameLength;
			else
				row->IndexLength = 0x1C + row->NameLength + (4 - (row->NameLength % 4));
			row->FileKey = (unit32)FileInfo.time_create;//这里Key就选用创建时间来代替，保证是32位数就行
			row->Length = FileInfo.size;
			if (q->FileCount == 0)//不带头结点了
			{
				q->file_index = row;
				p = q->file_index;
			}
			else
			{
				p->next = row;
				p = p->next;
			}
			q->FileCount++;
			FileNum++;
		} while (_wfindnext(Handle, &FileInfo) == 0);
		_wchdir(L"..");
		CPZ_Header.DirCount++;
	}
	_chdir("..");
	return FileNum;
}

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

void CPZHeaderEncrypt()
{
	CPZ_Header.DirCount ^= 0xFE3A53DA;
	CPZ_Header.DirIndexLength ^= 0x37F298E8;
	CPZ_Header.FileIndexLength ^= 0x7A6F3A2D;
	CPZ_Header.IndexKey ^= 0xAE7D39B7;
	CPZ_Header.IsEncrypt ^= 0xFB73A956;
	CPZ_Header.IndexKeySize ^= 0x65EF99F3;
	unit32 InitCheckcrc = CPZ_Header.IndexKeySize - 0x6DC5A9B4;
	CPZ_Header.HeaderCRC = CheckCRC((unit8*)&CPZ_Header, 0x40, InitCheckcrc);
}

void IndexMD5(unit8* data, unit32 len)
{
	unit32 verify[4];
	MD5_CTX CTX;
	MD5Init(&CTX);
	MD5Update(&CTX, data, len);
	MD5Final((unit8*)verify, &CTX);
	CPZ_Header.IndexVerify[0] = verify[0];
	CPZ_Header.IndexVerify[1] = verify[1];
	CPZ_Header.IndexVerify[2] = verify[2];
	CPZ_Header.IndexVerify[3] = verify[3];
}

unit8* UnpackIndexKey(unit8* srcdata, unit32 offset, unit32 length)
{
	unit32 key_offset = offset + 0x14;
	unit32 packed_offset = offset + 0x18;
	unit32 packed_length = length - 0x18;
	unit32 unpacked_length = *(unit32*)&srcdata[offset + 0x10];
	unit32 i = 0;
	for (i = 0; i < packed_length; i++)
		srcdata[packed_offset + i] ^= srcdata[key_offset + (i & 3)];//其实就是key_offset那的4字节循环异或
	unit8* dstdata = malloc(unpacked_length);
	HuffmanDecoder(srcdata, packed_offset, packed_length, unpacked_length, dstdata);
	return dstdata;
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
	Key[0] = cmvsMd5Data[0] ^ (CPZ_Header.IndexKey + 0x76a3bf29);
	Key[1] = CPZ_Header.IndexKey ^ cmvsMd5Data[1];
	Key[2] = cmvsMd5Data[2] ^ (CPZ_Header.IndexKey + 0x10000000);
	Key[3] = CPZ_Header.IndexKey ^ cmvsMd5Data[3];
	return Key;
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
	Key[0] = DirKey ^ cmvsMd5Data[0];
	Key[2] = DirKey ^ cmvsMd5Data[2];
	Key[1] = (DirKey + 0x11003322) ^ cmvsMd5Data[1];
	DirKey += 0x34216785;
	Key[3] = DirKey ^ cmvsMd5Data[3];
	return Key;
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

void MakeRandData()
{
	//随便搞波数
	srand(time(NULL));
	CPZ_Header.IndexSeed = rand();
	srand(time(NULL) + 0x131);
	CPZ_Header.Md5Data[0] = rand();
	srand(time(NULL) + 0x1313);
	CPZ_Header.Md5Data[1] = rand();
	srand(time(NULL) + 0x13131);
	CPZ_Header.Md5Data[2] = rand();
	srand(time(NULL) + 0x131313);
	CPZ_Header.Md5Data[3] = rand();
	cmvs_md5_ctx OCTX;
	cmvs_md5(CPZ_Header.Md5Data, &OCTX);
	CPZ_Header.IndexSeed ^= CPZ_Header.Md5Data[0] + CPZ_Header.Md5Data[1] + CPZ_Header.Md5Data[2] + CPZ_Header.Md5Data[3];
	//加密用
	IndexSeed = CPZ_Header.IndexSeed;
	IndexSeed ^= 0x37ACF832;
	IndexSeed = Ror(IndexSeed, 5);
	IndexSeed *= 0x7DA8F173;
	IndexSeed += 0x13712765;
	cmvsMd5Data[0] = CPZ_Header.Md5Data[0] ^ 0x43DE7C1A;
	cmvsMd5Data[1] = CPZ_Header.Md5Data[1] ^ 0xCC65F416;
	cmvsMd5Data[2] = CPZ_Header.Md5Data[2] ^ 0xD016A93D;
	cmvsMd5Data[3] = CPZ_Header.Md5Data[3] ^ 0x97A3BA9B;
	cmvs_md5_ctx CTX;
	cmvs_md5(cmvsMd5Data, &CTX);
	cmvsMd5Data[0] ^= 0x53A76D2E;
	cmvsMd5Data[1] += 0x5BB17FDA;
	cmvsMd5Data[2] += 0x6853E14D;
	cmvsMd5Data[3] ^= 0xF5C6A9A3;
}

void MakeCPZ7File(char* fname)
{
	FILE* src = NULL, * dst = NULL;
	unit32 savepos = 0, i = 0;
	unit8* data = NULL, *index_key = NULL;
	unit8 filename[MAX_PATH];
	CPZ_Header.IsEncrypt = 1;//开关文件加密
	CPZ_Header.DirIndexLength = 0;
	CPZ_Header.FileIndexLength = 0;
	CPZ_Header.IndexKey = time(NULL);//当前时间做key
	MakeRandData();//初始化随机数
	CPZ_Header.IndexKeySize = HuffmanSize;
	if (CPZ_Header.DirCount > 0)
	{
		LinkCPZ_Dir_Index q;
		q = CPZ_Dir_Index;
		while (q->next)
		{
			q = q->next;
			CPZ_Header.DirIndexLength += q->IndexLength;
		}
		q = CPZ_Dir_Index;
		savepos = 0;
		while (q->next)
		{
			q = q->next;
			q->FileIndexOffset = savepos;
			LinkCPZ_FIle_Index p = q->file_index;
			while (p)
			{
				CPZ_Header.FileIndexLength += p->IndexLength;
				savepos += p->IndexLength;
				p = p->next;
			}
		}
		//构建FileIndexLength
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
		//写文件
		sprintf(filename, "%s.cpz", fname);
		dst = fopen(filename, "wb+");
		fseek(dst, sizeof(CPZ_Header) + CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength + CPZ_Header.IndexKeySize, SEEK_SET);
		_chdir(fname);
		q = CPZ_Dir_Index;
		while (q->next)
		{
			q = q->next;
			_wchdir(q->DirName);
			wprintf(L"dirname:%ls file_num:%d file_index_offset:0x%X dir_key:0x%X\n", q->DirName, q->FileCount, q->FileIndexOffset, q->DirKey);
			LinkCPZ_FIle_Index p = q->file_index;
			while (p)
			{
				src = _wfopen(p->FileName, L"rb");
				data = malloc(p->Length);
				fread(data, p->Length, 1, src);
				fclose(src);
				if (CPZ_Header.IsEncrypt)
					CPZResourceEncrypt(data, p->Length, CPZ_Header.IndexKey, cmvsMd5Data, IndexSeed ^ ((CPZ_Header.IndexKey ^ (q->DirKey + p->FileKey)) + CPZ_Header.DirCount + 0xa3c61785));
				p->CRC = CheckCRC(data, p->Length, 0x5A902B7C);//sub_455D90 in ChronoClock
				p->Offset = ftell(dst) - sizeof(CPZ_Header) - CPZ_Header.DirIndexLength - CPZ_Header.FileIndexLength - CPZ_Header.IndexKeySize;
				fwrite(data, p->Length, 1, dst);
				free(data);
				wprintf(L"\t%s offset:0x%X size:0x%X file_key:0x%X crc:0x%X\n", p->FileName, p->Offset, p->Length, p->FileKey, p->CRC);
				p = p->next;
			}
			_wchdir(L"..");
		}
		_chdir("..");
		//构造index
		data = malloc(CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength + CPZ_Header.IndexKeySize);
		fseek(dst, sizeof(CPZ_Header), SEEK_SET);
		q = CPZ_Dir_Index;
		while (q->next)
		{
			q = q->next;
			fwrite(&q->IndexLength, 4, 1, dst);
			fwrite(&q->FileCount, 4, 1, dst);
			fwrite(&q->FileIndexOffset, 4, 1, dst);
			fwrite(&q->DirKey, 4, 1, dst);
			fwrite(q->DirNameSave, q->IndexLength - 0x10, 1, dst);
		}
		q = CPZ_Dir_Index;
		while (q->next)
		{
			q = q->next;
			LinkCPZ_FIle_Index p = q->file_index;
			while (p)
			{
				fwrite(&p->IndexLength, 4, 1, dst);
				fwrite(&p->Offset, 4, 1, dst);
				p->unk1 = 0;
				fwrite(&p->unk1, 4, 1, dst);
				fwrite(&p->Length, 4, 1, dst);
				p->unk2 = 0;
				fwrite(&p->unk2, 4, 1, dst);
				fwrite(&p->CRC, 4, 1, dst);
				fwrite(&p->FileKey, 4, 1, dst);
				fwrite(p->FileNameSave, p->IndexLength - 0x1C, 1, dst);
				p = p->next;
			}
		}
		fseek(dst, sizeof(CPZ_Header), SEEK_SET);
		fread(data, CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength, 1, dst);
		//加密与写index
		q = CPZ_Dir_Index;
		while (q->next)
		{
			q = q->next;
			CPZFileIndexEncrypt2(data + CPZ_Header.DirIndexLength + q->FileIndexOffset, q->FileIndexLength, q->DirKey);
			CPZFileIndexEncrypt1(data + CPZ_Header.DirIndexLength + q->FileIndexOffset, q->FileIndexLength, CPZ_Header.IndexKey, cmvsMd5Data[2]);
		}
		unit32* Key = GetIndexKey3();
		CPZIndexEncrypt3(data, CPZ_Header.DirIndexLength, Key, 0x76548aef);
		CPZIndexEncrypt2(data, CPZ_Header.DirIndexLength, CPZ_Header.IndexKey, cmvsMd5Data[1]);
		CPZIndexEncrypt1(data, CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength, CPZ_Header.IndexKey);
		memcpy(data + CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength, HuffmanData, CPZ_Header.IndexKeySize);
		index_key = UnpackIndexKey(data, CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength, CPZ_Header.IndexKeySize);
		for (i = 0; i < CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength; i++)
			data[i] ^= index_key[(i + 3) % 0x3FF];
		memcpy(data + CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength, HuffmanData, CPZ_Header.IndexKeySize);
		IndexMD5(data, CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength + CPZ_Header.IndexKeySize);
		fseek(dst, sizeof(CPZ_Header), SEEK_SET);
		fwrite(data, CPZ_Header.DirIndexLength + CPZ_Header.FileIndexLength + CPZ_Header.IndexKeySize, 1, dst);
		free(data);
		//写CPZ_Header
		CPZ_Header.Magic = 0x375A5043;
		CPZ_Header.unk = 0xA7B09C16;
		CPZHeaderEncrypt();
		fseek(dst, 0, SEEK_SET);
		fwrite(&CPZ_Header, sizeof(CPZ_Header), 1, dst);
		fclose(dst);
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-cmvs\n用于生成文件头为CPZ7的cpz文件。\n将文件夹拖到程序上。\nby Darkness-TX 2023.02.17\n\n");
	process_dir(argv[1]);
	MakeCPZ7File(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}