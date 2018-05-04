#include "sg.h"

sg::sg(string sgname)
{
	filename = sgname;
	sg::ReadHeader(sgname);
}

bool sg::ReadHeader(string sgname)
{
	fp = fopen(sgname.c_str(), "rb");
	if (fp)
		fread(&fSG_header, 1, sizeof(fSG_header_t), fp);
	else
	{
		cout << "文件不存在!\n";
		Header_OK = false;
		return Header_OK;
	}
	if (strncmp(fSG_header.magic, "fSG ", 4))
	{
		cout << "文件头不是fSG \n";
		Header_OK = false;
		return Header_OK;
	}
	fread(sec_head, 4, 1, fp);
	fseek(fp, sizeof(fSG_header_t), SEEK_SET);
	if (!ReadSecHeader())
	{
		Header_OK = false;
		return Header_OK;
	}
	Header_OK = true;
	return Header_OK;
}

bool sg::ReadSecHeader()
{
	if (strncmp(sec_head, "cJPG", 4) == 0)
	{
		fread(&cJPG_header, 1, sizeof(cJPG_header_t), fp);
	}
	else if (strncmp(sec_head, "cRGB", 4) == 0)
	{
		fread(&cRGB_header, 1, sizeof(cRGB_header_t), fp);
		printf("name:%s type:cRGB mode:%d width:%d height:%d bpp:%d data_size:0x%X\n",
			filename.c_str(), cRGB_header.mode, cRGB_header.width, cRGB_header.height, cRGB_header.bpp, cRGB_header.data_length);
	}
	else
	{
		printf("未知的类型：%s\n", sec_head);
		system("pause");
		return false;
	}
	return true;
}

void sg::_cRGB_decode(BYTE *srcdata, BYTE *dstdata)
{
	if (cRGB_header.mode == 0)
	{
		if (cRGB_header.bpp >= 24)
		{
			for (DWORD i = 0; i < cRGB_header.width * cRGB_header.height * (DWORD)cRGB_header.bpp / 8; i++)
				dstdata[i] = srcdata[i];
		}
		else
		{
			printf("未处理bpp:%d\n", cRGB_header.bpp);
			system("pause");
			exit(0);
		}
	}
	else if (cRGB_header.mode == 1)
	{
		if (cRGB_header.bpp >= 24)
		{
			BYTE *line = new BYTE[cRGB_header.width * cRGB_header.bpp / 8];
			BYTE *src = srcdata;
			BYTE *adst = new BYTE[cRGB_header.width * cRGB_header.height * cRGB_header.bpp / 8];
			BYTE *nadst = adst;
			for (DWORD y = 0; y < cRGB_header.height; y++)
			{
				BYTE *dst = line;
				//一行的BGRA分成所有B一组所有G一组所有R一组所有A一组
				for (DWORD p = 0; p < (DWORD)cRGB_header.bpp / 8; p++)
				{
					DWORD x_count = cRGB_header.width;
					while (x_count > 0)
					{
						BYTE code = *src++;
						DWORD x_copy = code & 0x3F;
						if (code & 0x40)
							x_copy = (x_copy << 8) | *src++;
						x_count -= x_copy;
						if (code & 0x80)
						{
							BYTE pixel_val = *src++;
							for (DWORD j = 0; j < x_copy; j++)
								*dst++ = pixel_val;
						}
						else
						{
							for (DWORD j = 0; j < x_copy; j++)
								*dst++ = *src++;
						}
					}
				}
				memcpy(nadst, line, cRGB_header.width * cRGB_header.bpp / 8);
				nadst += cRGB_header.width * cRGB_header.bpp / 8;
			}
			nadst = dstdata;
			for (DWORD y = 0; y < cRGB_header.height; y++)
			{
				for (DWORD x = 0; x < cRGB_header.width; x++)
				{
					*nadst++ = adst[x + y * cRGB_header.width * cRGB_header.bpp / 8 + cRGB_header.width * 2];
					*nadst++ = adst[x + y * cRGB_header.width * cRGB_header.bpp / 8 + cRGB_header.width];
					*nadst++ = adst[x + y * cRGB_header.width * cRGB_header.bpp / 8];
					if (cRGB_header.bpp == 0x20)
						*nadst++ = adst[x + y * cRGB_header.width * cRGB_header.bpp / 8 + cRGB_header.width * 3];
				}
			}
			delete[] line;
			delete[] adst;
		}
		else
		{
			printf("未处理mode:%d bpp:%d\n", cRGB_header.mode, cRGB_header.bpp);
			system("pause");
			exit(0);
		}
	}
	else
	{
		printf("未处理mode:%d\n", cRGB_header.mode);
		system("pause");
		exit(0);
	}
}

