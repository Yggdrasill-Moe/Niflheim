/*
用于解包文件头为iar的iar文件
made by Darkness-TX
2018.05.24
*/
#define _CRT_SECURE_NO_WARNINGS
#define _USE_32BIT_TIME_T
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <Windows.h>
#include <locale.h>
#include <time.h>
#include <png.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;
typedef unsigned __int64 unit64;

unit32 FileNum = 0;//总文件数，初始计数为0

struct iar_header {
	unit8 magic[4];//"iar "
	unit32 version;
	unit32 header_size;
	unit32 info_header_size;
	time_t build_time;
	unit32 dir_num;
	unit32 file_num;
	unit32 total_num;
}IAR_Header;

struct iar_image_header {
	unit16 flag;
	unit8 zerobyte;
	unit8 is_compress;//0 / 1
	unit32 unk0;//0
	unit32 uncomprlen;
	unit32 palettesize;
	unit32 comprlen;
	unit32 unk1;//0
	unit32 X;
	unit32 Y;
	unit32 width;
	unit32 height;
	unit32 stride;
	unit8 bytes[0x1C];
}IAR_Image_Header;

struct iar_image_delta_header {
	unit32 base_image_id;
	unit32 start_line;
	unit32 lines;
} IAR_Image_Delta_Header;

typedef struct fileinfo
{
	char filename[MAX_PATH];
	unit32 index;
	struct fileinfo *next;
}NodeFileInfo, *LinkFileInfo;
LinkFileInfo FileInfo = NULL;

#define flag_shift \
	flag >>= 1;	\
	if (flag <= 0xffff) \
	{ \
		flag = compr[0] | (compr[1] << 8) | 0xffff0000; \
		compr += 2; \
	}

void iar_uncompress(unit8 *uncompr_org, unit8 *compr_org)
{
	unit32 flag = 0;
	unit8 *uncompr = uncompr_org, *compr = compr_org;
	while (1)
	{
		unit32 offset, copy_bytes;
		flag_shift;
		if (flag & 1)
			*uncompr++ = *compr++;
		else
		{
			unit32 tmp;
			flag_shift;
			if (flag & 1)
			{
				offset = 1;
				flag_shift;
				tmp = flag & 1;
				flag_shift;
				if (!(flag & 1))
				{
					offset = 513;
					flag_shift;
					if (!(flag & 1))
					{
						offset = 1025;
						flag_shift;
						tmp = (flag & 1) | (tmp << 1);
						flag_shift;
						if (!(flag & 1))
						{
							offset = 2049;
							flag_shift;
							tmp = (flag & 1) | (tmp << 1);
							flag_shift;
							if (!(flag & 1))
							{
								offset = 4097;
								flag_shift;
								tmp = (flag & 1) | (tmp << 1);
							}
						}
					}
				}
				offset = offset + ((tmp << 8) | *compr++);
				flag_shift;
				if (flag & 1)
					copy_bytes = 3;
				else
				{
					flag_shift;
					if (flag & 1)
						copy_bytes = 4;
					else
					{
						flag_shift;
						if (flag & 1)
							copy_bytes = 5;
						else
						{
							flag_shift;
							if (flag & 1)
								copy_bytes = 6;
							else
							{
								flag_shift;
								if (flag & 1)
								{
									flag_shift;
									if (flag & 1)
										copy_bytes = 8;
									else
										copy_bytes = 7;
								}
								else
								{
									flag_shift;
									if (flag & 1)
										copy_bytes = *compr++ + 17;
									else
									{
										flag_shift;
										copy_bytes = (flag & 1) << 2;
										flag_shift;
										copy_bytes |= (flag & 1) << 1;
										flag_shift;
										copy_bytes |= flag & 1;
										copy_bytes += 9;
									}
								}
							}
						}
					}
				}
			}
			else//外while外
			{
				flag_shift;
				copy_bytes = 2;
				if (flag & 1)
				{
					flag_shift;
					offset = (flag & 1) << 10;
					flag_shift;
					offset |= (flag & 1) << 9;
					flag_shift;
					offset = (offset | *compr++ | ((flag & 1) << 8)) + 256;
				}
				else
				{
					offset = *compr++ + 1;
					if (offset == 256)
						break;
				}
			}
			for (unit32 i = 0; i < copy_bytes; i++)
			{
				*uncompr = *(uncompr - offset);
				uncompr++;
			}
		}
	}
}

