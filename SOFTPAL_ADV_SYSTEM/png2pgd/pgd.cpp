#include "pgd.h"

PGD::PGD(string pgdname)
{
	filename = pgdname;
	PGD::ReadHeader(pgdname);
}

bool PGD::ReadHeader(string pgdname)
{
	fp = fopen(pgdname.c_str(), "rb");
	if (fp)
		fread(&pgd32_header, 1, sizeof(pgd32_header_t), fp);
	else
	{
		cout << "文件不存在";
		Header_OK = false;
		return Header_OK;
	}
	Header_OK = true;
	return Header_OK;
}

void PGD::_pgd_uncompress32(BYTE *compr, BYTE *uncompr, DWORD uncomprlen)
{
	DWORD act_uncomprlen = 0;
	DWORD flag = *compr++ | 0xff00;

	while (act_uncomprlen != uncomprlen)
	{
		DWORD base_offset, copy_bytes;
		BYTE *src, *dst;

		if (flag & 1)
		{
			DWORD tmp = *(WORD *)compr;
			compr += 2;
			if (tmp & 8)
			{
				base_offset = tmp >> 4;
				copy_bytes = (tmp & 0x7) + 4;
			}
			else
			{
				base_offset = (tmp << 8) | *compr++;
				copy_bytes = (base_offset & 0xfff) + 4;
				base_offset >>= 12;
			}
			src = uncompr + act_uncomprlen - base_offset;
		}
		else
		{
			copy_bytes = *compr++;
			src = compr;
			compr += copy_bytes;
		}
		dst = uncompr + act_uncomprlen;
		for (DWORD i = 0; i < copy_bytes; ++i)
			*dst++ = *src++;
		flag >>= 1;
		act_uncomprlen += copy_bytes;
		if (!(flag & 0x0100))
			flag = *compr++ | 0xff00;
	}
}

void PGD::_pgd3_ge_restore_32(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length, WORD width, WORD height)
{
	BYTE *data = out + out_len - 1;
	BYTE *src = __ge + __ge_length - 1;
	BYTE *pre = __ge + __ge_length - 5;
	WORD org_width = width;
	for (DWORD h = 0; h < height; h++)
	{
		width = org_width - 1;
		while (width-- > 0)
		{
			*data-- = *pre-- - *src--;
			*data-- = *pre-- - *src--;
			*data-- = *pre-- - *src--;
			*data-- = *pre-- - *src--;
		}
		data -= 4;
		src -= 4;
		*(DWORD *)(data+1) = *(DWORD *)(pre + 1);
		pre -= 4;
	}
	for (DWORD h = 0; h < height; h++)
		*data-- = 1;
	memcpy(out, &ge_header, sizeof(ge_header_t));
}

void PGD::_pgd3_ge_restore_24(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length, WORD width, WORD height)
{
	BYTE *data = out + out_len - 1;
	BYTE *src = __ge + __ge_length - 1;
	BYTE *pre = __ge + __ge_length - 4;
	WORD org_width = width;
	for (DWORD h = 0; h < height; h++)
	{
		width = org_width - 1;
		while (width-- > 0)
		{
			*data-- = *pre-- - *src--;
			*data-- = *pre-- - *src--;
			*data-- = *pre-- - *src--;
		}
		*data-- = *src--;
		*data-- = *src--;
		*data-- = *src--;
		pre -= 3;
	}
	for (DWORD h = 0; h < height; h++)
		*data-- = 1;
	memcpy(out, &ge_header, sizeof(ge_header_t));
}

void PGD::pgd_ge_restore3(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length, WORD width, WORD height, DWORD bpp)
{
	cout << "restore to GE...\n";
	if (bpp == 32)
		_pgd3_ge_restore_32(out, out_len, __ge, __ge_length, width, height);
	else if (bpp = 24)
		_pgd3_ge_restore_24(out, out_len, __ge, __ge_length, width, height);
	else
		printf("未知的GE类型，bpp:%d\n", bpp);
}

DWORD PGD::_pgd_compress32(BYTE *uncompr, DWORD uncomprlen,FILE *pgdfile)
{
	cout << "build PGD...\n";
	printf("width:%d height:%d bpp:%d\n", ge_header.width, ge_header.height, ge_header.bpp);
	DWORD count = uncomprlen / 8;
	DWORD yu = uncomprlen % 8;
	BYTE zero = 0;
	BYTE one = 1;
	for (DWORD i = 0; i < count; i++)
	{
		fwrite(&zero, 1, 1, pgdfile);
		for (DWORD j = 0; j < 8; j++)
		{
			fwrite(&one, 1, 1, pgdfile);
			fwrite(&uncompr[i * 8 + j], 1, 1, pgdfile);
		}
	}
	fwrite(&zero, 1, 1, pgdfile);
	for (DWORD k = 0; k < yu; k++)
	{
		fwrite(&one, 1, 1, pgdfile);
		fwrite(&uncompr[count * 8 + k], 1, 1, pgdfile);
	}
	return ftell(pgdfile) - sizeof(pgd32_header_t) - sizeof(pgd32_info_t);
}

