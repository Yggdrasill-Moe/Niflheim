#define UNICODE
#include "xp3.h"

UNCOM unCom;

int is_xp3_file(HANDLE hFile)
{
    DWORD R;
    char magic[11];

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    ReadFile(hFile, magic, 11, &R, NULL);
    return !memcmp(magic, "XP3\r\n \n\x1A\x8B\x67\x01", 11);
}


PBYTE uncompress_xp3_idx(HANDLE hFile, PDWORD idx_len, UNCOM unCom)
{
    DWORD ByteRead;
    xp3_file_header header;

    SetFilePointer(hFile, 11, NULL, FILE_BEGIN);
    ReadFile(hFile, &header.offset_lo, 4, &ByteRead, NULL);
    ReadFile(hFile, &header.offset_hi, 4, &ByteRead, NULL);

    if (header.offset_lo != 0x17)
        SetFilePointer(hFile, header.offset_lo, (PLONG)&header.offset_hi, FILE_BEGIN);
    else
    {
        ReadFile(hFile, &header.minor_version, 4, &ByteRead, NULL);
        ReadFile(hFile, &header.flag, 1, &ByteRead, NULL);
        ReadFile(hFile, &header.index_size_lo, 4, &ByteRead, NULL);
        ReadFile(hFile, &header.index_size_hi, 4, &ByteRead, NULL);
        ReadFile(hFile, &header.index_offset_lo, 4, &ByteRead, NULL);
        ReadFile(hFile, &header.index_offset_hi, 4, &ByteRead, NULL);

        SetFilePointer(hFile, header.index_offset_lo, (PLONG)&header.index_offset_hi, FILE_BEGIN);
    }

    BYTE  idx_flag;
    DWORD idx_size_lo;
    DWORD idx_size_hi;
    DWORD idx_uncom_lo;
    DWORD idx_uncom_hi;

    ReadFile(hFile,     &idx_flag, 1, &ByteRead, NULL);
    ReadFile(hFile,  &idx_size_lo, 4, &ByteRead, NULL);
    ReadFile(hFile,  &idx_size_hi, 4, &ByteRead, NULL);
    if (idx_flag)
    {
        ReadFile(hFile, &idx_uncom_lo, 4, &ByteRead, NULL);
        ReadFile(hFile, &idx_uncom_hi, 4, &ByteRead, NULL);
    }
    else
    {
        idx_uncom_lo = idx_size_lo;
        idx_uncom_hi = idx_size_hi;
    }

    PBYTE idx = (u8*)VirtualAlloc(NULL, idx_size_lo, MEM_COMMIT, PAGE_READWRITE);
    PBYTE idx_raw = (u8*)VirtualAlloc(NULL, idx_uncom_lo, MEM_COMMIT, PAGE_READWRITE);
    if (!idx || !idx_raw)
    {
        AppendMsg(L"内存分配失败！");
        return 0;
    }

    ReadFile(hFile, idx, idx_size_lo, &ByteRead, NULL);
    if (idx_flag)
        unCom((PBYTE)idx_raw, &idx_uncom_lo, (PBYTE)idx, idx_size_lo);
    else
        memcpy(idx_raw, idx, idx_size_lo);

    VirtualProtect(idx_raw, idx_uncom_lo, PAGE_READONLY, NULL);
    VirtualFree(idx, idx_size_lo, MEM_DECOMMIT);
    VirtualFree(idx, 0, MEM_RELEASE);

    *idx_len = idx_uncom_lo;
    return idx_raw;
}


