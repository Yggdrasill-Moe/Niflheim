/*
用于封包文件头为iar的iar文件
made by Darkness-TX
2018.05.25
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
	unit64 offset;
	struct fileinfo *next;
}NodeFileInfo, *LinkFileInfo;
LinkFileInfo FileInfo = NULL;

unit32 ReadNumber(FILE *src, unit32 length_code)
{
	unit32 count = 0, i = 0, n = 0, rank = 0, b = 0;
	count = (length_code & 7) + 1;
	if (count > 4)
	{
		printf("count大于4！pos:0x%X opcode:0x%X\n", ftell(src), count);
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
	if ((opcode & 0xE0) != 0)
	{
		if ((opcode & 0xF8) != 0x80)
		{
			printf("opcode不为0x80！pos:0x%X opcode:0x%X\n", ftell(src) - 1, opcode);
			system("pause");
			exit(0);
		}
		opcode = ReadNumber(src, opcode);
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

void ReadString(FILE *src, char *dstname)
{
	unit32 opcode = 0, offset = 4, name_length = 0, savepos = 0;
	char name_str[MAX_PATH];
	fread(&opcode, 1, 1, src);
	if ((opcode & 0xF8) != 0x90)
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
				ReadString(src, name);
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
	fclose(src);
}

void ReadPng(FILE *pngfile, unit8 *bitmapdata,unit32 org_bpp)
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
	png_init_io(png_ptr, pngfile);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)&width, (png_uint_32*)&height, &bpp, &format, NULL, NULL, NULL);
	if (width != IAR_Image_Header.width || height != IAR_Image_Header.height)
	{
		printf("图片的长宽与原图不符！org_width:%d org_height:%d width:%d height:%d\n", IAR_Image_Header.width, IAR_Image_Header.height, width, height);
		system("pause");
		exit(0);
	}
	if (org_bpp == 32)
	{
		if (format != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			printf("原始图片为32位图，而png文件不是32位图，请转换！format:%d org_bpp:%d\n", format, org_bpp);
			system("pause");
			exit(0);
		}
		rows = (png_bytep*)malloc(IAR_Image_Header.height * sizeof(char*));
		for (i = 0; i < IAR_Image_Header.height; i++)
			rows[i] = (png_bytep)(bitmapdata + IAR_Image_Header.width*i * 4);
		png_read_image(png_ptr, rows);
		free(rows);
		png_read_end(png_ptr, info_ptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
		for (i = 0; i < IAR_Image_Header.height * IAR_Image_Header.width; i++)
		{
			buff = bitmapdata[i * 4 + 0];
			bitmapdata[i * 4 + 0] = bitmapdata[i * 4 + 2];
			bitmapdata[i * 4 + 2] = buff;
		}
	}
	else if (org_bpp == 24)
	{
		if (format != PNG_COLOR_TYPE_RGB)
		{
			printf("原始图片为24位图，而png文件不是24位图，请转换！format:%d org_bpp:%d\n", format, org_bpp);
			system("pause");
			exit(0);
		}
		rows = (png_bytep*)malloc(IAR_Image_Header.height * sizeof(char*));
		for (i = 0; i < IAR_Image_Header.height; i++)
			rows[i] = (png_bytep)(bitmapdata + IAR_Image_Header.width*i * 3);
		png_read_image(png_ptr, rows);
		free(rows);
		png_read_end(png_ptr, info_ptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
		for (i = 0; i < IAR_Image_Header.height * IAR_Image_Header.width; i++)
		{
			buff = bitmapdata[i * 3 + 0];
			bitmapdata[i * 3 + 0] = bitmapdata[i * 3 + 2];
			bitmapdata[i * 3 + 2] = buff;
		}
	}
	else if (org_bpp == 8)
	{
		if (format != PNG_COLOR_TYPE_RGB_ALPHA)
		{
			printf("原始图片为8位转32位图，而png文件不是32位图，请转换！format:%d org_bpp:%d\n", format, org_bpp);
			system("pause");
			exit(0);
		}
		unit8 *data = malloc(IAR_Image_Header.height * IAR_Image_Header.width * 4);
		rows = (png_bytep*)malloc(IAR_Image_Header.height * sizeof(char*));
		for (i = 0; i < IAR_Image_Header.height; i++)
			rows[i] = (png_bytep)(data + IAR_Image_Header.width*i * 4);
		png_read_image(png_ptr, rows);
		free(rows);
		png_read_end(png_ptr, info_ptr);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
		for (i = 0; i < IAR_Image_Header.height * IAR_Image_Header.width; i++)
			bitmapdata[i] = data[i * 4 + 0];
		free(data);
	}
}

void PackFile(char *fname)
{
	FILE *src = NULL, *dst = NULL, *fp = NULL;
	unit32 i = 0, j = 0, filesize = 0;
	unit64 offset = 0;
	unit8 *udata = NULL, *dst_data = NULL;
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
		printf("header_size不是0x0C或info_header_size不是0x14!\nheader_size:0x%X info_header_size:0x%X\n", IAR_Header.header_size, IAR_Header.info_header_size);
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
	sprintf(dstname, "%s_new", fname);
	dst = fopen(dstname, "wb");
	sprintf(dstname, "%s_unpack", fname);
	_mkdir(dstname);
	_chdir(dstname);
	fwrite(&IAR_Header, sizeof(IAR_Header), 1, dst);
	fseek(dst, IAR_Header.file_num * 8, SEEK_CUR);
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
		if (IAR_Image_Header.palettesize == 0)
		{
			dst_data = malloc(IAR_Image_Header.uncomprlen);
			memset(dst_data, 0, IAR_Image_Header.uncomprlen);
			p->offset = _ftelli64(dst);
			if (IAR_Image_Header.flag == 0x3C)
			{
				IAR_Image_Header.is_compress = 0;
				printf("\t%s offset:0x%llX stride:0x%X width:%d height:%d bpp:32 flag:0x%X ", dstname, p->offset, IAR_Image_Header.stride, IAR_Image_Header.width, IAR_Image_Header.height, IAR_Image_Header.flag);
				fp = fopen(dstname, "rb");
				udata = malloc(IAR_Image_Header.width * IAR_Image_Header.height * 4);
				ReadPng(fp, udata, 32);
				fclose(fp);
				for (j = 0; j < IAR_Image_Header.height; j++)
					memcpy(&dst_data[j * IAR_Image_Header.stride], &udata[j * IAR_Image_Header.width * 4], IAR_Image_Header.width * 4);
			}
			else if (IAR_Image_Header.flag == 0x1C)
			{
				IAR_Image_Header.is_compress = 0;
				printf("\t%s offset:0x%llX stride:0x%X width:%d height:%d bpp:24 flag:0x%X ", dstname, p->offset, IAR_Image_Header.stride, IAR_Image_Header.width, IAR_Image_Header.height, IAR_Image_Header.flag);
				fp = fopen(dstname, "rb");
				udata = malloc(IAR_Image_Header.width * IAR_Image_Header.height * 3);
				ReadPng(fp, udata, 24);
				fclose(fp);
				for (j = 0; j < IAR_Image_Header.height; j++)
					memcpy(&dst_data[j * IAR_Image_Header.stride], &udata[j * IAR_Image_Header.width * 3], IAR_Image_Header.width * 3);
			}
			else if (IAR_Image_Header.flag == 0x02)
			{
				IAR_Image_Header.is_compress = 0;
				printf("\t%s offset:0x%llX stride:0x%X width:%d height:%d bpp:8 flag:0x%X ", dstname, p->offset, IAR_Image_Header.stride, IAR_Image_Header.width, IAR_Image_Header.height, IAR_Image_Header.flag);
				fp = fopen(dstname, "rb");
				udata = malloc(IAR_Image_Header.width * IAR_Image_Header.height);
				ReadPng(fp, udata, 8);
				fclose(fp);
				for (j = 0; j < IAR_Image_Header.height; j++)
					memcpy(&dst_data[j * IAR_Image_Header.stride], &udata[j * IAR_Image_Header.width], IAR_Image_Header.width);
			}
			else if (IAR_Image_Header.flag == 0x83C)
			{
				IAR_Image_Header.is_compress = 0;
				printf("\t%s offset:0x%llX stride:0x%X width:%d height:%d bpp:32 flag:0x%X ", dstname, p->offset, IAR_Image_Header.stride, IAR_Image_Header.width, IAR_Image_Header.height, IAR_Image_Header.flag);
				fp = fopen(dstname, "rb");
				udata = malloc(IAR_Image_Header.width * IAR_Image_Header.height * 4);
				ReadPng(fp, udata, 32);
				fclose(fp);
				IAR_Image_Header.flag = 0x3C;
				IAR_Image_Header.uncomprlen = IAR_Image_Header.stride * IAR_Image_Header.height;
				free(dst_data);
				dst_data = malloc(IAR_Image_Header.uncomprlen);
				for (j = 0; j < IAR_Image_Header.height; j++)
					memcpy(&dst_data[j * IAR_Image_Header.stride], &udata[j * IAR_Image_Header.width * 4], IAR_Image_Header.width * 4);
			}
			else if (IAR_Image_Header.flag == 0x81C)
			{
				IAR_Image_Header.is_compress = 0;
				printf("\t%s offset:0x%llX stride:0x%X width:%d height:%d bpp:24 flag:0x%X ", dstname, p->offset, IAR_Image_Header.stride, IAR_Image_Header.width, IAR_Image_Header.height, IAR_Image_Header.flag);
				fp = fopen(dstname, "rb");
				udata = malloc(IAR_Image_Header.width * IAR_Image_Header.height * 3);
				ReadPng(fp, udata, 24);
				fclose(fp);
				IAR_Image_Header.flag = 0x1C;
				IAR_Image_Header.uncomprlen = IAR_Image_Header.stride * IAR_Image_Header.height;
				free(dst_data);
				dst_data = malloc(IAR_Image_Header.uncomprlen);
				for (j = 0; j < IAR_Image_Header.height; j++)
					memcpy(&dst_data[j * IAR_Image_Header.stride], &udata[j * IAR_Image_Header.width * 3], IAR_Image_Header.width * 3);
			}
			else
			{
				printf("flag:0x%X\n", IAR_Image_Header.flag);
				printf("未处理的flag标志位!\n");
				system("pause");
				exit(0);
			}
			free(udata);
			IAR_Image_Header.comprlen = IAR_Image_Header.uncomprlen;
			printf("uncomprlen:0x%X comprlen:0x%X\n", IAR_Image_Header.uncomprlen, IAR_Image_Header.comprlen);
			fwrite(&IAR_Image_Header, sizeof(IAR_Image_Header), 1, dst);
			fwrite(dst_data, IAR_Image_Header.uncomprlen, 1, dst);
			free(dst_data);
			fseek(dst, i * 8 + 0x20, SEEK_SET);
			fwrite(&p->offset, 8, 1, dst);
			fseek(dst, 0, SEEK_END);
		}
		else
		{
			printf("palettesize不为0！palettesize:0x%X\n", IAR_Image_Header.palettesize);
			system("pause");
		}
		FileNum++;
	}
	fclose(src);
	fclose(dst);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-StudioSeldomAdventureSystem\n用于封包文件头为iar 的iar文件。\n将iar文件拖到程序上。\nby Darkness-TX 2018.05.25\n\n");
	ReadIndex(argv[1]);
	PackFile(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}