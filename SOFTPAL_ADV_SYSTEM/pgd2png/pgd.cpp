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

void PGD::pgd_ge_process2(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length, DWORD width, DWORD height)
{
	BYTE *src[4];

	src[0] = __ge;
	src[1] = src[0] + width * height / 4;
	src[2] = src[1] + width * height / 4;

	BYTE *dst = out;
	for (DWORD k = 0; k < height / 2; ++k)
	{
		for (DWORD j = 0; j < width / 2; ++j)
		{
			int tmp[2];
			int val, para[3];

			tmp[0] = (char)*src[0]++;
			tmp[1] = (char)*src[1]++;

			para[0] = tmp[0] * 226;
			para[1] = -43 * tmp[0] - 89 * tmp[1];
			para[2] = tmp[1] * 179;

			DWORD i = 0;
			for (i = 0; i < 3; ++i)
			{
				val = (para[i] + (src[2][0] << 7)) >> 7;
				if (val <= 255)
				{
					if (val < 0)
						val = 0;
				}
				else
					val = 255;
				dst[i] = (BYTE)val;
			}
			dst[i++] = 0xFF;

			for (; i < 7; ++i)
			{
				val = (para[i - 4] + (src[2][1] << 7)) >> 7;
				if (val <= 255)
				{
					if (val < 0)
						val = 0;
				}
				else
					val = 255;
				dst[i] = (BYTE)val;
			}
			dst[i] = 0xFF;

			dst += width * 4;
			src[3] = src[2] + width;

			for (i = 0; i < 3; ++i)
			{
				val = (para[i] + (src[3][0] << 7)) >> 7;
				if (val <= 255)
				{
					if (val < 0)
						val = 0;
				}
				else
					val = 255;
				dst[i] = (BYTE)val;
			}
			dst[i++] = 0xFF;

			for (; i < 7; ++i)
			{
				val = (para[i - 4] + (src[3][1] << 7)) >> 7;
				if (val <= 255)
				{
					if (val < 0)
						val = 0;
				}
				else
					val = 255;
				dst[i] = (BYTE)val;
			}
			dst[i] = 0xFF;

			dst -= width * 4;
			dst += 8;
			src[2] += 2;
		}
		src[2] += width;
		dst += width * 4;
	}
}

void PGD::_pgd3_ge_process_24(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length, DWORD width, DWORD height)
{
	BYTE *flag = __ge;
	BYTE *src = flag + height;
	BYTE *data = out;
	int org_width = width;
	for (DWORD h = 0; h < height; ++h)
	{
		width = org_width;
		if (flag[h] & 1)
		{
			*data++ = *src++;
			*data++ = *src++;
			*data++ = *src++;
			--width;
			BYTE *pre = data - 3;
			while (width-- > 0)
			{
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
			}
		}
		else if (flag[h] & 2)
		{
			BYTE *pre = data - width * 3;
			while (width-- > 0)
			{
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
			}
		}
		else if (flag[h] & 4)
		{
			*data++ = *src++;
			*data++ = *src++;
			*data++ = *src++;
			BYTE *pre = data - width * 3;
			--width;
			while (width-- > 0)
			{
				data[0] = (*pre++ + data[0 - 3]) / 2 - *src++;
				data[1] = (*pre++ + data[1 - 3]) / 2 - *src++;
				data[2] = (*pre++ + data[2 - 3]) / 2 - *src++;
				data += 3;
			}
		}
	}
}

void PGD::_pgd3_ge_process_32(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length, DWORD width, DWORD height)
{
	BYTE *flag = __ge;
	BYTE *data = out;
	BYTE *src = flag + height;
	int org_width = width;
	for (DWORD h = 0; h < height; ++h)
	{
		width = org_width;
		if (flag[h] & 1)
		{
			*(DWORD *)data = *(DWORD *)src;
			data += 4;
			src += 4;
			--width;
			BYTE *pre = data - 4;
			while (width-- > 0)
			{
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
			}
		}
		else if (flag[h] & 2)
		{
			BYTE *pre = data - width * 4;
			while (width-- > 0)
			{
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
			}
		}
		else
		{
			*(DWORD *)data = *(DWORD *)src;
			data += 4;
			BYTE *pre = data - width * 4;
			src += 4;
			--width;
			while (width-- > 0)
			{
				for (DWORD i = 0; i < 4; ++i)
					data[i] = (pre[i] + data[i - 4]) / 2 - src[i];
				data += 4;
				src += 4;
				pre += 4;
			}
		}
	}
}

