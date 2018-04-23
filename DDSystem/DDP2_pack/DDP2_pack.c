/*
用于解包文件头为DDP2且没有文件名的dat文件
made by Darkness-TX
2018.01.19
*/
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <Windows.h>
#include <locale.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

unit32 FileNum = 0;//总文件数，初始计数为0

struct dheader
{
	unit8 magic[4];//DDP2
	unit32 num;
	unit32 file_offset;
	unit32 filesize;//文件最后4字节
}dat_header;

struct hheader
{
	unit8 magic[8];// DDWuHXB或者DDSxHXB
	unit8 length[3];
	unit8 flag;
	unit32 unk;// 0
}hxb_header;

struct index
{
	unit32 offset;
	unit32 uncomprlen;
	unit32 comprlen;
}Index[7000];

void ddp_uncompress(unit8 *uncompr, unit32 uncomprlen, unit8 *compr, unit32 comprlen)
{
	unit32 curbyte = 0, i;
	unit32 act_uncomprlen = 0;
	while (act_uncomprlen < uncomprlen)
	{
		unit8 flag = compr[curbyte++];
		unit32 offset, copy_len;

		if (flag < 0x1D)
		{
			copy_len = flag + 1;
			offset = 0;
		}
		else if (flag == 0x1D)
		{
			copy_len = compr[curbyte++] + 0x1E;
			offset = 0;
		}
		else if (flag == 0x1E)
		{
			copy_len = ((compr[curbyte] << 8) | compr[curbyte + 1]) + 0x11E;
			curbyte += 2;
			offset = 0;
		}
		else if (flag == 0x1F)
		{
			copy_len = (compr[curbyte] << 24) | (compr[curbyte + 1] << 16)
				| (compr[curbyte + 2] << 8) | compr[curbyte + 3];
			curbyte += 4;
			offset = 0;
		}
		else
		{
			if (flag < 0x80)
			{
				if ((flag & 0x60) == 0x20)
				{
					copy_len = flag & 3;
					offset = (flag >> 2) & 7;
				}
				else if ((flag & 0x60) == 0x40)
				{
					copy_len = (flag & 0x1f) + 4;
					offset = compr[curbyte++];
				}
				else
				{
					offset = ((flag & 0x1F) << 8) | compr[curbyte++];
					flag = compr[curbyte++];
					switch (flag)
					{
					case 0xFE:
						copy_len = ((compr[curbyte] << 8) | compr[curbyte + 1]) + 0x102;
						curbyte += 2;
						break;
					case 0xFF:
						copy_len = (compr[curbyte] << 24) | (compr[curbyte + 1] << 16) | (compr[curbyte + 2] << 8) | compr[curbyte + 3];
						curbyte += 4;
						break;
					default:
						copy_len = flag + 4;
					}
				}
			}
			else
			{
				copy_len = (flag >> 5) & 3;
				offset = ((flag & 0x1F) << 8) | compr[curbyte++];
			}
			offset++;
			copy_len += 3;
		}

		if (offset)
		{
			for (i = 0; i < copy_len; i++)
			{
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
			}
		}
		else
		{
			for (i = 0; i < copy_len; i++)
				uncompr[act_uncomprlen++] = compr[curbyte++];
		}
	}
}

void hxb_encrypt(unit8 *data)
{
	int seed = hxb_header.length[0] << 16 | hxb_header.length[1] << 8 | hxb_header.length[2];
	int key = (((seed << 5) ^ 0xA5) * (seed + 0x6F349)) ^ 0x34A9B129;
	unit32 *p = (unit32 *)(data + 0x10);
	for (int i = 0; i < (seed - 13) / 4; i++)
		p[i] ^= key;
}

