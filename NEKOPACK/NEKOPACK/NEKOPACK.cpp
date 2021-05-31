#include "NEKOPACK.h"

NEKOPACK::NEKOPACK(string datname, string dirname, string filename)
{
	if (!dirname.empty())
		AddExtraName(dirname, "DIR");
	if (!filename.empty())
		AddExtraName(filename, "FILE");
	ReadIndex(datname);
}

NEKOPACK::NEKOPACK(string pathname)
{
	strncpy(dat_header.magic, "NEKOPACK", 8);
	dat_header.seed = 0xB92C6C17;//说是seed，但遇到的封包0x17,0x6C这两个都不变，所以不改吧
	dat_header.order = 0xFFFFFFFF;//直接调成最大，最优先读取
	dat_header.parity = 0;
	dat_header.index_length = 0;
	dirname = pathname;
	dir_names.clear();
	file_names.clear();
}

NEKOPACK::~NEKOPACK()
{
	if (src.is_open())
		src.close();
	findexs.clear();
	dir_names.clear();
	file_names.clear();
}

void NEKOPACK::GetKey(DWORD& key0, DWORD& key1, DWORD hash)
{
	DWORD tmp = hash ^ (hash + 0x5D588B65);
	DWORD tmp2 = tmp ^ (hash - 0x359D3E2A);
	key0 = tmp2 ^ (tmp - 0x70E44324);
	key1 = key0 ^ (tmp2 + 0x6C078965);
}

void NEKOPACK::Decode(BYTE* buf, DWORD len, WORD* key)
{
	for (DWORD i = 0; i < len / 2; ++i)
	{
		*(WORD*)buf ^= key[i & 3];
		key[i & 3] += *(WORD*)buf;
		buf += 2;
	}
}

void NEKOPACK::Encode(BYTE* buf, DWORD len, WORD* key)
{
	for (DWORD i = 0; i < len / 2; ++i)
	{
		WORD temp = *(WORD*)buf;
		*(WORD*)buf ^= key[i & 3];
		key[i & 3] += temp;
		buf += 2;
	}
}

DWORD NEKOPACK::GetNameHash(DWORD hash, char* name)
{
	while (1)
	{
		BYTE chr = *name++;
		if (!chr)
			break;
		hash = 81 * (name_table[chr] ^ hash);
	}
	return hash;
}

DWORD NEKOPACK::ParityCheck(DWORD key0, DWORD key1)
{
	DWORD tmp = (key1 ^ ((key0 ^ key1) + 0x5D588B65)) - 0x359D3E2A;
	DWORD v = (key1 ^ ((key1 ^ tmp) - 0x70E44324)) + 0x6C078965;
	return v << (tmp >> 27) | v >> (32 - (tmp >> 27));
}

bool NEKOPACK::ReadIndex(string datname)
{
	dirname = datname.substr(0, datname.find_last_of("."));
	Index_OK = false;
	src.open(datname, ios::in | ios::binary);
	if (!src.is_open())
	{
		cout << "文件打开失败！" << endl;
		system("pause");
		return Index_OK;
	}
	else
	{
		src.read((char*)&dat_header, sizeof(dat_header));
		if (strncmp(dat_header.magic, "NEKOPACK", 8) != 0)
		{
			cout << "文件头不是NEKOPACK！" << endl;
			system("pause");
			return Index_OK;
		}
		else
		{
			printf("filename:%s seed:0x%08X parity:0x%08X index_length:0x%X\n\n", datname.c_str(), dat_header.seed, dat_header.parity, dat_header.index_length);
			BYTE* index = new BYTE[dat_header.index_length];
			src.read((char*)index, dat_header.index_length);
			if (dat_header.parity != 0)
			{
				if (ParityCheck(dat_header.seed, dat_header.index_length) != dat_header.parity)
				{
					printf("Parity检查不一致，文件可能有错误！Parity_Check:0x%X hash:0x%X\n", ParityCheck(dat_header.seed, dat_header.index_length), dat_header.parity);
					return Index_OK;
				}
				DWORD key[2];
				GetKey(key[0], key[1], dat_header.parity);
				Decode(index, dat_header.index_length, (WORD*)key);
			}
			for (BYTE* p = index; p < index + dat_header.index_length; )
			{
				findex_t findex;
				findex.dir_name_hash = *(DWORD*)p;
				findex.filenum = *(DWORD*)(p + 4);
				p += 8;
				for (DWORD i = 0; i < findex.filenum; i++)
				{
					file_t file;
					file.file_name_hash = *(DWORD*)p;
					file.file_length = *(DWORD*)(p + 4);
					p += 8;
					findex.files.push_back(file);
				}
				findexs.push_back(findex);
			}
			delete[] index;
			ofstream datfilename(datname + ".txt", ios::out);
			datfilename << ";file:" + datname << endl;
			for (auto& findex : findexs)
			{
				for (auto dir_name : dir_names)
				{
					if (GetNameHash(dat_header.seed, (char*)dir_name.c_str()) == findex.dir_name_hash)
					{
						findex.name = dir_name;
						datfilename << ";dirname:" + dir_name << endl;
						break;
					}
				}
				if (findex.name.empty())
				{
					char buff[16];
					sprintf(buff, "%08X", findex.dir_name_hash);
					findex.name = string(buff);
				}
				for (auto& file : findex.files)
				{
					for (auto file_name : file_names)
					{
						if (GetNameHash(dat_header.seed, (char*)file_name.c_str()) == file.file_name_hash)
						{
							file.name = file_name;
							datfilename << file_name << endl;
							break;
						}
					}
					if (file.name.empty())
					{
						char buff[16];
						sprintf(buff, "%08X", file.file_name_hash);
						file.name = string(buff);
					}
				}
			}
			datfilename.close();
		}
		Index_OK = true;
		return Index_OK;
	}
}