static int SplitFileNameAndSave (
        const wchar_t *cur_dir,
        const wchar_t *file_name,
        PVOID unpack,
        DWORD file_length
        )
{
    DWORD ByteWrite;
    wchar_t buf[MAX_PATH] = {0}, buf2[MAX_PATH];

    StringCchCopy(buf, MAX_PATH, cur_dir);
    StringCchCat (buf, MAX_PATH, L"\\");
    StringCchCat (buf, MAX_PATH, file_name);

    int len = wcslen(buf);
    int i = wcslen(cur_dir) + 1;
    wchar_t *p = buf, *end = buf + len;
    while (p <= end && i < len)
    {
        while(buf[i] != '\\' && buf[i] != '/' && buf[i] != '\0') ++i;
        if (buf[i] == '/') buf[i] = '\\';
        if (i<len)
        {
            wchar_t tmp = buf[i];
            buf[i] = '\0';

            CreateDirectoryW(p, 0);
            buf[i] = tmp;
            ++i;
        }
    }

    HANDLE hFile;
    int ret = 0;
    do {
        hFile = CreateFile(buf, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            StringCchPrintf(buf2, MAX_PATH, L"[文件创建错误]%s\r\n", file_name);
            ret = ERR_FILE_CREATE;
            break;
        }

        WriteFile(hFile, unpack, file_length, &ByteWrite, NULL);

        if (ByteWrite != file_length)
        {
            StringCchPrintf(buf2, MAX_PATH, L"[文件写入错误]%s\r\n", file_name);
            ret = ERR_FILE_ERITE;
            break;
        }

        int t = GetLastError();
        if (!t || t == ERROR_ALREADY_EXISTS)
            StringCchPrintf(buf2, MAX_PATH, L"[已保存]%s\r\n", file_name);
        else
        {
            StringCchPrintf(buf2, MAX_PATH, L"[无法保存]%s\r\n", file_name);
            ret = ERR_FILE_OTHERS;
        }
    } while(0);

    AppendMsg(buf2);
    CloseHandle(hFile);
    return ret;
}


DWORD HaveExtraEntryChunk(const char *game)
{
    for (int i=_countof(XP3EntryExtraChunk)-1; i>=0; --i)
        if (!strcmp(game, XP3EntryExtraChunk[i].name))
        {
            return XP3EntryExtraChunk[i].chunk;
        }

    return 0;
}

int XP3ArcPraseEntryStage0 (
        PVOID _idx, DWORD _len,
        std::vector<file_entry>& Entry
        )
{
    static const DWORD _file = 0x656C6946, _adlr = 0x726c6461,
        _segm = 0x6d676573, _info = 0x6f666e69,
        flag_file = 0x1,    flag_adlr = 0x2,
        flag_segm = 0x4,    flag_info = 0x8,
        flag_all  = 0xf;

    PBYTE info_sec_end = nullptr;
    PBYTE pEnd = (PBYTE)_idx + _len, p = (PBYTE)_idx;

    assert(*(PDWORD)p == _file);

    p += *(PDWORD)(p + 4) + 0xc;  // skip protection warning

    while (p < pEnd)
    {
        //////////////////////////////////////
        // 31<-------3----2----1--->0
        //          info segm adlr file
        //////////////////////////////////////
        int flag = 0;
        file_entry fe;
        memset(&fe, 0, sizeof(fe));
        PBYTE SingleEnd = pEnd;

        while (p < SingleEnd && flag != flag_all)
        {
            switch (*(PDWORD)p)
            {
            default:
                ++p;
                break;

            case _file:
                assert(!(flag & flag_file));
                SingleEnd = p + *(PDWORD)(p + 4) + 0xc;
                p += 0xc;
                flag |= flag_file;
                break;

            case _adlr:
                assert(!(flag & flag_adlr));
                p += 0xc;
                fe.checksum = *((PDWORD)p);
                p += 4;
                flag |= flag_adlr;
                break;

            case _segm:
                assert(!(flag & flag_segm));
                if (*(PDWORD)(p + 4) % 0x1c == 0)
                {
                    fe.part = *(PDWORD)(p + 4) / 0x1c;
                    p += 0xC;
                    for (int i = 0; i < fe.part; ++i)
                    {
                        fe.info[i].compress_flag = *(PDWORD)p;
                        p += 4;    // 1 compressed
                        fe.info[i].offset = *(PULONG64)p;
                        p += 8;
                        fe.info[i].orig_length = *(PULONG64)p;
                        p += 8;
                        fe.info[i].pkg_length = *(PULONG64)p;
                        p += 8;
                    }
                }
                else
                {   // 不应该进来
                    assert(0);
                    AppendMsg(L"错误的文件索引记录\r\n");
                    while (*(PDWORD)p != _file) ++p;    // 跳过这个索引
                }

                flag |= flag_segm;
                break;

            case _info:
                assert(!(flag & flag_info));
                info_sec_end = p + 0xc + *((PDWORD)(p + 0x4));
                p += 0xc;
                fe.encryption_flag = *((PDWORD)p);    // 好像这个标志也没啥用
                p += 0x14;  // 跳过info中的长度信息

                // 剩下的是文件名长度和文件名
                int buf_size = (int)*((PWORD)p);
                if (buf_size >= _countof(fe.file_name))
                {
                    MessageBox(0, L"文件名超出缓冲区长度\r\n", L"提示", MB_ICONWARNING | MB_OK);
                    buf_size = _countof(fe.file_name) - 1;
                }
                p += 0x2;
                memset(fe.file_name, 0, _countof(fe.file_name));
                memcpy(fe.file_name, (wchar_t*)p, buf_size * sizeof(wchar_t));
                
                p = info_sec_end;

                flag |= flag_info;
                break;
            }
        }   // end while (p < pEnd && flag != flag_all)

        assert(flag == flag_all);
        Entry.push_back(fe);
    }

    return 0;
}


