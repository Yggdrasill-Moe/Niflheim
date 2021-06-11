#include "NEKOPACK_mng.h"

NEKOPACK_mng::NEKOPACK_mng(string pathname, string mode)
{
	dirname = pathname.substr(0, pathname.find_last_of("."));
	CheckHeader(pathname, mode);
}

NEKOPACK_mng::~NEKOPACK_mng()
{
	if (src.is_open())
		src.close();
	if (info != NULL)
		fclose(info);
}

DWORD NEKOPACK_mng::BE2LE(DWORD bbuff)
{
	char* p = (char*)&bbuff;
	DWORD lbuff = p[3] & 0xFF;
	lbuff |= (p[2] << 8 & 0xFF00);
	lbuff |= ((p[1] << 16) & 0xFF0000);
	lbuff |= ((p[0] << 24) & 0xFF000000);
	return lbuff;
}

USHORT NEKOPACK_mng::BE2LE(USHORT bbuff)
{
	char* p = (char*)&bbuff;
	USHORT lbuff = p[1] & 0xFF;
	lbuff |= (p[0] << 8 & 0xFF00);
	return lbuff;
}

bool NEKOPACK_mng::CheckHeader(string pathname, string mode)
{
	Check_OK = false;
	src.open(pathname, ios::in | ios::binary | ios::ate);
	if (!src.is_open())
	{
		cout << "文件打开失败！" << endl;
		system("pause");
	}
	else
	{
		filesize = (DWORD)src.tellg();
		src.seekg(0, ios::beg);
		DWORD magic, magic2, sig;
		src.read((char*)&magic, 4);
		src.read((char*)&magic2, 4);
		if (mode == "MNG")
			sig = MNG_SIG;
		else if (mode == "PNG")
			sig = PNG_SIG;
		if (BE2LE(magic) == sig && BE2LE(magic2) == POST_SIG)
			Check_OK = true;
		else
			cout << pathname << "不是规范文件！" << endl;
	}
	return Check_OK;
}

