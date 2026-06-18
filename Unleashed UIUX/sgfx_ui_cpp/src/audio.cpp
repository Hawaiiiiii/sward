// =============================================================================
// audio.cpp — SDL2 one-shot SFX mixer (see audio.h). Each clip is loaded via
// SDL_LoadWAV and converted ONCE to the device format (stereo F32 @48kHz) with an
// SDL_AudioStream (this also downmixes the 5.1 source). A fixed pool of voices is
// summed in the audio callback. Play() latches a voice under the device lock.
// Everything degrades gracefully: if the device can't open or a clip is missing,
// the subsystem stays disabled and Play() is a no-op.
// =============================================================================
#include "audio.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

// stb_vorbis (thirdparty/stb/stb_vorbis.c is added to the build) — whole-file OGG decode.
extern "C" int stb_vorbis_decode_filename(const char* filename, int* channels, int* sample_rate, short** output);

namespace audio {
namespace {

constexpr int   DST_RATE = 48000;
constexpr int   DST_CH   = 2;
constexpr int   MAX_VOICES = 12;

const char* const CLIP_FILE[SFX_COUNT] = {
    "assets/sfx/sys_actstg_pausecursor.wav",
    "assets/sfx/sys_actstg_pausedecide.wav",
    "assets/sfx/sys_actstg_pausecansel.wav",
    "assets/sfx/sys_actstg_pausewinopen.wav",
    "assets/sfx/sys_actstg_pausewinclose.wav",
    "assets/sfx/sys_actstg_statescharachange.wav",
    "assets/sfx/sys_actstg_stateslevup.wav",
    "assets/sfx/sys_actstg_stateserror.wav",
};

std::vector<float> g_clip[SFX_COUNT];   // interleaved stereo F32 @48k

struct Voice { const std::vector<float>* clip = nullptr; size_t pos = 0; };
Voice g_voices[MAX_VOICES];

std::vector<float> g_music;        // active looping BGM (interleaved stereo F32 @48k)
size_t g_musicPos = 0;
size_t g_musicLoopIdx = 0;         // wrap target (stereo-sample index) = loop-start
bool   g_musicOn  = false;
float  g_musicGain = 0.5f;
std::map<std::string, std::vector<float>> g_musicCache;   // decoded tracks, keyed by path (instant switching)

SDL_AudioDeviceID g_dev = 0;

void SDLCALL Mix(void*, Uint8* stream, int len) {
    float* out = reinterpret_cast<float*>(stream);
    const int frames = len / int(sizeof(float) * DST_CH);
    SDL_memset(stream, 0, size_t(len));
    for (Voice& v : g_voices) {
        if (!v.clip) continue;
        const std::vector<float>& p = *v.clip;
        const size_t n = p.size();
        for (int f = 0; f < frames; ++f) {
            if (v.pos + 1 >= n) { v.clip = nullptr; break; }
            out[f * 2 + 0] += p[v.pos + 0];
            out[f * 2 + 1] += p[v.pos + 1];
            v.pos += 2;
        }
    }
    if (g_musicOn && !g_music.empty()) {              // looping background music
        const size_t n = g_music.size();
        for (int f = 0; f < frames; ++f) {
            if (g_musicPos + 1 >= n) g_musicPos = g_musicLoopIdx;  // wrap to loop-start
            out[f * 2 + 0] += g_music[g_musicPos + 0] * g_musicGain;
            out[f * 2 + 1] += g_music[g_musicPos + 1] * g_musicGain;
            g_musicPos += 2;
        }
    }
    const int total = frames * DST_CH;
    for (int i = 0; i < total; ++i) {                 // hard clamp the sum
        if (out[i] >  1.0f) out[i] =  1.0f;
        else if (out[i] < -1.0f) out[i] = -1.0f;
    }
}

bool LoadClip(int i) {
    SDL_AudioSpec spec; Uint8* buf = nullptr; Uint32 blen = 0;
    if (!SDL_LoadWAV(CLIP_FILE[i], &spec, &buf, &blen)) {
        fprintf(stderr, "[audio] load '%s': %s\n", CLIP_FILE[i], SDL_GetError());
        return false;
    }
    SDL_AudioStream* st = SDL_NewAudioStream(spec.format, spec.channels, spec.freq,
                                             AUDIO_F32SYS, DST_CH, DST_RATE);
    bool ok = st && SDL_AudioStreamPut(st, buf, int(blen)) == 0 && SDL_AudioStreamFlush(st) == 0;
    SDL_FreeWAV(buf);
    if (!ok) { if (st) SDL_FreeAudioStream(st); return false; }
    const int avail = SDL_AudioStreamAvailable(st);
    if (avail > 0) {
        g_clip[i].resize(size_t(avail) / sizeof(float));
        SDL_AudioStreamGet(st, g_clip[i].data(), avail);
    }
    SDL_FreeAudioStream(st);
    return !g_clip[i].empty();
}

} // namespace

bool Init() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "[audio] init audio subsystem: %s\n", SDL_GetError());
        return false;
    }
    SDL_AudioSpec want; SDL_zero(want);
    want.freq = DST_RATE; want.format = AUDIO_F32SYS; want.channels = DST_CH;
    want.samples = 1024; want.callback = Mix;
    SDL_AudioSpec have;
    g_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);  // 0 = device must match `want`
    if (!g_dev) {
        fprintf(stderr, "[audio] open device: %s\n", SDL_GetError());
        return false;
    }
    int loaded = 0;
    for (int i = 0; i < SFX_COUNT; ++i) if (LoadClip(i)) ++loaded;
    SDL_PauseAudioDevice(g_dev, 0);   // start playback
    fprintf(stderr, "[audio] ready (%d/%d clips, %dHz F32 stereo)\n", loaded, int(SFX_COUNT), have.freq);
    return true;
}

