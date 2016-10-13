#include "krkr2.h"
#include "TLGDecoder.h"

CKrkr2::CKrkr2()
{
    m_pbXP3Index = NULL;
#if defined(NO_DECRYPT)
    m_pfFilter   = NULL;
#endif
}

CKrkr2::~CKrkr2()
{
    ReleaseAll();
}

LONG CKrkr2::ReleaseAll()
{
    SafeFree(&m_pbXP3Index);
    return __super::ReleaseAll();
}

#if defined(NO_DECRYPT)
CKrkr2::XP3ExtractionFilterFunc CKrkr2::SetXP3ExtractionFilter(XP3ExtractionFilterFunc Filter)
{
    XP3ExtractionFilterFunc OldFilter = m_pfFilter;
    m_pfFilter = Filter;
    return OldFilter;
}
#endif

BOOL FASTCALL IsNeedCompress(SMyXP3Index *pIndex)
{
    ULONG64 Extension;
    PWCHAR  pExtension;

    pExtension = findextw(pIndex->info.FileName);
    if (*pExtension == 0)
        return TRUE;

    Extension = *(PULONG64)(pExtension + 1);
    switch (CHAR_UPPER4W(Extension))
    {
        case TAG4W('UCI'):
        case TAG4W('PNG'):
        case TAG4W('OGG'):
        case TAG4W('AVI'):
        case TAG4W('MPG'):
        case TAG4W('MKV'):
        case CHAR_UPPER4W(TAG4W('M4A')):
            return FALSE;
    }

/*
    if (!StrICompareW(pExtension, L".uci") ||
        !StrICompareW(pExtension, L".png") ||
        !StrICompareW(pExtension, L".m4a") ||
        !StrICompareW(pExtension, L".ogg") ||
        !StrICompareW(pExtension, L".avi") ||
        !StrICompareW(pExtension, L".mpg") ||
        !StrICompareW(pExtension, L".mkv"))
        return FALSE;
*/
    return TRUE;
}

LONG
STDCALL
EnumFilesCallback(
    PACK_FILE_INFO      *pPackFileInfo,
    LPWIN32_FIND_DATAW   pFindData,
    ULONG_PTR            Context
)
{
    StrCopyW(pPackFileInfo->FileName, pFindData->cFileName + Context);
    pPackFileInfo->Compress = 0;
    pPackFileInfo->Encrypt  = 0;

    return 1;
}

ULONG CKrkr2::Pack(LPCWSTR pszPath)
{
    ULONG Length;
    WCHAR  szFileXP3[MAX_PATH];
    LARGE_INTEGER  FileCount;
    PACK_FILE_INFO *pPackFileInfo;

    StrCopyW(szFileXP3, pszPath);
    Length = StrLengthW(szFileXP3);

    if (szFileXP3[Length - 1] == '\\')
        szFileXP3[--Length] = 0;

    if (!EnumDirectoryFiles(
             (PVOID *)&pPackFileInfo,
             L"*.*",
             sizeof(*pPackFileInfo),
             pszPath,
             &FileCount,
             (EnumDirectoryFilesCallBackRoutine)EnumFilesCallback,
             (ULONG_PTR)(Length + 1),
             EDF_SUBDIR))
    {
        return 0;
    }

    *(PULONG64)&szFileXP3[Length] = TAG4W('.xp3');
    szFileXP3[Length + 4] = 0;

    Length = PackFiles(pPackFileInfo, FileCount.LowPart, szFileXP3, pszPath);

    EnumDirectoryFilesFree(pPackFileInfo);

    return Length;
}

