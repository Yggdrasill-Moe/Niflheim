

#pragma once


#include <Windows.h>
#include <vector>
#include <assert.h>
#include "types.h"
#include "error.h"
#include "cxdec\cxdec.h"
#include "xp3filter_decode.h"
#include <strsafe.h>


#pragma pack(1)
struct xp3_file_header
{
	BYTE magic[11]; // = {'\x58', '\x50', '\x33', '\x0D', '\x0A', '\x20', '\0A', '\x1A', '\x8B', '\x67', '\x01'};
	DWORD offset_lo;
	DWORD offset_hi;
	DWORD minor_version;	// 1
	BYTE flag;	// 0x80 TVP_XP3_INDEX_CONTINUE
	DWORD index_size_lo;
	DWORD index_size_hi;
	DWORD index_offset_lo;
	DWORD index_offset_hi;
};
#pragma pack()


struct file_entry
{
    enum {
        StrCapacity = 128,
        Section = 16
    };
	DWORD checksum;
	DWORD encryption_flag;	// info
    int part;
    struct {
        DWORD compress_flag;		// segm
        u64 offset;
        u64 orig_length;
        u64 pkg_length;
    } info[Section];
    wchar_t file_name[StrCapacity];
};


extern void AppendMsg(const wchar_t *szBuffer);


int		is_xp3_file(HANDLE hFile);
PBYTE	uncompress_xp3_idx(HANDLE hFile, PDWORD idx_len, UNCOM unCom);
int     XP3SaveResource(const HANDLE hFile, std::vector<file_entry>& Entry, const char *game, const wchar_t *cur_dir);
DWORD   HaveExtraEntryChunk(const char *game);
int     XP3ArcPraseEntryStage0(PVOID _idx, DWORD _len, std::vector<file_entry>& Entry);
int     XP3ArcPraseEntryStage1(PVOID _idx, DWORD _len, std::vector<file_entry>& Entry, DWORD chunk);
void    XP3Entrance(const wchar_t *packName, const wchar_t *curDirectory, const char *choosedGame);


typedef void(*_XOR_DECODE)	(DWORD hash, BYTE extend_key, DWORD offset, PBYTE buf, DWORD len);
void xor_decode				(DWORD hash, BYTE extend_key, DWORD offset, PBYTE buf, DWORD len);
void xor_decode_prettycation(DWORD hash, BYTE extend_key, DWORD offset, PBYTE buf, DWORD len);
void xor_decode_swansong	(DWORD hash, BYTE extend_key, DWORD offset, PBYTE buf, DWORD len);


static const char unencry_game [] = "Unencrypted";      // 数据没有加密的游戏


static const struct _SIMPLE_XOR     // 数据xor过的游戏
{
	char *name;         // 游戏名
	void (*p_decode)(DWORD hash, BYTE extend_key, DWORD offset, PBYTE buf, DWORD len);	
	BYTE    extend_key;   // 除去hash外的额外的key
	DWORD   offset;       // 从数据的offset字节开始解码
}
simple_xor_game [] = {
    "kuranokunchi",	xor_decode,              0xCD, 0x0,
    "amakoi",       xor_decode,              0x0, 0x0,
    "prettycation", xor_decode_prettycation, 0x0, 0x5,
    "swansong",     xor_decode_swansong,     0x0, 0x0,
};


static const struct _XP3ENTRYEXTRACHUNK {
    char *name;
    DWORD chunk;
}
XP3EntryExtraChunk[] = {
    "seiiki",   0x676e6566,
    "nekopara", 0x0,
};