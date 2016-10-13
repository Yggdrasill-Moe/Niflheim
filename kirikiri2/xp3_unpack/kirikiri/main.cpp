#define UNICODE
#include <Windows.h>
#include <string>
#include <vector>
#include "types.h"
#include "error.h"
#include "resource.h"
#include "xp3.h"
#include <strsafe.h>


using std::vector;
using std::wstring;


static int THREAD_NUM = 2;


struct thread_param
{
    HANDLE hEvent;
    HANDLE hThread;
    UNCOM unCom;
    bool thread_exit;
    char ChooseGame[32];
    vector<wstring> queue;
};

HWND             hEdit, hCombo;
HANDLE             hThread;
CRITICAL_SECTION cs;

extern UNCOM unCom;

DWORD WINAPI Thread(PVOID pv);
void OnDropFiles(HDROP hDrop, HWND hwnd, thread_param* ptp);
BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);


#define MESSAGE(x) MessageBox(0, x, L"提示", MB_ICONINFORMATION|MB_OK)


void AppendMsg(const wchar_t *szBuffer)
{
    static DWORD dwPos;
    if (0 == szBuffer)
    {
        dwPos = 0;
        SendMessage(hEdit, EM_SETSEL, 0, -1);
        SendMessage(hEdit, EM_REPLACESEL, FALSE, 0);
        return;
    }
    SendMessage(hEdit, EM_SETSEL, (WPARAM)&dwPos, (LPARAM)&dwPos);
    SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)szBuffer);
    SendMessage(hEdit, EM_GETSEL, 0, (LPARAM)&dwPos);
    return;
}

void SetThreadNumByCPUCore(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    if (!GetLastError())
        THREAD_NUM = si.dwNumberOfProcessors - 1;
    else
        THREAD_NUM = 2;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR pCmdLine, int)
{
    SetThreadNumByCPUCore();
    DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, DlgProc, 0);
    return 0;
}


BOOL CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static thread_param *tp = new thread_param[THREAD_NUM];
    static HMODULE        hZlib;
    static bool            thread_paused;
    UNCOM                tmp = 0;
    TCHAR                szBuffer[MAX_PATH];

    switch (msg)
    {
    case WM_INITDIALOG:
        hZlib = LoadLibrary(L"zlib.dll");
        if (!hZlib)
        {
            MESSAGE(L"缺少zlib.dll文件！");
            EndDialog(hDlg, 0);
        }
        
        if (!(tmp = (UNCOM)GetProcAddress(hZlib, "uncompress")))
        {
            MESSAGE(L"解码函数获取失败！");
            EndDialog(hDlg, 0);
        }

//----------------------------------------------------------

        hEdit = GetDlgItem(hDlg, IDC_EDIT);
        SendMessage(hEdit, EM_LIMITTEXT, -1, 0);
        AppendMsg(L"选择对应游戏后拖放xp3文件到此处...\r\n");

//----------------------------------------------------------

        hCombo = GetDlgItem(hDlg, IDC_COMBO);
        for (int i=IDS_STRING099; i<=IDS_STRING120; ++i)    // 改为对应游戏(字符串)数量
        {
            LoadString((HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE), i, szBuffer, MAX_PATH);
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)szBuffer);
        }

//----------------------------------------------------------

        if (!tp)
        {
            AppendMsg(L"内存分配错误！\r\n");
            EndDialog(hDlg, 0);
        }
        for (int i=0; i<THREAD_NUM; ++i)
        {
            if (!(tp[i].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
            {
                MESSAGE(L"事件初始化错误！\r\n");
                EndDialog(hDlg, 0);
            }
            if (!(tp[i].hThread = CreateThread(NULL, 0, Thread, &tp[i], 0, NULL)))
            {
                MESSAGE(L"线程创建失败！\r\n");
                EndDialog(hDlg, 0);
            }
            
            tp[i].thread_exit = false;
            tp[i].unCom = tmp;
            unCom = tmp;
        }
        InitializeCriticalSection(&cs);
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_PAUSE)
        {
            if (thread_paused)
            {
                for (int i=0; i<THREAD_NUM; ++i)
                    ResumeThread(tp[i].hThread);
                SetDlgItemText(hDlg, IDC_PAUSE, L"暂停(&P)");
            }
            else
            {
                for (int i=0; i<THREAD_NUM; ++i)
                    SuspendThread(tp[i].hThread);
                SetDlgItemText(hDlg, IDC_PAUSE, L"继续(&R)");
            }
            thread_paused ^= 1;
        }
        return TRUE;
                
    case WM_DROPFILES:
        OnDropFiles((HDROP)wParam, hDlg, tp);
        return TRUE;

    case WM_CLOSE:
        for (int i=0; i<THREAD_NUM; ++i)
        {
            tp[i].thread_exit = true;
            SetEvent(tp[i].hEvent);
        }
        FreeLibrary(hZlib);
        EndDialog(hDlg, 0);
        return TRUE;
    }
    return FALSE;
}


