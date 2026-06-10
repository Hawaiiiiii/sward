// =============================================================================
// sgfx_ui — dynamic, 1:1 runtime reconstruction of the Sonic Unleashed CSD UI.
//
// Loads a screen exported by tools/emit_runtime_data.py (cast hierarchy + base
// transforms + UV + textures + per-cast animation keyframe tracks) and PLAYS it:
// it evaluates the CSD animations (Const/Linear/Hermite) per frame, composes the
// cast hierarchy, and draws each visible cast as a UV-cropped, scaled/rotated,
// per-vertex-colored quad through the GAME'S EXACT GPU PATH — the real csd_vs /
// csd_filter_ps DXIL shaders, CSD vertex format, alpha/additive blends and MSAA,
// via the recomp's own plume D3D12 RHI (see gfx_d3d12.cpp). Live, not static.
//
// SDL2 here is only the window + input + main loop; all rendering is D3D12.
// Controls: [1..6] pick screen, [Space] replay intro, [Tab] cycle animation,
//           [Left/Right] scrub, [P] pause/play, [Esc] quit.
// =============================================================================
#define SDL_MAIN_HANDLED   // we provide a plain main(); don't let SDL2main hijack it
#include <SDL.h>
#include <SDL_syswm.h>     // HWND for the D3D12 swapchain
#include "gfx_d3d12.h"     // the game's exact CSD GPU path (plume D3D12 backend)
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using json = nlohmann::json;

static const float REF_W = 1280.0f, REF_H = 720.0f;

// ---- data model (mirrors emit_runtime_data.py output) -----------------------
struct Keyframe { int f; float v; std::string t; float it, ot; };
using Track = std::vector<Keyframe>;
struct CastAnim { float frames = 0; std::map<std::string, Track> tracks; };
struct Base { float tx=0,ty=0,sx=1,sy=1,rot=0,sub=0; unsigned color=0xFFFFFFFF; int hide=0; };
struct Cast {
    std::string id; int gi=0, ci=0, parent=-1;
    bool hasQuad=false; float quad[4]{0,0,0,0};      // normalised corners tl,br
    std::string tex; bool hasUV=false; float uv[4]{0,0,1,1};
    bool add=false;                                  // additive blend (glow/shine/flash FX)
    Base base;
    std::map<std::string, CastAnim> anims;
};
struct Scene { std::string name; float framerate=60; std::vector<Cast> casts;
               std::vector<std::pair<std::string,float>> anims;
               std::string restAnim; float restFrame=0; };  // resolved settled pose
struct Screen { std::string id; float framerate=60; std::vector<std::string> textures;
                std::vector<Scene> scenes; };

static unsigned parseColor(const std::string& s) {
    if (s.size()==10 && (s[1]=='x'||s[1]=='X')) return (unsigned)strtoul(s.c_str()+2,nullptr,16);
    return 0xFFFFFFFFu;
}

