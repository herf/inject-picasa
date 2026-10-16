// version.dll proxy: strips FILE_FLAG_NO_BUFFERING from CreateFile on network paths
// so Windows read-ahead applies. Real exports forward to version_orig.dll (see version.def).
#include <windows.h>
#include <wctype.h>
#include "MinHook.h"

typedef HANDLE(WINAPI *CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef HANDLE(WINAPI *CreateFileA_t)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
static CreateFileW_t realW;
static CreateFileA_t realA;
static char drive_remote[26];  // 0 unknown, 1 local, 2 remote
static LONG logged;

static BOOL drive_is_remote(WCHAR d) {
    int i = towupper(d) - L'A';
    if (i < 0 || i >= 26) return FALSE;
    if (!drive_remote[i]) {
        WCHAR root[] = {L'A' + i, L':', L'\\', 0};
        drive_remote[i] = GetDriveTypeW(root) == DRIVE_REMOTE ? 2 : 1;
    }
    return drive_remote[i] == 2;
}

static BOOL is_remote(LPCWSTR p) {
    if (!p) return FALSE;
    if (p[0] == L'\\' && p[1] == L'\\') {
        if (p[2] == L'.') return FALSE;                       // \\.\device
        if (p[2] != L'?') return TRUE;                        // \\server\share
        p += 4;                                               // \\?\...
        if (!_wcsnicmp(p, L"UNC\\", 4)) return TRUE;
    }
    return iswalpha(p[0]) && p[1] == L':' && drive_is_remote(p[0]);
}

static DWORD fix(BOOL remote, DWORD flags) {
    if (remote && (flags & FILE_FLAG_NO_BUFFERING)) {
        flags = (flags & ~FILE_FLAG_NO_BUFFERING) | FILE_FLAG_SEQUENTIAL_SCAN;
        if (InterlockedIncrement(&logged) <= 20) OutputDebugStringA("picasa-nocache: buffered open\n");
    }
    return flags;
}

static HANDLE WINAPI hookW(LPCWSTR n, DWORD a, DWORD s, LPSECURITY_ATTRIBUTES sa, DWORD c, DWORD f, HANDLE t) {
    return realW(n, a, s, sa, c, fix(is_remote(n), f), t);
}

static HANDLE WINAPI hookA(LPCSTR n, DWORD a, DWORD s, LPSECURITY_ATTRIBUTES sa, DWORD c, DWORD f, HANDLE t) {
    WCHAR w[MAX_PATH] = L"";
    if (n && (f & FILE_FLAG_NO_BUFFERING)) MultiByteToWideChar(CP_ACP, 0, n, -1, w, MAX_PATH);
    return realA(n, a, s, sa, c, fix(is_remote(w), f), t);
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        if (MH_Initialize() != MH_OK) return TRUE;
        MH_CreateHookApi(L"kernelbase", "CreateFileW", hookW, (LPVOID *)&realW);
        MH_CreateHookApi(L"kernelbase", "CreateFileA", hookA, (LPVOID *)&realA);
        MH_EnableHook(MH_ALL_HOOKS);
        OutputDebugStringA("picasa-nocache: hooks installed\n");
    } else if (reason == DLL_PROCESS_DETACH && !reserved) {
        MH_Uninitialize();
    }
    return TRUE;
}
