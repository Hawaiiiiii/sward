#include <kernel/function.h>
#include <patches/ui_lab_patches.h>

PPC_FUNC_IMPL(__imp__sub_830BF300);
PPC_FUNC(sub_830BF300)
{
    const uint32_t nodeAddress = ctx.r3.u32;
    const uint32_t patternIndex = ctx.r4.u32;

    __imp__sub_830BF300(ctx, base);

    UiLab::OnCsdNodeSetPatternIndex(
        nodeAddress,
        patternIndex,
        "CSD::CNode::SetPatternIndex/sub_830BF300");
}

PPC_FUNC_IMPL(__imp__sub_830BF080);
PPC_FUNC(sub_830BF080)
{
    const uint32_t nodeAddress = ctx.r3.u32;
    const uint32_t hideFlag = ctx.r4.u32;

    __imp__sub_830BF080(ctx, base);

    UiLab::OnCsdNodeSetHideFlag(
        nodeAddress,
        hideFlag,
        "CSD::CNode::SetHideFlag/sub_830BF080");
}

PPC_FUNC_IMPL(__imp__sub_830BF090);
PPC_FUNC(sub_830BF090)
{
    const uint32_t nodeAddress = ctx.r3.u32;
    const float scaleX = static_cast<float>(ctx.f1.f64);
    const float scaleY = static_cast<float>(ctx.f2.f64);

    __imp__sub_830BF090(ctx, base);

    UiLab::OnCsdNodeSetScale(
        nodeAddress,
        scaleX,
        scaleY,
        "CSD::CNode::SetScale/sub_830BF090");
}