static Screen loadScreen(const std::string& path) {
    Screen S; FILE* fp = fopen(path.c_str(),"rb");
    if (!fp) { fprintf(stderr,"cannot open %s\n",path.c_str()); return S; }
    std::string buf; fseek(fp,0,SEEK_END); long n=ftell(fp); fseek(fp,0,SEEK_SET);
    buf.resize(n); fread(&buf[0],1,n,fp); fclose(fp);
    json j = json::parse(buf, nullptr, false);
    if (j.is_discarded()) { fprintf(stderr,"bad json %s\n",path.c_str()); return S; }
    S.id = j.value("screen",""); S.framerate = j.value("framerate",60.0f);
    for (auto& t : j.value("textures",json::array())) S.textures.push_back(t.get<std::string>());
    for (auto& js : j.value("scenes",json::array())) {
        Scene sc; sc.name = js.value("name",""); sc.framerate = js.value("framerate",S.framerate);
        for (auto& ja : js.value("anims",json::array()))
            sc.anims.push_back({ja.value("name",""), ja.value("frames",0.0f)});
        for (auto& jc : js.value("casts",json::array())) {
            Cast c; c.id=jc.value("id",""); c.gi=jc.value("gi",0); c.ci=jc.value("ci",0);
            c.parent=jc.value("parent",-1); c.tex=jc.value("tex","");
            if (jc.contains("quad") && jc["quad"].is_array()) { c.hasQuad=true; for(int i=0;i<4;i++) c.quad[i]=jc["quad"][i].get<float>(); }
            if (jc.contains("uv") && jc["uv"].is_array()) { c.hasUV=true; for(int i=0;i<4;i++) c.uv[i]=jc["uv"][i].get<float>(); }
            c.add = jc.value("add",0) != 0;
            auto& b=jc["base"]; c.base.tx=b.value("tx",0.0f); c.base.ty=b.value("ty",0.0f);
            c.base.sx=b.value("sx",1.0f); c.base.sy=b.value("sy",1.0f); c.base.rot=b.value("rot",0.0f);
            c.base.sub=b.value("sub",0.0f); c.base.hide=b.value("hide",0); c.base.color=parseColor(b.value("color","0xFFFFFFFF"));
            for (auto it=jc["anims"].begin(); it!=jc["anims"].end(); ++it) {
                CastAnim ca; ca.frames=it.value().value("frames",0.0f);
                for (auto tk=it.value()["tracks"].begin(); tk!=it.value()["tracks"].end(); ++tk) {
                    Track tr; for (auto& k : tk.value()) tr.push_back({k.value("f",0),k.value("v",0.0f),k.value("t","Const"),k.value("it",0.0f),k.value("ot",0.0f)});
                    ca.tracks[tk.key()] = std::move(tr);
                }
                c.anims[it.key()] = std::move(ca);
            }
            sc.casts.push_back(std::move(c));
        }
        S.scenes.push_back(std::move(sc));
    }
    return S;
}

// ---- CSD keyframe interpolation (Const / Linear / Hermite) -------------------
static float evalTrack(const Track& kf, float frame) {
    if (kf.empty()) return 0.0f;
    if (frame <= kf.front().f) return kf.front().v;
    if (frame >= kf.back().f)  return kf.back().v;
    for (size_t i=0;i+1<kf.size();++i) {
        const auto& a=kf[i]; const auto& b=kf[i+1];
        if (frame>=a.f && frame<=b.f) {
            float dt = float(b.f-a.f); if (dt<=0) return a.v;
            float t = (frame-a.f)/dt;
            if (a.t=="Const") return a.v;
            if (a.t=="Linear") return a.v + (b.v-a.v)*t;
            // Hermite (matches imgui_utils Hermite: tangents scaled by segment length)
            float t2=t*t, t3=t2*t;
            float h00=2*t3-3*t2+1, h10=t3-2*t2+t, h01=-2*t3+3*t2, h11=t3-t2;
            return h00*a.v + h10*(a.ot*dt) + h01*b.v + h11*(b.it*dt);
        }
    }
    return kf.back().v;
}

struct Xform { float tx,ty,sx,sy,rot,alpha; bool visible; };

static Xform evalCast(const Cast& c, const std::string& anim, float frame) {
    Xform x; x.tx=c.base.tx; x.ty=c.base.ty; x.sx=c.base.sx; x.sy=c.base.sy;
    x.rot=c.base.rot; x.alpha=((c.base.color>>24)&0xFF)/255.0f; x.visible=(c.base.hide==0);
    auto it=c.anims.find(anim);
    if (it!=c.anims.end()) {
        const auto& tr=it->second.tracks;
        auto has=[&](const char* k){ return tr.find(k)!=tr.end(); };
        if (has("XPosition")) x.tx=evalTrack(tr.at("XPosition"),frame);
        if (has("YPosition")) x.ty=evalTrack(tr.at("YPosition"),frame);
        if (has("XScale"))    x.sx=evalTrack(tr.at("XScale"),frame);
        if (has("YScale"))    x.sy=evalTrack(tr.at("YScale"),frame);
        if (has("Rotation"))  x.rot=evalTrack(tr.at("Rotation"),frame);
        if (has("Color"))     x.alpha=std::min(1.0f,std::max(0.0f,evalTrack(tr.at("Color"),frame)));
        if (has("HideFlag"))  x.visible=evalTrack(tr.at("HideFlag"),frame) < 0.5f;
    }
    return x;
}

