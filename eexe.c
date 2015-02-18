/*  ======================================================================

		Editor Executer			version 1.1.3

	Copyright (c) 1998,1999,2003,2008,2013,2014 by U.Nakamura
						All rights reserved.

	Redistribution and use in source and binary forms, with or
	without modification, are permitted provided that the following
	conditions are met:

	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above
	   copyright notice, this list of conditions and the following
	  disclaimer in the documentation and/or other materials provided
	  with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
	TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
	PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
	OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
	USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
	AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
	IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
	THE POSSIBILITY OF SUCH DAMAGE.

    ======================================================================  */


/*  ----------------------------------------------------------------------
	Include Headers
    ----------------------------------------------------------------------  */

#include <windows.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eexe.h"


/*  ----------------------------------------------------------------------
	Macro Definitions
    ----------------------------------------------------------------------  */

#define LINES 100

#ifndef ATTACH_PARENT_PROCESS
#define ATTACH_PARENT_PROCESS ((DWORD)-1)
#endif


/*  ----------------------------------------------------------------------
	Grobal Variables
    ----------------------------------------------------------------------  */

HINSTANCE g_hInst;			/* カレントのインスタンス */
char szClass[] = "eexe";		/* ウィンドウクラス */
HANDLE hProcess = INVALID_HANDLE_VALUE;	/* 子プロセスハンドル */
char *glpCmdLine;			/* コマンドライン */
BOOL fWait = FALSE;			/* 待ちフラグ */


/*  ----------------------------------------------------------------------
	Functions
    ----------------------------------------------------------------------  */

/*  ------------------------------------
	ShowError
	エラー表示
    ------------------------------------  */
static void
ShowError(char *fmt, ...)
{
    char buf[1024];
    va_list list;
    va_start(list, fmt);
    _vsnprintf(buf, sizeof(buf) - 1, fmt, list);
    va_end(list);
    buf[sizeof(buf) - 1] = '\0';
    MessageBox(NULL, buf, szClass, MB_OK | MB_ICONERROR);
}

/*  ------------------------------------
	GetPipeInput
	リダイレクト入力
    ------------------------------------  */
static BOOL
GetPipeInput(char *pszTmpname)
{
    char szBuf[1024];
    FILE *fp;
    int size;
    int fd;

    fd = fileno(stdin);
    errno = 0;
    if (_isatty(fd) || errno == EBADF) {
	return FALSE;
    }
    _setmode(fd, _O_BINARY);
    if (GetTempFileName(".", "eexe", 0, pszTmpname)) {
	if ((fp = fopen(pszTmpname, "wb")) != NULL) {
	    while ((size = fread(szBuf, 1, sizeof(szBuf), stdin)) > 0) {
		fwrite(szBuf, 1, size, fp);
	    }
	    fclose(fp);
	    return TRUE;
	}
    }

    return FALSE;
}

/*  ------------------------------------
	GetConsoleText
	コンソールバッファ取得
    ------------------------------------  */
static BOOL
GetConsoleText(char *pszTmpname)
{
    HANDLE hStd = INVALID_HANDLE_VALUE;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    CHAR_INFO *phci;
    FILE *fp;
    BOOL start;
    int top;
    static BOOL (WINAPI *pAttachConsole)(DWORD) = NULL;
    static const DWORD table[] = { STD_OUTPUT_HANDLE, STD_ERROR_HANDLE };
    int i;

    if (!pAttachConsole) {
	pAttachConsole =
	    (BOOL (WINAPI *)(DWORD))GetProcAddress(GetModuleHandle("kernel32"),
						   "AttachConsole");
	if (!pAttachConsole) {
	    return FALSE;
	}
    }
    if (!pAttachConsole(ATTACH_PARENT_PROCESS)) {
	return FALSE;
    }

    for (i = 0; i < sizeof(table)/sizeof(table[0]); i++) {
	hStd = GetStdHandle(table[i]);
	if (hStd == INVALID_HANDLE_VALUE) {
	    continue;
	}

	if (GetConsoleScreenBufferInfo(hStd, &csbi)) {
	    break;
	}

	hStd = INVALID_HANDLE_VALUE;
    }
    if (hStd == INVALID_HANDLE_VALUE) {
	ShowError("Cannot get console screen buffer info for stdout/stderr: %d",
		  GetLastError());
	return FALSE;
    }

    phci = malloc((csbi.dwSize.X * LINES) * sizeof(CHAR_INFO));
    if (!phci) {
	ShowError("Not enough memory");
	return FALSE;
    }

    if (!GetTempFileName(".", "eexe", 0, pszTmpname)) {
	return FALSE;
    }
    fp = fopen(pszTmpname, "w");
    if (!fp) {
	ShowError("Cannot open temporary file: %s", pszTmpname);
	free(phci);
	return FALSE;
    }
    start = FALSE;
    for (top = 0; top < csbi.dwSize.Y; top += LINES) {
	SMALL_RECT sr;
	int lines;
	COORD crdSize;
	COORD crdCoord;
	int x, y;

	sr.Left = 0;
	sr.Top = top;
	sr.Right = csbi.dwSize.X - 1;
	lines = LINES;
	if (top + LINES > csbi.dwSize.Y) {
	    lines = csbi.dwSize.Y - top;
	}
	sr.Bottom = top + lines - 1;

	crdSize.X = csbi.dwSize.X;
	crdSize.Y = lines;

	crdCoord.X = 0;
	crdCoord.Y = 0;

	if (!ReadConsoleOutput(hStd, phci, crdSize, crdCoord, &sr)) {
	    ShowError("Cannot read from console screen buffer: %d",
		      GetLastError());
	    fclose(fp);
	    free(phci);
	    return FALSE;
	}

	for (y = 0; y < lines; ++y) {
	    char *buf = _alloca(csbi.dwSize.X + 1);
	    if (!buf) {
		ShowError("Not enough memory");
		fclose(fp);
		free(phci);
		return FALSE;
	    }
	    for (x = 0; x < csbi.dwSize.X; ++x) {
		buf[x] = phci[x + y * csbi.dwSize.X].Char.AsciiChar;
	    }
	    buf[x] = '\0';
	    for (x = csbi.dwSize.X - 1; x >= 0; --x) {
		if (isspace(buf[x])) {
		    buf[x] = '\0';
		}
		else {
		    break;
		}
	    }
	    if (!start && buf[0]) {
		start = TRUE;
	    }
	    if (start) {
		fprintf(fp, "%s\n", buf);
	    }
	}
    }
    fclose(fp);

    free(phci);

    return TRUE;
}