ULONG
CKrkr2::
PackFiles(
    PACK_FILE_INFO *pPackFileInfo,
    ULONG           EntryCount,
    LPCWSTR         pszOutput,
    LPCWSTR         pszFullInputPath
)
{
    NTSTATUS                Status;
    HANDLE                  hHeap, hFile, hFileXP3;
    PBYTE                   pbIndex;
    ULONG                   BufferSize, CompressedSize;
    WCHAR                   szPath[MAX_PATH];
    PVOID                   lpBuffer, lpCompressBuffer;
    LARGE_INTEGER           Size, Offset, BytesTransfered;
    SMyXP3Index            *pXP3Index, *pIndex;
    PACK_FILE_INFO         *pInfo;
    KRKR2_XP3_DATA_HEADER   IndexHeader;
    KRKR2_XP3_HEADER        XP3Header = { { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 } };

    Status = CNtFileDisk::Create(&hFileXP3, pszOutput);
    if (!NT_SUCCESS(Status))
        return 0;

    Nt_GetCurrentDirectory(countof(szPath), szPath);
    Nt_SetCurrentDirectory(pszFullInputPath);

    hHeap               = CMem::GetGlobalHeap();
    BufferSize          = 0x10000;
    CompressedSize      = BufferSize;
    lpBuffer            = RtlAllocateHeap(hHeap, 0, BufferSize);
    lpCompressBuffer    = RtlAllocateHeap(hHeap, 0, CompressedSize);
    pXP3Index           = (SMyXP3Index *)RtlAllocateHeap(hHeap, 0, sizeof(*pXP3Index) * EntryCount);
    pIndex              = pXP3Index;
    pInfo               = pPackFileInfo;

    CNtFileDisk::Write(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered);

    Offset.QuadPart = BytesTransfered.LowPart;
    for (ULONG i = EntryCount; i; ++pIndex, ++pInfo, --i)
    {
        ZeroMemory(pIndex, sizeof(*pIndex));
        pIndex->file.Magic              = CHUNK_MAGIC_FILE;
        pIndex->info.Magic              = CHUNK_MAGIC_INFO;
        pIndex->segm.Magic              = CHUNK_MAGIC_SEGM;
        pIndex->adlr.Magic              = CHUNK_MAGIC_ADLR;
        pIndex->segm.ChunkSize.QuadPart = sizeof(pIndex->segm.segm);
        pIndex->adlr.ChunkSize.QuadPart = sizeof(pIndex->adlr) - sizeof(pIndex->adlr.Magic) - sizeof(pIndex->adlr.ChunkSize);

        Status = CNtFileDisk::Open(&hFile, pInfo->FileName);
        if (!NT_SUCCESS(Status))
            continue;

        CNtFileDisk::GetSize(hFile, &Size);
        if (Size.LowPart > BufferSize)
        {
            BufferSize = Size.LowPart;
            lpBuffer = RtlReAllocateHeap(hHeap, 0, lpBuffer, BufferSize);
        }

        Status = CNtFileDisk::Read(hFile, lpBuffer, Size.LowPart, &BytesTransfered);
        NtClose(hFile);
        if (!NT_SUCCESS(Status) || BytesTransfered.LowPart != Size.LowPart)
            continue;

        pIndex->segm.segm->Offset   = Offset;
        pIndex->info.FileName       = pInfo->FileName;
        pIndex->info.FileNameLength = StrLengthW(pInfo->FileName);

        pIndex->file.ChunkSize.QuadPart = sizeof(*pIndex) - sizeof(pIndex->file);
        pIndex->info.ChunkSize.QuadPart = sizeof(pIndex->info) - sizeof(pIndex->info.Magic) - sizeof(pIndex->info.ChunkSize);
        pIndex->file.ChunkSize.QuadPart = pIndex->file.ChunkSize.QuadPart - sizeof(pIndex->info.FileName) + pIndex->info.FileNameLength * 2;
        pIndex->info.ChunkSize.QuadPart = pIndex->info.ChunkSize.QuadPart - sizeof(pIndex->info.FileName) + pIndex->info.FileNameLength * 2;

        pIndex->adlr.Hash = adler32(1/*adler32(0, 0, 0)*/, (Bytef *)lpBuffer, BytesTransfered.LowPart);
        pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered.LowPart;
        pIndex->info.OriginalSize.LowPart = BytesTransfered.LowPart;

#if defined(CRAZY_XP3)

//        if (!StrICompareW(findnamew(pInfo->FileName), L"startup.tjs"))
        {
//            _asm int 4;
        }

        pIndex->segm.segm->Offset.QuadPart  = -pIndex->segm.segm->Offset.QuadPart;
        pIndex->segm.segm->Offset.LowPart  ^= TAG4('AMNO');
//        pIndex->info.ArchiveSize.QuadPart = -pIndex->info.ArchiveSize.QuadPart;
//        pIndex->segm.segm->OriginalSize.LowPart = _rotl(pIndex->segm.segm->OriginalSize.LowPart, 7);
//        pIndex->segm.segm->OriginalSize.HighPart = _rotr(pIndex->segm.segm->OriginalSize.HighPart, 25);

//        pIndex->segm.segm->ArchiveSize.LowPart = _rotl(pIndex->segm.segm->ArchiveSize.LowPart, 25);
//        pIndex->segm.segm->ArchiveSize.HighPart = _rotr(pIndex->segm.segm->ArchiveSize.HighPart, 7);

#endif

        LARGE_INTEGER EncryptOffset;

        EncryptOffset.QuadPart = 0;
        DecryptWorker(EncryptOffset, lpBuffer, BytesTransfered.LowPart, pIndex->adlr.Hash);
        pIndex->info.EncryptedFlag = 0x80000000;
/*
        {

            XP3_EXTRACTION_INFO Info = { sizeof(Info), { 0 }, lpBuffer, BytesTransfered, pIndex->adlr.Hash };

            UNREFERENCED_PARAMETER(Info);
#if defined (FATE_STAY_NIGHT)
            DecryptFSN(&Info);
#elif defined(REAL_SISTER) || defined(FATE_HA) || defined(NATSU_ZORA) || defined(TENSHIN) || defined(IMOUTO_STYLE)
            DecryptCxdec(&Info);
#elif defined(SAKURA)
            DecryptSakura(&Info);
#endif
            pIndex->info.EncryptedFlag = 0x80000000;
        }
*/
        if (IsNeedCompress(pIndex))
        {
            if (Size.LowPart > CompressedSize)
            {
                CompressedSize = Size.LowPart;
                lpCompressBuffer = RtlReAllocateHeap(hHeap, 0, lpCompressBuffer, CompressedSize);
            }
            if (Size.LowPart * 2 > BufferSize)
            {
                BufferSize = Size.LowPart * 2;
                lpBuffer = RtlReAllocateHeap(hHeap, 0, lpBuffer, BufferSize);
            }

            pIndex->segm.segm->bZlib = 1;
            CopyMemory(lpCompressBuffer, lpBuffer, Size.LowPart);
            BytesTransfered.LowPart = BufferSize;
            compress2((PBYTE)lpBuffer, &BytesTransfered.LowPart, (PBYTE)lpCompressBuffer, Size.LowPart, Z_BEST_COMPRESSION);
        }

        pIndex->segm.segm->ArchiveSize.LowPart = BytesTransfered.LowPart;
        pIndex->info.ArchiveSize.LowPart = BytesTransfered.LowPart;
        Offset.QuadPart += BytesTransfered.LowPart;

        CNtFileDisk::Write(hFileXP3, lpBuffer, BytesTransfered.LowPart);
    }

    EntryCount = pIndex - pXP3Index;
    XP3Header.IndexOffset = Offset;

    // generate index, calculate index size first
    Size.LowPart = 0;
    pIndex = pXP3Index;
    for (ULONG i = 0; i != EntryCount; ++i, ++pIndex)
    {
        Size.LowPart += pIndex->file.ChunkSize.LowPart + sizeof(pIndex->file);
    }

    if (Size.LowPart > CompressedSize)
    {
        CompressedSize = Size.LowPart;
        lpCompressBuffer = RtlReAllocateHeap(hHeap, 0, lpCompressBuffer, CompressedSize);
    }
    if (Size.LowPart * 2 > BufferSize)
    {
        BufferSize = Size.LowPart * 2;
        lpBuffer = RtlReAllocateHeap(hHeap, 0, lpBuffer, BufferSize);
    }

    // generate index to lpCompressBuffer
    pIndex = pXP3Index;
    pbIndex = (PBYTE)lpCompressBuffer;
    for (ULONG i = EntryCount; i; ++pIndex, --i)
    {
        ULONG n = (PBYTE)&pIndex->info.FileName - (PBYTE)pIndex;
        CopyMemory(pbIndex, &pIndex->file, n);
        pbIndex += n;
        n = pIndex->info.FileNameLength * 2;
        CopyMemory(pbIndex, pIndex->info.FileName, n);
        pbIndex += n;
        n = (PBYTE)&pIndex->adlr.Hash - (PBYTE)&pIndex->segm + 4;
        CopyMemory(pbIndex, &pIndex->segm, n);
        pbIndex += n;
    }

    IndexHeader.bZlib = 1;
    IndexHeader.OriginalSize.QuadPart = Size.LowPart;
    IndexHeader.ArchiveSize.LowPart = BufferSize;
    BufferSize = Size.LowPart;
    compress2((PBYTE)lpBuffer, &IndexHeader.ArchiveSize.LowPart, (PBYTE)lpCompressBuffer, BufferSize, Z_BEST_COMPRESSION);
    IndexHeader.ArchiveSize.HighPart = 0;

    CNtFileDisk::Write(hFileXP3, &IndexHeader, sizeof(IndexHeader));
    CNtFileDisk::Write(hFileXP3, lpBuffer, IndexHeader.ArchiveSize.LowPart);
    Offset.QuadPart = 0;
//    CNtFileDisk::Seek(hFileXP3, Offset, FILE_BEGIN);
    CNtFileDisk::Write(hFileXP3, &XP3Header, sizeof(XP3Header), NULL, &Offset);

    NtClose(hFileXP3);

    RtlFreeHeap(hHeap, 0, lpBuffer);
    RtlFreeHeap(hHeap, 0, lpCompressBuffer);
    RtlFreeHeap(hHeap, 0, pXP3Index);

    Nt_SetCurrentDirectory(szPath);

    return EntryCount;
}

