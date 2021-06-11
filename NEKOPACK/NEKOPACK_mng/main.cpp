/*
用于处理mng文件
made by Destinyの火狐
2021.06.10
*/
#include "NEKOPACK_mng.h"

int main(int argc, char* argv[])
{
	cout << "project：Niflheim-NEKOPACK\n用于处理mng文件。\nby Destinyの火狐 2021.06.10\n\n";
	if (argc != 3 || (strcmp(argv[1], "-e") != 0 && strcmp(argv[1], "-i") != 0 && strcmp(argv[1], "-gm") != 0 && strcmp(argv[1], "-gp") != 0))
	{
		cout << "\nUsage:" << endl;
		cout << "\texport:exe -e file.mng" << endl;
		cout << "\timport:exe -i file.mng" << endl;
		cout << "\tgetmnginfo:exe -gm file.mng" << endl;
		cout << "\tgetpnginfo:exe -gp file.png" << endl;
	}
	else
	{
		if (strcmp(argv[1], "-e") == 0)
		{
			NEKOPACK_mng mng(argv[2], "MNG");
			if (mng.Export())
				cout << "all " << mng.filenum << " files export" << endl;
			else
				cout << "export fail!" << endl;
		}
		else if (strcmp(argv[1], "-i") == 0)
		{
			NEKOPACK_mng mng(argv[2], "MNG");
			if (mng.Import())
				cout << "all " << mng.filenum << " files import" << endl;
			else
				cout << "import fail!" << endl;
		}
		else if (strcmp(argv[1], "-gm") == 0)
		{
			NEKOPACK_mng mng(argv[2], "MNG");
			if (mng.GetChunkInfo("MNG"))
				cout << "all " << mng.filenum << " files get" << endl;
			else
				cout << "get fail!" << endl;
		}
		else if (strcmp(argv[1], "-gp") == 0)
		{
			NEKOPACK_mng mng(argv[2], "PNG");
			if (mng.GetChunkInfo("PNG"))
				cout << "all " << mng.filenum << " files get" << endl;
			else
				cout << "get fail!" << endl;
		}
	}
	system("pause");
	return 0;
}