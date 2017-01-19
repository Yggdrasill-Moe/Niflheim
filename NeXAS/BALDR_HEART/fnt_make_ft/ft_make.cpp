#include "ft_make.h"

FT_Make::FT_Make(string font_path, DWORD font_height)
{
	FT_Init(font_path, font_height);
}

bool FT_Make::FT_Init(string font_path, DWORD font_height)
{
	ifstream infile;
	infile.open(font_path, ios::in | ios::binary | ios::ate);
	DWORD filesize = infile.tellg();
	infile.seekg(0, ios::beg);
	char* buff = new char[filesize];
	infile.read(buff, filesize);
	infile.close();
	error = FT_Init_FreeType(&library);
	if (error)
	{
		MessageBoxA(NULL, "Init_FreeType Error!", "Error", MB_OK);
		return false;
	}
	error = FT_New_Memory_Face(library,(BYTE *)buff, filesize, 0, &face);
	if (error)
	{
		MessageBoxA(NULL, "New_Memory_Face Error!", "Error", MB_OK);
		return false;
	}
	error = FT_Set_Char_Size(face, 0, font_height << 6, 0, 0);
	if (error)
	{
		MessageBoxA(NULL, "Set_Char_Size Error!", "Error", MB_OK);
		return false;
	}
	return true;
}

CharBitmap FT_Make::GetCharBitmap(WCHAR *wchar)
{

}