BOOL CKrkr2::Open(LPCWSTR pszFileName)
{
    ReleaseAll();

    NTSTATUS                Status;
    BOOL                    Result;
    PVOID                   pvCompress, pvDecompress;
    ULONG                   CompresseBufferSize, DecompressBufferSize;
    LARGE_INTEGER           BeginOffset;
    KRKR2_XP3_HEADER        XP3Header;
    KRKR2_XP3_DATA_HEADER   DataHeader;

    Status = file.Open(pszFileName);

    if (!NT_SUCCESS(Status))
        return FALSE;

    file.Read(&XP3Header, sizeof(XP3Header));
    if ((*(PUSHORT)XP3Header.Magic) == IMAGE_DOS_SIGNATURE)
    {
    }
    else
    {
        BeginOffset.QuadPart = 0;
    }

    BeginOffset.QuadPart = 0;

    if ((*(PULONG)XP3Header.Magic & 0xFFFFFF) != TAG3('XP3'))
        return FALSE;

    CompresseBufferSize     = 0x1000;
    DecompressBufferSize    = 0x1000;
    pvCompress     = Alloc(CompresseBufferSize);
    pvDecompress   = Alloc(DecompressBufferSize);
    DataHeader.OriginalSize = XP3Header.IndexOffset;

    Result = FALSE;
    do
    {
        LARGE_INTEGER Offset;

        Offset.QuadPart = DataHeader.OriginalSize.QuadPart + BeginOffset.QuadPart;
        file.Seek(Offset, FILE_BEGIN);
        if (!NT_SUCCESS(file.Read(&DataHeader, sizeof(DataHeader))))
            break;

        if (DataHeader.ArchiveSize.HighPart != 0 || DataHeader.ArchiveSize.LowPart == 0)
            continue;

        if (DataHeader.ArchiveSize.LowPart > CompresseBufferSize)
        {
            CompresseBufferSize = DataHeader.ArchiveSize.LowPart;
            pvCompress = ReAlloc(pvCompress, CompresseBufferSize);
        }

        if ((DataHeader.bZlib & 7) == 0)
        {
            Offset.QuadPart = -8;
            file.Seek(Offset, FILE_CURRENT);
        }

        file.Read(pvCompress, DataHeader.ArchiveSize.LowPart);

        switch (DataHeader.bZlib & 7)
        {
            case FALSE:
                if (DataHeader.ArchiveSize.LowPart > DecompressBufferSize)
                {
                    DecompressBufferSize = DataHeader.ArchiveSize.LowPart;
                    pvDecompress = ReAlloc(pvDecompress, DecompressBufferSize);
                }
                CopyMemory(pvDecompress, pvCompress, DataHeader.ArchiveSize.LowPart);
                DataHeader.OriginalSize.LowPart = DataHeader.ArchiveSize.LowPart;
                break;

            case TRUE:
                if (DataHeader.OriginalSize.LowPart > DecompressBufferSize)
                {
                    DecompressBufferSize = DataHeader.OriginalSize.LowPart;
                    pvDecompress = ReAlloc(pvDecompress, DecompressBufferSize);
                }

                DataHeader.OriginalSize.HighPart = DataHeader.OriginalSize.LowPart;
                if (uncompress((PBYTE)pvDecompress,
                               (PULONG)&DataHeader.OriginalSize.HighPart,
                               (PBYTE)pvCompress,
                               DataHeader.ArchiveSize.LowPart) == Z_OK)
                {
                    DataHeader.OriginalSize.LowPart = DataHeader.OriginalSize.HighPart;
                    break;
                }

            default:
                DataHeader.bZlib = 0;
                break;
        }

        Result = InitIndex(pvDecompress, DataHeader.OriginalSize.LowPart);
        if (Result)
            break;

    } while (DataHeader.bZlib & 0x80);

    Free(pvCompress);
    if (!Result)
        Free(pvDecompress);

    return Result;
}

