#include "ft_make.h"

FT_Make::FT_Make(string font_path, DWORD font_height)
{
	if (!FT_Init(font_path, font_height))
	{
		cout << "ft Init fail!\n";
		exit(0);
	}
}

bool FT_Make::FT_Init(string font_path, DWORD font_height)
{
	FT_Error error;
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
		cout << "Init_FreeType Error!\n";
		return false;
	}
	cout << "read ttf...\n";
	error = FT_New_Memory_Face(library, (BYTE *)buff, filesize, 0, &face);
	if (error)
	{
		cout << "New_Memory_Face Error!\n";
		return false;
	}
	error = FT_Set_Char_Size(face, 0, font_height * 64, 0, 0);
	if (error)
	{
		cout << "Set_Char_Size Error!\n";
		return false;
	}
	/*error = FT_Set_Pixel_Sizes(face, 0, font_height);
	if (error)
	{
	cout << "Set_Pixel_Sizes Error!\n";
	return false;
	}*/
	return true;
}

CharBitmap FT_Make::GetCharBitmap(WCHAR wchar)
{
	FT_GlyphSlot slot = face->glyph;
	FT_Error error;
	FT_Bitmap bmp;
	CharBitmap cbmp;
	error = FT_Load_Char(face, wchar, FT_LOAD_RENDER);
	if (error)
	{
		cout << "Load_Char Error!\n";
		exit(0);
	}
	bmp = slot->bitmap;
	cbmp.bmp_width = bmp.width;
	cbmp.bmp_height = bmp.rows;
	cbmp.bearingX = slot->bitmap_left;
	cbmp.bearingY = slot->bitmap_top;
	cbmp.Advance = slot->advance.x / 64;
	cbmp.bmpBuffer = bmp.buffer;
	return cbmp;
}

FT_Make::~FT_Make()
{
	FT_Done_Face(face);
	FT_Done_FreeType(library);
}