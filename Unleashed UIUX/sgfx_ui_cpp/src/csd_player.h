// =============================================================================
// csd_player.h — a runtime player for the real Sonic Unleashed CSD layout data
// (data/<id>.json, parsed from the game's .yncp). This is the true-1:1 path: it
// evaluates the actual cast hierarchy + animation tracks each frame and emits the
// real game quads — exactly what the runtime game does — instead of hand-authoring.
//
// Faithful port of resolve_nodes.py / the browser CSD player: parent transforms,
// RRGGBBAA colour + gradient/colour tracks, Const/Linear/Hermite interpolation.
// =============================================================================
#pragma once

namespace csd {

bool        Load(const char* id);     // parse data/<id>.json + load its textures; false on failure
void        Draw(double elapsedSec);  // evaluate every scene at the current animation time + emit ui quads
void        SetLoop(bool on);         // true = loop anims (fmod); false (default) = play-once-and-hold (clamp)
void        SetState(const char* tag); // "so"=day / "ev"=night cast state (re-resolves rest poses); ""=clear bias
void        Unload();
const char* LoadedId();               // id currently loaded ("" if none)

} // namespace csd
