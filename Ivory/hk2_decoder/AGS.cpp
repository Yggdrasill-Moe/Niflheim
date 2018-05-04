#include "AGS.h"

AGS::AGS(string agsname)
{
	filename = agsname;
	AGS::ReadHeader(agsname);
}

bool AGS::ReadHeader(string agsname)
{
	fp = fopen(agsname.c_str(), "rb");
	if (fp)
		fread(&fAGS_fHKQ_header, 1, sizeof(fAGS_fHKQ_header_t), fp);
	else
	{
		cout << "文件不存在!\n";
		Header_OK = false;
		return Header_OK;
	}
	if (!(strncmp(fAGS_fHKQ_header.magic, "fAGS", 4) == 0) && !(strncmp(fAGS_fHKQ_header.magic, "fHKQ", 4) == 0))
	{
		cout << "文件头不是fAGS或fHKQ\n";
		Header_OK = false;
		return Header_OK;
	}
	Header_OK = true;
	return Header_OK;
}

void AGS::_cTEX_decode(BYTE *srcdata, BYTE *dstdata)
{
	DWORD key[2][32];
	DWORD seed = cTEX_header.seed;
	for (DWORD i = 0; i < 32; ++i)
	{
		DWORD _key = 0;
		DWORD _seed = seed;
		for (DWORD j = 0; j < 16; ++j)
		{
			_key = (_key >> 1) | (USHORT)((_seed ^ (_seed >> 1)) << 15);
			_seed >>= 2;
		}
		key[0][i] = seed;
		key[1][i] = _key;
		seed = (seed << 1) | (seed >> 31);
	}
	DWORD *enc = (DWORD *)srcdata;
	DWORD *dec = (DWORD *)dstdata;
	for (DWORD i = 0; i < (cTEX_header.data_length - cTEX_header.head_size) / 4; ++i)
	{
		DWORD _key = key[1][i & 0x1F];
		DWORD flag3 = 3;
		DWORD flag2 = 2;
		DWORD flag1 = 1;
		DWORD result = 0;
		for (DWORD j = 0; j < 16; ++j)
		{
			DWORD tmp;
			if (_key & 1)
				tmp = 2 * (enc[i] & flag1) | (enc[i] >> 1) & (flag2 >> 1);
			else
				tmp = enc[i] & flag3;
			_key >>= 1;
			result |= tmp;
			flag3 <<= 2;
			flag2 <<= 2;
			flag1 <<= 2;
		}
		dec[i] = result ^ key[0][i & 0x1F];
	}
	memcpy(dec + (cTEX_header.data_length - cTEX_header.head_size) / 4, enc + (cTEX_header.data_length - cTEX_header.head_size) / 4, (cTEX_header.data_length - cTEX_header.head_size) & 3);
}

void AGS::_cFNM_decode(BYTE *srcdata, BYTE *dstdata)
{
	DWORD key[2][32];
	DWORD seed = cFNM_header.seed;
	for (DWORD i = 0; i < 32; ++i)
	{
		DWORD _key = 0;
		DWORD _seed = seed;
		for (DWORD j = 0; j < 16; ++j)
		{
			_key = (_key >> 1) | (USHORT)((_seed ^ (_seed >> 1)) << 15);
			_seed >>= 2;
		}
		key[0][i] = seed;
		key[1][i] = _key;
		seed = (seed << 1) | (seed >> 31);
	}
	DWORD *enc = (DWORD *)srcdata;
	DWORD *dec = (DWORD *)dstdata;
	for (DWORD i = 0; i < (cFNM_header.data_length - cFNM_header.head_size) / 4; ++i)
	{
		DWORD _key = key[1][i & 0x1F];
		DWORD flag3 = 3;
		DWORD flag2 = 2;
		DWORD flag1 = 1;
		DWORD result = 0;
		for (DWORD j = 0; j < 16; ++j)
		{
			DWORD tmp;
			if (_key & 1)
				tmp = 2 * (enc[i] & flag1) | (enc[i] >> 1) & (flag2 >> 1);
			else
				tmp = enc[i] & flag3;
			_key >>= 1;
			result |= tmp;
			flag3 <<= 2;
			flag2 <<= 2;
			flag1 <<= 2;
		}
		dec[i] = result ^ key[0][i & 0x1F];
	}
	memcpy(dec + (cFNM_header.data_length - cFNM_header.head_size) / 4, enc + (cFNM_header.data_length - cFNM_header.head_size) / 4, (cFNM_header.data_length - cFNM_header.head_size) & 3);
}

