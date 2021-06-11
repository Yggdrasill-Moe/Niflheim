#define _CRT_SECURE_NO_WARNINGS
#define _USE_32BIT_TIME_T
//说是读写高级支持必须的
#define MNG_SUPPORT_READ
#define MNG_SUPPORT_WRITE
//说是读写文件必须的，配套的宏
#define MNG_STORE_CHUNKS
#define MNG_ACCESS_CHUNKS
//cms找下载找了半天还没找到所以就没包含进去编译，只包含了libjpeg
#define MNG_NO_CMS
//说是包含库头文件是必须的，似乎不定义也可以编译通的样子
#define MNG_USE_DLL
/*
这就要特别说明下libmng库本人是用cmake生成的再编译，libmng 2.0.3中的libmng_types.h有这么一句话
TODO: this may require some elaboration for other platforms;
only works with BCB for now
是的，win32是针对bcb（C++ Builder）写的，在vs下编译没问题，但使用有大的问题
就是__stdcall和__cdecl的问题，libmng使用的是__stdcall，将其全部改成__cdecl后就没问题了
本人的魔改方式：
将libmng_types.h的第258行#define MNG_DECL __stdcall改成了
#if defined(_WINDOWS)
#define MNG_DECL __cdecl
#else
#define MNG_DECL __stdcall
#endif
然后再编译。
_WINDOWS宏是cmake生成的，vs中没有这东西，所以这里再定义一下，没有什么特殊的意义
当然要是有其他处理方式就不需要定义_WINDOWS了。
*/
#define _WINDOWS
//mng的标准文档：http://www.libpng.org/pub/mng/spec/
//mng的开发指南：https://libmng.com/developers.html
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <libmng.h>

using namespace std;

typedef struct mng_s
{
	FILE* src;
	FILE* info;
	DWORD filenum;
	char filename[MAX_PATH];
	char dirname[MAX_PATH];
} mng_t;
typedef mng_t* mng_p;

//申请内存的回调函数
mng_ptr NEKOPACK_mng_calloc(mng_size_t size)
{
	return (mng_ptr)calloc(1, size);
}
//释放内存的回调函数
void NEKOPACK_mng_free(mng_ptr buff, mng_size_t size)
{
	free(buff);
	return;
}
//openstream的回调函数
mng_bool NEKOPACK_mng_openstream(mng_handle mng)
{
	return MNG_TRUE;
}
//closestream的回调函数
mng_bool NEKOPACK_mng_closestream(mng_handle mng)
{
	return MNG_TRUE;
}
//读取数据的回调函数
mng_bool NEKOPACK_mng_readdata(mng_handle mng, mng_ptr buff, mng_uint32 size, mng_uint32p mread)
{
	mng_p mng_data = (mng_p)mng_get_userdata(mng);
	//必须返回的不能省略，mread在库内部需要处理
	*mread = fread(buff, 1, size, mng_data->src);
	return MNG_TRUE;
}
//迭代数据块的回调函数
mng_bool NEKOPACK_mng_read_iteratechunks(mng_handle mng, mng_handle chunk, mng_chunkid chunktype, mng_uint32 chunkseq)
{
	/*if (chunktype == MNG_UINT_MHDR)//MHDR块处理
	{
		//MHDR标签内容
		mng_uint32 width;
		mng_uint32 height;
		mng_uint32 ticks;
		mng_uint32 layercount;
		mng_uint32 framecount;
		mng_uint32 playtime;
		mng_uint32 profile;
		mng_p mng_data = (mng_p)mng_get_userdata(mng);
		mng_getchunk_mhdr(mng, chunk, &width, &height, &ticks, &layercount, &framecount, &playtime, &profile);
		printf("filename:%s width:%d height:%d ticks:%d layercount:%d framecount:%d playtime:%d profile:%d\n", mng_data->filename, width, height, ticks, layercount, framecount, playtime, profile);
		char infoname[MAX_PATH];
		sprintf(infoname, "%s.ini", mng_data->filename);
		mng_data->info = fopen(infoname, "wt,ccs=UNICODE");
		if (mng_data->info == NULL)
		{
			printf("创建info文件失败！\n");
			return MNG_FALSE;
		}
		fwprintf(mng_data->info, L";ALL\n");
		fwprintf(mng_data->info, L"width=%d\n", width);
		fwprintf(mng_data->info, L"height=%d\n", height);
		fwprintf(mng_data->info, L"ticks=%d\n", ticks);
		fwprintf(mng_data->info, L"layercount=%d\n", layercount);
		fwprintf(mng_data->info, L"framecount=%d\n", framecount);
		fwprintf(mng_data->info, L"playtime=%d\n", playtime);
		fwprintf(mng_data->info, L"profile=%d\n\n", profile);
	}
	else if (chunktype == MNG_UINT_IHDR)
	{
		//IHDR标签内容
		mng_uint32 width;
		mng_uint32 height;
		mng_uint8 bpp;
		mng_uint8 colortype;
		mng_uint8 comp;
		mng_uint8 filter;
		mng_uint8 interlace;
		mng_p mng_data = (mng_p)mng_get_userdata(mng);
		mng_getchunk_ihdr(mng, chunk, &width, &height, &bpp, &colortype, &comp, &filter, &interlace);
		fwprintf(mng_data->info, L"width=%d\n", width);
		fwprintf(mng_data->info, L"height=%d\n", height);
		fwprintf(mng_data->info, L"bpp=%d\n", bpp);
	}
	else if (chunktype == MNG_UINT_tEXt)
	{
		//tEXt标签内容
		mng_uint32 keywordsize;
		mng_pchar keyword;
		mng_uint32 ztextsize;
		mng_pchar ztext;
		mng_p mng_data = (mng_p)mng_get_userdata(mng);
		mng_getchunk_text(mng, chunk, &keywordsize, &keyword, &ztextsize, &ztext);
		fwprintf(mng_data->info, L";tEXt\n");
		WCHAR wkeyword[200];
		DWORD wkeywordsize = MultiByteToWideChar(932, 0, keyword, keywordsize, wkeyword, 0);
		MultiByteToWideChar(932, 0, keyword, keywordsize, wkeyword, wkeywordsize);
		wkeyword[wkeywordsize] = L'\0';
		WCHAR wztext[200];
		DWORD wztextsize = MultiByteToWideChar(932, 0, ztext, ztextsize, wztext, 0);
		MultiByteToWideChar(932, 0, ztext, ztextsize, wztext, wztextsize);
		wztext[wztextsize] = L'\0';
		fwprintf(mng_data->info, L"%ls\t%ls\n", wkeyword, wztext);
	}
	else if (chunktype == MNG_UINT_MEND)
	{
		//mng_p mng_data = (mng_p)mng_get_userdata(mng);
		//fclose(mng_data->info);
	}*/
	return MNG_TRUE;
}