bool NEKOPACK_mng::Export()
{
	if (Check_OK)
		_mkdir(dirname.c_str());
	else
		return false;
	info = fopen(string(dirname + ".ini").c_str(), "wt,ccs=UNICODE");
	_chdir(dirname.c_str());
	ofstream dst;
	while (src.tellg() < filesize)
	{
		DWORD size = 0;
		src.read((char*)&size, 4);
		DWORD sig = 0;
		src.read((char*)&sig, 4);
		BYTE* data = new BYTE[BE2LE(size)];
		src.read((char*)data, BE2LE(size));
		DWORD crc = 0;
		src.read((char*)&crc, 4);
		char dstname[MAX_PATH];
		switch (BE2LE(sig))
		{
			case MHDR_SIG:
			{
				mng_mhdrp mng_data = (mng_mhdrp)data;
				printf("filename:%s.mng width:%d height:%d ticks:%d layercount:%d framecount:%d playtime:%d profile:%d\n", dirname.c_str(), BE2LE(mng_data->iWidth), BE2LE(mng_data->iHeight), BE2LE(mng_data->iTicks), BE2LE(mng_data->iLayercount), BE2LE(mng_data->iFramecount), BE2LE(mng_data->iPlaytime), BE2LE(mng_data->iSimplicity));
				fwprintf(info, L";ALL\n");
				fwprintf(info, L"width=%d\n", BE2LE(mng_data->iWidth));
				fwprintf(info, L"height=%d\n", BE2LE(mng_data->iHeight));
				fwprintf(info, L"ticks=%d\n", BE2LE(mng_data->iTicks));
				fwprintf(info, L"layercount=%d\n", BE2LE(mng_data->iLayercount));
				fwprintf(info, L"framecount=%d\n", BE2LE(mng_data->iFramecount));
				fwprintf(info, L"playtime=%d\n", BE2LE(mng_data->iPlaytime));
				fwprintf(info, L"profile=%d\n", BE2LE(mng_data->iSimplicity));
				break;
			}
			case IHDR_SIG:
			{
				sprintf(dstname, "%08d.png", filenum);
				mng_ihdrp mng_data = (mng_ihdrp)data;
				printf("\t%s width:%d height:%d bpp:%d\n", dstname, BE2LE(mng_data->iWidth), BE2LE(mng_data->iHeight), mng_data->iBitdepth);
				if (dst.is_open())
				{
					cout << dstname << "写入出错！(IHDR_SIG)";
					return false;
				}
				fwprintf(info, L";%08d.png\n", filenum++);
				fwprintf(info, L"width=%d\n", BE2LE(mng_data->iWidth));
				fwprintf(info, L"height=%d\n", BE2LE(mng_data->iHeight));
				fwprintf(info, L"bpp=%d\n", mng_data->iBitdepth);
				dst.open(dstname, ios::out | ios::binary);
				DWORD png_sig = BE2LE((DWORD)PNG_SIG);
				dst.write((char*)&png_sig, 4);
				png_sig = BE2LE((DWORD)POST_SIG);
				dst.write((char*)&png_sig, 4);
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				break;
			}
			case tEXt_SIG:
			{
				mng_text_t mng_data;
				mng_data.zKeyword = (char*)data;
				mng_data.iKeywordsize = strlen(mng_data.zKeyword);
				mng_data.iTextsize = BE2LE(size) - mng_data.iKeywordsize - 1;
				mng_data.zText = new char[mng_data.iTextsize + 1];
				strncpy(mng_data.zText, (char*)(data + mng_data.iKeywordsize + 1), mng_data.iTextsize);
				mng_data.zText[mng_data.iTextsize + 1] = 0;
				fwprintf(info, L";tEXt\n");
				fwprintf(info, L"keywordsize=%d\n", mng_data.iKeywordsize);
				fwprintf(info, L"textsize=%d\n", mng_data.iTextsize);
				WCHAR wkeyword[200];
				DWORD wkeywordsize = MultiByteToWideChar(932, 0, mng_data.zKeyword, mng_data.iKeywordsize, wkeyword, 0);
				MultiByteToWideChar(932, 0, mng_data.zKeyword, mng_data.iKeywordsize, wkeyword, wkeywordsize);
				wkeyword[wkeywordsize] = L'\0';
				WCHAR wztext[200];
				DWORD wztextsize = MultiByteToWideChar(932, 0, mng_data.zText, mng_data.iTextsize, wztext, 0);
				MultiByteToWideChar(932, 0, mng_data.zText, mng_data.iTextsize, wztext, wztextsize);
				wztext[wztextsize] = L'\0';
				fwprintf(info, L"%ls\t%ls\n", wkeyword, wztext);
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				delete[] mng_data.zText;
				break;
			}
			case IDAT_SIG:
			{
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				break;
			}
			case DEFI_SIG:
			{
				mng_defip mng_data = (mng_defip)data;
				fwprintf(info, L";DEFI\n");
				DWORD i = 0;
				if (i < BE2LE(size))
				{
					fwprintf(info, L"objectid=%d\n", BE2LE(mng_data->iObjectid));
					i += 2;
				}
				if (i < BE2LE(size))
				{
					fwprintf(info, L"donotshow=%d\n", mng_data->bHasdonotshow);
					i++;
				}
				if (i < BE2LE(size))
				{
					fwprintf(info, L"concrete=%d\n", mng_data->bHasconcrete);
					i++;
				}
				if (i < BE2LE(size))
				{
					fwprintf(info, L"xlocation=%d\n", BE2LE(mng_data->iXlocation));
					i += 4;
				}
				if (i < BE2LE(size))
				{
					fwprintf(info, L"ylocation=%d\n", BE2LE(mng_data->iYlocation));
					i += 4;
				}
				if (i < BE2LE(size))
				{
					fwprintf(info, L"leftcb=%d\n", BE2LE(mng_data->iLeftcb));
					i += 4;
				}
				if (i < BE2LE(size))
				{
					fwprintf(info, L"rightcb=%d\n", BE2LE(mng_data->iRightcb));
					i += 4;
				}
				if (i < BE2LE(size))
				{
					fwprintf(info, L"topcb=%d\n", BE2LE(mng_data->iTopcb));
					i += 4;
				}
				if (i < BE2LE(size))
				{
					fwprintf(info, L"bottomcb=%d\n", BE2LE(mng_data->iBottomcb));
					i += 4;
				}
				break;
			}
			case oFFs_SIG:
			{
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				break;
			}
			case IEND_SIG:
			{
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				if (!dst.is_open())
				{
					cout << dstname << "写入出错！(IEND_SIG)";
					return false;
				}
				dst.close();
				break;
			}
			case MEND_SIG:
				break;
			default:
				printf("\t%4.4s size:0x%X offset:0x%X\n", (char*)&sig, BE2LE(size), (DWORD)src.tellg() - BE2LE(size) - 0x0C);
				if (dst.is_open())
				{
					dst.write((char*)&size, 4);
					dst.write((char*)&sig, 4);
					dst.write((char*)data, BE2LE(size));
					dst.write((char*)&crc, 4);
				}
				break;
		}
		delete[] data;
	}
	return true;
}