typedef int (*CallBack)(struct CB* pcb, PTSTR path);


struct CB
{
    int cnt;
    thread_param* ptp;
    wchar_t *filter;
};


int callback(struct CB* pcb, wchar_t *path)
{
    int len = wcslen(path);
    while(len>=0 && path[len-1] != '.') --len;

    if (!pcb->filter || !wcscmp(&path[len], pcb->filter))
    {
        EnterCriticalSection(&cs);
        {
            pcb->ptp[pcb->cnt].queue.push_back(wstring(path));
            if (pcb->ptp[pcb->cnt].queue.size() == 1)
                SetEvent(pcb->ptp[pcb->cnt].hEvent);
        }
        LeaveCriticalSection(&cs);

        pcb->cnt = (pcb->cnt+1) % THREAD_NUM;    // 转下一个线程
    }
    return 0;
}


int ExpandDirectory(PTSTR lpszPath, CallBack callback, struct CB* pcb)
{
    wchar_t            lpFind[MAX_PATH], lpSearch[MAX_PATH], lpPath[MAX_PATH];
    HANDLE            hFindFile;
    WIN32_FIND_DATA FindData;
    int                cnt = 0;

    // Path\*.*
    StringCchCopy(lpPath,   MAX_PATH, lpszPath);
    StringCchCat (lpPath,   MAX_PATH, L"\\");
    StringCchCopy(lpSearch, MAX_PATH, lpPath);
    StringCchCat (lpSearch, MAX_PATH, L"*.*");

    if (INVALID_HANDLE_VALUE != (hFindFile = FindFirstFile(lpSearch, &FindData)))
    {
        do{
            // 完整文件名
            StringCchCopy(lpFind, MAX_PATH, lpPath);
            StringCchCat(lpFind, MAX_PATH, FindData.cFileName);

            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (FindData.cFileName[0] != '.')
                    ExpandDirectory(lpFind, callback, pcb);
            }
            else callback(pcb, lpFind);
        }while(FindNextFile(hFindFile, &FindData));
        FindClose(hFindFile);
        return 0;
    }
    return -2;
}


DWORD AppendFileToQueue(wchar_t *pInBuf, CallBack callback, struct CB *pcb)
{    
    if (FILE_ATTRIBUTE_DIRECTORY == GetFileAttributes(pInBuf))
        ExpandDirectory(pInBuf, callback, pcb);
    else callback(pcb, pInBuf);

    return 0;
}


void OnDropFiles(HDROP hDrop, HWND hDlg, thread_param* ptp)
{
    struct CB cb;
    wchar_t FileName[MAX_PATH];
    char szBuffer[128];
    DWORD i, FileNum;

    cb.cnt      = 0;
    cb.filter = 0;
    cb.ptp      = ptp;

    u32 idx = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (idx == CB_ERR)
    {
        MessageBox(hDlg, L"请先选择对应的游戏", L"提示", MB_ICONINFORMATION);
        return;
    }


    LoadStringA((HINSTANCE)GetWindowLong(hDlg, GWL_HINSTANCE), idx+499, szBuffer, 128);
    for (int i=0; i<THREAD_NUM; ++i)
        StringCchCopyA(ptp[i].ChooseGame, sizeof(ptp[i].ChooseGame), szBuffer);


    FileNum  = DragQueryFile(hDrop, -1, NULL, 0);

    for (i=0; i<FileNum; ++i)
    {
        DragQueryFile(hDrop, i, (LPTSTR)FileName, MAX_PATH);
        AppendFileToQueue(FileName, callback, &cb);
    }
    DragFinish(hDrop);

    return;
}

DWORD WINAPI Thread(PVOID pv)
{
    DWORD          dwNowProcess = 0;
    thread_param *ptp = (thread_param*) pv;
    wchar_t          cur_dir[MAX_PATH], CurrentFile[MAX_PATH];
    
    
    while (1)
    {
        WaitForSingleObject(ptp->hEvent, INFINITE);
        if (ptp->thread_exit) break;


        EnterCriticalSection(&cs);
        {
            StringCchCopy(CurrentFile, MAX_PATH, (wchar_t *)ptp->queue.back().c_str());
            ptp->queue.pop_back();            
        }
        LeaveCriticalSection(&cs);


        StringCchCopy(cur_dir, MAX_PATH, CurrentFile);

        DWORD l = wcslen(cur_dir);
        while(l && cur_dir[l-1] != '\\') --l;
        cur_dir[l] = '\0';

        StringCchCat(cur_dir, MAX_PATH, L"[extract] ");
        StringCchCat(cur_dir, MAX_PATH, &CurrentFile[l]);
        CreateDirectory(cur_dir, 0);
        

        XP3Entrance(CurrentFile, cur_dir, ptp->ChooseGame);
        

        if (0 == ptp->queue.size())
                ResetEvent(ptp->hEvent);
        
    }
    return 0;
}