void WritePng(FILE *Pngname, unit32 Width, unit32 Height, unit32 bpp, unit8* BitmapData)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unit32 i = 0;
	unit8 buff;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		printf("PNG信息创建失败!\n");
		exit(0);
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		printf("info信息创建失败!\n");
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		exit(0);
	}
	png_init_io(png_ptr, Pngname);
	if (bpp == 32)
	{
		png_set_IHDR(png_ptr, info_ptr, Width, Height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		for (i = 0; i < Width * Height; i++)
		{
			buff = BitmapData[i * 4 + 0];
			BitmapData[i * 4 + 0] = BitmapData[i * 4 + 2];
			BitmapData[i * 4 + 2] = buff;
		}
	}
	else if (bpp == 24)
	{
		png_set_IHDR(png_ptr, info_ptr, Width, Height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		for (i = 0; i < Width * Height; i++)
		{
			buff = BitmapData[i * 3 + 0];
			BitmapData[i * 3 + 0] = BitmapData[i * 3 + 2];
			BitmapData[i * 3 + 2] = buff;
		}
	}
	else
	{
		printf("不支持的bpp类型!bpp:%d\n", bpp);
		system("pause");
		exit(0);
	}
	png_write_info(png_ptr, info_ptr);

	for (i = 0; i < Height; i++)
		png_write_row(png_ptr, BitmapData + i*Width * bpp / 8);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
}

unit32 ReadNumber(FILE *src,unit32 length_code)
{
	unit32 count = 0, i = 0, n = 0, rank = 0, b = 0;
	count = (length_code & 7) + 1;
	if (count > 4)
	{
		printf("count大于4！pos:0x%X count:0x%X\n", ftell(src), count);
		system("pause");
		exit(0);
	}
	for (i = 0; i < count; ++i)
	{
		fread(&b, 1, 1, src);
		n |= b << rank;
		rank += 8;
	}
	if (count <= 3)
	{
		unit32 sign = n & (1 << (8 * count - 1));
		if (sign != 0)
			n -= sign << 1;
	}
	return n;
}

unit32 ReadInteger(FILE *src)
{
	unit32 opcode = 0;
	fread(&opcode, 1, 1, src);
	if ((opcode & 0xE0)!=0)
	{
		if ((opcode & 0xF8)!=0x80)
		{
			printf("opcode不为0x80！pos:0x%X opcode:0x%X\n", ftell(src) - 1, opcode);
			system("pause");
			exit(0);
		}
		opcode = ReadNumber(src,opcode);
	}
	else
		opcode = (opcode & 0x0F) - (opcode & 0x10);
	return opcode;
}

void SkipObject(FILE *src)
{
	unit32 opcode = 0;
	fread(&opcode, 1, 1, src);
	if ((opcode & 0xE0) != 0)
		ReadNumber(src, opcode);
}

void ReadString(FILE *src,char *dstname)
{
	unit32 opcode = 0, offset = 4, name_length = 0, savepos = 0;
	char name_str[MAX_PATH];
	fread(&opcode, 1, 1, src);
	if ((opcode & 0xF8)!=0x90)
	{
		printf("opcode不等于0x90！pos:0x%X opcode:0x%X\n", ftell(src) - 1, opcode);
		system("pause");
		exit(0);
	}
	offset += ReadNumber(src, opcode);
	savepos = ftell(src);
	fseek(src, offset, SEEK_SET);
	fread(&name_length, 4, 1, src);
	fread(name_str, name_length, 1, src);
	name_str[name_length] = '\0';
	sprintf(dstname, "%s", name_str);
	fseek(src, savepos, SEEK_SET);
}

void ReadIndex(char *fname)
{
	unit32 opcode_off = 0, name_count = 0, param_count = 0, arc_index = 0, i = 0, j = 0;
	char name[MAX_PATH], type[MAX_PATH], arc_type[MAX_PATH], arc_name[MAX_PATH], param_name[MAX_PATH], *filename = NULL;
	FileInfo = malloc(sizeof(NodeFileInfo));
	FileInfo->next = NULL;
	LinkFileInfo p = FileInfo;
	if (strrchr(fname, '\\') != NULL)
		filename = strrchr(fname, '\\') + 1;
	else
		filename = fname;
	FILE *src = fopen("SEC5/RES2", "rb");
	if (src == NULL)
	{
		printf("RES2打开错误，请检查SEC5文件夹下是否有RES2\n");
		system("pause");
		exit(0);
	}
	fread(&opcode_off, 4, 1, src);
	opcode_off += 4;
	fseek(src, opcode_off, SEEK_SET);
	fread(&name_count, 4, 1, src);
	for (i = 0; i < name_count; i++)
	{
		ReadString(src, name);
		ReadString(src, type);
		ReadString(src, arc_type);
		param_count = ReadInteger(src);
		arc_index = 0;
		arc_name[0] = '\0';
		for (j = 0; j < param_count; j++)
		{
			ReadString(src, param_name);
			if (strncmp("path", param_name, 4) == 0)
				ReadString(src, arc_name);
			else if (strncmp("arc-index", param_name, 9) == 0)
				arc_index = ReadInteger(src);
			else if (strncmp("arc-path", param_name, 8) == 0)
				SkipObject(src);
			else
			{
				printf("发现未知的参数名：%s offset:0x%X\n", param_name, ftell(src));
				system("pause");
				SkipObject(src);
			}
		}
		if (strcmp(filename, arc_name) == 0)
		{
			p->next = malloc(sizeof(NodeFileInfo));
			sprintf(p->next->filename, "%s.png", name);
			p->next->index = arc_index;
			p = p->next;
			p->next = NULL;
			//printf("arc_name:%s arc_index:%d arc_type:%s file_name:%s file_type:%s \n", arc_name, arc_index, arc_type, name, type);
		}
	}
}

void UnpackFile(char *fname)
{
	FILE *src = NULL, *dst = NULL;
	unit32 i = 0, j = 0;
	unit64 offset = 0;
	unit8 *cdata = NULL, *udata = NULL, *dst_data = NULL;
	LinkFileInfo p = FileInfo;
	char dstname[MAX_PATH];
	src = fopen(fname, "rb");
	fread(&IAR_Header, sizeof(IAR_Header), 1, src);
	if (strncmp(IAR_Header.magic, "iar ", 4))
	{
		printf("文件头不是iar !\n");
		system("pause");
		exit(0);
	}
	if (IAR_Header.version != 4)
	{
		printf("version不是4!\n");
		system("pause");
		exit(0);
	}
	if (IAR_Header.header_size != 0x0C || IAR_Header.info_header_size != 0x14)
	{
		printf("header_size不是0x0C或info_header_size不是0x14!\nheader_size:0x%X info_header_size:0x%X\n",IAR_Header.header_size,IAR_Header.info_header_size);
		system("pause");
		exit(0);
	}
	if (IAR_Header.file_num != IAR_Header.total_num)
	{
		printf("file_num不等于total_num!file_num:%d total_num%d\n", IAR_Header.file_num, IAR_Header.total_num);
		system("pause");
		exit(0);
	}
	printf("version:%d dir_num:%d file_num:%d build_time:%s\n", IAR_Header.version, IAR_Header.dir_num, IAR_Header.file_num, ctime(&IAR_Header.build_time));
	sprintf(dstname, "%s_unpack", fname);
	_mkdir(dstname);
	_chdir(dstname);
	for (i = 0; i < IAR_Header.file_num; i++)
	{
		fseek(src, i * 8 + 0x20, SEEK_SET);
		fread(&offset, 8, 1, src);
		_fseeki64(src, offset, SEEK_SET);
		fread(&IAR_Image_Header, sizeof(IAR_Image_Header), 1, src);
		if (p->next != NULL)
		{
			p = p->next;
			/*
			纠错功能，但是如果sec5文件是官方补丁中的话，补丁文件起index就会不对
			if (p->index != i)
			{
				printf("arc_index不对! index:%d 文件index:%d\n", p->index, i);
				system("pause");
				exit(0);
			}*/
			sprintf(dstname, p->filename);
		}
		else
			sprintf(dstname, "%08d.png", i);
		printf("\t%s offset:0x%llX stride:0x%X uncomprlen:0x%X comprlen:0x%X width:%d height:%d flag:0x%X ", dstname, offset, IAR_Image_Header.stride, IAR_Image_Header.uncomprlen, IAR_Image_Header.comprlen, IAR_Image_Header.width, IAR_Image_Header.height, IAR_Image_Header.flag);
		if (IAR_Image_Header.is_compress == 1)
		{
			cdata = malloc(IAR_Image_Header.comprlen);
			fread(cdata, IAR_Image_Header.comprlen, 1, src);
			udata = malloc(IAR_Image_Header.uncomprlen);
			iar_uncompress(udata, cdata);
			free(cdata);
		}
		else if(IAR_Image_Header.is_compress == 0)
		{
			udata = malloc(IAR_Image_Header.uncomprlen);
			fread(udata, IAR_Image_Header.uncomprlen, 1, src);
		}
		else
		{
			printf("未知的is_compress标志位,is_compress:0x%X\n", IAR_Image_Header.is_compress);
			system("pause");
			exit(0);
		}
		dst = fopen(dstname, "wb");
		if (IAR_Image_Header.palettesize == 0)
		{
			if (IAR_Image_Header.flag == 0x3C)
			{
				printf("bpp:32\n");
				dst_data = malloc(IAR_Image_Header.width * IAR_Image_Header.height * 4);
				for (j = 0; j < IAR_Image_Header.height; j++)
					memcpy(&dst_data[j * IAR_Image_Header.width * 4], &udata[j * IAR_Image_Header.stride], IAR_Image_Header.width * 4);
				WritePng(dst, IAR_Image_Header.width, IAR_Image_Header.height, 32, dst_data);
				free(dst_data);
			}
			else if (IAR_Image_Header.flag == 0x02)
			{
				printf("bpp:8\n");
				cdata = malloc(IAR_Image_Header.width * IAR_Image_Header.height);
				for (j = 0; j < IAR_Image_Header.height; j++)
					memcpy(&cdata[j * IAR_Image_Header.width], &udata[j * IAR_Image_Header.stride], IAR_Image_Header.width);
				dst_data = malloc(IAR_Image_Header.width * IAR_Image_Header.height * 4);
				for (j = 0; j < IAR_Image_Header.height * IAR_Image_Header.width; j++)
				{
					memset(&dst_data[j * 4 + 0], cdata[j], 3);
					dst_data[j * 4 + 3] = 255;
				}
				free(cdata);
				WritePng(dst, IAR_Image_Header.width, IAR_Image_Header.height, 32, dst_data);
				free(dst_data);
			}
			else if (IAR_Image_Header.flag == 0x1C)
			{
				printf("bpp:24\n");
				dst_data = malloc(IAR_Image_Header.width * IAR_Image_Header.height * 3);
				for (j = 0; j < IAR_Image_Header.height; j++)
					memcpy(&dst_data[j * IAR_Image_Header.width * 3], &udata[j * IAR_Image_Header.stride], IAR_Image_Header.width * 3);
				WritePng(dst, IAR_Image_Header.width, IAR_Image_Header.height, 24, dst_data);
				free(dst_data);
			}
			else if (IAR_Image_Header.flag == 0x83C)
			{
				printf("bpp:32\n");
				cdata = malloc(IAR_Image_Header.stride * IAR_Image_Header.height);
				memset(cdata, 0, IAR_Image_Header.stride * IAR_Image_Header.height);
				memcpy(&IAR_Image_Delta_Header, udata, sizeof(IAR_Image_Delta_Header));
				unit8 *start = udata + sizeof(IAR_Image_Delta_Header);
				unit8 *cur_line = cdata + IAR_Image_Header.stride * IAR_Image_Delta_Header.start_line;
				for (j = 0; j < IAR_Image_Delta_Header.lines; j++)
				{
					unit8 *datadst = cur_line;
					unit32 cnt = *(unit16 *)start;
					start += 2;
					for (DWORD l = 0; l < cnt; l++)
					{
						unit16 pos = *(unit16 *)start * 4;
						start += 2;
						unit16 count = *(unit16 *)start * 4;
						start += 2;
						datadst += pos;
						memcpy(datadst, start, count);
						start += count;
						datadst += count;
					}
					cur_line += IAR_Image_Header.stride;
				}
				dst_data = malloc(IAR_Image_Header.width * IAR_Image_Header.height * 4);
				for (j = 0; j < IAR_Image_Header.height; j++)
					memcpy(&dst_data[j * IAR_Image_Header.width * 4], &cdata[j * IAR_Image_Header.stride], IAR_Image_Header.width * 4);
				free(cdata);
				WritePng(dst, IAR_Image_Header.width, IAR_Image_Header.height, 32, dst_data);
				free(dst_data);
			}
			else if (IAR_Image_Header.flag == 0x81C)
			{
				printf("bpp:24\n");
				cdata = malloc(IAR_Image_Header.stride * IAR_Image_Header.height);
				memset(cdata, 0, IAR_Image_Header.stride * IAR_Image_Header.height);
				memcpy(&IAR_Image_Delta_Header, udata, sizeof(IAR_Image_Delta_Header));
				unit8 *start = udata + sizeof(IAR_Image_Delta_Header);
				unit8 *cur_line = cdata + IAR_Image_Header.stride * IAR_Image_Delta_Header.start_line;
				for (j = 0; j < IAR_Image_Delta_Header.lines; j++)
				{
					unit8 *datadst = cur_line;
					unit32 cnt = *(unit16 *)start;
					start += 2;
					for (DWORD l = 0; l < cnt; l++)
					{
						unit16 pos = *(unit16 *)start * 3;
						start += 2;
						unit16 count = *(unit16 *)start * 3;
						start += 2;
						datadst += pos;
						memcpy(datadst, start, count);
						start += count;
						datadst += count;
					}
					cur_line += IAR_Image_Header.stride;
				}
				dst_data = malloc(IAR_Image_Header.width * IAR_Image_Header.height * 3);
				for (j = 0; j < IAR_Image_Header.height; j++)
					memcpy(&dst_data[j * IAR_Image_Header.width * 3], &cdata[j * IAR_Image_Header.stride], IAR_Image_Header.width * 3);
				free(cdata);
				WritePng(dst, IAR_Image_Header.width, IAR_Image_Header.height, 24, dst_data);
				free(dst_data);
			}
			else
			{
				printf("flag:0x%X\n", IAR_Image_Header.flag);
				printf("未处理的flag标志位!\n");
				system("pause");
				exit(0);
			}
		}
		else
		{
			printf("palettesize不为0！palettesize:0x%X\n", IAR_Image_Header.palettesize);
			system("pause");
		}
		fclose(dst);
		free(udata);
		FileNum++;
	}
	fclose(src);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-StudioSeldomAdventureSystem\n用于解包文件头为iar 的iar文件。\n将iar文件拖到程序上。\nby Darkness-TX 2018.05.24\n\n");
	ReadIndex(argv[1]);
	UnpackFile(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}