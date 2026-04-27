#include <patches/ui_lab_patches.h>

// CGameModeStage's stage update path contains the original Start-button pause
// gate. Injecting here lets the real pause owner consume the input before the
// translated runtime evaluates pause transitions.
PPC_FUNC_IMPL(__imp__sub_8253B7C0);
PPC_FUNC(sub_8253B7C0)
{
    UiLab::ApplyPauseRouteInput("CGameModeStage::Update pause gate sub_8253B7C0");
    __imp__sub_8253B7C0(ctx, base);
}