// world transform of a cast = parent-chain composition (translation in normalised
// screen units, scale multiplied). Returns origin (normalised) + scale + rot + alpha.
struct World { float ox,oy,sx,sy,rot,alpha; bool visible; };

static World worldOf(const std::vector<Cast>& casts, std::vector<World>& cache,
                     std::vector<char>& done, int i, const std::string& anim, float frame) {
    if (done[i]) return cache[i];
    Xform x = evalCast(casts[i], anim, frame);
    World w;
    if (casts[i].parent < 0) {
        w.ox = x.tx; w.oy = x.ty; w.sx = x.sx; w.sy = x.sy; w.rot = x.rot;
        w.alpha = x.alpha; w.visible = x.visible;
    } else {
        World p = worldOf(casts, cache, done, casts[i].parent, anim, frame);
        w.ox = p.ox + x.tx * p.sx; w.oy = p.oy + x.ty * p.sy;
        w.sx = p.sx * x.sx; w.sy = p.sy * x.sy; w.rot = p.rot + x.rot;
        w.alpha = p.alpha * x.alpha; w.visible = p.visible && x.visible;
    }
    cache[i]=w; done[i]=1; return w;
}

// Score a (scene, anim, frame) pose: how many casts are visible, on-screen, and
// not a runaway 9-slice fill. Used to auto-pick each scene's settled resting pose,
// because scenes use DIFFERENT anim names (Intro_so_Anim/Intro_ev_Anim/Usual_Anim/
// Switch_Anim/…) — a single global anim leaves mismatched scenes on raw base
// transforms (e.g. the gauge parked off-screen, result rows ballooning).
static int poseScore(const Scene& sc, const std::string& anim, float frame) {
    std::vector<World> cache(sc.casts.size()); std::vector<char> done(sc.casts.size(),0);
    int score=0;
    for (size_t i=0;i<sc.casts.size();++i) {
        const Cast& c=sc.casts[i];
        if (!c.hasQuad || c.tex.empty() || !c.hasUV) continue;
        World w=worldOf(sc.casts,cache,done,(int)i,anim,frame);
        if (!w.visible || w.alpha<=0.003f) continue;
        float qx=(w.ox+c.quad[0]*w.sx), qy=(w.oy+c.quad[1]*w.sy);
        float qw=(c.quad[2]-c.quad[0])*w.sx, qh=(c.quad[3]-c.quad[1])*w.sy;
        float x0=qx*REF_W,y0=qy*REF_H,x1=(qx+qw)*REF_W,y1=(qy+qh)*REF_H;
        if (x1<x0) std::swap(x0,x1); if (y1<y0) std::swap(y0,y1);
        float bw=x1-x0, bh=y1-y0;
        if (bw<0.5f||bh<0.5f) continue;
        if (bw>1.5f*REF_W||bh>1.5f*REF_H) continue;          // runaway fill, don't reward
        if (x1<=0||y1<=0||x0>=REF_W||y0>=REF_H) continue;    // off-screen
        score++;
    }
    return score;
}

// Resolve each scene's settled pose = the (anim, frame) that brings the most casts
// on-screen. Intro scenes settle at their end; name/emphasis scenes peak mid-anim;
// usual/switch-only scenes pick their loop. Ties favour the later (more settled) frame.
static void resolveRestPose(Scene& sc) {
    sc.restAnim=""; sc.restFrame=0; int best=poseScore(sc,"",0);   // base as the floor
    const int SAMPLES=8;
    for (auto& a : sc.anims) {
        float maxf=a.second;
        for (int k=0;k<=SAMPLES;k++) {
            float f=(maxf>0)? maxf*float(k)/SAMPLES : 0.0f;
            int s=poseScore(sc,a.first,f);
            if (s>best || (s==best && s>0 && f>sc.restFrame)) { best=s; sc.restAnim=a.first; sc.restFrame=f; }
            if (maxf<=0) break;
        }
    }
}

