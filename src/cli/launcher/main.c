/*
 * KiraWavTar CLI Launcher
 *
 * A tiny statically-linked (.exe) launcher that replaces the .cmd wrapper.
 * It reads a sidecar config file to locate the real kirawavtar-cli.exe,
 * sets the DLL search directory, and forwards execution.
 *
 * Built with /MT (static CRT) so it has zero DLL dependencies.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Suppress Ctrl+C in the launcher — the child process shares our console
   group and receives the signal directly from the OS. We just wait for
   it to finish and forward its exit code. */
static BOOL WINAPI ctrlHandler(DWORD type)
{
    (void)type;
    return TRUE;
}

static void fatal(const wchar_t *msg)
{
    HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
    DWORD mode;
    if (GetConsoleMode(err, &mode)) {
        WriteConsoleW(err, msg, (DWORD)lstrlenW(msg), NULL, NULL);
        WriteConsoleW(err, L"\n", 1, NULL, NULL);
    } else {
        /* Redirected stderr — write UTF-8 */
        int len = WideCharToMultiByte(CP_UTF8, 0, msg, -1, NULL, 0, NULL, NULL);
        if (len > 0) {
            char buf[1024];
            if (len > (int)(sizeof(buf) - 2))
                len = (int)(sizeof(buf) - 2);
            WideCharToMultiByte(CP_UTF8, 0, msg, -1, buf, len, NULL, NULL);
            buf[len - 1] = '\n';
            WriteFile(err, buf, (DWORD)len, NULL, NULL);
        }
    }
    ExitProcess(1);
}

int wmain(void)
{
    /* 1. Determine own directory */
    wchar_t selfPath[MAX_PATH];
    if (!GetModuleFileNameW(NULL, selfPath, MAX_PATH))
        fatal(L"kirawavtar-cli: cannot determine own path");

    wchar_t *lastSlash = NULL;
    for (wchar_t *p = selfPath; *p; ++p) {
        if (*p == L'\\' || *p == L'/')
            lastSlash = p;
    }
    if (lastSlash)
        *lastSlash = L'\0';

    /* 2. Read sidecar config: <selfDir>\kirawavtar-cli.conf */
    wchar_t confPath[MAX_PATH];
    wsprintfW(confPath, L"%s\\kirawavtar-cli.conf", selfPath);

    HANDLE hConf = CreateFileW(confPath, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hConf == INVALID_HANDLE_VALUE)
        fatal(L"kirawavtar-cli: cannot open kirawavtar-cli.conf");

    char confBuf[4096];
    DWORD bytesRead = 0;
    ReadFile(hConf, confBuf, sizeof(confBuf) - 1, &bytesRead, NULL);
    CloseHandle(hConf);
    confBuf[bytesRead] = '\0';

    /* Trim trailing whitespace/newlines */
    while (bytesRead > 0 && (confBuf[bytesRead - 1] == '\r' ||
                              confBuf[bytesRead - 1] == '\n' ||
                              confBuf[bytesRead - 1] == ' '))
        confBuf[--bytesRead] = '\0';

    if (bytesRead == 0)
        fatal(L"kirawavtar-cli: config file is empty");

    /* Convert UTF-8 path to wide string */
    wchar_t binDir[MAX_PATH];
    if (!MultiByteToWideChar(CP_UTF8, 0, confBuf, -1, binDir, MAX_PATH))
        fatal(L"kirawavtar-cli: cannot decode config file path");

    /* 3. Set DLL search directory so the child finds Qt/TBB DLLs */
    SetDllDirectory(binDir);

    /* 4. Build the real exe path */
    wchar_t realExe[MAX_PATH];
    wsprintfW(realExe, L"%s\\kirawavtar-cli.exe", binDir);

    /* 5. Build command line: "<realExe>" + original args after argv[0] */
    LPWSTR cmdLine = GetCommandLineW();
    LPWSTR rest = cmdLine;

    /* Skip past argv[0] (may be quoted) */
    if (*rest == L'"') {
        rest++;
        while (*rest && *rest != L'"')
            rest++;
        if (*rest == L'"')
            rest++;
    } else {
        while (*rest && *rest != L' ' && *rest != L'\t')
            rest++;
    }
    /* rest now points at the arguments after argv[0] */

    wchar_t newCmdLine[32768];
    wsprintfW(newCmdLine, L"\"%s\"%s", realExe, rest);

    /* 6. Install Ctrl+C handler before spawning child */
    SetConsoleCtrlHandler(ctrlHandler, TRUE);

    /* 7. Create the real CLI process with inherited handles */
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(realExe, newCmdLine, NULL, NULL,
                        TRUE, 0, NULL, NULL, &si, &pi))
        fatal(L"kirawavtar-cli: failed to start CLI binary");

    CloseHandle(pi.hThread);

    /* 8. Wait and forward exit code */
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);

    ExitProcess(exitCode);
}