/*  ------------------------------------
	GetRegValue
	レジストリ値取得
    ------------------------------------  */
static BOOL
GetRegValue(HKEY hKey, const char *pszValue, const char *pszName, char *pszBuf,
	    DWORD dwSize)
{
    HKEY hSubKey;
    DWORD dwRet;
    DWORD dwType;
    DWORD dwRetSize = dwSize;

    if ((dwRet = RegOpenKeyEx(hKey, pszValue, 0, KEY_READ,
			      &hSubKey)) != ERROR_SUCCESS) {
	return FALSE;
    }
    if ((dwRet = RegQueryValueEx(hSubKey, pszName, NULL, &dwType, pszBuf,
				 &dwRetSize)) != ERROR_SUCCESS) {
	RegCloseKey(hSubKey);
	return FALSE;
    }
    RegCloseKey(hSubKey);

    if (dwType == REG_EXPAND_SZ) {
	char *p = _alloca(strlen(pszBuf) + 1);
	if (!p) {
	    return FALSE;
	}
	strcpy(p, pszBuf);
	ExpandEnvironmentStrings(p, pszBuf, dwSize);
    }

    return TRUE;
}

/*  ------------------------------------
	CreateChildProcess
	対象プロセス生成
    ------------------------------------  */
static HANDLE
CreateChildProcess(LPSTR lpCmdLine)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char szReg[1024] = "";
    char *pszCmd;
    char *p;

    if (!GetRegValue(HKEY_CURRENT_USER, "Software\\U'sa\\eexe", NULL, szReg,
		     sizeof(szReg))) {
	char szBuf[1024];
	if (GetRegValue(HKEY_CLASSES_ROOT, ".txt", NULL, szBuf,
			sizeof(szBuf))) {
	    char szBuf2[1024];
	    _snprintf(szBuf2, sizeof(szBuf2), "%s\\shell\\open\\command",
		      szBuf);
	    szReg[sizeof(szReg) - 1] = '\0';
	    GetRegValue(HKEY_CLASSES_ROOT, szBuf2, NULL, szReg, sizeof(szReg));
	}
    }
    if (!szReg[0]) {
	ShowError("No editor is registered");
	return INVALID_HANDLE_VALUE;
    }
    pszCmd = _alloca(strlen(szReg) + strlen(lpCmdLine) + 1 + 1);
    if (!pszCmd) {
	ShowError("Not enough memory");
	return INVALID_HANDLE_VALUE;
    }

    strcpy(pszCmd, szReg);
    if ((p = strstr(pszCmd, "\"%1\"")) != NULL && *lpCmdLine == '"') {
	memmove(p+strlen(lpCmdLine), p+4, strlen(p+4)+1);
	memcpy(p, lpCmdLine, strlen(lpCmdLine));
    }
    else if ((p = strstr(pszCmd, "%1")) != NULL) {
	memmove(p+strlen(lpCmdLine), p+2, strlen(p+2)+1);
	memcpy(p, lpCmdLine, strlen(lpCmdLine));
    }
    else {
	strcat(pszCmd, " ");
	strcat(pszCmd, lpCmdLine);
    }

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));
    if (!CreateProcess(NULL, pszCmd, NULL, NULL, FALSE, 0, NULL, NULL,
		       &si, &pi)) {
	ShowError("Cannot invoke process: [%s] %d", pszCmd, GetLastError());
	return INVALID_HANDLE_VALUE;
    }
    CloseHandle(pi.hThread);
    WaitForInputIdle(pi.hProcess, 5000);

    return pi.hProcess;
}


/*  ------------------------------------
	MainWndProc
	メインウィンドウプロシージャ
    ------------------------------------  */