BOOL CKrkr2::InitIndex(PVOID lpIndex, ULONG uSize)
{
    PBYTE           pbIndex, pbEnd;
    Int64           MaxFileCount;
    LARGE_INTEGER   ChunkSize;
    MY_XP3_ENTRY   *pEntry;

    pbIndex = (PBYTE)lpIndex;
    if (*(PULONG)pbIndex != CHUNK_MAGIC_FILE)
        return FALSE;

    MaxFileCount = 100;
    m_Index.cbEntrySize = sizeof(*pEntry);
    pEntry = (MY_XP3_ENTRY *)Alloc(MaxFileCount * m_Index.cbEntrySize, HEAP_ZERO_MEMORY);
    if (pEntry == NULL)
        return FALSE;

    m_Index.pEntry = pEntry;
    m_Index.FileCount.QuadPart = 0;
    m_pbXP3Index = pbIndex;
    pbEnd = pbIndex + uSize;
    for (; pbIndex < pbEnd; ++m_Index.FileCount.QuadPart, ++pEntry)
    {
        if (m_Index.FileCount.QuadPart == MaxFileCount)
        {
            MaxFileCount *= 2;
            m_Index.pEntry = (MY_XP3_ENTRY *)ReAlloc(m_Index.pEntry, MaxFileCount * m_Index.cbEntrySize);
            pEntry = (MY_XP3_ENTRY *)m_Index.pEntry + m_Index.FileCount.QuadPart;
        }

        pEntry->file = (KRKR2_XP3_INDEX_CHUNK_FILE *)pbIndex;
        pbIndex += sizeof(pEntry->file->Magic) + sizeof(pEntry->file->ChunkSize);
        ChunkSize = pEntry->file->ChunkSize;

        while (ChunkSize.QuadPart > 0)
        {
            switch (CHAR_UPPER4(*(PULONG)pbIndex))
            {
                case CHAR_UPPER4(CHUNK_MAGIC_INFO):
                    pEntry->info = (KRKR2_XP3_INDEX_CHUNK_INFO *)pbIndex;
                    pbIndex += pEntry->info->ChunkSize.QuadPart + sizeof(pEntry->info->Magic) + sizeof(pEntry->info->ChunkSize);
                    ChunkSize.QuadPart -= pEntry->info->ChunkSize.QuadPart + sizeof(pEntry->info->Magic) + sizeof(pEntry->info->ChunkSize);
                    break;

                case CHAR_UPPER4(CHUNK_MAGIC_SEGM):
                    pEntry->segm = (KRKR2_XP3_INDEX_CHUNK_SEGM *)pbIndex;
                    pbIndex += pEntry->segm->ChunkSize.QuadPart + sizeof(pEntry->segm->Magic) + sizeof(pEntry->segm->ChunkSize);
                    ChunkSize.QuadPart -= pEntry->segm->ChunkSize.QuadPart + sizeof(pEntry->segm->Magic) + sizeof(pEntry->segm->ChunkSize);
                    break;

                case CHAR_UPPER4(CHUNK_MAGIC_ADLR):
                    pEntry->adlr = (KRKR2_XP3_INDEX_CHUNK_ADLR *)pbIndex;
                    pbIndex += pEntry->adlr->ChunkSize.QuadPart + sizeof(pEntry->adlr->Magic) + sizeof(pEntry->adlr->ChunkSize);
                    ChunkSize.QuadPart -= pEntry->adlr->ChunkSize.QuadPart + sizeof(pEntry->adlr->Magic) + sizeof(pEntry->adlr->ChunkSize);
                    break;

                default:
                    ChunkSize.QuadPart -= ((PLARGE_INTEGER)(pbIndex + 4))->QuadPart + 4 + 8;
                    pbIndex += ((PLARGE_INTEGER)(pbIndex + 4))->QuadPart + sizeof(pEntry->adlr->Magic) + sizeof(pEntry->adlr->ChunkSize);
            }
        }

        ULONG Length;

        Length = pEntry->info->FileNameLength;
        Length = min(Length, countof(pEntry->FileName) - 1);
        CopyMemory(pEntry->FileName, pEntry->info->FileName, Length * sizeof(*pEntry->info->FileName));
        pEntry->FileName[Length] = 0;
        pEntry->Offset.QuadPart = pEntry->segm->segm->Offset.QuadPart;

#if defined(CRAZY_XP3)
        
        pEntry->Offset.LowPart ^= TAG4('AMNO');
        pEntry->Offset.QuadPart = -pEntry->Offset.QuadPart;

        pEntry->segm->segm->Offset.QuadPart = pEntry->Offset.QuadPart;

#endif // CRAZY_XP3

        pEntry->Size.QuadPart = pEntry->info->OriginalSize.QuadPart;
        pEntry->CompressedSize.QuadPart = pEntry->info->ArchiveSize.QuadPart;
    }

    return TRUE;
}