void PGD::pgd_ge_process3(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length, DWORD width, DWORD height, DWORD bpp)
{
	if (bpp == 32)
		_pgd3_ge_process_32(out, out_len, __ge, __ge_length, width, height);
	else if (bpp = 24)
		_pgd3_ge_process_24(out, out_len, __ge, __ge_length, width, height);
	else
		printf("未知的GE类型，bpp:%d\n", bpp);
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

bool PGD::pgd_uncompress()
{
	if (Header_OK)
	{
		if (strncmp("GE", pgd32_header.maigc, 2) == 0)
		{
			if (pgd32_header.sizeof_header == 0x20)
			{
				fread(&pgd32_info, sizeof(pgd32_info_t), 1, fp);
				BYTE* data = new BYTE[pgd32_info.comprlen];
				fread(data, 1, pgd32_info.comprlen, fp);
				BYTE* uncompr = new BYTE[pgd32_info.uncomprlen];
				if (pgd32_header.compr_method == 3)
				{
					cout << "uncompress GE...\n";
					_pgd_uncompress32(data, uncompr, pgd32_info.uncomprlen);
					delete[] data;
					memcpy(&ge_header, uncompr, sizeof(ge_header_t));
					DWORD out_len = pgd32_header.width * pgd32_header.height * ge_header.bpp / 8;
					BYTE* out = new BYTE[out_len];
					cout << "process GE...\n";
					pgd_ge_process3(out, out_len, (BYTE*)(uncompr + sizeof(ge_header_t)), pgd32_info.uncomprlen - sizeof(ge_header_t), ge_header.width, ge_header.height, ge_header.bpp);
					cout << "解压完成!\n";
					delete[] uncompr;
					if (!PGD::ge2png(out))
					{
						delete[] out;
						cout << "生成png失败!\n";
					}
					else
					{
						delete[] out;
						return true;
					}
				}
				else if (pgd32_header.compr_method == 2)
				{
					cout << "uncompress GE...\n";
					_pgd_uncompress32(data, uncompr, pgd32_info.uncomprlen);
					delete[] data;
					ge_header.bpp = 32;
					ge_header.width = (WORD)pgd32_header.width;
					ge_header.height = (WORD)pgd32_header.height;
					ge_header.unknown = 7;
					DWORD out_len = pgd32_header.width * pgd32_header.height * 4;
					BYTE* out = new BYTE[out_len];
					cout << "process GE...\n";
					pgd_ge_process2(out, out_len, uncompr, pgd32_info.uncomprlen, pgd32_header.width, pgd32_header.height);
					cout << "解压完成!\n";
					delete[] uncompr;
					if (!PGD::ge2png(out))
					{
						delete[] out;
						cout << "生成png失败!\n";
					}
					else
					{
						delete[] out;
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

bool PGD::ge2png(BYTE *data)
{
	BYTE buff;
	FILE *Pngname = fopen((filename.substr(0, filename.find_last_of(".")) + ".png").c_str(), "wb");
	png_structp png_ptr;
	png_infop info_ptr;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	printf("width:%d height:%d bpp:%d\n", ge_header.width, ge_header.height, ge_header.bpp);
	if (png_ptr == NULL)
	{
		printf("PNG信息创建失败!\n");
		return false;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		printf("info信息创建失败!\n");
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return false;
	}
	png_init_io(png_ptr, Pngname);
	if (ge_header.bpp == 32)
		png_set_IHDR(png_ptr, info_ptr, ge_header.width, ge_header.height, ge_header.bpp / 4, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	else if (ge_header.bpp == 24)
		png_set_IHDR(png_ptr, info_ptr, ge_header.width, ge_header.height, ge_header.bpp / 3, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	else
	{
		printf("未知bpp:%d\n", ge_header.bpp);
		return false;
	}
	png_write_info(png_ptr, info_ptr);
	for (DWORD i = 0; i < (DWORD)(ge_header.width * ge_header.height); i++)
	{
		buff = data[i*ge_header.bpp / 8 + 0];
		data[i*ge_header.bpp / 8 + 0] = data[i*ge_header.bpp / 8 + 2];
		data[i*ge_header.bpp / 8 + 2] = buff;
	}
	if (ge_header.bpp == 32)
		for (DWORD i = 0; i < ge_header.height; i++)
			png_write_row(png_ptr, data + i*ge_header.width * 4);
	else if (ge_header.bpp = 24)
		for (DWORD i = 0; i < ge_header.height; i++)
			png_write_row(png_ptr, data + i*ge_header.width * 3);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(Pngname);
	return true;
}

PGD::~PGD()
{
	fclose(fp);
}