bool sg::_cRGB2png(FILE *file, BYTE *data)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
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
	png_init_io(png_ptr, file);
	if (cRGB_header.bpp == 32)
		png_set_IHDR(png_ptr, info_ptr, cRGB_header.width, cRGB_header.height, cRGB_header.bpp / 4, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	else if (cRGB_header.bpp == 24)
		png_set_IHDR(png_ptr, info_ptr, cRGB_header.width, cRGB_header.height, cRGB_header.bpp / 3, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	else
	{
		printf("未知bpp:%d\n", cRGB_header.bpp);
		return false;
	}
	png_write_info(png_ptr, info_ptr);
	if (cRGB_header.bpp == 32)
		for (DWORD i = 0; i < cRGB_header.height; i++)
			png_write_row(png_ptr, data + i * cRGB_header.width * 4);
	else if (cRGB_header.bpp = 24)
		for (DWORD i = 0; i < cRGB_header.height; i++)
			png_write_row(png_ptr, data + i * cRGB_header.width * 3);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return true;
}

void sg::_cJPG_decode(BYTE *srcdata, BYTE *dstdata)
{
	DWORD key[2][32];
	DWORD seed = cJPG_header.seed;
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
	for (DWORD i = 0; i < (cJPG_header.data_length - cJPG_header.head_size) / 4; ++i)
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
	memcpy(dec + (cJPG_header.data_length - cJPG_header.head_size) / 4, enc + (cJPG_header.data_length - cJPG_header.head_size) / 4, (cJPG_header.data_length - cJPG_header.head_size) & 3);
}

void sg::sg_decode()
{
	if (strncmp(sec_head, "cJPG", 4) == 0)
	{
		cout << "decode cJPG...\n";
		FILE *dstfile = fopen((filename.substr(0, filename.find_last_of(".")) + ".jpg").c_str(), "wb");
		BYTE *srcdata = new BYTE[cJPG_header.data_length - cJPG_header.head_size];
		BYTE *dstdata = new BYTE[cJPG_header.data_length - cJPG_header.head_size];
		fread(srcdata, 1, cJPG_header.data_length - cJPG_header.head_size, fp);
		_cJPG_decode(srcdata, dstdata);
		delete[] srcdata;
		fwrite(dstdata, 1, cJPG_header.data_length - cJPG_header.head_size, dstfile);
		delete[] dstdata;
		fclose(dstfile);
	}
	else if (strncmp(sec_head, "cRGB", 4) == 0)
	{
		cout << "decode cRGB...\n";
		FILE *dstfile = fopen((filename.substr(0, filename.find_last_of(".")) + ".png").c_str(), "wb");
		BYTE *srcdata = new BYTE[cRGB_header.data_length];
		BYTE *dstdata = new BYTE[cRGB_header.width * cRGB_header.height * cRGB_header.bpp / 8];
		fread(srcdata, 1, cRGB_header.data_length, fp);
		_cRGB_decode(srcdata, dstdata);
		delete[] srcdata;
		if (!_cRGB2png(dstfile, dstdata))
		{
			cout << "写入png中失败！\n";
			system("pause");
			exit(0);
		}
		delete[] dstdata;
		fclose(dstfile);
	}
}

sg::~sg()
{
	fclose(fp);
}