BOOL
CKrkr2::
ExtractCallBack(
    MY_FILE_ENTRY_BASE *pEntry,
    UNPACKER_FILE_INFO  FileInfo,
    LPCWSTR             pszOutPath,
    LPCWSTR             pszOutFileName,
    PLarge_Integer      pFileSize
)
{
    ULONG       Size;
    CNtFileDisk file;
    WCHAR       szFile[MAX_PATH];

    UNREFERENCED_PARAMETER(pEntry);
    UNREFERENCED_PARAMETER(pszOutFileName);

    Size = 0;
    if (FileInfo.FileType == UNPACKER_FILE_TYPE_BIN)
    {
        if (NT_SUCCESS(file.Create(pszOutPath)))
        {
            file.Write(FileInfo.BinData.pbBuffer, FileInfo.BinData.BufferSize);
            Size = FileInfo.BinData.BufferSize;
        }
    }
    else if (FileInfo.FileType == UNPACKER_FILE_TYPE_BMP)
    {
        StrCopyW(szFile, pszOutPath);
        chextw(szFile, L".bmp");
        if (NT_SUCCESS(file.Create(szFile)))
        {
            file.Write(FileInfo.ImgData.pbBuffer, FileInfo.ImgData.BufferSize);
            Size = FileInfo.ImgData.BufferSize;
        }
        if (FileInfo.lpExtraData)
        {
            chextw(szFile, L".meta");
            if (NT_SUCCESS(file.Create(szFile)))
            {
                file.Write(FileInfo.lpExtraData, FileInfo.ExtraSize);
                Size += FileInfo.ImgData.BufferSize;
            }
        }
    }
    else
    {
        return FALSE;
    }

    pFileSize->QuadPart = Size;

    return TRUE;
}