bool NEKOPACK_mng::Import()
{
	if (!Check_OK)
		return false;
	if (_access(dirname.c_str(), 0) == -1)
	{
		cout << dirname << "文件夹不存在！" << endl;
		return false;
	}
	ofstream dst(dirname + ".new", ios::out | ios::binary);
	_chdir(dirname.c_str());
	ifstream png;
	bool hasfile = false, doidat = false;
	DWORD png_size = 0;
	while (src.tellg() < filesize)
	{
		DWORD size = 0;
		src.read((char*)&size, 4);
		DWORD sig = 0;
		src.read((char*)&sig, 4);
		BYTE* data = new BYTE[BE2LE(size)];
		src.read((char*)data, BE2LE(size));
		DWORD crc = 0;
		src.read((char*)&crc, 4);
		char dstname[MAX_PATH];
		switch (BE2LE(sig))
		{
			case MHDR_SIG:
			{
				mng_mhdrp mng_data = (mng_mhdrp)data;
				printf("filename:%s.mng width:%d height:%d ticks:%d layercount:%d framecount:%d playtime:%d profile:%d\n", dirname.c_str(), BE2LE(mng_data->iWidth), BE2LE(mng_data->iHeight), BE2LE(mng_data->iTicks), BE2LE(mng_data->iLayercount), BE2LE(mng_data->iFramecount), BE2LE(mng_data->iPlaytime), BE2LE(mng_data->iSimplicity));
				DWORD mng_sig = BE2LE((DWORD)MNG_SIG);
				dst.write((char*)&mng_sig, 4);
				mng_sig = BE2LE((DWORD)POST_SIG);
				dst.write((char*)&mng_sig, 4);
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				break;
			}
			case IHDR_SIG:
			{
				sprintf(dstname, "%08d.png", filenum++);
				if (png.is_open())
				{
					cout << dstname << "读取出错！(IHDR_SIG)";
					return false;
				}
				png.open(dstname, ios::in | ios::binary | ios::ate);
				if (png.is_open())
					hasfile = true;
				if (hasfile)
				{
					png_size = (DWORD)png.tellg();
					png.seekg(0, ios::beg);
					DWORD magic, magic2;
					png.read((char*)&magic, 4);
					png.read((char*)&magic2, 4);
					if (BE2LE(magic) != PNG_SIG || BE2LE(magic2) != POST_SIG)
					{
						cout << dstname << "不是png文件！(IHDR_SIG)" << endl;
						dst.close();
						png.close();
						return false;
					}
					delete[] data;
					png.read((char*)&size, 4);
					png.read((char*)&sig, 4);
					data = new BYTE[BE2LE(size)];
					png.read((char*)data, BE2LE(size));
					png.read((char*)&crc, 4);
				}
				mng_ihdrp mng_data = (mng_ihdrp)data;
				printf("\t%s width:%d height:%d bpp:%d\n", dstname, BE2LE(mng_data->iWidth), BE2LE(mng_data->iHeight), mng_data->iBitdepth);
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				break;
			}
			case tEXt_SIG:
			{
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				break;
			}
			case IDAT_SIG:
			{
				if (!hasfile)
				{
					dst.write((char*)&size, 4);
					dst.write((char*)&sig, 4);
					dst.write((char*)data, BE2LE(size));
					dst.write((char*)&crc, 4);
				}
				else
				{
					if (!doidat)
					{
						do
						{
							png.read((char*)&size, 4);
							png.read((char*)&sig, 4);
							png.seekg(BE2LE(size), ios::cur);
							png.read((char*)&crc, 4);
						} while (BE2LE(sig) != IDAT_SIG);
						png.seekg(-(int)(BE2LE(size) + 12), ios::cur);
						do
						{
							delete[] data;
							png.read((char*)&size, 4);
							png.read((char*)&sig, 4);
							data = new BYTE[BE2LE(size)];
							png.read((char*)data, BE2LE(size));
							png.read((char*)&crc, 4);
							if (BE2LE(sig) == IDAT_SIG)
							{
								dst.write((char*)&size, 4);
								dst.write((char*)&sig, 4);
								dst.write((char*)data, BE2LE(size));
								dst.write((char*)&crc, 4);
							}
						} while (BE2LE(sig) == IDAT_SIG);
						doidat = true;
					}
				}
				break;
			}
			case DEFI_SIG:
			{
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				break;
			}
			case oFFs_SIG:
			{
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				break;
			}
			case IEND_SIG:
			{
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				if (hasfile)
				{
					hasfile = false;
					if (!png.is_open())
					{
						cout << dstname << "读取出错！(IEND_SIG)";
						return false;
					}
					png.close();
				}
				doidat = false;
				break;
			}
			case MEND_SIG:
			{
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				dst.close();
				break;
			}
			default:
				printf("\t%4.4s size:0x%X offset:0x%X\n", (char*)&sig, BE2LE(size), (DWORD)src.tellg() - BE2LE(size) - 0x0C);
				dst.write((char*)&size, 4);
				dst.write((char*)&sig, 4);
				dst.write((char*)data, BE2LE(size));
				dst.write((char*)&crc, 4);
				break;
		}
		delete[] data;
	}
	return true;
}

