#pragma once

// DECOUPLED from UnleashedRecomp/ui/options_menu_thumbnails.h:
//   <gpu/video.h>   -> the render contract (GuestTexture* -> sgfx::render::Texture*)
//   <user/config.h> -> the shim (IConfigDef + the option enums live there now)
#include "../platform/sgfx_platform.h"
#include "../render/sgfx_render.h"

void LoadThumbnails();

sgfx::render::Texture* GetThumbnail(const IConfigDef* cfg);