void AGS::_cCOD_decode(BYTE *srcdata, BYTE *dstdata)
{
	DWORD key[2][32];
	DWORD seed = cCOD_header.seed;
	for (DWORD i = 0; i < 32; ++i)
	{
		DWORD _key = 0;
		DWORD _seed = seed;
		for (DWORD j = 0; j < 16; ++j)
		{
			_key = (_key >> 1) | (USHORT)((_seed ^ (_seed >> 1)) << 15);
			_seed >>= 2;
		}
		key[0][i] = seed;
		key[1][i] = _key;
		seed = (seed << 1) | (seed >> 31);
	}
	DWORD *enc = (DWORD *)srcdata;
	DWORD *dec = (DWORD *)dstdata;
	for (DWORD i = 0; i < (cCOD_header.data_length - cCOD_header.head_size) / 4; ++i)
	{
		DWORD _key = key[1][i & 0x1F];
		DWORD flag3 = 3;
		DWORD flag2 = 2;
		DWORD flag1 = 1;
		DWORD result = 0;
		for (DWORD j = 0; j < 16; ++j)
		{
			DWORD tmp;
			if (_key & 1)
				tmp = 2 * (enc[i] & flag1) | (enc[i] >> 1) & (flag2 >> 1);
			else
				tmp = enc[i] & flag3;
			_key >>= 1;
			result |= tmp;
			flag3 <<= 2;
			flag2 <<= 2;
			flag1 <<= 2;
		}
		dec[i] = result ^ key[0][i & 0x1F];
	}
	memcpy(dec + (cCOD_header.data_length - cCOD_header.head_size) / 4, enc + (cCOD_header.data_length - cCOD_header.head_size) / 4, (cCOD_header.data_length - cCOD_header.head_size) & 3);
}

void AGS::_cQZT_decode(BYTE *srcdata, BYTE *dstdata)
{
	DWORD key[2][32];
	DWORD seed = cQZT_header.seed;
	for (DWORD i = 0; i < 32; ++i)
	{
		DWORD _key = 0;
		DWORD _seed = seed;
		for (DWORD j = 0; j < 16; ++j)
		{
			_key = (_key >> 1) | (USHORT)((_seed ^ (_seed >> 1)) << 15);
			_seed >>= 2;
		}
		key[0][i] = seed;
		key[1][i] = _key;
		seed = (seed << 1) | (seed >> 31);
	}
	DWORD *enc = (DWORD *)srcdata;
	DWORD *dec = (DWORD *)dstdata;
	for (DWORD i = 0; i < (cQZT_header.data_length - cQZT_header.head_size) / 4; ++i)
	{
		DWORD _key = key[1][i & 0x1F];
		DWORD flag3 = 3;
		DWORD flag2 = 2;
		DWORD flag1 = 1;
		DWORD result = 0;
		for (DWORD j = 0; j < 16; ++j)
		{
			DWORD tmp;
			if (_key & 1)
				tmp = 2 * (enc[i] & flag1) | (enc[i] >> 1) & (flag2 >> 1);
			else
				tmp = enc[i] & flag3;
			_key >>= 1;
			result |= tmp;
			flag3 <<= 2;
			flag2 <<= 2;
			flag1 <<= 2;
		}
		dec[i] = result ^ key[0][i & 0x1F];
	}
	memcpy(dec + (cQZT_header.data_length - cQZT_header.head_size) / 4, enc + (cQZT_header.data_length - cQZT_header.head_size) / 4, (cQZT_header.data_length - cQZT_header.head_size) & 3);
}

