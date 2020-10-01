/*
用于创建文件头为iar的iar文件
made by Darkness-TX
2018.05.26
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

struct index
{
	char FileName[MAX_PATH];//文件名
	unit32 FileSize;//文件大小
	unit32 opcode_offset;//opcode起始
	unit32 index;//文件中索引
	char arc_name[MAX_PATH];//所在文件
}Index[5000];

struct file_opcode
{
	unit8 name_opcode;
	unit8 name_offset[4];
	unit8 index_opcode;
	unit8 index_num[4];
}File_Opcode;

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

unit32 process_dir(char *dname)
{
	long Handle;
	unit32 i = 0;
	struct _finddata32_t FileInfo;
	_chdir(dname);//跳转路径
	if ((Handle = _findfirst("*.*", &FileInfo)) == -1L)
	{
		printf("没有找到文件！\n");
		system("pause");
		exit(0);
	}
	do
	{
		if (FileInfo.name[0] == '.')  //过滤本级目录和父目录
			continue;
		sprintf(Index[FileNum].FileName, FileInfo.name);
		Index[FileNum].FileSize = FileInfo.size;
		FileNum++;
	} while (_findnext(Handle, &FileInfo) == 0);
	_chdir("..");
	return FileNum;
}

void WriteRTFC(char *fname)
{
	FILE *src = fopen("SEC5/RTFC", "rb");
	if (src == NULL)
	{
		printf("RTFC打开错误，请检查SEC5文件夹下是否有RTFC\n");
		system("pause");
		exit(0);
	}
	FILE *dst = fopen("SEC5/RTFC.new", "wb");
	unit32 num = 0, filesize = 0;
	unit8 *data = NULL;
	fseek(src, 0, SEEK_END);
	filesize = ftell(src);
	fseek(src, 0, SEEK_SET);
	fread(&num, 4, 1, src);
	num += 1;
	fwrite(&num, 4, 1, dst);
	data = malloc(filesize - 4);
	fread(data, filesize - 4, 1, src);
	fwrite(data, filesize - 4, 1, dst);
	fwrite(fname, strlen(fname) + 1, 1, dst);
	fclose(src);
	fclose(dst);
	free(data);
}

unit32 ReadNumber(FILE *src, FILE *dst, unit32 length_code, BOOL open)
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
		if (open)
			fwrite(&b, 1, 1, dst);
		n |= b << rank;
		rank += 8;
	}
	if (count <= 3)
	{
		unit32 sign = n & (1 << (8 * count - 1));
		if (sign != 0)
		{
			n -= sign << 1;
			printf("off:0x%X num:0x%X old_num:0x%X\n", ftell(src) - count, n, n + (sign << 1));
			system("pause");
		}
	}
	return n;
}

unit32 ReadInteger(FILE *src, FILE *dst, BOOL open)
{
	unit32 opcode = 0;
	fread(&opcode, 1, 1, src);
	if (open)
		fwrite(&opcode, 1, 1, dst);
	if ((opcode & 0xE0) != 0)
	{
		if ((opcode & 0xF8) != 0x80)
		{
			printf("opcode不为0x80！pos:0x%X opcode:0x%X\n", ftell(src) - 1, opcode);
			system("pause");
			exit(0);
		}
		opcode = ReadNumber(src, dst, opcode, open);
	}
	else
		opcode = (opcode & 0x0F) - (opcode & 0x10);
	return opcode;
}

void SkipObject(FILE *src, FILE *dst, BOOL open)
{
	unit32 opcode = 0;
	fread(&opcode, 1, 1, src);
	if (open)
		fwrite(&opcode, 1, 1, dst);
	if ((opcode & 0xE0) != 0)
		ReadNumber(src, dst, opcode, open);
}

void ReadString(FILE *src, FILE *dst, char *dstname, BOOL open)
{
	unit32 opcode = 0, offset = 4, name_length = 0, savepos = 0;
	char name_str[MAX_PATH];
	fread(&opcode, 1, 1, src);
	if (open)
		fwrite(&opcode, 1, 1, dst);
	if ((opcode & 0xF8) != 0x90)
	{
		printf("opcode不等于0x90！pos:0x%X opcode:0x%X\n", ftell(src) - 1, opcode);
		system("pause");
		exit(0);
	}
	offset += ReadNumber(src, dst, opcode, open);
	savepos = ftell(src);
	fseek(src, offset, SEEK_SET);
	fread(&name_length, 4, 1, src);
	fread(name_str, name_length, 1, src);
	name_str[name_length] = '\0';
	sprintf(dstname, "%s", name_str);
	fseek(src, savepos, SEEK_SET);
}

void Build_Name_Opcode(unit32 offset)
{
	unit8 *p = (unit8 *)&offset;
	File_Opcode.name_opcode = 0;
	for (unit32 i = 0; i < 4; i++)
	{
		File_Opcode.name_offset[i] = p[i];
		if (p[i] != 0)
			File_Opcode.name_opcode++;
	}
	if (File_Opcode.name_opcode != 0)
		File_Opcode.name_opcode--;
	File_Opcode.name_opcode |= 0x90;
}

void Build_Index_Opcode(unit32 index)
{
	unit8 *p = (unit8 *)&index;
	File_Opcode.index_opcode = 0;
	for (unit32 i = 0; i < 4; i++)
		File_Opcode.index_num[i] = p[i];
	if (index < 0x100)
		File_Opcode.index_opcode = 1;
	else if (index < 0x10000)
		File_Opcode.index_opcode = 2;
	else if (index < 0x1000000)
		File_Opcode.index_opcode = 3;
	if (index >= 0x7F)
	{
		if (index >= 0x100)
			File_Opcode.index_opcode--;
		File_Opcode.index_opcode |= 0x80;
	}
	else
		File_Opcode.index_opcode = 0x80;
}

void WriteRES2(char *fname)
{
	unit32 opcode_off = 0, name_count = 0, param_count = 0, arc_index = 0, i = 0, j = 0, k = 0, buff = 0;
	char name[MAX_PATH], type[MAX_PATH], arc_type[MAX_PATH], arc_name[MAX_PATH], param_name[MAX_PATH];
	unit8 *data = NULL;
	FILE *src = fopen("SEC5/RES2", "rb");
	if (src == NULL)
	{
		printf("RES2打开错误，请检查SEC5文件夹下是否有RES2\n");
		system("pause");
		exit(0);
	}
	FILE *dst = fopen("SEC5/RES2.new", "wb");
	fread(&opcode_off, 4, 1, src);
	data = malloc(opcode_off);
	fread(data, opcode_off, 1, src);
	fseek(dst, 4, SEEK_SET);
	fwrite(data, opcode_off, 1, dst);
	free(data);
	Build_Name_Opcode(ftell(dst) - 4);
	buff = strlen(fname);
	fwrite(&buff, 4, 1, dst);
	fwrite(fname, strlen(fname), 1, dst);
	buff = ftell(dst) - 4;
	fseek(dst, 0, SEEK_SET);
	fwrite(&buff, 4, 1, dst);
	fseek(dst, 0, SEEK_END);
	opcode_off += 4;
	fseek(src, opcode_off, SEEK_SET);
	fread(&name_count, 4, 1, src);
	fwrite(&name_count, 4, 1, dst);
	for (i = 0; i < name_count; i++)
	{
		buff = ftell(src);
		ReadString(src, dst, name, TRUE);
		ReadString(src, dst, type, TRUE);
		ReadString(src, dst, arc_type, TRUE);
		param_count = ReadInteger(src, dst, TRUE);
		arc_index = 0;
		arc_name[0] = '\0';
		for (j = 0; j < param_count; j++)
		{
			ReadString(src, dst, param_name, TRUE);
			if (strncmp("path", param_name, 4) == 0)
				ReadString(src, dst, arc_name, TRUE);
			else if (strncmp("arc-index", param_name, 9) == 0)
				arc_index = ReadInteger(src, dst, TRUE);
			else if (strncmp("arc-path", param_name, 8) == 0)
				SkipObject(src, dst, TRUE);
			else
			{
				printf("发现未知的参数名：%s offset:0x%X\n", param_name, ftell(src));
				system("pause");
				SkipObject(src, dst, TRUE);
			}
		}
		for (j = 0; j < IAR_Header.file_num; j++)
		{
			if (strcmp(name, Index[j].FileName) == 0)
			{
				Index[j].index = arc_index;
				sprintf(Index[j].arc_name, "%s", arc_name);
				Index[j].opcode_offset = buff;
				buff = ftell(src) - buff;
				fseek(dst, 0 - buff, SEEK_CUR);
				Build_Index_Opcode(j);
				fseek(src, Index[j].opcode_offset, SEEK_SET);
				buff = ftell(dst);
				ReadString(src, dst, name, TRUE);
				ReadString(src, dst, type, TRUE);
				ReadString(src, dst, arc_type, TRUE);
				param_count = ReadInteger(src, dst, TRUE);
				arc_index = 0;
				arc_name[0] = '\0';
				for (k = 0; k < param_count; k++)
				{
					ReadString(src, dst, param_name, TRUE);
					if (strncmp("path", param_name, 4) == 0)
					{
						ReadString(src, dst, arc_name, FALSE);
						fwrite(&File_Opcode.name_opcode, 1, 1, dst);
						fwrite(File_Opcode.name_offset, File_Opcode.name_opcode - 0x90 + 1, 1, dst);
					}
					else if (strncmp("arc-index", param_name, 9) == 0)
					{
						arc_index = ReadInteger(src, dst, FALSE);
						fwrite(&File_Opcode.index_opcode, 1, 1, dst);
						fwrite(File_Opcode.index_num, File_Opcode.index_opcode - 0x80 + 1, 1, dst);
					}
					else if (strncmp("arc-path", param_name, 8) == 0)
						SkipObject(src, dst, TRUE);
					else
					{
						printf("发现未知的参数名：%s offset:0x%X\n", param_name, ftell(src));
						system("pause");
						SkipObject(src, dst, TRUE);
					}
				}
				printf("arc_name:%s arc_index:%d arc_type:%s file_name:%s file_type:%s op_offset:0x%X\n", fname, i, arc_type, name, type, buff);
				break;
			}
		}
	}
	fclose(src);
	fclose(dst);
}

void BuildHeader()
{
	strcpy(IAR_Header.magic, "iar ");
	IAR_Header.version = 4;
	IAR_Header.header_size = 0x0C;
	IAR_Header.info_header_size = 0x14;
	time(&IAR_Header.build_time);
	IAR_Header.dir_num = 0;
	IAR_Header.file_num = FileNum;
	IAR_Header.total_num = FileNum;
	printf("version:%d dir_num:%d file_num:%d build_time:%s\n", IAR_Header.version, IAR_Header.dir_num, IAR_Header.file_num, ctime(&IAR_Header.build_time));
}

void WriteIndex(char *fname)
{
	char dstname[MAX_PATH], *filename = NULL;
	unit32 i = 0;
	if (strrchr(fname, '\\') != NULL)
		filename = strrchr(fname, '\\') + 1;
	else
		filename = fname;
	sprintf(dstname, "%s.iar", filename);
	BuildHeader();
	for (i = 0; i < IAR_Header.file_num; i++)
	{
		if (strrchr(Index[i].FileName, '.') != NULL)
			*strrchr(Index[i].FileName, '.') = '\0';
	}
	WriteRTFC(dstname);
	WriteRES2(dstname);
}

unit8* ReadPng(FILE *pngfile, char* filename)
{
	png_structp png_ptr;
	png_infop info_ptr, end_ptr;
	png_bytep *rows;
	unit32 i, width = 0, height = 0, bpp = 0, format = 0;
	unit8 buff = 0, *bitmapdata = NULL, *dst_data = NULL;
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
		printf("图片的长宽与原图不符！%s org_width:%d org_height:%d width:%d height:%d\n", filename, IAR_Image_Header.width, IAR_Image_Header.height, width, height);
		system("pause");
		exit(0);
	}
	if (format == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		if (IAR_Image_Header.flag != 0x3C && IAR_Image_Header.flag != 0x83C)
		{
			printf("原始图片的flag与png的bpp不对应,flag应为0x3C或0x83C！%s flag:0x%X bpp:32\n", filename, IAR_Image_Header.flag);
			system("pause");
			exit(0);
		}
		if (IAR_Image_Header.flag == 0x83C)
		{
			IAR_Image_Header.flag = 0x3C;
			IAR_Image_Header.uncomprlen = IAR_Image_Header.stride * IAR_Image_Header.height;
		}
		IAR_Image_Header.comprlen = IAR_Image_Header.uncomprlen;
		rows = (png_bytep*)malloc(IAR_Image_Header.height * sizeof(char*));
		bitmapdata = malloc(IAR_Image_Header.width * IAR_Image_Header.height * 4);
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
		dst_data = malloc(IAR_Image_Header.uncomprlen);
		for (i = 0; i < IAR_Image_Header.height; i++)
			memcpy(&dst_data[i * IAR_Image_Header.stride], &bitmapdata[i * IAR_Image_Header.width * 4], IAR_Image_Header.width * 4);
		free(bitmapdata);
	}
	else if (format == PNG_COLOR_TYPE_RGB)
	{
		if (IAR_Image_Header.flag != 0x1C && IAR_Image_Header.flag != 0x81C)
		{
			printf("原始图片的flag与png的bpp不对应,flag应为0x1CC或0x81C！%s flag:0x%X bpp:24\n", filename, IAR_Image_Header.flag);
			system("pause");
			exit(0);
		}
		if (IAR_Image_Header.flag == 0x81C)
		{
			IAR_Image_Header.flag = 0x1C;
			IAR_Image_Header.uncomprlen = IAR_Image_Header.stride * IAR_Image_Header.height;
		}
		IAR_Image_Header.comprlen = IAR_Image_Header.uncomprlen;
		rows = (png_bytep*)malloc(IAR_Image_Header.height * sizeof(char*));
		bitmapdata = malloc(IAR_Image_Header.width * IAR_Image_Header.height * 3);
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
		dst_data = malloc(IAR_Image_Header.uncomprlen);
		for (i = 0; i < IAR_Image_Header.height; i++)
			memcpy(&dst_data[i * IAR_Image_Header.stride], &bitmapdata[i * IAR_Image_Header.width * 3], IAR_Image_Header.width * 3);
		free(bitmapdata);
	}
	else if (format == PNG_COLOR_TYPE_PALETTE)
	{
		printf("索引图片未做处理！ %s\n", filename);
		system("pause");
		exit(0);
	}
	else
	{
		printf("未知的图片类型！\n");
		system("pause");
		exit(0);
	}
	return dst_data;
}

void PackFile(char *fname)
{
	char dstname[MAX_PATH];
	FILE *src = NULL, *dst = NULL, *fp = NULL;
	unit32 i = 0;
	unit64 offset = 0;
	unit8 *data = NULL;
	sprintf(dstname, "%s.iar", fname);
	dst = fopen(dstname, "wb");
	fwrite(&IAR_Header, sizeof(IAR_Header), 1, dst);
	fseek(dst, IAR_Header.file_num * 8, SEEK_CUR);
	for (i = 0; i < IAR_Header.file_num; i++)
	{
		fp = fopen(Index[i].arc_name, "rb");
		if (fp == NULL)
		{
			printf("原始文件%s不存在！\n", Index[i].arc_name);
			system("pause");
			exit(0);
		}
		fseek(fp, sizeof(IAR_Header) + Index[i].index * 8, SEEK_SET);
		fread(&offset, 8, 1, fp);
		_fseeki64(fp, offset, SEEK_SET);
		fread(&IAR_Image_Header, sizeof(IAR_Image_Header), 1, fp);
		IAR_Image_Header.is_compress = 0;
		fclose(fp);
		_chdir(fname);
		offset = _ftelli64(dst);
		Index[i].FileName[strlen(Index[i].FileName)] = '.';
		src = fopen(Index[i].FileName, "rb");
		data = ReadPng(src, Index[i].FileName);
		if (IAR_Image_Header.flag == 0x3C)
			printf("\t%s offset:0x%llX stride:0x%X width:%d height:%d bpp:32 flag:0x%X uncomprlen:0x%X comprlen:0x%X\n", Index[i].FileName, offset, IAR_Image_Header.stride, IAR_Image_Header.width, IAR_Image_Header.height, IAR_Image_Header.flag, IAR_Image_Header.uncomprlen, IAR_Image_Header.comprlen);
		else
			printf("\t%s offset:0x%llX stride:0x%X width:%d height:%d bpp:24 flag:0x%X uncomprlen:0x%X comprlen:0x%X\n", Index[i].FileName, offset, IAR_Image_Header.stride, IAR_Image_Header.width, IAR_Image_Header.height, IAR_Image_Header.flag, IAR_Image_Header.uncomprlen, IAR_Image_Header.comprlen);
		fclose(src);
		fwrite(&IAR_Image_Header, sizeof(IAR_Image_Header), 1, dst);
		fwrite(data, IAR_Image_Header.comprlen, 1, dst);
		free(data);
		fseek(dst, i * 8 + sizeof(IAR_Header), SEEK_SET);
		fwrite(&offset, 8, 1, dst);
		fseek(dst, 0, SEEK_END);
		_chdir("..");
	}
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-StudioSeldomAdventureSystem\n用于封包文件头为iar 的iar文件。\n将要封包的文件夹拖到程序上。\nby Darkness-TX 2018.05.26\n\n");
	process_dir(argv[1]);
	WriteIndex(argv[1]);
	PackFile(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}