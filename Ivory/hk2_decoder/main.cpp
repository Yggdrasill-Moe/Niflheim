#include "AGS.h"

int main(int agrc, char* agrv[])
{
	cout << "project：Niflheim-Ivory\n用于解密hk2和dat文件。\nby Destinyの火狐 2018.04.27\n";
	if (agrc != 2)
		cout << "\nUsage:hk2_decoder hk2file\n";
	else
	{
		AGS AGS(agrv[1]);
		AGS.AGS_decode();
		cout << "完成!\n";
	}
}