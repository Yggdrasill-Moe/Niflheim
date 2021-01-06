#include "ft_make.h"

int main(int agrc, char* agrv[])
{
	setlocale(LC_ALL, "chs");
	wprintf(L"project：Niflheim-BALDR HEART\n用于生成fnt用的png。\n将码表txt文件拖到程序上。\nby Destinyの火狐 2017.01.19\n\n");
	if (agrc != 3)
		wprintf(L"Usage:fnt_make_ft txtfile fnttype\n      fnt_make_ft tbl_chs.txt 12ss\n");
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
		DWORD slen, k = 0, i = 1577, height = 0, width = 0, gradient = 0, interval = 0, fill = 0;
		char dstname[200], buff[256];
		wchar_t tbl, data[256], *find;
		FILE *tbl_xy = fopen("tbl_xy.txt", "wt,ccs=UNICODE");
		FILE *tbl_cell = fopen("tbl_cell.txt", "wt,ccs=UNICODE");
		FILE *tbl_txt = fopen(agrv[1], "rt,ccs=UNICODE");
		FILE *pngfile;
		_mkdir("tbl_fnt");
		GetPrivateProfileStringA(agrv[2], "Font", "SourceHanSansCN-Medium.otf", buff, 100, iniPath);
		height = GetPrivateProfileIntA(agrv[2], "Height", 0, iniPath);
		width = GetPrivateProfileIntA(agrv[2], "Width", 0, iniPath);
		gradient = GetPrivateProfileIntA(agrv[2], "Gradient", 0, iniPath);
		interval = GetPrivateProfileIntA(agrv[2], "Interval", 0, iniPath);
		fill = GetPrivateProfileIntA(agrv[2], "Fill", 0, iniPath);
		if (width == 0)
			width = height;
		FT_Make ft(buff, height, width);
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
				fwprintf(tbl_xy, L"%d %d\n", width / 2, height / 2);
				fwprintf(tbl_cell, L"%d\n", width);
				continue;
			}
			cb = ft.GetCharBitmap(tbl);
			sprintf(dstname, "%08d.png", i);
			pngfile = fopen(dstname, "wb");
			WritePng(pngfile, cb.bmp_width, cb.bmp_height, interval, gradient, fill, cb.bmpBuffer);
			wprintf(L"ch:%lc size:%d width:%d height:%d x:%d y:%d cell:%d\n", tbl, (cb.bmp_width + fill * 2) * (cb.bmp_height + fill * 2), cb.bmp_width + fill * 2, cb.bmp_height + fill * 2, cb.bearingX + GetPrivateProfileIntA(agrv[2], "X_mod", 0, iniPath), height - cb.bearingY + GetPrivateProfileIntA(agrv[2], "Y_fix", 0, iniPath), cb.Advance + fill * 2);
			fwprintf(tbl_xy, L"%d %d\n", cb.bearingX + GetPrivateProfileIntA(agrv[2], "X_mod", 0, iniPath), height - cb.bearingY + GetPrivateProfileIntA(agrv[2], "Y_fix", 0, iniPath));
			fwprintf(tbl_cell, L"%d\n", cb.Advance + fill * 2);
			i++;
			fclose(pngfile);
		}
		fclose(tbl_txt);
		fclose(tbl_xy);
		fclose(tbl_cell);
#ifdef DEBUG
		system("pause");
#endif // DEBUG
		return 0;
	}
}