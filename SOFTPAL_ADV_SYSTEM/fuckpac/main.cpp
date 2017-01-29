#include "pac.h"
#include <fstream>  


int main(int agrc, char* agrv[])
{
	cout << "project：Niflheim-SOFTPAL_ADV_SYSTEM\n用于解包及封包pac；\n用于加密解密文件（仅支持文件首字符为$）。\nby Destinyの火狐 2016.12.21\n";
	if (agrc != 3)
		cout << "\nUsage:\n\texport:\texe -e pacfile\n\tpack:\texe -p oldpacfile\n\tmake:\texe -m folder\n\tdec:\texe -de file\n\tenc:\texe -en file\n";
	else
	{
		if (strcmp(agrv[1],"-e")==0)
		{
			PAC pac(agrv[2]);
			cout << "load index...\n";
			for (DWORD i = 0; i < pac.filenum; i++)
				printf("name:%s offset:0x%X size:0x%X\n", pac.findexs[i].filename, pac.findexs[i].offset, pac.findexs[i].size);
			cout << "\nexport...\n";
			if (pac.pacexport())
				printf("all %d files export\n", pac.filenum);
			else
				cout << "提取失败！\n";
		}
		else if (strcmp(agrv[1],"-p")==0)
		{
			PAC pac(agrv[2]);
			cout << "load old index...\n";
			for (DWORD i = 0; i < pac.filenum; i++)
				printf("name:%s offset:0x%X size:0x%X\n", pac.findexs[i].filename, pac.findexs[i].offset, pac.findexs[i].size);
			cout << "\npack...\n";
			if (pac.pacpack())
				printf("all %d files pack\n", pac.filenum);
			else
				cout << "封包失败！\n";
		}
		else if (strcmp(agrv[1], "-m") == 0)
		{
			PAC pac;
			string filename = agrv[2];
			FILE *in = fopen((filename + ".pac").c_str(), "wb");
			_chdir(agrv[2]);
			if(pac.pacmake(in))
				printf("all %d files pack\n", pac.filenum);
			else
				cout << "封包失败！\n";
		}
		else if (strcmp(agrv[1], "-de") == 0)
		{
			PAC pac;
			ifstream in;
			ofstream out;
			in.open(agrv[2], ios::in | ios::binary | ios::ate);
			DWORD size = in.tellg();
			in.seekg(0, ios::beg);
			char* buff = new char[size];
			in.read(buff, size);
			in.close();
			pac.decrypt((BYTE *)buff, size);
			string outname = agrv[2];
			out.open((outname + ".de").c_str(), ios::out | ios::binary);
			out.write(buff, size);
			delete[] buff;
			out.close();
			cout << "解密完成！\n";
		}
		else if (strcmp(agrv[1], "-en") == 0)
		{
			PAC pac;
			ifstream in;
			ofstream out;
			in.open(agrv[2], ios::in | ios::binary | ios::ate);
			DWORD size = in.tellg();
			in.seekg(0, ios::beg);
			char* buff = new char[size];
			in.read(buff, size);
			in.close();
			pac.encrypt((BYTE *)buff, size);
			string outname = agrv[2];
			out.open((outname + ".en").c_str(), ios::out | ios::binary);
			out.write(buff, size);
			delete[] buff;
			out.close();
			cout << "加密完成！\n";
		}
		else
			cout << "未知参数！\n";
	}
	return 0;
}