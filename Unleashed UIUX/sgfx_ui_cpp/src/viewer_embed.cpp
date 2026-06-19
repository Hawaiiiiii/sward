// =============================================================================
// viewer_embed.cpp — child-HWND embed of the external Ramses viewer into the shell
// window (see viewer_embed.h). Spawns the viewer with CreateProcess, finds its top
// window by pid, re-parents it as a WS_CHILD of the shell window, and positions it
// in a pane (scaling the reference-space rect to the live client size each frame).
// Every step is guarded; Stop() terminates the process so its window cannot linger.
// =============================================================================
#include "viewer_embed.h"

#include <windows.h>

namespace viewer_embed {
namespace {

HWND      g_host    = nullptr;
HWND      g_child   = nullptr;
HANDLE    g_proc    = nullptr;
DWORD     g_pid     = 0;
ULONGLONG g_startTick = 0;
bool      g_attached = false;

constexpr float REF_W = 1280.0f, REF_H = 720.0f;

struct EnumCtx { DWORD pid; HWND found; };
BOOL CALLBACK EnumProc(HWND h, LPARAM lp) {
    auto* c = reinterpret_cast<EnumCtx*>(lp);
    DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
    if (pid == c->pid && IsWindowVisible(h) && GetWindow(h, GW_OWNER) == nullptr) {
        c->found = h; return FALSE;
    }
    return TRUE;
}

void Cleanup() {
    if (g_proc) { TerminateProcess(g_proc, 0); CloseHandle(g_proc); }
    g_proc = nullptr; g_child = nullptr; g_pid = 0; g_attached = false;
}

} // namespace

void SetHostWindow(void* hwnd) { g_host = reinterpret_cast<HWND>(hwnd); }

bool Start(const std::string& exe, const std::string& args) {
    Stop();
    if (!g_host || !IsWindow(g_host)) return false;

    std::string cmd = "\"" + exe + "\" " + args;
    std::string workdir;
    auto slash = exe.find_last_of("\\/");
    if (slash != std::string::npos) workdir = exe.substr(0, slash);

    STARTUPINFOA si{}; si.cb = sizeof si;
    PROCESS_INFORMATION pi{};
    if (!CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr,
                        workdir.empty() ? nullptr : workdir.c_str(), &si, &pi))
        return false;
    CloseHandle(pi.hThread);
    g_proc = pi.hProcess; g_pid = pi.dwProcessId; g_startTick = GetTickCount64(); g_attached = false;
    return true;
}

void Place(int rx0, int ry0, int rx1, int ry1) {
    if (!g_proc) return;
    if (WaitForSingleObject(g_proc, 0) == WAIT_OBJECT_0) { Cleanup(); return; }   // viewer exited

    if (!g_attached) {
        EnumCtx ctx{ g_pid, nullptr };
        EnumWindows(EnumProc, reinterpret_cast<LPARAM>(&ctx));
        if (ctx.found) {
            g_child = ctx.found;
            SetWindowLongPtr(g_child, GWL_STYLE, WS_CHILD | WS_VISIBLE);
            if (SetParent(g_child, g_host)) g_attached = true;
            else { Cleanup(); return; }
        } else if (GetTickCount64() - g_startTick > 6000) {   // window never showed -> give up
            Cleanup(); return;
        }
    }
    if (g_attached && IsWindow(g_child) && IsWindow(g_host)) {
        RECT cr; if (!GetClientRect(g_host, &cr) || cr.right <= 0 || cr.bottom <= 0) return;
        const float sx = cr.right / REF_W, sy = cr.bottom / REF_H;
        MoveWindow(g_child, (int)(rx0 * sx), (int)(ry0 * sy),
                   (int)((rx1 - rx0) * sx), (int)((ry1 - ry0) * sy), TRUE);
    }
}

void Stop()  { Cleanup(); }
bool Active() { return g_proc != nullptr; }

} // namespace viewer_embed