bool NEKOPACK_mng_read(char* filename)
{
	mng_retcode mng_rc;
	mng_p mng_data = (mng_p)calloc(1, sizeof(mng_p));
	if (mng_data == NULL)
	{
		printf("创建mng_data失败！\n");
		return false;
	}
	strcpy(mng_data->filename, filename);
	strcpy(mng_data->dirname, filename);
	*(char*)strchr(mng_data->dirname, '.') = '\0';
	if (_access(mng_data->dirname, 0) == -1)
		_mkdir(mng_data->dirname);
	mng_data->filenum = 0;
	if ((mng_data->src = fopen(filename, "rb")) == NULL)
	{
		printf("文件打开失败！\n");
		free(mng_data);
		return false;
	}
	mng_handle mng = mng_initialize((mng_ptr)mng_data, NEKOPACK_mng_calloc, NEKOPACK_mng_free, MNG_NULL);
	if (!mng)
	{
		printf("mng_initialize初始化失败！\n");
		mng_cleanup(&mng);
		fclose(mng_data->src);
		free(mng_data);
		return false;
	}
	if ((((mng_rc = mng_setcb_openstream(mng, NEKOPACK_mng_openstream)) != 0) || ((mng_rc = mng_setcb_closestream(mng, NEKOPACK_mng_closestream)) != 0) || ((mng_rc = mng_setcb_readdata(mng, NEKOPACK_mng_readdata)) != 0)))
	{
		printf("mng_setcb初始化失败！\n");
		mng_cleanup(&mng);
		fclose(mng_data->src);
		free(mng_data);
		return false;
	}
	if ((mng_rc = mng_read(mng)) != 0)
	{
		printf("mng_read初始化失败！\n");
		mng_cleanup(&mng);
		fclose(mng_data->src);
		free(mng_data);
		return false;
	}
	if (mng_rc = mng_iterate_chunks(mng, 0, NEKOPACK_mng_read_iteratechunks) != 0)
	{
		printf("mng_iterate_chunks调用失败！\n");
		mng_cleanup(&mng);
		fclose(mng_data->src);
		free(mng_data);
		return false;
	}
	mng_cleanup(&mng);
	fclose(mng_data->src);
	free(mng_data);
	return true;
}