/*
用于解包文件头为WARC 1.7的WAR文件
made by Darkness-TX
2018.05.07
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
#include <png.h>
#define PI 3.1415926535897931

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

struct warc_header
{
	char magic[8];//WARC 1.7
	unit32 index_offset;
	//索引部分的头8字节
	unit32 flag1;
	unit32 flag2;
}WARC_Header;

struct warc_info {
	unit8 name[32];
	unit32 offset;
	unit32 comprlen;
	unit32 uncomprlen;
	FILETIME time_stamp;
	unit32 flags;
} WARC_Info[5000];

unit32 FileNum = 0;//总文件数，初始计数为0

unit32 Rand = 0;
char WARC_key[MAX_PATH];
unit32 Version = 0;//BR是2.48版本,2480或者0x9B0
unit8 *RioShiinaImage = NULL;//RioShiina图片
unit8 *Region = NULL;//RioShiina2.png的BGRA排列
unit32 key_src[5] = { 0, 0, 0, 0, 0 };

unit32 warc_max_index_length(unit32 WARC_version)
{
	unit32 entry_size, max_index_entries;

	if (WARC_version < 150)
	{
		entry_size = 0x38;//sizeof(warc_info)
		max_index_entries = 8192;
	}
	else
	{
		entry_size = 0x38;//sizeof(warc_info)
		max_index_entries = 16384;
	}
	return entry_size * max_index_entries;
}

unit32 get_rand()
{
	Rand = 0x5D588B65 * Rand + 1;
	return Rand;
}

unit32 BigEndian(unit32 u)
{
	return u << 24 | (u & 0xff00) << 8 | (u & 0xff0000) >> 8 | u >> 24;
}

DWORD CheckString(wchar_t *buff)
{
	if (wcsncmp(buff, L"0x", 2) == 0 || wcsncmp(buff, L"0X", 2) == 0)
		return wcstoul(buff + 2, NULL, 16);
	else
		return wcstoul(buff, NULL, 10);
}

void ReadPng(FILE* src)
{
	png_structp png_ptr;
	png_infop info_ptr, end_ptr;
	png_bytep *rows;
	unit32 i, width = 0, height = 0, bpp = 0, format = 0;
	unit8 buff;
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
	png_init_io(png_ptr, src);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)&width, (png_uint_32*)&height, &bpp, &format, NULL, NULL, NULL);
	if (bpp == 8 && format == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		Region = malloc(width * height * 4);
		rows = (png_bytep*)malloc(height * sizeof(char*));
		for (i = 0; i < height; i++)
			rows[i] = (png_bytep)(Region + width*i * 4);
		png_read_image(png_ptr, rows);
		free(rows);
		png_read_end(png_ptr, info_ptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
		for (i = 0; i < height * width; i++)
		{
			buff = Region[i * 4];
			Region[i * 4] = Region[i * 4 + 2];
			Region[i * 4 + 2] = buff;
		}
	}
	else
	{
		printf("Region块的图片不是bpp为8或不是32位RGBA图片，不符合规范！\n");
		system("pause");
		exit(0);
	}
}

void Init()
{
	wchar_t filePath[MAX_PATH];
	wchar_t dirPath[MAX_PATH];
	wchar_t iniPath[MAX_PATH];
	wchar_t buff[20];
	GetCurrentDirectoryW(MAX_PATH, dirPath);
	wsprintfW(iniPath, L"%ls\\%ls", dirPath, L"RioShiina.ini");
	if (_waccess(iniPath, 4) == -1)
	{
		wprintf(L"初始化失败，请确认目录下是否含有RioShiina.ini\n");
		system("pause");
		exit(0);
	}
	GetPrivateProfileStringW(L"RioShiina", L"Version", L"2480", buff, MAX_PATH, iniPath);
	Version = CheckString(buff);
	GetPrivateProfileStringW(L"RioShiina", L"Key1", L"0", buff, MAX_PATH, iniPath);
	key_src[0] = CheckString(buff);
	GetPrivateProfileStringW(L"RioShiina", L"Key2", L"0", buff, MAX_PATH, iniPath);
	key_src[1] = CheckString(buff);
	GetPrivateProfileStringW(L"RioShiina", L"Key3", L"0", buff, MAX_PATH, iniPath);
	key_src[2] = CheckString(buff);
	GetPrivateProfileStringW(L"RioShiina", L"Key4", L"0", buff, MAX_PATH, iniPath);
	key_src[3] = CheckString(buff);
	GetPrivateProfileStringW(L"RioShiina", L"Key5", L"0", buff, MAX_PATH, iniPath);
	key_src[4] = CheckString(buff);
	printf("Version:%d key_src:0x%X|0x%X|0x%X|0x%X|0x%X\n", Version, key_src[0], key_src[1], key_src[2], key_src[3], key_src[4]);
	GetPrivateProfileStringW(L"Image", L"RioShiinaImage", L"", filePath, MAX_PATH, iniPath);
	if (wcscmp(filePath, L"") == 0)
	{
		wprintf(L"初始化图片失败，请确认RioShiina.ini中RioShiinaImage属性是否有值\n");
		system("pause");
		exit(0);
	}
	wprintf(L"Image:%ls\n", filePath);
	FILE *src = _wfopen(filePath, L"rb");
	if (src == NULL)
	{
		wprintf(L"初始化图片失败，请确认目录下是否含有%ls\n", filePath);
		system("pause");
		exit(0);
	}
	fseek(src, 0, SEEK_END);
	unit32 size = ftell(src);
	RioShiinaImage = malloc(size);
	fseek(src, 0, SEEK_SET);
	fread(RioShiinaImage, size, 1, src);
	fclose(src);
	GetPrivateProfileStringW(L"Image", L"Region", L"", filePath, MAX_PATH, iniPath);
	if (wcscmp(filePath, L"") == 0)
	{
		wprintf(L"初始化图片失败，请确认RioShiina.ini中Region属性是否有值\n");
		system("pause");
		exit(0);
	}
	wprintf(L"Region:%ls\n\n", filePath);
	src = _wfopen(filePath, L"rb");
	if (src == NULL)
	{
		wprintf(L"初始化图片失败，请确认目录下是否含有%ls\n", filePath);
		system("pause");
		exit(0);
	}
	ReadPng(src);
	fclose(src);
}

unit32* InitCrcTable()
{
	unit32 table[0x100];
	for (unit32 i = 0; i != 256; ++i)
	{
		unit32 poly = i;
		for (int j = 0; j < 8; ++j)
		{
			unit32 bit = poly & 1;
			poly = (poly >> 1) | (poly << 31);
			if (0 == bit)
				poly ^= 0x6DB88320;
		}
		table[i] = poly;
	}
	return table;
}

unit32 RegionCrc32(unit8 *src, unit32 flags, unit32 rgb)
{
	unit32 *CustomCrcTable = InitCrcTable();
	int src_alpha = (int)flags & 0x1ff;
	int dst_alpha = (int)(flags >> 12) & 0x1ff;
	flags >>= 24;
	if (0 == (flags & 0x10))
		dst_alpha = 0;
	if (0 == (flags & 8))
		src_alpha = 0x100;
	int y_step = 0;
	int x_step = 4;
	int width = 48;
	int pos = 0;
	if (0 != (flags & 0x40))//horizontal flip
	{
		y_step += width;
		pos += (width - 1) * 4;
		x_step = -x_step;
	}
	if (0 != (flags & 0x20))//vertical flip
	{
		y_step -= width;
		pos += width * 0x2f * 4;//width*(height-1)*4;
	}
	y_step <<= 3;
	unit32 checksum = 0;
	for (int y = 0; y < 48; ++y)
	{
		for (int x = 0; x < 48; ++x)
		{
			int alpha = src[pos + 3] * src_alpha;
			alpha >>= 8;
			unit32 color = rgb;
			for (int i = 0; i < 3; ++i)
			{
				int v = src[pos + i];
				int c = (int)(color & 0xff);//rgb[i];
				c -= v;
				c = (c * dst_alpha) >> 8;
				c = (c + v) & 0xff;
				c = (c * alpha) >> 8;
				checksum = (checksum >> 8) ^ CustomCrcTable[(c ^ checksum) & 0xff];
				color >>= 8;
			}
			pos += x_step;
		}
		pos += y_step;
	}
	return checksum;
}

double encrypt_helper1(double a)
{
	if (a < 0)
		return -encrypt_helper1(-a);
	if (a < 18.0)
	{
		double v0 = a;
		double v1 = a;
		double v2 = -(a * a);

		for (int i = 3; i < 1000; i += 2)
		{
			v1 *= v2 / (i * (i - 1));
			v0 += v1 / i;
			if (v0 == v2)
				break;
		}
		return v0;
	}
	int flags = 0;
	double v0_l = 0;
	double v1 = 0;
	double div = 1 / a;
	double v1_h = 2.0;
	double v0_h = 2.0;
	double v1_l = 0;
	double v0 = 0;
	int i = 0;
	do
	{
		v0 += div;
		div *= ++i / a;
		if (v0 < v0_h)
			v0_h = v0;
		else
			flags |= 1;
		v1 += div;
		div *= ++i / a;
		if (v1 < v1_h)
			v1_h = v1;
		else
			flags |= 2;
		v0 -= div;
		div *= ++i / a;
		if (v0 > v0_l)
			v0_l = v0;
		else
			flags |= 4;
		v1 -= div;
		div *= ++i / a;
		if (v1 > v1_l)
			v1_l = v1;
		else
			flags |= 8;
	} while (flags != 15);
	return ((PI - cos(a) * (v0_l + v0_h)) - (sin(a) * (v1_l + v1_h))) / 2.0;
}

unit32 encrypt_helper2(unit32 WARC_version, double a)
{
	double v0, v1, v2, v3;

	if (a > 1.0)
	{
		v0 = sqrt(a * 2 - 1);
		while (1)
		{
			v1 = 1 - (double)get_rand() / 4294967296.0;
			v2 = 2.0 * (double)get_rand() / 4294967296.0 - 1.0;
			if (v1 * v1 + v2 * v2 > 1.0)
				continue;
			v2 /= v1;
			v3 = v2 * v0 + a - 1.0;
			if (v3 <= 0)
				continue;
			v1 = (a - 1.0) * log(v3 / (a - 1.0)) - v2 * v0;
			if (v1 < -50.0)
				continue;
			if (((double)get_rand() / 4294967296.0) <= (exp(v1) * (v2 * v2 + 1.0)))
				break;
		}
	}
	else
	{
		v0 = exp(1.0) / (a + exp(1.0));
		do
		{
			v1 = (double)get_rand() / 4294967296.0;
			v2 = (double)get_rand() / 4294967296.0;
			if (v1 < v0)
			{
				v3 = pow(v2, 1.0 / a);
				v1 = exp(-v3);
			}
			else
			{
				v3 = 1.0 - log(v2);
				v1 = pow(v3, a - 1.0);
			}
		} while ((double)get_rand() / 4294967296.0 >= v1);
	}
	if (WARC_version > 120)
		return (unit32)(v3 * 256.0);
	else
		return (unit8)((double)get_rand() / 4294967296.0);
}

unit32 encrypt_helper3(unit32 key)
{
	unit32 v0, v1, v2, v3;
	unit8 b0, b1, b2, b3;
	b3 = key >> 24;
	b2 = (key & 0xFF0000) >> 16;
	b1 = (key & 0xFF00) >> 8;
	b0 = key & 0xFF;
	float f;
	f = (float)(1.5 * (double)b0 + 0.1);
	v0 = BigEndian(*(unit32 *)&f);
	f = (float)(1.5 * (double)b1 + 0.1);
	v1 = (unit32)f;
	f = (float)(1.5 * (double)b2 + 0.1);
	v2 = (unit32)-*(int *)&f;
	f = (float)(1.5 * (double)b3 + 0.1);
	v3 = ~*(unit32 *)&f;
	return (v0 + v1) | (v2 - v3);
}

void encrypt_helper4(unit8 *data)
{
	unit32 code[80], key[10], i = 0;
	unit32 k0, k1, k2, k3, k4;
	unit32 *p = (unit32 *)(data + 44);
	for (i = 0; i < 0x10; i++)
		code[i] = ((p[i] & 0xFF00 | (p[i] << 16)) << 8) | (((p[i] >> 16) | p[i] & 0xFF0000) >> 8);
	for (unit32 k = 0; k < 80 - i; ++k)
	{
		unit32 tmp = code[0 + k] ^ code[2 + k] ^ code[8 + k] ^ code[13 + k];
		code[16 + k] = (tmp >> 31) | (tmp << 1);
	}
	memcpy(key, key_src, 4 * 5);
	k0 = key[0];
	k1 = key[1];
	k2 = key[2];
	k3 = key[3];
	k4 = key[4];
	for (i = 0; i < 0x50; i++)
	{
		unit32 f, c;
		if (i < 0x10)
		{
			f = k1 ^ k2 ^ k3;
			c = 0;
		}
		else if (i < 0x20)
		{
			f = k1 & k2 | k3 & ~k1;
			c = 0x5A827999;
		}
		else if (i < 0x30)
		{
			f = k3 ^ (k1 | ~k2);
			c = 0x6ED9EBA1;
		}
		else if (i < 0x40)
		{
			f = k1 & k3 | k2 & ~k3;
			c = 0x8F1BBCDC;
		}
		else
		{
			f = k1 ^ (k2 | ~k3);
			c = 0xA953FD4E;
		}
		unit32 new_k0 = code[i] + k4 + f + c + ((k0 >> 27) | (k0 << 5));
		unit32 new_k2 = (k1 >> 2) | (k1 << 30);
		k1 = k0;
		k4 = k3;
		k3 = k2;
		k2 = new_k2;
		k0 = new_k0;
	}
	key[0] += k0;
	key[1] += k1;
	key[2] += k2;
	key[3] += k3;
	key[4] += k4;
	FILETIME ft;
	ft.dwLowDateTime = key[1];
	ft.dwHighDateTime = key[0] & 0x7FFFFFFF;
	SYSTEMTIME sys_time;
	if (!FileTimeToSystemTime(&ft, &sys_time))
	{
		printf("decrypt_helper4中FileTimeToSystemTime失败!\n");
		system("pause");
		exit(0);
	}
	key[5] = (unit32)(sys_time.wYear | sys_time.wMonth << 16);
	key[7] = (unit32)(sys_time.wHour | sys_time.wMinute << 16);
	key[8] = (unit32)(sys_time.wSecond | sys_time.wMilliseconds << 16);
	unit32 flags = *p | 0x80000000;
	unit32 rgb = code[1] >> 8;
	if ((flags & 0x78000000) == 0)
		flags |= 0x98000000;
	key[6] = RegionCrc32(Region, flags, rgb);
	key[9] = (unit32)(((int)key[2] * (INT64)(int)key[3]) >> 8);
	if (Version > 2390)
		key[6] += key[9];
	unit32* encoded = (unit32 *)(data + 4);
	for (i = 0; i < 10; i++)
		encoded[i] ^= key[i];
}

char *WARC_key_string(unit32 WARC_version)
{
	char *key = "Crypt Type %s - Copyright(C) 2000 Y.Yamada/STUDIO 傛偟偔傫";//よしくん

	if (WARC_version <= 120)
		sprintf(WARC_key, key, "20000823");
	else
		sprintf(WARC_key, key, "20011002");

	return WARC_key;
}

void encrypt(unit32 WARC_version, unit8 *cipher, unit32 cipher_length)
{
	int a, b;
	unit32 fac = 0, idx = 0, index = 0;
	Rand = cipher_length;
	unit32 _cipher_length = cipher_length;
	if (cipher_length < 3)
		return;
	if (cipher_length > 1024)
		cipher_length = 1024;
	if (WARC_version > 120)
	{
		a = (char)cipher[0] ^ (char)_cipher_length;
		b = (char)cipher[1] ^ (char)(_cipher_length / 2);
		if (_cipher_length != warc_max_index_length(170))
		{
			idx = (unit32)((double)get_rand() * (_msize(RioShiinaImage) / 4294967296.0));
			if (WARC_version >= 160)
			{
				fac = RioShiinaImage[idx] + Rand;
				fac = encrypt_helper3(fac) & 0xfffffff;
				if (cipher_length > 0x80)
				{
					encrypt_helper4(cipher);
					index += 0x80;
					cipher_length -= 0x80;
				}
			}
			else if (WARC_version == 150)
			{
				fac = Rand + RioShiinaImage[idx];
				fac ^= (fac & 0xfff) * (fac & 0xfff);
				unit32 v = 0;
				for (unit32 i = 0; i < 32; ++i)
				{
					unit32 bit = fac & 1;
					fac >>= 1;
					if (0 != bit)
						v += fac;
				}
				fac = v;
			}
			else if (WARC_version == 140)
				fac = RioShiinaImage[idx];
			else if (WARC_version == 130)
				fac = RioShiinaImage[idx & 0xff];
			else
			{
				printf("不支持的WARC版本! WARC_version:%d\n", WARC_version);
				system("pause");
				exit(0);
			}
		}
	}
	else
	{
		a = cipher[0];
		b = cipher[1];
	}
	Rand ^= (DWORD)(encrypt_helper1(a) * 100000000.0);
	double tmp = 0.0;
	if (0 != (a | b))
	{
		tmp = acos((double)a / sqrt(a*a + b*b));
		tmp = tmp / PI * 180.0;
	}
	if (b < 0)
		tmp = 360.0 - tmp;
	char *key_string = WARC_key_string(WARC_version);
	unit32 key_string_len = strlen(key_string);
	unit32 i = ((unit8)encrypt_helper2(WARC_version, tmp) + fac) % key_string_len;
	unit32 n = 0;
	for (unit32 k = 2; k < cipher_length; k++)
	{
		char buff = key_string[n++] ^ key_string[i];
		i = cipher[index + k] % key_string_len;
		cipher[index + k] ^= buff;
		cipher[index + k] = ((cipher[index + k] & 0x80) >> 7) | (cipher[index + k] << 1);
		if (WARC_version > 120)
			cipher[index + k] ^= (unit8)((double)get_rand() / 16777216.0);
		else
			cipher[index + k] ^= (unit8)((double)get_rand() / 4294967296.0);
		if (n >= key_string_len)
			n = 0;
	}
}

void ReadIndex(char *fname)
{
	FILE *src;
	unit8 *data = NULL;
	char dstname[MAX_PATH];
	src = fopen(fname, "rb");
	fread(WARC_Header.magic, 1, 8, src);
	fread(&WARC_Header.index_offset, 4, 1, src);
	fclose(src);
	if (strncmp(WARC_Header.magic, "WARC 1.7", 8) == 0)
	{
		sprintf(dstname, "%s.idx", fname);
		src = fopen(dstname, "rb");
		if (src == NULL)
		{
			printf("无法打开%s，请至少运行一次解包生成%s\n", dstname, dstname);
			system("pause");
			exit(0);
		}
		fseek(src, 0, SEEK_END);
		unit32 filesize = ftell(src);
		filesize -= 8;
		data = malloc(filesize);
		fseek(src, 0, SEEK_SET);
		fread(&WARC_Header.flag1, 4, 1, src);
		fread(&WARC_Header.flag2, 4, 1, src);
		fread(data, filesize, 1, src);
		for (FileNum = 0; FileNum < filesize / 0x38; FileNum++)
			memcpy(&WARC_Info[FileNum], data + FileNum * 0x38, 0x38);
		fclose(src);
	}
	else
	{
		printf("不支持的文件类型，请确认文件头为WARC 1.7\n");
		system("pause");
		exit(0);
	}
}

void packFile(char *fname)
{
	FILE *src, *dst;
	unit32 i = 0, l = 0;
	unit8 *cdata = NULL, *udata = NULL;
	unit32 flag = 0x014B5059;
	unit32 max_index_len = warc_max_index_length(170);
	unit32 index_len = warc_max_index_length(170);
	char dstname[MAX_PATH];
	sprintf(dstname, "%s.new", fname);
	dst = fopen(dstname, "wb");
	fwrite(WARC_Header.magic, 8, 1, dst);
	fwrite(&WARC_Header.index_offset, 4, 1, dst);
	sprintf(dstname, "%s_unpack", fname);
	_chdir(dstname);
	for (i = 0; i < FileNum; i++)
	{
		sprintf(dstname, "%04d_%s", i, WARC_Info[i].name);
		src = fopen(dstname, "rb");
		fseek(src, 0, SEEK_END);
		WARC_Info[i].uncomprlen = ftell(src);
		WARC_Info[i].comprlen = WARC_Info[i].uncomprlen * 2;
		fseek(src, 0, SEEK_SET);
		udata = malloc(WARC_Info[i].uncomprlen);
		cdata = malloc(WARC_Info[i].comprlen);
		fread(udata, WARC_Info[i].uncomprlen, 1, src);
		compress2(cdata, &WARC_Info[i].comprlen, udata, WARC_Info[i].uncomprlen, Z_BEST_COMPRESSION);
		free(udata);
		if ((flag & 0xFF000000) != 0)//因为之前的定义，这里是永远加密的
		{
			unit32 key = ~0x4B4D4B4D;//KMKM
			unit32 *enc = (unit32 *)cdata;
			for (l = 0; l < WARC_Info[i].comprlen / 4; l++)
				enc[l] ^= key;
			for (l *= 4; l < WARC_Info[i].comprlen; l++)
				cdata[l] ^= (unit8)key;
		}
		if (WARC_Info[i].flags & 0x80000000)
			encrypt(170, cdata, WARC_Info[i].comprlen);
		WARC_Info[i].offset = ftell(dst);
		unit32 buff = flag ^ (WARC_Info[i].uncomprlen ^ 0x82AD82) & 0xFFFFFF;
		fwrite(&buff, 4, 1, dst);
		fwrite(&WARC_Info[i].uncomprlen, 4, 1, dst);
		fwrite(cdata, WARC_Info[i].comprlen, 1, dst);
		WARC_Info[i].comprlen += 8;
		free(cdata);
		printf("%s offset:0x%X comprlen:0x%X uncomprlen:0x%X flags:0x%X\n", WARC_Info[i].name, WARC_Info[i].offset, WARC_Info[i].comprlen, WARC_Info[i].uncomprlen, WARC_Info[i].flags);
	}
	WARC_Header.index_offset = ftell(dst);
	udata = malloc(FileNum * 0x38);
	for (i = 0; i < FileNum; i++)
		memcpy(udata + i * 0x38, &WARC_Info[i], 0x38);
	cdata = malloc(index_len);
	memcpy(cdata, &WARC_Header.flag1, 4);
	memcpy(cdata + 4, &WARC_Header.flag2, 4);
	compress2(cdata + 8, &index_len, udata, FileNum * 0x38, Z_BEST_COMPRESSION);
	free(udata);
	for (i = 0; i < max_index_len; i++)
		cdata[i] ^= (unit8)~170;
	for (i = 0; i < max_index_len / 4; ++i)
		((unit32 *)cdata)[i] ^= WARC_Header.index_offset;
	encrypt(170, cdata, max_index_len);
	fwrite(cdata, index_len + 8, 1, dst);
	free(cdata);
	printf("%s.new WARC 1.7 index_offset:0x%X filesize:0x%X\n\n", fname, WARC_Header.index_offset, ftell(dst));
	WARC_Header.index_offset ^= 0xF182AD82;//くん
	fseek(dst, 8, SEEK_SET);
	fwrite(&WARC_Header.index_offset, 4, 1, dst);
	fclose(dst);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-BLOODY RONDO\n用于封包文件头为WARC 1.7的WAR文件。\n将war文件拖到程序上。\nby Darkness-TX 2018.05.07\n\n");
	ReadIndex(argv[1]);
	Init();
	packFile(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}