#pragma pack(1)

typedef struct
{
    ULONG ChunkSize;
    ULONG Magic;
} PNG_CHUNK_HEADER;

typedef struct
{
    BYTE Magic[8];
    PNG_CHUNK_HEADER Chunk[1];
} IMAGE_PNG_HEADER;

#pragma pack()

#define MAX_FILE_HEADER_SIZE max(max(sizeof(KRKR2_TLG5_HEADER), sizeof(KRKR2_TLG6_HEADER)), sizeof(IMAGE_PNG_HEADER))

BOOL CKrkr2::GetFileData(UNPACKER_FILE_INFO *pFileInfo, const MY_FILE_ENTRY_BASE *pBaseEntry, BOOL KeepRaw)
{
    ULONG           SegmentCount;
    ULONG           BufferSize;
    PBYTE           pbBuffer, pbRead;
    LARGE_INTEGER   ArchiveSize, OriginalSize, DecryptOffset;
    MY_XP3_ENTRY   *pEntry;
    KRKR2_XP3_INDEX_CHUNK_SEGM_DATA *pSegmData;

    enum { TYPE_NORNAL, TYPE_PNG, TYPE_TLG5, TYPE_TLG6 };

    pEntry = (MY_XP3_ENTRY *)pBaseEntry;

    if (pEntry->info->FileNameLength > MAX_PATH)
        return FALSE;

//    if (StrICompareW(pBaseEntry->FileName, L"bgimage/bg12_01.png")) return FALSE;

    if (wcsstr(pBaseEntry->FileName, L"/../"))
        return FALSE;

    ArchiveSize.QuadPart    = pEntry->CompressedSize.QuadPart;
    OriginalSize.QuadPart   = pEntry->Size.QuadPart;
    pSegmData               = pEntry->segm->segm;
    SegmentCount            = (ULONG)(pEntry->segm->ChunkSize.QuadPart / sizeof(*pEntry->segm->segm));

    if (SegmentCount == 0)
        return FALSE;

    if (!segm.Open(file, pEntry->segm))
        return FALSE;

    BufferSize = pEntry->CompressedSize.LowPart;
    pbBuffer = (PBYTE)Alloc(BufferSize);
    if (pbBuffer == NULL)
        return FALSE;

    pbRead = pbBuffer;
    for (ULONG FileSize = BufferSize; FileSize; )
    {
        ULONG BytesToRead, Position;

        if (segm.IsCurrentSegmentCompressed())
        {
            BytesToRead = segm.GetCurrentSegmentOriginalSize();
        }
        else
        {
            BytesToRead = FileSize;
        }

        Position = pbRead - pbBuffer;
        if (Position + BytesToRead > BufferSize)
        {
            BufferSize = BytesToRead + Position;
            pbBuffer = (PBYTE)ReAlloc(pbBuffer, BufferSize);
            if (pbBuffer == NULL)
                goto DEFAULT_FAIL_PROC;

            pbRead = pbBuffer + Position;
        }

        if (!segm.Read(pbRead, BytesToRead, &BytesToRead))
            goto DEFAULT_FAIL_PROC;

        pbRead   += BytesToRead;
        FileSize -= segm.IsCurrentSegmentCompressed() ? segm.GetCurrentSegmentArchiveSize() : BytesToRead;
    }

    DecryptOffset.QuadPart = 0;
    OriginalSize.LowPart = pbRead - pbBuffer;
    DecryptWorker(DecryptOffset, pbBuffer, OriginalSize.LowPart, pEntry->adlr->Hash);

    ULONG Magic;
    Magic = *(PULONG)pbBuffer & 0xFFDFDFDF;
    if (!KeepRaw && (Magic & 0xFFFFFF) == TAG3('TLG'))
    {
        BOOL    r;
        PBYTE   pbTlg;
        ULONG   TlgSize;

        if ((Magic >> 24) != '0')
        {
            pFileInfo->lpExtraData = NULL;
            pbTlg = pbBuffer;
            TlgSize = OriginalSize.LowPart;
        }
        else
        {
            TlgSize = *(PULONG)(pbBuffer + 0xB);
            pbTlg = pbBuffer + 0xF;
            pFileInfo->ExtraSize = OriginalSize.LowPart - TlgSize - 0xF;
            pFileInfo->lpExtraData = Alloc(pFileInfo->ExtraSize);
            CopyMemory(pFileInfo->lpExtraData, pbTlg + TlgSize, pFileInfo->ExtraSize);
        }

        pFileInfo->FileType  = UNPACKER_FILE_TYPE_BMP;
        pFileInfo->FileNum = 1;
        r = DecodeTLG(
                pbTlg,
                TlgSize,
                (PVOID *)&pFileInfo->ImgData.pbBuffer,
                (PULONG)&pFileInfo->ImgData.BufferSize);

        if (!r)
        {
            Free(pFileInfo->ImgData.pbBuffer);
            Free(pFileInfo->lpExtraData);
            goto EXT_AS_BIN;
        }

        Free(pbBuffer);
    }
    else
    {
EXT_AS_BIN:
        pFileInfo->lpExtraData = NULL;
        pFileInfo->FileType = UNPACKER_FILE_TYPE_BIN;
        pFileInfo->BinData.BufferSize = OriginalSize.LowPart;
        pFileInfo->BinData.pbBuffer = pbBuffer;
    }

    return TRUE;

DEFAULT_FAIL_PROC:

    Free(pbBuffer);
    return FALSE;
}