int XP3ArcPraseEntryStage1 (
        PVOID _idx,
        DWORD _len,
        std::vector<file_entry>& Entry,
        DWORD chunk
        )
{
    /////////////////////////////////////////
    // Format:
    // <magic> <length>  <adlr>   <filename>
    // 4Bytes + 8Bytes + 4Bytes + 2Bytes+wcharstring
    /////////////////////////////////////////
    if (!chunk)
        return 1;

    PBYTE pEnd = (PBYTE)_idx + _len, p = (PBYTE)_idx;
    int walk = -1;
    while (p < pEnd && *(PDWORD)p != chunk) ++p;
    for (DWORD i=0; i<Entry.size(); ++i)
        if (*(PDWORD)(p+0xc) == Entry[i].checksum)
        {
            walk = i + 1;
            wcscpy_s(Entry[i].file_name, file_entry::StrCapacity, (wchar_t*)(p + 0x12));
            p += 0x12 + wcslen((wchar_t*)(p + 0x12));
        }

    assert(walk != -1);

    while (p < pEnd)
    {
        if (*(PDWORD)p == chunk)
        {
            wcscpy_s(Entry[walk++].file_name, file_entry::StrCapacity, (wchar_t*)(p + 0x12));
            p += 0x12 + wcslen((wchar_t*)(p + 0x12));
        }
        else
            ++p;
    }

    return 0;
}


int XP3SaveResource (
        const HANDLE hFile,
        std::vector<file_entry>& Entry,
        const char *game,
        const wchar_t *cur_dir
        )
{
    _XOR_DECODE p_decode = (_XOR_DECODE)0x80000000;
    DWORD R, game_idx, offset_hi, cnt_savefile = 0;


    if (!strcmp(game, unencry_game))    // 决定解密使用的函数
        p_decode = 0;
    else for (int i=0; i<sizeof(simple_xor_game)/sizeof(simple_xor_game[0]); ++i)
        if (!strcmp(game, simple_xor_game[i].name))
        {
            p_decode = simple_xor_game[i].p_decode;
            game_idx = i;
            break;
        }


    for each(file_entry fe in Entry)
    {
        DWORD file_pkg_len = 0;
        DWORD file_org_len = 0;
        for (int i=0; i<fe.part; ++i)
        {
            file_pkg_len += (DWORD)fe.info[i].pkg_length;
            file_org_len += (DWORD)fe.info[i].orig_length;
        }

        DWORD file_read = 0;
        PBYTE cipher = (PBYTE)VirtualAlloc(NULL, file_pkg_len, MEM_COMMIT, PAGE_READWRITE);

        for (int i=0; i<fe.part; ++i)
        {
            offset_hi = (DWORD)(fe.info[i].offset >> 32);
            SetFilePointer(hFile, (DWORD)fe.info[i].offset, (PLONG)&offset_hi, FILE_BEGIN);
            ReadFile(hFile, cipher + file_read, (DWORD)fe.info[i].pkg_length, &R, NULL);
            file_read += (DWORD)fe.info[i].pkg_length;
        }

        PBYTE unpack = (PBYTE)VirtualAlloc(NULL, file_org_len, MEM_COMMIT, PAGE_READWRITE);
        DWORD unpack_len = (DWORD)file_org_len;
        DWORD unpack_offset = 0;

        if (fe.info[0].compress_flag)
            unCom(unpack, &unpack_len, cipher, file_pkg_len);
        else
            memcpy(unpack, cipher, file_org_len);

//*****************************************************************************//
        do {
            if (!p_decode) break;
            else if (p_decode == (_XOR_DECODE)0x80000000)
                xp3filter_decode(const_cast<char*>(game), fe.file_name, unpack, file_org_len, unpack_offset, file_org_len, fe.checksum);
            else
                p_decode(fe.checksum, simple_xor_game[game_idx].extend_key, simple_xor_game[game_idx].offset, unpack, file_org_len);

        }while(0);
//*****************************************************************************//

        if (!SplitFileNameAndSave(cur_dir, fe.file_name, unpack, file_org_len))
            ++cnt_savefile;

        VirtualFree(unpack, 0, MEM_RELEASE);
        VirtualFree(cipher, 0, MEM_RELEASE);
    }

    return cnt_savefile;
}