bool NEKOPACK::Export()
{
	if (Index_OK)
		_mkdir(dirname.c_str());
	else
	{
		cout << "读取文件列表失败！" << endl;
		return false;
	}
	_chdir(dirname.c_str());
	for (auto& findex : findexs)
	{
		printf("dir_name:%s dir_name_hash:0x%08X filenum:%d\n", findex.name.c_str(), findex.dir_name_hash, findex.filenum);
		findex.name += "/";
		for (char* tag = (char*)findex.name.c_str(); *tag; tag++)
		{
			if (*tag == '/')
			{
				char buf[1000], path[1000];
				strcpy(buf, findex.name.c_str());
				buf[strlen(findex.name.c_str()) - strlen(tag) + 1] = NULL;
				strcpy(path, buf);
				if (_access(path, 0) == -1)
					_mkdir(path);
			}
		}
		for (auto& file : findex.files)
		{
			src.read((char*)&file.parity, 4);
			src.read((char*)&file.length, 4);
			printf("\tfile_name:%s file_name_hash:0x%08X file_parity:0x%08X file_length:0x%X file_offset:0x%X\n", file.name.c_str(), file.file_name_hash, file.parity, file.length, (DWORD)src.tellg() - 8);
			if (file.length != file.file_length)
			{
				printf("两处file_length大小不一致，文件可能有错误！file_length:0x%X length:0x%X\n", file.file_length, file.length);
				return false;
			}
			//针对单字节对齐的文件，避免末字节没有被正确解密
			BYTE* data = new BYTE[file.length + 1];
			src.read((char*)data, file.length);
			if (file.parity != 0)
			{
				if (ParityCheck(dat_header.seed, file.length) != file.parity)
				{
					printf("Parity检查不一致，文件可能有错误！Parity_Check:0x%X hash:0x%X\n", ParityCheck(dat_header.seed, file.length), file.parity);
					return false;
				}
				DWORD key[2];
				GetKey(key[0], key[1], file.parity);
				Decode(data, file.length + 1, (WORD*)key);
			}
			ofstream dst(findex.name + file.name, ios::out | ios::binary);
			dst.write((char*)data, file.length);
			dst.close();
			delete[] data;
			filenum++;
		}
	}
	return true;
}