int main(int argc, char** argv) {
    std::string dataDir = "data", assetDir = "assets";
    const char* screens[] = {"title","pause","world_map","result","boss","loading"};
    int cur = 1; // pause
    bool shotMode=false; float shotFrame=0; std::string shotOut;
    if (argc>=5 && std::string(argv[1])=="--shot") {   // --shot <screen> <frame> <out.png>
        shotMode=true;
        for (int i=0;i<6;i++) if (std::string(argv[2])==screens[i]) cur=i;
        shotFrame=(float)atof(argv[3]); shotOut=argv[4];
    } else if (argc>1) {
        for (int i=0;i<6;i++) if (std::string(argv[1])==screens[i]) cur=i;
    }

    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO)!=0) { fprintf(stderr,"SDL init: %s\n",SDL_GetError()); return 1; }
    SDL_Window* win = SDL_CreateWindow("sgfx_ui — CSD runtime (D3D12)", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE | (shotMode?SDL_WINDOW_HIDDEN:0));

    void* hwnd=nullptr;
    SDL_SysWMinfo wm; SDL_VERSION(&wm.version);
    if (SDL_GetWindowWMInfo(win,&wm)) hwnd=(void*)wm.info.win.window;
    if (!gfx::init(hwnd, (int)REF_W, (int)REF_H, shotMode, 4)) { fprintf(stderr,"gfx init failed\n"); return 1; }

    std::map<std::string,int> texSlot;            // texture name -> bindless slot
    Screen S; std::string activeAnim; float frame=0; bool playing=true;

    auto loadTex=[&](const std::string& name)->int{
        auto it=texSlot.find(name); if (it!=texSlot.end()) return it->second;
        int slot=gfx::loadTexture(assetDir+"/"+S.id+"/"+name+".png");
        texSlot[name]=slot; return slot;
    };
    auto reload=[&](){
        gfx::clearTextures(); texSlot.clear();
        S = loadScreen(dataDir+"/"+std::string(screens[cur])+".json");
        for (auto& sc:S.scenes) resolveRestPose(sc);   // each scene's settled pose
        activeAnim.clear();                             // empty = per-scene auto (Tab forces one)
        frame=0; playing=true;
        printf("loaded %s: %zu scenes\n", S.id.c_str(), S.scenes.size());
    };
    // Evaluate the screen at an animation frame into a flat CSD quad list (the
    // game's vertex stream): 4 corners in 1280x720 ref pixels, per-vertex color, UV.
    auto buildQuads=[&](float frameArg, std::vector<gfx::Quad>& out){
        out.clear();
        for (auto& sc : S.scenes) {
            // per-scene anim: auto-resolved settled pose, or a forced global anim (Tab).
            const std::string& anim = activeAnim.empty() ? sc.restAnim : activeAnim;
            float holdF = sc.restFrame;
            if (!activeAnim.empty()) { holdF=0; for (auto& a:sc.anims) if(a.first==activeAnim) holdF=a.second; }
            float f = (holdF>0)? std::min(frameArg,holdF) : frameArg;   // play 0->settle, then hold
            std::vector<World> cache(sc.casts.size()); std::vector<char> done(sc.casts.size(),0);
            for (size_t i=0;i<sc.casts.size();++i) {
                const Cast& c=sc.casts[i];
                if (!c.hasQuad || c.tex.empty() || !c.hasUV) continue;
                World w=worldOf(sc.casts,cache,done,(int)i,anim,f);
                if (!w.visible || w.alpha<=0.003f) continue;
                float qx=(w.ox + c.quad[0]*w.sx), qy=(w.oy + c.quad[1]*w.sy);
                float qw=(c.quad[2]-c.quad[0])*w.sx, qh=(c.quad[3]-c.quad[1])*w.sy;
                float x0=qx*REF_W, y0=qy*REF_H, x1=(qx+qw)*REF_W, y1=(qy+qh)*REF_H;
                if (x1<x0) std::swap(x0,x1); if (y1<y0) std::swap(y0,y1);
                float bw=x1-x0, bh=y1-y0;
                if (bw<0.5f || bh<0.5f) continue;
                // skip clearly-runaway 9-slice fill pieces (no true 9-slice clip yet)
                if (bw>2.5f*REF_W || bh>2.5f*REF_H) continue;
                int slot=loadTex(c.tex); if(slot<0) continue;
                gfx::Quad q;
                float cx=(x0+x1)*0.5f, cy=(y0+y1)*0.5f;
                float rad=w.rot*3.14159265358979f/180.0f, cs=cosf(rad), sn=sinf(rad);
                float corners[4][2]={{x0,y0},{x1,y0},{x1,y1},{x0,y1}};   // TL,TR,BR,BL
                for(int k=0;k<4;k++){ float dx=corners[k][0]-cx, dy=corners[k][1]-cy;
                    q.px[k]=cx+dx*cs-dy*sn; q.py[k]=cy+dx*sn+dy*cs; }
                q.u[0]=c.uv[0];q.v[0]=c.uv[1]; q.u[1]=c.uv[2];q.v[1]=c.uv[1];
                q.u[2]=c.uv[2];q.v[2]=c.uv[3]; q.u[3]=c.uv[0];q.v[3]=c.uv[3];
                unsigned A=(unsigned)(w.alpha*255.0f+0.5f); if(A>255)A=255;
                q.color[0]=q.color[1]=q.color[2]=q.color[3]=(A<<24) | (c.base.color & 0x00FFFFFFu);
                q.texIndex=slot; q.blend = c.add ? gfx::Blend::Additive : gfx::Blend::Alpha;
                out.push_back(q);
            }
        }
    };

    reload();
    std::vector<gfx::Quad> quads;

    // Headless screenshot mode: render <screen> at <frame> via D3D12, read back, PNG.
    if (shotMode) {
        buildQuads(shotFrame, quads);
        gfx::beginFrame(0.04f,0.05f,0.06f,1.0f);
        gfx::drawQuads(quads.data(),(int)quads.size());
        gfx::endFrame();
        std::vector<unsigned char> px((size_t)REF_W*(size_t)REF_H*4);
        gfx::readbackRGBA(px.data(),(int)REF_W,(int)REF_H);
        stbi_write_png(shotOut.c_str(),(int)REF_W,(int)REF_H,4,px.data(),(int)REF_W*4);
        printf("wrote %s (screen=%s anim=%s frame=%.1f, %d quads, MSAA x%d)\n",
               shotOut.c_str(),S.id.c_str(),activeAnim.c_str(),shotFrame,(int)quads.size(),gfx::sampleCount());
        gfx::shutdown(); SDL_DestroyWindow(win); SDL_Quit(); return 0;
    }

    bool run=true; Uint64 prev=SDL_GetPerformanceCounter();
    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type==SDL_QUIT) run=false;
            else if (e.type==SDL_KEYDOWN) {
                SDL_Keycode k=e.key.keysym.sym;
                if (k==SDLK_ESCAPE) run=false;
                else if (k>=SDLK_1 && k<=SDLK_6){ cur=k-SDLK_1; reload(); }
                else if (k==SDLK_SPACE) frame=0;
                else if (k==SDLK_p) playing=!playing;
                else if (k==SDLK_LEFT) { playing=false; frame=std::max(0.0f,frame-1); }
                else if (k==SDLK_RIGHT){ playing=false; frame+=1; }
                else if (k==SDLK_TAB) { // cycle anim across the screen's anim set
                    std::vector<std::string> all; for(auto&sc:S.scenes)for(auto&a:sc.anims) if(std::find(all.begin(),all.end(),a.first)==all.end()) all.push_back(a.first);
                    if(!all.empty()){ auto p=std::find(all.begin(),all.end(),activeAnim); size_t idx=(p==all.end())?0:(p-all.begin()+1)%all.size(); activeAnim=all[idx]; frame=0; printf("anim '%s'\n",activeAnim.c_str()); }
                }
            }
        }
        Uint64 now=SDL_GetPerformanceCounter();
        float dt=float(now-prev)/SDL_GetPerformanceFrequency(); prev=now;
        if (playing) frame += dt * S.framerate;
        buildQuads(frame, quads);
        gfx::beginFrame(0.04f,0.05f,0.06f,1.0f);
        gfx::drawQuads(quads.data(),(int)quads.size());
        gfx::endFrame();
    }
    gfx::shutdown(); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}