BOOL CKrkr2::DecodeTLG(PVOID lpInBuffer, ULONG uInSize, PVOID *ppOutBuffer, PULONG pOutSize)
{
    PBYTE pbInBuffer;

    if (ppOutBuffer == NULL || pOutSize == NULL)
        return FALSE;

    pbInBuffer = (PBYTE)lpInBuffer;
    *ppOutBuffer = NULL;
    *pOutSize = 0;

    switch (*(PULONG)lpInBuffer)
    {
        case TAG4('TLG5'):
            return DecodeTLG5(lpInBuffer, uInSize, ppOutBuffer, pOutSize);

        case TAG4('TLG6'):
            return DecodeTLG6(lpInBuffer, uInSize, ppOutBuffer, pOutSize);
    }

    return FALSE;
}

BOOL CKrkr2::DecryptWorker(LARGE_INTEGER Offset, PVOID pvBuffer, ULONG BufferSize, ULONG FileHash)
{
    XP3_EXTRACTION_INFO ExtInfo =
    {
        sizeof(ExtInfo),
        Offset,
        pvBuffer,
        BufferSize,
        FileHash
    };

    UNREFERENCED_PARAMETER(ExtInfo);

#if defined (FATE_STAY_NIGHT)
    DecryptFSN(&ExtInfo);
#elif defined(REAL_SISTER) || defined(FATE_HA) || defined(NATSU_ZORA) || defined(TENSHIN) || defined(IMOUTO_STYLE) || defined(MAHOYO)
    DecryptCxdec(&ExtInfo);
#elif defined(SAKURA)
    DecryptSakura(&ExtInfo);
#elif defined(NO_DECRYPT)
    if (m_pfFilter != NULL)
        m_pfFilter(&ExtInfo);
#elif defined(CRAZY_XP3)
    DecryptCxdec(&ExtInfo);
#endif

    return TRUE;
}

#if !defined(NO_DECRYPT)

