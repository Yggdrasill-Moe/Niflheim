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
	if (strncmp("GE", pgd32_header.maigc0, 2) == 0)
	{
		if (pgd32_header.sizeof_header == 0x20)
		{
			fread(&pgd32_info, sizeof(pgd32_info_t), 1, fp);
			FILE *rp = fopen((filename.substr(0, filename.find_last_of(".")) + ".GEU").c_str(), "rb");
			fread(&ge_header, sizeof(ge_header_t), 1, rp);
			fclose(rp);
			Header_OK = true;
			return Header_OK;
		}
		else
			cout << "非32位类型!";
	}
	else
		cout << "文件头不是GE!";
	Header_OK = false;
	return Header_OK;
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
	cout << "restore to GEU...\n";
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
	BYTE *TexData = new BYTE[ge_header.height*ge_header.width*ge_header.bpp / 8];
	if (pngtogep(TexData))
	{
		BYTE *geu = new BYTE[pgd32_info.uncomprlen];
		pgd_ge_restore3(geu, pgd32_info.uncomprlen, TexData, ge_header.height*ge_header.width*ge_header.bpp / 8, ge_header.width, ge_header.height, ge_header.bpp);
		delete[] TexData;
		FILE *pgdfile = fopen((filename.substr(0, filename.find_last_of(".")) + ".PGDN").c_str(), "wb");
		fwrite(&pgd32_header, sizeof(pgd32_header_t), 1, pgdfile);
		fwrite(&pgd32_info, sizeof(pgd32_info_t), 1, pgdfile);
		pgd32_info.comprlen = _pgd_compress32(geu, pgd32_info.uncomprlen, pgdfile);
		delete[] geu;
		fseek(pgdfile, sizeof(pgd32_header_t), SEEK_SET);
		fwrite(&pgd32_info, sizeof(pgd32_info_t), 1, pgdfile);
		fclose(pgdfile);
		return true;
	}
	return false;
}

bool PGD::pngtogep(BYTE *TexData)
{
	BYTE buff;
	png_structp png_ptr;
	png_infop info_ptr, end_ptr;
	png_bytep *rows;
	DWORD i = 0;
	FILE *OpenPng = fopen((filename.substr(0, filename.find_last_of(".")) + ".png").c_str(), "rb");
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	cout << "restore to GEP...\n";
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