bool PGD::pgd_compress()
{
	if (Header_OK)
	{
		if (strncmp("GE", pgd32_header.maigc, 2) == 0)
		{
			if (pgd32_header.sizeof_header == 0x20)
			{
				if (pgd32_header.compr_method == 3|| pgd32_header.compr_method == 2)
				{
					fread(&pgd32_info, sizeof(pgd32_info_t), 1, fp);
					if (pgd32_header.compr_method == 3)
					{
						BYTE* data = new BYTE[pgd32_info.comprlen];
						fread(data, 1, pgd32_info.comprlen, fp);
						BYTE* uncompr = new BYTE[pgd32_info.uncomprlen];
						_pgd_uncompress32(data, uncompr, pgd32_info.uncomprlen);
						delete[] data;
						memcpy(&ge_header, uncompr, sizeof(ge_header_t));
						delete[] uncompr;
					}
					else
					{
						ge_header.bpp = 32;
						ge_header.width = (WORD)pgd32_header.width;
						ge_header.height = (WORD)pgd32_header.height;
						ge_header.unknown = 7;
						pgd32_info.uncomprlen = ge_header.width*ge_header.height * 4 + 8 + ge_header.height;
						/*
						pgd32_header.compr_method == 2时使用的算法看起来应该是类似一种rgb转16位灰度或者类似种插值算法的。
						这种算法用一次图片质量就降一次，所以这里还是使用pgd32_header.compr_method == 3的压缩算法，
						原游戏本身压的时候就损耗了一次，我们转换再还原又会损耗一次，所以还是用无损吧。
						（哪有那么复杂！其实就是反写不出压缩算法。。。。。。虽然有取巧的方法）
						*/
						pgd32_header.compr_method = 3;
					}
					BYTE *TexData = new BYTE[ge_header.height*ge_header.width*ge_header.bpp / 8];
					if (png2raw(TexData))
					{
						BYTE *ge = new BYTE[pgd32_info.uncomprlen];
						pgd_ge_restore3(ge, pgd32_info.uncomprlen, TexData, ge_header.height*ge_header.width*ge_header.bpp / 8, ge_header.width, ge_header.height, ge_header.bpp);
						delete[] TexData;
						FILE *pgdfile = fopen((filename.substr(0, filename.find_last_of(".")) + ".PGDN").c_str(), "wb");
						fwrite(&pgd32_header, sizeof(pgd32_header_t), 1, pgdfile);
						fwrite(&pgd32_info, sizeof(pgd32_info_t), 1, pgdfile);
						pgd32_info.comprlen = _pgd_compress32(ge, pgd32_info.uncomprlen, pgdfile);
						delete[] ge;
						fseek(pgdfile, sizeof(pgd32_header_t), SEEK_SET);
						fwrite(&pgd32_info, sizeof(pgd32_info_t), 1, pgdfile);
						fclose(pgdfile);
						return true;
					}
				}
				else
					cout << "非类型2或3!\n";
			}
			else
				cout << "非PGD32类型!\n";
		}
		else
			cout << "文件头不是GE!\n";
	}
	else
		cout << "读取文件头失败!\n";
	system("pause");
	return false;
}

bool PGD::png2raw(BYTE *TexData)
{
	BYTE buff;
	png_structp png_ptr;
	png_infop info_ptr, end_ptr;
	png_bytep *rows;
	DWORD i = 0;
	FILE *OpenPng = fopen((filename.substr(0, filename.find_last_of(".")) + ".png").c_str(), "rb");
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	cout << "restore to raw...\n";
	if (png_ptr == NULL)
	{
		printf("PNG信息创建失败!\n");
		return false;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		printf("info信息创建失败!\n");
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return false;
	}
	end_ptr = png_create_info_struct(png_ptr);
	if (end_ptr == NULL)
	{
		printf("end信息创建失败!\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		return false;
	}
	png_init_io(png_ptr, OpenPng);
	png_read_info(png_ptr, info_ptr);
	if (ge_header.bpp == 32)
		png_set_IHDR(png_ptr, info_ptr, ge_header.width, ge_header.height, ge_header.bpp / 4, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	else if (ge_header.bpp == 24)
		png_set_IHDR(png_ptr, info_ptr, ge_header.width, ge_header.height, ge_header.bpp / 3, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	else
	{
		printf("未知bpp:%d\n", ge_header.bpp);
		return false;
	}
	rows = (png_bytep*)malloc(ge_header.height * sizeof(BYTE*));
	if (ge_header.bpp == 32)
		for (i = 0; i < ge_header.height; i++)
			rows[i] = (png_bytep)(TexData + ge_header.width*i * 4);
	else if (ge_header.bpp == 24)
		for (i = 0; i < ge_header.height; i++)
			rows[i] = (png_bytep)(TexData + ge_header.width*i * 3);
	png_read_image(png_ptr, rows);
	free(rows);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
	fclose(OpenPng);
	for (DWORD i = 0; i < (DWORD)(ge_header.width * ge_header.height); i++)
	{
		buff = TexData[i*ge_header.bpp / 8 + 0];
		TexData[i*ge_header.bpp / 8 + 0] = TexData[i*ge_header.bpp / 8 + 2];
		TexData[i*ge_header.bpp / 8 + 2] = buff;
	}
	return true;
}

PGD::~PGD()
{
	fclose(fp);
}