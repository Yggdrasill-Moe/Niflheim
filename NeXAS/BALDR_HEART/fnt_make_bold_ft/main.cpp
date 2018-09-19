#include "ft_make.h"

int main(int agrc, char* agrv[])
{
	setlocale(LC_ALL, "chs");
	wprintf(L"project：Niflheim-BALDR HEART\n用于生成描边fnt用的png。\n将码表txt文件拖到程序上。\nby Destinyの火狐 2017.01.19\n\n");
	if (agrc != 3)
		wprintf(L"Usage:fnt_make_bold_ft txtfile fnttype\n      fnt_make_bold_ft tbl_chs.txt 12ss\n");
	else
	{
		char dirPath[MAX_PATH];
		char iniPath[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, dirPath);
		wsprintfA(iniPath, "%s\\%s", dirPath, "fnt_make.ini");
		if (_access(iniPath, 4) == -1)
		{
			wprintf(L"fnt_make.ini文件不存在！");
			exit(0);
		}
		DWORD slen, k = 0, i = 1577, height = 0;
		char dstname[200], buff[256];
		wchar_t tbl, data[256], *find;
		FILE *tbl_xy = fopen("tbl_xy.txt", "wt,ccs=UNICODE");
		FILE *tbl_cell = fopen("tbl_cell.txt", "wt,ccs=UNICODE");
		FILE *tbl_txt = fopen(agrv[1], "rt,ccs=UNICODE");
		FILE *pngfile;
		_mkdir("tbl_fnt");
		GetPrivateProfileStringA(agrv[2], "Font", "SourceHanSansCN-Medium.otf", buff, 100, iniPath);
		height = GetPrivateProfileIntA(agrv[2], "Height", 0, iniPath);
		FT_Make ft(buff, height);
		_chdir("tbl_fnt");
		CharBitmap cb;
		while (fgetws(data, 256, tbl_txt) != NULL)
		{
			slen = wcslen(data);
			find = wcschr(data, L'=');
			tbl = data[find - data + 1];
			if (tbl == 0x0A)
			{
				i++;
				fwprintf(tbl_xy, L"%d %d\n", height / 2, height / 2);
				fwprintf(tbl_cell, L"%d\n", height);
				continue;
			}
			cb = ft.GetCharBitmap(tbl);
			sprintf(dstname, "%08d.png", i);
			pngfile = fopen(dstname, "wb");
			WritePng(pngfile, cb.bmp_width, cb.bmp_height, cb.bmpBuffer);
			wprintf(L"ch:%lc size:%d width:%d height:%d x:%d y:%d cell:%d\n", tbl, (cb.bmp_width + 2) * (cb.bmp_height + 2), cb.bmp_width + 2, cb.bmp_height + 2, cb.bearingX + GetPrivateProfileIntA(agrv[2], "X_mod", 0, iniPath), height - cb.bearingY + GetPrivateProfileIntA(agrv[2], "Y_fix", 0, iniPath), cb.Advance + 2);
			fwprintf(tbl_xy, L"%d %d\n", cb.bearingX + GetPrivateProfileIntA(agrv[2], "X_mod", 0, iniPath), height - cb.bearingY + GetPrivateProfileIntA(agrv[2], "Y_fix", 0, iniPath));
			fwprintf(tbl_cell, L"%d\n", cb.Advance + 2);
			i++;
			fclose(pngfile);
		}
		fclose(tbl_txt);
		fclose(tbl_xy);
		fclose(tbl_cell);
		system("pause");
		return 0;
	}
}