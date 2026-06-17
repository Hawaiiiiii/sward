// =============================================================================
// sgfx_sdl.h — the SDL event-dispatch hub (was sdl_listener.h). VERBATIM from the
// recomp: it is pure SDL with no game-class dependency, so it ports as a platform
// service. The menus subclass SDLEventListener to read raw input when out-of-game
// (the installer / message windows). Included ONLY by the SDL-using ui/ files, so
// the rest of the lib doesn't take an SDL dependency.
// =============================================================================
#pragma once

#include <vector>
#include <SDL.h>

class ISDLEventListener
{
public:
    virtual ~ISDLEventListener() = default;
    virtual bool OnSDLEvent(SDL_Event* event) = 0;
};

extern std::vector<ISDLEventListener*>& GetEventListeners();

class SDLEventListener : public ISDLEventListener
{
public:
    SDLEventListener()
    {
        GetEventListeners().emplace_back(this);
    }

    bool OnSDLEvent(SDL_Event* event) override { return false; }
};