bool NEKOPACK::Import()
{
	if (!Index_OK)
	{
		cout << "读取文件列表失败！" << endl;
		return false;
	}
	ofstream dst(dirname + ".new", ios::out | ios::binary);
	dst.write((char*)&dat_header, sizeof(dat_header));
	_chdir(dirname.c_str());
	for (auto& findex : findexs)
	{
		for (auto& file : findex.files)
		{
			src.read((char*)&file.parity, 4);
			src.read((char*)&file.length, 4);
			if (file.length != file.file_length)
			{
				printf("两处file_length大小不一致，文件可能有错误！dirname:%s filename:%s file_length:0x%X length:0x%X\n", findex.name.c_str(), file.name.c_str(), file.file_length, file.length);
				return false;
			}
			src.seekg(file.length, ios::cur);
		}
	}
	dst.seekp(dat_header.index_length, ios::cur);
	for (auto& findex : findexs)
	{
		printf("dir_name:%s dir_name_hash:0x%08X filenum:%d\n", findex.name.c_str(), findex.dir_name_hash, findex.filenum);
		for (auto& file : findex.files)
		{
			ifstream member(findex.name + '/' + file.name, ios::in | ios::binary | ios::ate);
			if (!member.is_open())
			{
				cout << findex.name + '/' + file.name << "打开失败！" << endl;
				return false;
			}
			else
			{
				file.file_length = (DWORD)member.tellg();
				file.length = file.file_length;
				member.seekg(0, ios::beg);
				BYTE* data = new BYTE[file.length + 1 + 8];
				*(DWORD*)data = file.parity;
				*(DWORD*)(data + 4) = file.length;
				member.read((char*)(data + 8), file.length);
				if (file.parity != 0)
				{
					file.parity = ParityCheck(dat_header.seed, file.length);
					*(DWORD*)data = file.parity;
					DWORD key[2];
					GetKey(key[0], key[1], file.parity);
					Encode(data + 8, file.length + 1, (WORD*)key);
				}
				printf("\tfile_name:%s file_name_hash:0x%08X file_parity:0x%08X file_length:0x%X file_offset:0x%X\n", file.name.c_str(), file.file_name_hash, file.parity, file.length, (DWORD)dst.tellp());
				dst.write((char*)data, file.length + 8);
				delete[] data;
				member.close();
				filenum++;
			}
		}
	}
	dst.seekp(sizeof(dat_header), ios::beg);
	BYTE* index = new BYTE[dat_header.index_length];
	BYTE* p = index;
	for (auto& findex : findexs)
	{
		*(DWORD*)p = findex.dir_name_hash;
		*(DWORD*)(p + 4) = findex.filenum;
		p += 8;
		for (auto& file : findex.files)
		{
			*(DWORD*)p = file.file_name_hash;
			*(DWORD*)(p + 4) = file.file_length;
			p += 8;
		}
	}
	DWORD key[2];
	GetKey(key[0], key[1], dat_header.parity);
	Encode(index, dat_header.index_length, (WORD*)key);
	dst.write((char*)index, dat_header.index_length);
	delete[] index;
	dst.close();
	return true;
}

void NEKOPACK::AddExtraName(string txt, string type)
{
	//经测试，用这引擎的厂商使用的filename全是ASCII，没有哪个厂商使用宽字节的样子，所以就不做宽字节处理了
	ifstream txtfile(txt, ios::in);
	if (!txtfile.is_open())
	{
		cout << txt << "打开失败！" << endl;
		system("pause");
	}
	else
	{
		string name;
		if (type == "DIR")
		{
			while (getline(txtfile, name))
			{
				if (name[0] == ';')//注释功能
					continue;
				dir_names.push_back(string(name));
			}
		}
		else if (type == "FILE")
		{
			while (getline(txtfile, name))
			{
				if (name[0] == ';')//注释功能
					continue;
				file_names.push_back(string(name));
			}
		}
		txtfile.close();
	}
}

void NEKOPACK::GetFiles(string name, DWORD indexsize)
{
	long Handle;
	struct _finddata_t FileInfo;
	string p;
	if ((Handle = _findfirst(p.assign(name).append("/*").c_str(), &FileInfo)) != -1)
	{
		do
		{
			if ((FileInfo.attrib & _A_SUBDIR))
			{
				if (strcmp(FileInfo.name, ".") != 0 && strcmp(FileInfo.name, "..") != 0)
				{
					findex_t findex;
					findex.name = p.assign(name).append("/").append(FileInfo.name);
					findex.name = findex.name.substr(dirname.length() + 1, findex.name.length() - dirname.length() - 1);
					findex.filenum = 0;
					findex.dir_name_hash = 0;
					findexs.push_back(findex);
					GetFiles(p.assign(name).append("/").append(FileInfo.name), findexs.size() - 1);
				}
			}
			else
			{
				file_t file;
				file.name = FileInfo.name;
				file.file_name_hash = 0;
				findexs[indexsize].files.push_back(file);
				findexs[indexsize].filenum++;
			}
		} while (_findnext(Handle, &FileInfo) == 0);
		_findclose(Handle);
	}
}

