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

void PGD::_pgd3_ge_process_24(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length, WORD width, WORD height)
{
	BYTE *flag = __ge;
	BYTE *src = flag + height;
	BYTE *data = out;
	int org_width = width;
	for (int h = 0; h < height; ++h)
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

void PGD::_pgd3_ge_process_32(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length, WORD width, WORD height)
{
	BYTE *flag = __ge;
	BYTE *data = out;
	BYTE *src = flag + height;
	int org_width = width;
	for (int h = 0; h < height; ++h)
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

void PGD::pgd_ge_process3(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length, WORD width, WORD height, DWORD bpp)
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
		if (strncmp("GE", pgd32_header.maigc0, 2) == 0)
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
					ge_header_t *ge_header = (ge_header_t *)uncompr;
					DWORD bpp = ge_header->bpp;
					DWORD out_len = pgd32_header.width * pgd32_header.height * bpp / 8;
					BYTE* out = new BYTE[out_len];
					cout << "process GE...\n";
					FILE* wf = fopen((filename.substr(0, filename.find_last_of(".")) + ".GEU").c_str(), "wb");
					fwrite(uncompr, pgd32_info.uncomprlen, 1, wf);
					fclose(wf);
					pgd_ge_process3(out, out_len, (BYTE*)(ge_header + 1), pgd32_info.uncomprlen - sizeof(ge_header_t), ge_header->width, ge_header->height, bpp);
					wf = fopen((filename.substr(0, filename.find_last_of("."))+".GEP").c_str(), "wb");
					fwrite(out, 1, out_len, wf);
					cout << "解压完成!\n";
					delete[] uncompr;
					delete[] out;
					fclose(wf);
					return true;
				}
				else
					cout << "非类型3!";
			}
			else
				cout << "非32位类型!";
		}
		else
			cout << "文件头不是GE!";
	}
	else
		cout << "读取文件头失败!";
	return false;
}

bool PGD::geptopng()
{
	BYTE buff;
	FILE *fp = fopen((filename.substr(0, filename.find_last_of(".")) + ".GEU").c_str(), "rb");
	fread(&ge_header, sizeof(ge_header_t), 1, fp);
	fclose(fp);
	fp = fopen((filename.substr(0, filename.find_last_of(".")) + ".GEP").c_str(), "rb");
	BYTE *data = new BYTE[ge_header.height*ge_header.width*ge_header.bpp / 8];
	fread(data, 1, ge_header.height*ge_header.width*ge_header.bpp / 8, fp);
	fclose(fp);
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
	delete[] data;
	fclose(Pngname);
	return true;
}

PGD::~PGD()
{
	fclose(fp);
}