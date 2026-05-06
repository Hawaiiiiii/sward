#include <api/SWA.h>
#include <patches/ui_lab_patches.h>

// CGameModeStage's stage update path contains the original Start-button pause
// gate. Injecting here lets the real pause owner consume the input before the
// translated runtime evaluates pause transitions.
//
// Phase 363 also uses this hook as the gameplay-skip / UI-only-input-lock
// site. The pad-state mask runs only for the duration of __imp__ so all
// other consumers (pause menu, world-map cursor, options) see real input.
PPC_FUNC_IMPL(__imp__sub_8253B7C0);
PPC_FUNC(sub_8253B7C0)
{
    UiLab::ApplyPauseRouteInput("CGameModeStage::Update pause gate sub_8253B7C0");

    if (UiLab::IsGameplaySkipMode())
        UiLab::OnGameplaySkipStageEntered(ctx.r3.u32);

    SWA::SPadState savedPadState{};
    SWA::SPadState* livePadStateSlot = nullptr;
    if (UiLab::IsUiOnlyInputMode())
    {
        if (auto* pInputState = SWA::CInputState::GetInstance())
        {
            const uint32_t idx = pInputState->m_CurrentPadStateIndex.get() & 7u;
            SWA::SPadState& live = pInputState->m_PadStates[idx];
            savedPadState = live;
            livePadStateSlot = &live;
            UiLab::OnUiOnlyInputLockApplied(ctx.r3.u32);

            // Keep Start + Select live so the player can still raise the
            // pause menu and Quit back to World Map. Mask everything else
            // so the gameplay tick sees a fully-released controller.
            constexpr uint32_t keepBits =
                static_cast<uint32_t>(SWA::eKeyState_Start) |
                static_cast<uint32_t>(SWA::eKeyState_Select);

            const uint32_t down = live.DownState.get();
            const uint32_t up   = live.UpState.get();
            const uint32_t tap  = live.TappedState.get();
            const uint32_t rel  = live.ReleasedState.get();

            live.DownState     = down & keepBits;
            live.UpState       = (up & keepBits) | (~keepBits);
            live.TappedState   = tap & keepBits;
            live.ReleasedState = rel & keepBits;

            live.LeftStickHorizontal  = 0.0f;
            live.LeftStickVertical    = 0.0f;
            live.RightStickHorizontal = 0.0f;
            live.RightStickVertical   = 0.0f;
            live.LeftTrigger          = 0.0f;
            live.RightTrigger         = 0.0f;
        }
    }

    __imp__sub_8253B7C0(ctx, base);

    if (livePadStateSlot != nullptr)
        *livePadStateSlot = savedPadState;
}
