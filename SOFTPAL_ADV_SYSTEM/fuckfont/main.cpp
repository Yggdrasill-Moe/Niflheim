#include "font.h"

int main(int agrc, char* agrv[])
{
	cout << "project：Niflheim-SOFTPAL_ADV_SYSTEM\n用于解包及封包FONT；\n导入请确保目录下有tbl_chs.txt\nby Destinyの火狐 2017.01.20\n";
	if (agrc != 3)
		cout << "\nUsage:\n\texport:\tfuckfont -e fontfile\n\timport:\tfuckfont -i fontfile\n\timport(use freetype):\tfuckfont -fi fontfile";
	else
	{
		if (strcmp(agrv[1], "-e") == 0)
		{
			FONT font(agrv[2]);
			_mkdir(font.dirname.c_str());
			_chdir(font.dirname.c_str());
			for (DWORD i = 0; i < font.count; i++)
				if (!font.font2png(i))
				{
					cout << "export fail!\n";
					system("pause");
				}
			cout << "all " << dec << font.count << " fonts.\n";
			cout << "done！\n";
		}
		else if (strcmp(agrv[1], "-i") == 0)
		{
			FONT font(agrv[2]);
			cout << "all " << dec << font.count << " fonts.\n";
			char dstname[200];
			DWORD pos;
			BYTE *data;
			wchar_t tbl, tbl_data[256], *find;
			FILE *tbl_txt = fopen("tbl_chs.txt", "rt,ccs=UNICODE");
			sprintf(dstname, "%s_new", agrv[2]);
			FILE *dstfile = fopen(dstname, "wb");
			pos = font.count * 4;
			fseek(dstfile, pos, SEEK_SET);
			for (DWORD i = 0; i < font.count; i++)
			{
				if (font.findexs[i].offset == 0)
					continue;
				if (i < font.tbl_start)
				{
					fwrite(&font.findexs[i].gmBlackBoxX, 0x18, 1, dstfile);
					if (font.findexs[i].size != 0)
					{
						data = new BYTE[font.findexs[i].size];
						fseek(font.fp, font.findexs[i].offset + 0x18, SEEK_SET);
						fread(data, 1, font.findexs[i].size, font.fp);
						fwrite(data, 1, font.findexs[i].size, dstfile);
						delete[] data;
					}
				}
				else
				{
					if (fgetws(tbl_data, 256, tbl_txt) != NULL)
					{
						font.findexs[i].offset = pos;
						find = wcschr(tbl_data, L'=');
						tbl = tbl_data[find - tbl_data + 1];
						data = font.makefont(tbl, i);
						fwrite(&font.findexs[i].gmBlackBoxX, 0x18, 1, dstfile);
						fwrite(data, 1, font.findexs[i].size, dstfile);
						delete[] data;
					}
					else
					{
						fwrite(&font.findexs[i].gmBlackBoxX, 0x18, 1, dstfile);
						if (font.findexs[i].size != 0)
						{
							data = new BYTE[font.findexs[i].size];
							fseek(font.fp, font.findexs[i].offset + 0x18, SEEK_SET);
							fread(data, 1, font.findexs[i].size, font.fp);
							fwrite(data, 1, font.findexs[i].size, dstfile);
							delete[] data;
						}
					}
				}
				font.findexs[i].offset = pos;
				pos = ftell(dstfile);
			}
			BYTE *buff = new BYTE[0x38];
			fseek(font.fp, -0x38, SEEK_END);
			fread(buff, 1, 0x38, font.fp);
			fwrite(buff, 1, 0x38, dstfile);
			delete[] buff;
			fseek(dstfile, 0, SEEK_SET);
			for (DWORD i = 0; i < font.count; i++)
				fwrite(&font.findexs[i].offset, 1, 4, dstfile);
			fclose(tbl_txt);
			fclose(dstfile);
		}
		else if (strcmp(agrv[1], "-fi") == 0)
		{
			FONT font(agrv[2]);
			cout << "all " << dec << font.count << " fonts.\n";
			char dstname[200];
			DWORD pos;
			BYTE *data;
			wchar_t tbl, tbl_data[256], *find;
			FILE *tbl_txt = fopen("tbl_chs.txt", "rt,ccs=UNICODE");
			sprintf(dstname, "%s_new", agrv[2]);
			FILE *dstfile = fopen(dstname, "wb");
			pos = font.count * 4;
			fseek(dstfile, pos, SEEK_SET);
			font.FT_Init("SourceHanSansCN-Medium.otf", 35);
			for (DWORD i = 0; i < font.count; i++)
			{
				if (font.findexs[i].offset == 0)
					continue;
				if (i < font.tbl_start)
				{
					fwrite(&font.findexs[i].gmBlackBoxX, 0x18, 1, dstfile);
					if (font.findexs[i].size != 0)
					{
						data = new BYTE[font.findexs[i].size];
						fseek(font.fp, font.findexs[i].offset + 0x18, SEEK_SET);
						fread(data, 1, font.findexs[i].size, font.fp);
						fwrite(data, 1, font.findexs[i].size, dstfile);
						delete[] data;
					}
				}
				else
				{
					if (fgetws(tbl_data, 256, tbl_txt) != NULL)
					{
						font.findexs[i].offset = pos;
						find = wcschr(tbl_data, L'=');
						tbl = tbl_data[find - tbl_data + 1];
						data = font.makefont_ft(tbl, i);
						fwrite(&font.findexs[i].gmBlackBoxX, 0x18, 1, dstfile);
						fwrite(data, 1, font.findexs[i].size, dstfile);
						delete[] data;
					}
					else
					{
						fwrite(&font.findexs[i].gmBlackBoxX, 0x18, 1, dstfile);
						if (font.findexs[i].size != 0)
						{
							data = new BYTE[font.findexs[i].size];
							fseek(font.fp, font.findexs[i].offset + 0x18, SEEK_SET);
							fread(data, 1, font.findexs[i].size, font.fp);
							fwrite(data, 1, font.findexs[i].size, dstfile);
							delete[] data;
						}
					}
				}
				font.findexs[i].offset = pos;
				pos = ftell(dstfile);
			}
			BYTE *buff = new BYTE[0x38];
			fseek(font.fp, -0x38, SEEK_END);
			fread(buff, 1, 0x38, font.fp);
			fwrite(buff, 1, 0x38, dstfile);
			delete[] buff;
			fseek(dstfile, 0, SEEK_SET);
			for (DWORD i = 0; i < font.count; i++)
				fwrite(&font.findexs[i].offset, 1, 4, dstfile);
			fclose(tbl_txt);
			fclose(dstfile);
		}
		else
			cout << "unknown parameter!\n";
	}
	return 0;
}