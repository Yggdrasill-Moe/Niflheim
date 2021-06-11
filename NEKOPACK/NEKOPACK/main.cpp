/*
用于处理文件头为NEKOPACK的dat文件
made by Destinyの火狐
2021.05.31
*/
#include "NEKOPACK.h"

int main(int argc, char* argv[])
{
	cout << "project：Niflheim-NEKOPACK\n用于处理文件头为NEKOPACK的dat文件。\nby Destinyの火狐 2021.05.31\n\n";
	if (argc < 3)
	{
		cout << "\nUsage:" << endl;
		cout << "\tUse built-in dirnames and filenames for export:" << endl;
		cout << "\t\texe -e datfile" << endl;
		cout << "\tAdd extra dirnames and built-in filenames for export:" << endl;
		cout << "\t\texe -ed datfile dirnametxt" << endl;
		cout << "\tAdd extra filenames and built-in dirnames for export:" << endl;
		cout << "\t\texe -ef datfile filenametxt" << endl;
		cout << "\tAdd extra filenames and extra dirnames for export:" << endl;
		cout << "\t\texe -edf datfile dirnametxt filenametxt" << endl;

		cout << "\tUse built-in dirnames and filenames for import:" << endl;
		cout << "\t\texe -i datfile" << endl;
		cout << "\tAdd extra dirnames and built-in filenames for import:" << endl;
		cout << "\t\texe -id datfile dirnametxt" << endl;
		cout << "\tAdd extra filenames and built-in dirnames for import:" << endl;
		cout << "\t\texe -if datfile filenametxt" << endl;
		cout << "\tAdd extra filenames and extra dirnames for import:" << endl;
		cout << "\t\texe -idf datfile dirnametxt filenametxt" << endl;

		cout << "\tMake a NEKOPACK dat:" << endl;
		cout << "\t\texe -m folder" << endl;
	}
	else
	{
		if (strncmp(argv[1], "-e", 2) == 0)
		{
			if (strcmp(argv[1], "-e") == 0)
			{
				NEKOPACK nekopack(argv[2], "", "");
				if (nekopack.Export())
					cout << "all " << nekopack.filenum << " files export" << endl;
				else
					cout << "export fail!" << endl;
			}
			else if (strcmp(argv[1], "-ed") == 0)
			{
				NEKOPACK nekopack(argv[2], argv[3], "");
				if (nekopack.Export())
					cout << "all " << nekopack.filenum << " files export" << endl;
				else
					cout << "export fail!" << endl;
			}
			else if (strcmp(argv[1], "-ef") == 0)
			{
				NEKOPACK nekopack(argv[2], "", argv[3]);
				if (nekopack.Export())
					cout << "all " << nekopack.filenum << " files export" << endl;
				else
					cout << "export fail!" << endl;
			}
			else if (strcmp(argv[1], "-edf") == 0)
			{
				NEKOPACK nekopack(argv[2], argv[3], argv[4]);
				if (nekopack.Export())
					cout << "all " << nekopack.filenum << " files export" << endl;
				else
					cout << "export fail!" << endl;
			}
			else
				cout << "unknow argument!" << endl;
		}
		else if (strncmp(argv[1], "-i", 2) == 0)
		{
			if (strcmp(argv[1], "-i") == 0)
			{
				NEKOPACK nekopack(argv[2], "", "");
				if (nekopack.Import())
					cout << "all " << nekopack.filenum << " files import" << endl;
				else
					cout << "import fail!" << endl;
			}
			else if (strcmp(argv[1], "-id") == 0)
			{
				NEKOPACK nekopack(argv[2], argv[3], "");
				if (nekopack.Import())
					cout << "all " << nekopack.filenum << " files import" << endl;
				else
					cout << "import fail!" << endl;
			}
			else if (strcmp(argv[1], "-if") == 0)
			{
				NEKOPACK nekopack(argv[2], "", argv[3]);
				if (nekopack.Import())
					cout << "all " << nekopack.filenum << " files import" << endl;
				else
					cout << "import fail!" << endl;
			}
			else if (strcmp(argv[1], "-idf") == 0)
			{
				NEKOPACK nekopack(argv[2], argv[3], argv[4]);
				if (nekopack.Import())
					cout << "all " << nekopack.filenum << " files import" << endl;
				else
					cout << "import fail!" << endl;
			}
			else
				cout << "unknow argument!" << endl;
		}
		else if (strcmp(argv[1], "-m") == 0)
		{
			if (string(argv[2]).rfind('/') != string::npos || string(argv[2]).rfind('\\') != string::npos)
				cout << "don't include \"/\" or \"\\\" in folder name!" << endl << "make fail!" << endl;
			else
			{
				if (_access(argv[2], 0) == -1)
					cout << "folder doesn't exist!" << endl << "make fail!" << endl;
				else
				{
					NEKOPACK nekopack(argv[2]);
					if (nekopack.Make())
						cout << "all " << nekopack.filenum << " files import" << endl;
					else
						cout << "make fail!" << endl;
				}
			}
		}
		else
			cout << "unknow argument!" << endl;
	}
	system("pause");
	return 0;
}