void XP3Entrance (
        const wchar_t *packName,
        const wchar_t *curDirectory,
        const char *choosedGame
        )
{
    DWORD idx_size = 0;
    PBYTE uncompress_idx;
    HANDLE hFile;
    wchar_t szBuffer[MAX_PATH];

    do {
        hFile = CreateFile(packName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            StringCchPrintf(szBuffer, MAX_PATH, L"无法打开文件, 跳过\r\n%s\r\n", packName);
            AppendMsg(szBuffer);
            break;
        }

        if (!is_xp3_file(hFile))
        {
            StringCchPrintf(szBuffer, MAX_PATH, L"错误的xp3文件:%s\r\n", packName);
            AppendMsg(szBuffer);
            break;
        }

        uncompress_idx = uncompress_xp3_idx(hFile, &idx_size, unCom);

        if (!uncompress_idx)
        {
            AppendMsg(L"xp3索引提取失败\r\n");
            break;
        }

        std::vector<file_entry> Entry;
        XP3ArcPraseEntryStage0(uncompress_idx, idx_size, Entry);

        DWORD chunk = 0;
        if ((chunk = HaveExtraEntryChunk(choosedGame)))
            XP3ArcPraseEntryStage1(uncompress_idx, idx_size, Entry, chunk);

        DWORD save_file = XP3SaveResource(hFile, Entry, choosedGame, curDirectory);

        if (Entry.size() == save_file)
        {
            StringCchPrintf(szBuffer, MAX_PATH,
                L"[提取完成(%d/%d)]%s\r\n", save_file, save_file, packName);
            AppendMsg(szBuffer);
        }
        else
        {
            StringCchPrintf(szBuffer, MAX_PATH, L"提取%d个文件，共%d个，有%d个发生错误\r\n%s\r\n",
                save_file, Entry.size(), Entry.size() - save_file, packName);
            MessageBox(0, szBuffer, L"提示", MB_ICONWARNING);
        }
    } while (0);

    if (uncompress_idx)
        VirtualFree(uncompress_idx, 0, MEM_RELEASE);

    CloseHandle(hFile);
}


static void xor_decode(DWORD hash, BYTE extend_key, DWORD offset, PBYTE buf, DWORD len)    // 从offset开始解
{
    for (DWORD i=offset; i<len; ++i)
        buf[i] ^= (BYTE)hash ^ extend_key;
    return;
}


static void xor_decode_prettycation(DWORD hash, BYTE extend_key, DWORD offset, PBYTE buf, DWORD len)
{
    for (DWORD i=offset; i<len; ++i)
        buf[i] ^= (BYTE)(hash>>0xc);
    return;
}


static void xor_decode_swansong(DWORD hash, BYTE extend_key, DWORD offset, PBYTE buf, DWORD len)
{
    BYTE ror = (BYTE)hash & 7, key = (BYTE)(hash >> 8);
    for (DWORD i=offset; i<len; ++i)
    {
        buf[i] = buf[i] ^ key;
        buf[i] = buf[i] >> ror | buf[i] << (8-ror);
    }
    return;
}