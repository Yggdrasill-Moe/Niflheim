#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <detours.h>

typedef unsigned char  unit8;
typedef unsigned short unit16;
typedef unsigned int   unit32;

PVOID g_pOldCreateFontIndirectA = NULL;
typedef int (WINAPI *PfuncCreateFontIndirectA)(LOGFONTA *lplf);
int WINAPI NewCreateFontIndirectA(LOGFONTA *lplf)
{
	lplf->lfCharSet = GB2312_CHARSET;
	//lplf->lfHeight = 100;
	//strcpy(lplf->lfFaceName, "黑体");

	return ((PfuncCreateFontIndirectA)g_pOldCreateFontIndirectA)(lplf);
}

PVOID g_pOldCreateFontA = NULL;
typedef int (WINAPI *PfuncCreateFontA)(int cHeight, int cWidth, int cEscapement, int cOrientation, int cWeight, DWORD bItalic,
	DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision, DWORD iQuality, DWORD iPitchAndFamily, LPCSTR pszFaceName);
int WINAPI NewCreateFontA(int cHeight, int cWidth, int cEscapement, int cOrientation, int cWeight, DWORD bItalic,
	DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision, DWORD iQuality, DWORD iPitchAndFamily, LPCSTR pszFaceName)
{
	iCharSet = 0x86;
	//pszFaceName = "微软雅黑";
	return ((PfuncCreateFontA)g_pOldCreateFontA)(cHeight,  cWidth,  cEscapement,  cOrientation,  cWeight,  bItalic,
		 bUnderline,  bStrikeOut,  iCharSet,  iOutPrecision,  iClipPrecision,  iQuality,  iPitchAndFamily,  pszFaceName);
}

PVOID g_pOldMultiByteToWideChar = NULL;
typedef int (WINAPI *PfuncMultiByteToWideChar)(UINT CodePage, DWORD dwFlags, LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
int WINAPI NewMultiByteToWideChar(UINT CodePage, DWORD dwFlags,LPCCH lpMultiByteStr,int cbMultiByte,LPWSTR lpWideCharStr,int cchWideChar)
{
	CodePage = 936;
	return ((PfuncMultiByteToWideChar)g_pOldMultiByteToWideChar)(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
}

PVOID g_pOldGetGlyphOutlineW = NULL;
typedef int (WINAPI *PfuncGetGlyphOutlineW)(HDC hdc, UINT uChar, UINT fuFormat, LPGLYPHMETRICS lpgm, DWORD cjBuffer, LPVOID pvBuffer, MAT2 *lpmat2);
int WINAPI NewGetGlyphOutlineW(HDC hdc,UINT uChar,UINT fuFormat,LPGLYPHMETRICS lpgm,DWORD cjBuffer,LPVOID pvBuffer, MAT2* lpmat2)
{
	if (uChar == 0x8179)
		uChar = 0xA1BE;
	else if(uChar == 0x817A)
		uChar = 0xA1BF;
	return ((PfuncGetGlyphOutlineW)g_pOldGetGlyphOutlineW)(hdc, uChar, fuFormat, lpgm, cjBuffer, pvBuffer, lpmat2);
}
//边界检测
//cmp al,0x9F
//9处
void BorderPatch()
{
	unit8 Border = 0xFE;
	//memcpy((void*)0x40593B, &Border, 1);
	//memcpy((void*)0x4B13D3, &Border, 1);
	//memcpy((void*)0x4FE43E, &Border, 1);
	//memcpy((void*)0x4FF90E, &Border, 1);
	//memcpy((void*)0x643833, &Border, 1);
	memcpy((void*)0x64505D, &Border, 1);
	//memcpy((void*)0x64536A, &Border, 1);
	//memcpy((void*)0x645866, &Border, 1);
	//memcpy((void*)0x645BDE, &Border, 1);
}
//选择中文字体，配合CreateFontIndirectA
void EnumFontFamiliesAPatch()
{
	unit8 CharSet = 0x86;
	memcpy((void*)0x663CE7, &CharSet, 1);
}

//安装Hook 
BOOL APIENTRY SetHook()
{

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	g_pOldCreateFontIndirectA = DetourFindFunction("GDI32.dll", "CreateFontIndirectA");
	DetourAttach(&g_pOldCreateFontIndirectA, NewCreateFontIndirectA);
	g_pOldCreateFontA = DetourFindFunction("GDI32.dll", "CreateFontA");
	DetourAttach(&g_pOldCreateFontA, NewCreateFontA);
	g_pOldGetGlyphOutlineW = DetourFindFunction("GDI32.dll", "GetGlyphOutline");
	DetourAttach(&g_pOldGetGlyphOutlineW, NewGetGlyphOutlineW);
	//g_pOldCreateFontA = DetourFindFunction("kernel32.dll", "MultiByteToWideChar");
	//DetourAttach(&g_pOldMultiByteToWideChar, NewMultiByteToWideChar);
	LONG ret = DetourTransactionCommit();
	BorderPatch();
	EnumFontFamiliesAPatch();
	return ret == NO_ERROR;
}

//卸载Hook
BOOL APIENTRY DropHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&g_pOldCreateFontIndirectA, NewCreateFontIndirectA);
	DetourDetach(&g_pOldCreateFontA, NewCreateFontA);
	DetourDetach(&g_pOldGetGlyphOutlineW, NewGetGlyphOutlineW);
	//DetourDetach(&g_pOldMultiByteToWideChar, NewMultiByteToWideChar);
	LONG ret = DetourTransactionCommit();
	return ret == NO_ERROR;
}

static HMODULE s_hDll;
HMODULE WINAPI Detoured()
{
	return s_hDll;
}

__declspec(dllexport)void WINAPI Init()
{

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		s_hDll = hModule;
		DisableThreadLibraryCalls(hModule);
		SetHook();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		DropHook();
		break;
	}
	return TRUE;
}