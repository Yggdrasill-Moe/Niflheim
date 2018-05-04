#include "sg.h"

int main(int agrc, char* agrv[])
{
	cout << "project：Niflheim-Ivory\n用于将sg文件导出成PNG。\nby Destinyの火狐 2018.04.26\n";
	if (agrc != 2)
		cout << "\nUsage:sg2png sgfile\n";
	else
	{
		sg sg(agrv[1]);
		sg.sg_decode();
		cout << "完成!\n";
	}
}