void NEKOPACK::FileSort()
{
	for (auto& findex : findexs)
	{
		string::size_type sz;
		try
		{
			//如果执行成功，则代表目录名是十六进制数，会使用到十六进制数而不使用正常目录名的情况那还用说么
			findex.dir_name_hash = stoi(findex.name, &sz, 16);
		}
		catch (const std::invalid_argument&)
		{
			//如果发生错误，则代表目录名是正常目录名，需要计算dir_name_hash
			findex.dir_name_hash = GetNameHash(dat_header.seed, (char*)findex.name.c_str());
		}
		//但是，如果目录名是count这种情况的话，因为stoi的机制问题会把首字母c转换成功，然后整个stoi就算执行成功，
		//所以这里判断下sz，如果如果sz不等于name的长度就代表未完全转换成功，则代表目录名是正常目录名
		if (sz != findex.name.length())
			findex.dir_name_hash = GetNameHash(dat_header.seed, (char*)findex.name.c_str());
		for (auto& file : findex.files)
		{
			//本工具对未匹配成功的文件导出时是直接输出hash值做文件名的，所以就直接判断了
			//用了其他会添加后缀名的工具导出的话，请先去除后缀
			if (file.name.rfind('.') == string::npos)
				file.file_name_hash = stoi(file.name, NULL, 16);
			else
				file.file_name_hash = GetNameHash(dat_header.seed, (char*)file.name.c_str());
		}
	}
	vector<findex_t> temp(findexs);
	map<DWORD, string> hash_map;
	DWORD* name_hash;
	DWORD i = 0;
	//这引擎的目录名和文件名都是按hash值从小到大排列的，
	//这样游戏直接折半查找会减少索引时间，所以封包也要先排好序
	if (findexs.size() > 1)
	{
		findexs.clear();
		name_hash = new DWORD[temp.size()];
		for (auto& findex : temp)
			name_hash[i++] = findex.dir_name_hash;
		sort(name_hash, name_hash + temp.size());
		for (i = 0; i < temp.size(); i++)
		{
			for (auto& findex : temp)
			{
				if (findex.dir_name_hash == name_hash[i])
					findexs.push_back(findex);
			}
		}
		delete[] name_hash;
	}
	temp.clear();
	for (auto& findex : findexs)
	{
		if (findex.filenum > 1)
		{
			name_hash = new DWORD[findex.filenum];
			i = 0;
			for (auto& file : findex.files)
			{
				name_hash[i++] = file.file_name_hash;
				hash_map[file.file_name_hash] = file.name;
			}
			sort(name_hash, name_hash + findex.filenum);
			findex.files.clear();
			for (i = 0; i < findex.filenum; i++)
			{
				file_t file;
				file.file_name_hash = name_hash[i];
				file.name = hash_map[name_hash[i]];
				findex.files.push_back(file);
			}
			delete[] name_hash;
			hash_map.clear();
		}
	}
}

bool NEKOPACK::Make()
{
	GetFiles(dirname, findexs.size());
	for (DWORD i = 0; i < findexs.size(); i++)
		if (findexs[i].filenum == 0)//删除没有文件（可以有子目录）的目录
			findexs.erase(findexs.begin() + i);//删除后size()自动减一，所以这里i不用特别再减一
	if (findexs.size() == 0)
	{
		cout << "目录下无任何文件！" << endl;
		return false;
	}
	for (auto findex : findexs)
		dat_header.index_length += findex.filenum * 8 + 8;
	dat_header.parity = ParityCheck(dat_header.seed, dat_header.index_length);
	ofstream dst(dirname + ".new", ios::out | ios::binary);
	dst.write((char*)&dat_header, sizeof(dat_header));
	dst.seekp(dat_header.index_length, ios::cur);
	FileSort();
	for (auto& findex : findexs)
	{
		printf("dir_name:%s dir_name_hash:0x%08X filenum:%d\n", findex.name.c_str(), findex.dir_name_hash, findex.filenum);
		for (auto& file : findex.files)
		{
			ifstream member(dirname + '/' + findex.name + '/' + file.name, ios::in | ios::binary | ios::ate);
			if (!member.is_open())
			{
				cout << dirname + '/' + findex.name + '/' + file.name << "打开失败！" << endl;
				return false;
			}
			else
			{
				file.file_length = (DWORD)member.tellg();
				file.length = file.file_length;
				member.seekg(0, ios::beg);
				BYTE* data = new BYTE[file.length + 1 + 8];
				*(DWORD*)data = file.parity;
				*(DWORD*)(data + 4) = file.length;
				member.read((char*)(data + 8), file.length);
				file.parity = ParityCheck(dat_header.seed, file.length);
				*(DWORD*)data = file.parity;
				DWORD key[2];
				GetKey(key[0], key[1], file.parity);
				Encode(data + 8, file.length + 1, (WORD*)key);
				printf("\tfile_name:%s file_name_hash:0x%08X file_parity:0x%08X file_length:0x%X file_offset:0x%X\n", file.name.c_str(), file.file_name_hash, file.parity, file.length, (DWORD)dst.tellp());
				dst.write((char*)data, file.length + 8);
				delete[] data;
				member.close();
				filenum++;
			}
		}
	}
	dst.seekp(sizeof(dat_header), ios::beg);
	BYTE* index = new BYTE[dat_header.index_length];
	BYTE* p = index;
	for (auto& findex : findexs)
	{
		*(DWORD*)p = findex.dir_name_hash;
		*(DWORD*)(p + 4) = findex.filenum;
		p += 8;
		for (auto& file : findex.files)
		{
			*(DWORD*)p = file.file_name_hash;
			*(DWORD*)(p + 4) = file.file_length;
			p += 8;
		}
	}
	DWORD key[2];
	GetKey(key[0], key[1], dat_header.parity);
	Encode(index, dat_header.index_length, (WORD*)key);
	dst.write((char*)index, dat_header.index_length);
	delete[] index;
	dst.close();
	return true;
}

