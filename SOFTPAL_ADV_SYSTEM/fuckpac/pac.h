#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <io.h>
#include <direct.h>

using namespace std;

#pragma pack(1)

typedef struct header_s
{
	BYTE sig[4]; //PAC\x20
	DWORD uk;
	DWORD filenum;
} header_t;

typedef struct findex_s
{
	char filename[32];
	DWORD size;
	DWORD offset;
} findex_t;
typedef struct eof_s
{
	DWORD uk;
	DWORD eofs;
} eof_t;

#pragma pack()

class PAC
{
public:
	DWORD filenum;
	PAC();
	PAC(string pacname);
	bool ReadIndex(string pacname);
	bool pacexport();
	bool pacpack();
	bool pacmake(FILE* in);
	void decrypt(BYTE* data, DWORD size);
	void encrypt(BYTE* data, DWORD size);
	vector<findex_t> findexs;
	~PAC();

private:
	BYTE rol(int val, int n);
	BYTE ror(int val, int n);

	bool Index_OK;
	FILE *fp;
	string dirname;
	header_t header;
	eof_t uk_eof;
};