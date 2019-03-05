#include "pgd.h"

int main(int agrc, char* agrv[])
{
	cout << "project：Niflheim-SOFTPAL_ADV_SYSTEM\n用于将PGD文件导出成PNG，暂时支持32或24位且压缩类型为2或3的文件。\nby Destinyの火狐 2016.11.13\n";
	if (agrc != 2)
		cout << "\nUsage:pgd2png pgdfile\n";
	else
	{
		PGD pgd(agrv[1]);
		if (pgd.pgd_uncompress())
			cout << "完成!\n";
	}
	return 0;
}