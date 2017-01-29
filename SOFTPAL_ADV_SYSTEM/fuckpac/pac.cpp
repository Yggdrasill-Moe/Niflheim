#include "pac.h"

PAC::PAC()
{
}

PAC::PAC(string pacname)
{
	PAC::ReadIndex(pacname);
}

bool PAC::ReadIndex(string pacname)
{
	dirname = pacname.substr(0, pacname.find_last_of("."));

	fp = fopen(pacname.c_str(), "rb");
	if (fp)
		fread(&header, 1, sizeof(header), fp);
	else
	{
		cout << "文件不存在";
		Index_OK = false;
		return Index_OK;
	}
	filenum = header.filenum;
	fseek(fp, 0x804, SEEK_SET);
	findex_t findex;
	for (DWORD i = 0; i < filenum; i++)
	{
		fread(&findex, 1, sizeof(findex), fp);
		findexs.push_back(findex);
	}
	fseek(fp, -8, SEEK_END);
	fread(&uk_eof, 1, sizeof(uk_eof), fp);
	Index_OK = true;
	return Index_OK;
}

BYTE PAC::rol(int val, int n)
{
	n %= 8;
	return (val << n) | (val >> (8 - n));
}

BYTE PAC::ror(int val, int n)
{
	n %= 8;
	return (val >> n) | (val << (8 - n));
}

void PAC::decrypt(BYTE *data, DWORD size)
{
	if (data[0] != 0x24)//$
		return;

	DWORD count = (size - 0x10) / 4;
	BYTE *p = data + 0x10;

	DWORD key1 = 0x084DF873;
	DWORD key2 = 0xFF987DEE;
	DWORD c = 0x04;

	for (DWORD i = 0; i < count; i++)
	{
		*p = rol(*p, c++);
		c &= 0xFF;
		DWORD *dp = (DWORD *)p;
		*dp ^= key1;
		*dp ^= key2;
		p = (BYTE*)dp + 4;

	}
}

void PAC::encrypt(BYTE *data, DWORD size)
{
	if (data[0] != 0x24)
		return;

	DWORD count = (size - 0x10) / 4;
	BYTE *p = data + 0x10;

	DWORD key1 = 0x084DF873;
	DWORD key2 = 0xFF987DEE;
	DWORD c = 0x04;

	for (DWORD i = 0; i < count; i++)
	{
		DWORD *dp = (DWORD *)p;
		*dp ^= key2;
		*dp ^= key1;
		p = (BYTE*)dp;
		*p = ror(*p, c++);
		c &= 0xFF;
		p += 4;
	}
}

bool PAC::pacexport()
{
	if (Index_OK)
		_mkdir(dirname.c_str());
	else
	{
		cout << "读取文件列表失败";
		return false;
	}
	for (DWORD i = 0; i<filenum; i++)
	{
		string name(findexs[i].filename);
		string fullname = dirname + "\\" + name;
		FILE* fout = fopen(fullname.c_str(), "wb");

		DWORD filesize = findexs[i].size;
		BYTE* buff = new BYTE[filesize];
		fseek(fp, findexs[i].offset, SEEK_SET);
		fread(buff, 1, filesize, fp);

		decrypt(buff, filesize);

		if (fout)
			fwrite(buff, 1, filesize, fout);
		delete[] buff;
		fclose(fout);
	}
	return true;
}

bool PAC::pacpack()
{
	if (!Index_OK)
	{
		cout << "读取文件列表失败";
		return false;
	}
	FILE* bp = fopen((dirname + ".pac_new").c_str(), "wb");
	DWORD foffset = findexs[0].offset;
	BYTE* data = new BYTE[0x804];
	fseek(fp, 0, SEEK_SET);
	fread(data, 1, 0x804, fp);
	fwrite(data, 1, 0x804, bp);
	delete[] data;
	fseek(bp, foffset, SEEK_SET);
	for (DWORD i = 0; i < filenum; i++)
	{
		string name(findexs[i].filename);
		string fullname = dirname + "\\" + name;
		FILE* fin = fopen(fullname.c_str(), "rb");
		fseek(fin, 0, SEEK_END);
		findexs[i].size = ftell(fin);
		findexs[i].offset = ftell(bp);
		fseek(fin, 0, SEEK_SET);
		data = new BYTE[findexs[i].size];
		fread(data, 1, findexs[i].size, fin);

		encrypt(data, findexs[i].size);

		if (fin)
			fwrite(data, 1, findexs[i].size, bp);
		fclose(fin);
		delete[] data;
	}
	fseek(bp, 0x804, SEEK_SET);
	for (DWORD i = 0; i < filenum; i++)
	{
		fwrite(&findexs[i].filename, 1, 32, bp);
		fwrite(&findexs[i].size, 1, 4, bp);
		fwrite(&findexs[i].offset, 1, 4, bp);
	}
	fseek(bp, 0, SEEK_END);
	fwrite(&uk_eof, 1, sizeof(uk_eof), bp);
	fclose(bp);
	return true;
}

bool PAC::pacmake(FILE* in)
{
	if (!in)
	{
		cout << "新建的文件不存在";
		return false;
	}
	long Handle;
	struct _finddata64i32_t FileInfo;
	filenum = 0;
	if ((Handle = _findfirst("*.*", &FileInfo)) == -1L)
	{
		printf("没有找到匹配的项目。\n");
		system("pause");
		return false;
	}
	do
	{
		if (FileInfo.name[0] == '.')  //过滤本级目录和父目录
			continue;
		findex_t findex;
		memset(&findex, 0, sizeof(findex_t));
		sprintf(findex.filename, FileInfo.name);
		findex.size = FileInfo.size;
		findexs.push_back(findex);
		filenum++;
	} while (_findnext(Handle, &FileInfo) == 0);
	DWORD buff = 0x20434150;
	fwrite(&buff, 4, 1, in);
	fseek(in, 8, SEEK_SET);
	fwrite(&filenum, 1, 4, in);
	fseek(in, 0x804 + filenum * 40, SEEK_SET);
	for (DWORD i = 0; i < filenum; i++)
	{
		FILE *infile = fopen(findexs[i].filename, "rb");
		BYTE* data = new BYTE[findexs[i].size];
		fread(data, 1, findexs[i].size, infile);
		fclose(infile);
		findexs[i].offset = ftell(in);
		encrypt(data, findexs[i].size);
		fwrite(data, 1, findexs[i].size, in);
		delete[] data;
		printf("%s size:0x%X offset:0x%X\n", findexs[i].filename, findexs[i].size, findexs[i].offset);
	}
	fseek(in, 0x804, SEEK_SET);
	for (DWORD i = 0; i < filenum; i++)
	{
		fwrite(&findexs[i].filename, 1, 32, in);
		fwrite(&findexs[i].size, 1, 4, in);
		fwrite(&findexs[i].offset, 1, 4, in);
	}
	fseek(in, 4, SEEK_END);
	buff = 0x20464F45;
	fwrite(&buff, 1, 4, in);
	fclose(in);
	return true;
}

PAC::~PAC()
{
	if(fp)
		fclose(fp);
	findexs.clear();
}