void Shutdown() {
    if (g_dev) { SDL_CloseAudioDevice(g_dev); g_dev = 0; }
    for (auto& c : g_clip) { c.clear(); c.shrink_to_fit(); }
    g_musicOn = false; g_music.clear(); g_music.shrink_to_fit(); g_musicCache.clear();
}

static bool EndsWith(const char* s, const char* suffix) {
    size_t ls = strlen(s), lf = strlen(suffix);
    if (lf > ls) return false;
    for (size_t i = 0; i < lf; ++i) {
        char a = s[ls - lf + i], b = suffix[i];
        if (a >= 'A' && a <= 'Z') a = char(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = char(b - 'A' + 'a');
        if (a != b) return false;
    }
    return true;
}

bool PlayMusic(const char* path, int loopStartFrame) {
    if (!g_dev) return false;
    auto it = g_musicCache.find(path);
    if (it == g_musicCache.end()) {                        // decode once, then cache for instant re-use
        SDL_AudioStream* st = nullptr;
        if (EndsWith(path, ".wav")) {                      // WAV (vgmstream-decoded game audio)
            SDL_AudioSpec sp; Uint8* buf = nullptr; Uint32 bl = 0;
            if (!SDL_LoadWAV(path, &sp, &buf, &bl)) { fprintf(stderr, "[audio] music load '%s': %s\n", path, SDL_GetError()); return false; }
            st = SDL_NewAudioStream(sp.format, sp.channels, sp.freq, AUDIO_F32SYS, DST_CH, DST_RATE);
            bool ok = st && SDL_AudioStreamPut(st, buf, int(bl)) == 0 && SDL_AudioStreamFlush(st) == 0;
            SDL_FreeWAV(buf);
            if (!ok) { if (st) SDL_FreeAudioStream(st); return false; }
        } else {                                           // OGG via stb_vorbis
            short* pcm = nullptr; int ch = 0, rate = 0;
            int frames = stb_vorbis_decode_filename(path, &ch, &rate, &pcm);
            if (frames <= 0 || !pcm || ch < 1) { if (pcm) free(pcm); fprintf(stderr, "[audio] music decode '%s' failed\n", path); return false; }
            st = SDL_NewAudioStream(AUDIO_S16SYS, ch, rate, AUDIO_F32SYS, DST_CH, DST_RATE);
            bool ok = st && SDL_AudioStreamPut(st, pcm, frames * ch * int(sizeof(short))) == 0 && SDL_AudioStreamFlush(st) == 0;
            free(pcm);
            if (!ok) { if (st) SDL_FreeAudioStream(st); return false; }
        }
        std::vector<float> conv;
        int avail = SDL_AudioStreamAvailable(st);
        if (avail > 0) { conv.resize(size_t(avail) / sizeof(float)); SDL_AudioStreamGet(st, conv.data(), avail); }
        SDL_FreeAudioStream(st);
        if (conv.empty()) return false;
        it = g_musicCache.emplace(std::string(path), std::move(conv)).first;
    }
    std::vector<float> active = it->second;                // copy outside the audio lock (cheap vs decode)
    size_t loopIdx = size_t(loopStartFrame) * 2;
    if (loopIdx + 1 >= active.size()) loopIdx = 0;
    SDL_LockAudioDevice(g_dev);
    g_music.swap(active); g_musicPos = 0; g_musicLoopIdx = loopIdx; g_musicOn = true;
    SDL_UnlockAudioDevice(g_dev);
    fprintf(stderr, "[audio] music '%s' (loop frame %d, %zu tracks cached)\n", path, loopStartFrame, g_musicCache.size());
    return true;
}

void StopMusic() {
    if (!g_dev) return;
    SDL_LockAudioDevice(g_dev);
    g_musicOn = false;
    SDL_UnlockAudioDevice(g_dev);
}

void Play(Sfx s) {
    if (!g_dev || s < 0 || s >= SFX_COUNT || g_clip[s].empty()) return;
    SDL_LockAudioDevice(g_dev);
    int idx = -1;
    for (int i = 0; i < MAX_VOICES; ++i) if (!g_voices[i].clip) { idx = i; break; }
    if (idx < 0) idx = 0;   // all busy — steal voice 0
    g_voices[idx].clip = &g_clip[s];
    g_voices[idx].pos  = 0;
    SDL_UnlockAudioDevice(g_dev);
}

} // namespace audio