void PackFile(char *fname)
{
	FILE *src, *packdst, *dst;
	unit8 dstname[200], *cdata, *udata;
	unit32 i = 0;
	src = fopen(fname, "rb");
	fread(dat_header.magic, 4, 1, src);
	if (strncmp(dat_header.magic, "DDP2", 4) != 0)
	{
		printf("文件头不是DDP2!。\n");
		system("pause");
		exit(0);
	}
	sprintf(dstname, "%s_new", fname);
	packdst = fopen(dstname, "wb");
	sprintf(dstname, "%s_unpack", fname);
	fread(&dat_header.num, 1, 4, src);
	fread(&dat_header.file_offset, 4, 1, src);
	fseek(src, -4, SEEK_END);
	fread(&dat_header.filesize, 4, 1, src);
	udata = malloc(dat_header.file_offset);
	fseek(src, 0, SEEK_SET);
	fread(udata, dat_header.file_offset, 1, src);
	fwrite(udata, dat_header.file_offset, 1, packdst);
	free(udata);
	fseek(src, 0x20, SEEK_SET);
	for (i = 0; i < dat_header.num; i++)
	{
		fread(&Index[i], 0xC, 1, src);
		fseek(src, 4, SEEK_CUR);
	}
	_chdir(dstname);
	for (i = 0; i < dat_header.num; i++)
	{
		fseek(src, Index[i].offset, SEEK_SET);
		udata = malloc(Index[i].uncomprlen);
		if (Index[i].comprlen != 0)
		{
			cdata = malloc(Index[i].comprlen);
			fread(cdata, Index[i].comprlen, 1, src);
			ddp_uncompress(udata, Index[i].uncomprlen, cdata, Index[i].comprlen);
			free(cdata);
		}
		else
			fread(udata, Index[i].uncomprlen, 1, src);
		if (udata[0] == 'D' && udata[1] == 'D' && udata[4] == 'H' && udata[5] == 'X' && udata[6] == 'B')//DDWuHXB，似乎还有种DDSxHXB
		{
			free(udata);
			sprintf(dstname, "%08d.hxb", i);
			dst = fopen(dstname, "rb");
			fseek(dst, 0, SEEK_END);
			Index[i].uncomprlen = ftell(dst);
			Index[i].comprlen = 0;
			Index[i].offset = ftell(packdst);
			fseek(dst, 0, SEEK_SET);
			udata = malloc(Index[i].uncomprlen);
			fread(udata, Index[i].uncomprlen, 1, dst);
			memcpy(&hxb_header, udata, 0x10);
			hxb_encrypt(udata);
			fwrite(udata, Index[i].uncomprlen, 1, packdst);
			free(udata);
		}
		else if (udata[0] == 'B' && udata[1] == 'M')
		{
			free(udata);
			sprintf(dstname, "%08d.bmp", i);
			dst = fopen(dstname, "rb");
			fseek(dst, 0, SEEK_END);
			Index[i].uncomprlen = ftell(dst);
			Index[i].comprlen = 0;
			Index[i].offset = ftell(packdst);
			fseek(dst, 0, SEEK_SET);
			udata = malloc(Index[i].uncomprlen);
			fread(udata, Index[i].uncomprlen, 1, dst);
			fwrite(udata, Index[i].uncomprlen, 1, packdst);
			free(udata);
		}
		else if (udata[0] == 0x89 && udata[1] == 0x50 && udata[2] == 0x4E && udata[3] == 0x47)
		{
			free(udata);
			sprintf(dstname, "%08d.png", i);
			dst = fopen(dstname, "rb");
			fseek(dst, 0, SEEK_END);
			Index[i].uncomprlen = ftell(dst);
			Index[i].comprlen = 0;
			Index[i].offset = ftell(packdst);
			fseek(dst, 0, SEEK_SET);
			udata = malloc(Index[i].uncomprlen);
			fread(udata, Index[i].uncomprlen, 1, dst);
			fwrite(udata, Index[i].uncomprlen, 1, packdst);
			free(udata);
		}
		else if (udata[0] == 0 && udata[1] == 0 && (udata[2] == 0x0A || udata[2] == 0x02))
		{
			free(udata);
			sprintf(dstname, "%08d.tga", i);
			dst = fopen(dstname, "rb");
			fseek(dst, 0, SEEK_END);
			Index[i].uncomprlen = ftell(dst);
			Index[i].comprlen = 0;
			Index[i].offset = ftell(packdst);
			fseek(dst, 0, SEEK_SET);
			udata = malloc(Index[i].uncomprlen);
			fread(udata, Index[i].uncomprlen, 1, dst);
			fwrite(udata, Index[i].uncomprlen, 1, packdst);
			free(udata);
		}
		else
		{
			free(udata);
			sprintf(dstname, "%08d.bin", i);
			dst = fopen(dstname, "rb");
			fseek(dst, 0, SEEK_END);
			Index[i].uncomprlen = ftell(dst);
			Index[i].comprlen = 0;
			Index[i].offset = ftell(packdst);
			fseek(dst, 0, SEEK_SET);
			udata = malloc(Index[i].uncomprlen);
			fread(udata, Index[i].uncomprlen, 1, dst);
			fwrite(udata, Index[i].uncomprlen, 1, packdst);
			free(udata);
		}
		fclose(dst);
		printf("\t%s comprlen:0x%X uncomprlen:0x%X offset:0x%X\n", dstname, Index[i].comprlen, Index[i].uncomprlen, Index[i].offset);
		FileNum++;
	}
	fclose(src);
	fseek(packdst, 0x20, SEEK_SET);
	for (i = 0; i < dat_header.num; i++)
	{
		fwrite(&Index[i], 0xC, 1, packdst);
		fseek(packdst, 4, SEEK_CUR);
	}
	fseek(packdst, 0, SEEK_END);
	dat_header.filesize = ftell(packdst) + 4;
	fwrite(&dat_header.filesize, 1, 4, packdst);
	sprintf(dstname, "%s_new", fname);
	printf("%s num:%d data_offset:0x%X file_size:0x%X\n", dstname, dat_header.num, dat_header.file_offset, dat_header.filesize);
	fclose(packdst);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "chs");
	printf("project：Niflheim-三国恋战记\n用于封包文件头为DDP2的dat文件。\n将dat文件拖到程序上。\nby Darkness-TX 2018.01.18\n\n");
	PackFile(argv[1]);
	printf("已完成，总文件数%d\n", FileNum);
	system("pause");
	return 0;
}