LRESULT CALLBACK
MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DWORD dwResult;

    switch (uMsg) {
      case WM_CREATE:
	SetWindowText(hWnd, glpCmdLine);
	break;

      case WM_TIMER:
	if (hProcess != INVALID_HANDLE_VALUE) {
	    if (fWait) {
		if (!GetExitCodeProcess(hProcess, &dwResult) ||
		    dwResult != STILL_ACTIVE) {
		    PostQuitMessage(0);
		}
		else {
		    SetTimer(hWnd, 0, 250, NULL);
		}
	    }
	    else {			/* !fWait */
		PostQuitMessage(0);
	    }
	}
	return 0;

      case WM_DESTROY:
	PostQuitMessage(0);
	break;

      default:
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}


/*  ------------------------------------
	InitApplication
	アプリケーションの初期化
    ------------------------------------  */
static BOOL
InitApplication(void)
{
    WNDCLASS wc;

    /* メインウィンドウのクラスの定義 */
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = (WNDPROC)MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szClass;

    return RegisterClass(&wc);
}


/*  ------------------------------------
	InitInstance
	インスタンスの初期化
    ------------------------------------  */
static HWND
InitInstance(int nShow)
{
    HWND hWnd;

    /* ウィンドウクラス登録 */
    if (!InitApplication()) {
	return NULL;
    }

    /* メインウィンドウの作成 */
    hWnd = CreateWindowEx(0,
			  szClass,
			  szClass,
			  WS_OVERLAPPEDWINDOW,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  NULL,
			  NULL,
			  g_hInst,
			  NULL);

    if (!hWnd) {
	return NULL;
    }

    /* ウィンドウの表示 */
    ShowWindow(hWnd, nShow);
    InvalidateRect(hWnd, NULL, TRUE);
    UpdateWindow(hWnd);

    return hWnd;
}


/*  ------------------------------------
	WinMain
    ------------------------------------  */
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
	int nCmdShow)
{
    OSVERSIONINFO osVer;		/* Win32 バージョン取得構造体 */
    DWORD dwOS;
    int nExec;				/* 実行可能かどうか */
    HWND hWnd = NULL;
    MSG msg;
    HWND hParent;
    char *p;
    BOOL fTemp;
    char szTmp[1024];
    char szBuf[1024];

    /* Win32 バージョンチェック */
    osVer.dwOSVersionInfoSize = sizeof(osVer);
    if (!GetVersionEx(&osVer)) {
	return 1;
    }

    dwOS = osVer.dwPlatformId;
    switch (dwOS) {
      case VER_PLATFORM_WIN32s:		/* Win32s on Windows 3.1 */
	nExec = FALSE;
	break;
      case VER_PLATFORM_WIN32_WINDOWS:	/* Win32 on Windows 95 */
	nExec = FALSE;
	break;
      case VER_PLATFORM_WIN32_NT:	/* Windows NT */
	if (osVer.dwMajorVersion < 5) {
	    nExec = FALSE;
	}
	else {
	    nExec = TRUE;
	}
	break;
      default:
	nExec = FALSE;
	break;
    }
    if (!nExec) {
	ShowError("This program only runs under XP or later.");
	return 1;
    }

    strncpy(szBuf, lpCmdLine, sizeof(szBuf));
    szBuf[sizeof(szBuf) - 1] = '\0';
    glpCmdLine = szBuf;
    if ((p = strstr(glpCmdLine, "--wait")) != NULL) {
	fWait = TRUE;
	memmove(p, p + 6, strlen(p + 6) + 1);
    }

    fTemp = GetPipeInput(szTmp);
    while (TRUE) {
	if (fTemp) {
	    _snprintf(szBuf, sizeof(szBuf), "%s %s", glpCmdLine, szTmp);
	    szBuf[sizeof(szBuf) - 1] = '\0';
	    glpCmdLine = szBuf;
	}
	while (*glpCmdLine && strchr(" \t", *glpCmdLine)) {
	    glpCmdLine++;
	}

	if (!*glpCmdLine) {
	    fTemp = GetConsoleText(szTmp);
	    if (fTemp) {
		continue;
	    }
	}

	break;
    }
    if (!fTemp && !*glpCmdLine) {
	ShowError("Nothing to do.");
	return 1;
    }

    if (!(hWnd = InitInstance(SW_HIDE))) {
	ShowError("Cannot create window: %d", GetLastError());
	return 1;
    }
    hParent = GetParent(hWnd);

    /* 子プロセス生成 */
    hProcess = CreateChildProcess(glpCmdLine);
    if (hProcess == INVALID_HANDLE_VALUE) {
	/* エラーは表示済み */
	remove(glpCmdLine);
	PostQuitMessage(1);
	return 1;
    }
    SetTimer(hWnd, 0, 250, NULL);

    /* WM_QUIT が来るまでメッセージループを続ける */
    while (GetMessage(&msg, NULL, 0, 0)) {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }

    if (hProcess != INVALID_HANDLE_VALUE) {
	CloseHandle(hProcess);
    }

    SetForegroundWindow(hParent);

    if (fTemp) {
	remove(glpCmdLine);
    }

    return msg.wParam;
}


/* End of File */
