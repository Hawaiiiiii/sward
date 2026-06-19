#pragma once
#include <string>

// Hosts the external Ramses viewer's window INSIDE the shell window (a child-HWND
// embed), so the live 3D car can render in a pane of the operator UI. Deliberately
// guarded and isolated: every step is checked, any failure simply leaves no embed
// (the caller falls back to its snapshot), and Stop() terminates the viewer so its
// window can never linger over other screens. Windows-only; a no-op with no host
// window (e.g. the headless --shot path), so it cannot affect that flow.
namespace viewer_embed {

void SetHostWindow(void* hwnd);                                 // the shell HWND, set once at startup

bool Start(const std::string& exe, const std::string& args);   // spawn the viewer + begin embedding
void Place(int refX0, int refY0, int refX1, int refY1);        // attach + position into the pane (per frame)
void Stop();                                                    // terminate the viewer + detach
bool Active();

} // namespace viewer_embed