BOOL CKrkr2::DecryptCxdec(XP3_EXTRACTION_INFO *pInfo)
{
    ULONG SegmLength, Hash;
    LARGE_INTEGER CurrentPos, Offset2;
    XP3_EXTRACTION_INFO Info;

    Info = *pInfo;

#if defined(REAL_SISTER)
    SegmLength = (Info.FileHash & 0x2B2) + 0x2E6;       // real sister
#elif defined(FATE_HA)
    SegmLength = (Info.FileHash & 0x143) + 0x787;       // fate ha
#elif defined(NATSU_ZORA)
    SegmLength = (Info.FileHash & 0x2F5) + 0x6F0;       // natsuzora
#elif defined(TENSHIN)
    SegmLength = (Info.FileHash & 0x167) + 0x498;       // ten shin
#elif defined(IMOUTO_STYLE)
    SegmLength = (Info.FileHash & 0x278) + 0xD7;        // imouto style
#elif defined(CRAZY_XP3)
    SegmLength = (Info.FileHash & 0x233) + 0x466;
#elif defined(MAHOYO)
    SegmLength = (Info.FileHash & 0x22A) + 0x2A2;       // mahoyo
#endif

    Offset2.QuadPart = SegmLength;
    CurrentPos.QuadPart = Info.Offset.QuadPart + Info.BufferSize;
    if (Info.Offset.QuadPart < Offset2.QuadPart && CurrentPos.QuadPart > Offset2.QuadPart)
    {
        Hash = Info.FileHash;
        SegmLength = Offset2.LowPart - Info.Offset.LowPart;
        DecryptCxdecInternal(Hash, Info.Offset, Info.Buffer, SegmLength);

        Hash = (Info.FileHash >> 16) ^ Info.FileHash;
        Info.Buffer = (PBYTE)Info.Buffer + SegmLength;
        DecryptCxdecInternal(Hash, Offset2, Info.Buffer, Info.BufferSize - SegmLength);
    }
    else
    {
        if (Info.Offset.QuadPart < Offset2.LowPart)
        {
            Hash = Info.FileHash;
        }
        else
        {
            Hash = (Info.FileHash >> 16) ^ Info.FileHash;
        }

        DecryptCxdecInternal(Hash, Info.Offset, Info.Buffer, Info.BufferSize);
    }

    return TRUE;
}

BOOL CKrkr2::DecryptCxdecInternal(ULONG Hash, LARGE_INTEGER Offset, PVOID lpBuffer, ULONG BufferSize)
{
    PBYTE           pbBuffer;
    ULONG           Mask, Mask2;
    LARGE_INTEGER   CurrentPos;

    pbBuffer = (PBYTE)lpBuffer;
    Mask = m_cxdec.GetMask(Hash);

    Mask2 = LOWORD(Mask);
    CurrentPos.QuadPart = Offset.QuadPart + BufferSize;

    if (Mask2 >= Offset.QuadPart && Mask2 < CurrentPos.QuadPart)
    {
        *(pbBuffer + Mask2 - Offset.LowPart) ^= Hash >> 16;
    }

    Mask2 = HIWORD(Mask);
    if (Mask2 >= Offset.QuadPart && Mask2 < CurrentPos.QuadPart)
    {
        *(pbBuffer + Mask2 - Offset.LowPart) ^= Hash >> 8;
    }

    memxor(pbBuffer, Hash, BufferSize);

    return TRUE;
}

BOOL CKrkr2::DecryptFSN(XP3_EXTRACTION_INFO *Info)
{
    PBYTE pbBuffer;

    pbBuffer = (PBYTE)Info->Buffer;
    if (Info->Offset.QuadPart <= 0x13 && Info->Offset.QuadPart + Info->BufferSize > 0x13)
    {
        pbBuffer[0x13 - Info->Offset.QuadPart] ^= 1;
    }
    if (Info->Offset.QuadPart <= 0x2EA29 && Info->Offset.QuadPart + Info->BufferSize > 0x2EA29)
    {
        pbBuffer[0x2EA29 - Info->Offset.QuadPart] ^= 3;
    }

    memxor4(pbBuffer, 0x36363636, Info->BufferSize);

    return TRUE;
}

BOOL CKrkr2::DecryptSakura(XP3_EXTRACTION_INFO *Info)
{
    ULONG Hash;
    PBYTE  pbBuffer, pbEnd;

    Hash     = Info->FileHash;
    pbBuffer = (PBYTE)Info->Buffer;
    pbEnd    = pbBuffer + Info->BufferSize;

    while (TEST_BITS((ULONG_PTR)pbBuffer, 0x17) && pbBuffer < pbEnd)
        *pbBuffer++ ^= Hash;

    pbEnd -= 0xF;

    if (pbBuffer < pbEnd)
    {
        ULONG Hash2;

        Hash2 = (BYTE)Hash * 0x01010101;
        do
        {
            *(PULONG)pbBuffer ^= Hash2;
            pbBuffer += 4;
        } while (pbBuffer < pbEnd);
    }

    pbEnd += 0xF;

    memxor(pbBuffer, Hash, pbEnd - pbBuffer);

    return TRUE;
}

#endif // !NO_DECRYPT