void AGS::AGS_decode()
{
	fread(&cTEX_header, 1, sizeof(cTEX_header_t), fp);
	if (strncmp(cTEX_header.magic, "cTEX", 4) == 0)
	{
		FILE *dstfile = fopen((filename.substr(0, filename.find_last_of(".")) + ".TEX").c_str(), "wb");
		BYTE *srcdata = new BYTE[cTEX_header.data_length - cTEX_header.head_size];
		BYTE *dstdata = new BYTE[cTEX_header.data_length - cTEX_header.head_size];
		fread(srcdata, 1, cTEX_header.data_length - cTEX_header.head_size, fp);
		_cTEX_decode(srcdata, dstdata);
		delete[] srcdata;
		fwrite(dstdata, 1, cTEX_header.data_length - cTEX_header.head_size, dstfile);
		delete[] dstdata;
		fclose(dstfile);
	}
	else
	{
		printf("非cTEX块，实际为：%s\n", cTEX_header.magic);
		system("pause");
		exit(0);
	}
	char magic[4];
	fread(magic, 4, 1, fp);
	fseek(fp, -4, SEEK_CUR);
	if (strncmp(magic, "cQZT", 4) == 0)
	{
		fread(&cQZT_header, 1, sizeof(cQZT_header_t), fp);
		FILE *dstfile = fopen((filename.substr(0, filename.find_last_of(".")) + ".QZT").c_str(), "wb");
		BYTE *srcdata = new BYTE[cQZT_header.data_length - cQZT_header.head_size];
		BYTE *dstdata = new BYTE[cQZT_header.data_length - cQZT_header.head_size];
		fread(srcdata, 1, cQZT_header.data_length - cQZT_header.head_size, fp);
		_cQZT_decode(srcdata, dstdata);
		delete[] srcdata;
		fwrite(dstdata, 1, cQZT_header.data_length - cQZT_header.head_size, dstfile);
		delete[] dstdata;
		fclose(dstfile);
	}
	else
	{
		fread(&cFNM_header, 1, sizeof(cFNM_header_t), fp);
		if (strncmp(cFNM_header.magic, "cFNM", 4) == 0)
		{
			FILE *dstfile = fopen((filename.substr(0, filename.find_last_of(".")) + ".FNM").c_str(), "wb");
			BYTE *srcdata = new BYTE[cFNM_header.data_length - cFNM_header.head_size];
			BYTE *dstdata = new BYTE[cFNM_header.data_length - cFNM_header.head_size];
			fread(srcdata, 1, cFNM_header.data_length - cFNM_header.head_size, fp);
			_cFNM_decode(srcdata, dstdata);
			delete[] srcdata;
			fwrite(dstdata, 1, cFNM_header.data_length - cFNM_header.head_size, dstfile);
			delete[] dstdata;
			fclose(dstfile);
		}
		else
		{
			printf("非cFNM块，实际为：%s\n", cFNM_header.magic);
			system("pause");
			exit(0);
		}
		fread(&cCOD_header, 1, sizeof(cCOD_header_t), fp);
		if (strncmp(cCOD_header.magic, "cCOD", 4) == 0)
		{
			FILE *dstfile = fopen((filename.substr(0, filename.find_last_of(".")) + ".COD").c_str(), "wb");
			BYTE *srcdata = new BYTE[cCOD_header.data_length - cCOD_header.head_size];
			BYTE *dstdata = new BYTE[cCOD_header.data_length - cCOD_header.head_size];
			fread(srcdata, 1, cCOD_header.data_length - cCOD_header.head_size, fp);
			_cCOD_decode(srcdata, dstdata);
			delete[] srcdata;
			fwrite(dstdata, 1, cCOD_header.data_length - cCOD_header.head_size, dstfile);
			delete[] dstdata;
			fclose(dstfile);
		}
		else
		{
			printf("非cCOD块，实际为：%s\n", cCOD_header.magic);
			system("pause");
			exit(0);
		}
	}
}

AGS::~AGS()
{
	fclose(fp);
}