bool NEKOPACK_mng::GetChunkInfo(string mode)
{
	if (!Check_OK)
		return false;
	while (src.tellg() < filesize)
	{
		DWORD size = 0;
		src.read((char*)&size, 4);
		DWORD sig = 0;
		src.read((char*)&sig, 4);
		BYTE* data = new BYTE[BE2LE(size)];
		src.read((char*)data, BE2LE(size));
		DWORD crc = 0;
		src.read((char*)&crc, 4);
		char dstname[MAX_PATH];
		switch (BE2LE(sig))
		{
			case MHDR_SIG:
			{
				mng_mhdrp mng_data = (mng_mhdrp)data;
				printf("filename:%s.mng width:%d height:%d ticks:%d layercount:%d framecount:%d playtime:%d profile:%d\n", dirname.c_str(), BE2LE(mng_data->iWidth), BE2LE(mng_data->iHeight), BE2LE(mng_data->iTicks), BE2LE(mng_data->iLayercount), BE2LE(mng_data->iFramecount), BE2LE(mng_data->iPlaytime), BE2LE(mng_data->iSimplicity));
				break;
			}
			case IHDR_SIG:
			{
				sprintf(dstname, "%08d.png", filenum++);
				mng_ihdrp mng_data = (mng_ihdrp)data;
				if (mode == "MNG")
					printf("\t%s width:%d height:%d bpp:%d\n", dstname, BE2LE(mng_data->iWidth), BE2LE(mng_data->iHeight), mng_data->iBitdepth);
				else if (mode == "PNG")
					printf("%s.png width:%d height:%d bpp:%d\n", dirname.c_str(), BE2LE(mng_data->iWidth), BE2LE(mng_data->iHeight), mng_data->iBitdepth);
				break;
			}
			default:
				break;
		}
		if (mode == "MNG")
			printf("\t");
		printf("\t%4.4s size:0x%X offset:0x%X\n", (char*)&sig, BE2LE(size), (DWORD)src.tellg() - BE2LE(size) - 0x0C);
		delete[] data;
	}
	return true;
}