vector<string> NEKOPACK::dir_names =
{
	"image/actor", "image/back", "image/mask", "image/visual", "image/actor/big",
	"image/face", "image/actor/b", "image/actor/bb", "image/actor/s", "image/actor/ss",
	"sound/bgm", "sound/env", "sound/se", "voice", "script", "system", "count"
};

vector<string> NEKOPACK::file_names =
{
	"title.txt","story.txt","start.txt","setup.txt"
};

BYTE NEKOPACK::name_table[] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x38, 0x2F, 0x33, 0x3C, 0x40, 0x3B, 0x2A, 0x2E, 0x31, 0x30, 0x26, 0x44, 0x35, 0x28, 0x3E, 0x12,
	0x02, 0x22, 0x06, 0x20, 0x1A, 0x1C, 0x0F, 0x11, 0x18, 0x17, 0x42, 0x2B, 0x3A, 0x37, 0x34, 0x0C,
	0x41, 0x08, 0x1D, 0x07, 0x15, 0x21, 0x05, 0x1E, 0x0A, 0x14, 0x0E, 0x10, 0x09, 0x27, 0x1F, 0x0B,
	0x23, 0x16, 0x0D, 0x01, 0x25, 0x04, 0x1B, 0x03, 0x13, 0x24, 0x19, 0x2D, 0x12, 0x29, 0x32, 0x3F,
	0x3D, 0x08, 0x1D, 0x07, 0x15, 0x21, 0x05, 0x1E, 0x0A, 0x14, 0x0E, 0x10, 0x09, 0x27, 0x1F, 0x0B,
	0x23, 0x16, 0x0D, 0x01, 0x25, 0x04, 0x1B, 0x03, 0x13, 0x24, 0x19, 0x2C, 0x39, 0x43, 0x36, 0x00,
	0x4B, 0xA9, 0xA7, 0xAF, 0x50, 0x52, 0x91, 0x9F, 0x47, 0x6B, 0x96, 0xAB, 0x87, 0xB5, 0x9B, 0xBB,
	0x99, 0xA4, 0xBF, 0x5C, 0xC6, 0x9C, 0xC2, 0xC4, 0xB6, 0x4F, 0xB8, 0xC1, 0x85, 0xA8, 0x51, 0x7E,
	0x5F, 0x82, 0x73, 0xC7, 0x90, 0x4E, 0x45, 0xA5, 0x7A, 0x63, 0x70, 0xB3, 0x79, 0x83, 0x60, 0x55,
	0x5B, 0x5E, 0x68, 0xBA, 0x53, 0xA1, 0x67, 0x97, 0xAC, 0x71, 0x81, 0x59, 0x64, 0x7C, 0x9D, 0xBD,
	0x9D, 0xBD, 0x95, 0xA0, 0xB2, 0xC0, 0x6F, 0x6A, 0x54, 0xB9, 0x6D, 0x88, 0x77, 0x48, 0x5D, 0x72,
	0x49, 0x93, 0x57, 0x65, 0xBE, 0x4A, 0x80, 0xA2, 0x5A, 0x98, 0xA6, 0x62, 0x7F, 0x84, 0x75, 0xBC,
	0xAD, 0xB1, 0x6E, 0x76, 0x8B, 0x9E, 0x8C, 0x61, 0x69, 0x8D, 0xB4, 0x78, 0xAA, 0xAE, 0x8F, 0xC3,
	0x58, 0xC5, 0x74, 0xB7, 0x8E, 0x7D, 0x89, 0x8A, 0x56, 0x4D, 0x86, 0x94, 0x9A, 0x4C, 0x92, 0xB0,
};