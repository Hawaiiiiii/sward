// csd_modifier_ps.hlsl — our CSD filter pixel shader + a per-quad ShaderModifier,
// in the recomp's idiom. Identical to csd_filter_ps when g_Modifier == 0, so it is
// a safe drop-in replacement. Screen-space modifiers (scanline / checkerboard /
// grayscale) are ported from UnleashedRecomp's imgui_ps.hlsl. The modifier + bounds
// live in the per-quad Shared CB at free packoffsets the C++ side writes.
#include "shader_common.h"

cbuffer SharedConstants : register(b2, space4)
{
    uint   s0_Texture2DDescriptorIndex : packoffset(c0.x);
    uint   g_Modifier                  : packoffset(c1.x);   // 0=none (matches csd_filter_ps)
    float4 g_Bounds                    : packoffset(c2);     // minX,minY,maxX,maxY (reserved for bevel/marquee)
    uint   s0_SamplerDescriptorIndex   : packoffset(c12.x);
    DEFINE_SHARED_CONSTANTS();
};

#define MOD_NONE            0
#define MOD_SCANLINE        1
#define MOD_CHECKERBOARD    2
#define MOD_GRAYSCALE       3
#define MOD_SDF_TEXT        4
#define MOD_TITLE_BEVEL     5
#define MOD_SCANLINE_BUTTON 6
#define MOD_CATEGORY_BEVEL  7
#define MOD_MSDF_TEXT       8   // the recomp's 3-channel MSDF text (median of RGB), pxRange=8
#define SDF_PXRANGE      12.0   // SDF spread in atlas texels (matches sgfxui SDF_PAD*2)

float4 main(
    in float4 iPosition  : SV_Position,
    in float4 iTexCoord0 : TEXCOORD0,
    in float4 iTexCoord1 : TEXCOORD1) : SV_Target
{
    Texture2D<float4> texture = g_Texture2DDescriptorHeap[s0_Texture2DDescriptorIndex];
    SamplerState samplerState = g_SamplerDescriptorHeap[s0_SamplerDescriptorIndex];

    uint2 dimensions;
    texture.GetDimensions(dimensions.x, dimensions.y);

    // ---- MSDF text (MOD_MSDF_TEXT): the recomp's exact 3-channel MSDF (imgui_ps.hlsl) ----
    if (g_Modifier == MOD_MSDF_TEXT)
    {
        float2 uv = iTexCoord1.xy;
        float3 s  = texture.Sample(samplerState, uv).rgb;
        float  md = max(min(s.r, s.g), min(max(s.r, s.g), s.b));   // median -> signed distance
        float2 unitRange     = 8.0 / float2(dimensions);           // pxRange = 8 (msdf-atlas-gen)
        float2 screenTexSize = 1.0 / fwidth(uv);
        float  screenPxRange = max(0.5 * dot(unitRange, screenTexSize), 1.0);
        float  sd = md - 0.5;
        float  aa = saturate(sd * screenPxRange + 0.5);
        return float4(iTexCoord0.rgb, iTexCoord0.a * aa);
    }

    // ---- SDF text (MOD_SDF_TEXT) / beveled gold title (MOD_TITLE_BEVEL) ----
    // distance is in tex.r (single-channel SDF, edge at 0.5); rgb comes from the
    // vertex colour, alpha from the antialiased distance. Bevel ports imgui_ps.hlsl.
    if (g_Modifier == MOD_SDF_TEXT || g_Modifier == MOD_TITLE_BEVEL || g_Modifier == MOD_CATEGORY_BEVEL)
    {
        float2 uv = iTexCoord1.xy;
        float  dist = texture.Sample(samplerState, uv).r;
        float2 unitRange = SDF_PXRANGE / float2(dimensions);
        float2 screenTexSize = 1.0 / fwidth(uv);
        float  screenPxRange = max(0.5 * dot(unitRange, screenTexSize), 1.0);
        float  sd = dist - 0.5;
        float  aa = saturate(sd * screenPxRange + 0.5);
        float3 rgb = iTexCoord0.rgb;
        if (g_Modifier == MOD_TITLE_BEVEL)         // gold-foil emboss (imgui_ps.hlsl:95-104)
        {
            float2 n = normalize(float3(ddx(sd), ddy(sd), 0.01)).xy;
            float3 rimColor    = float3(1.0, 0.8, 0.29);
            float3 shadowColor = float3(0.84, 0.57, 0.0);
            float  cosTheta = dot(n, normalize(float2(1.0, 1.0)));
            float3 grad = lerp(rgb, cosTheta >= 0.0 ? rimColor : shadowColor, abs(cosTheta));
            rgb = lerp(grad, rgb, pow(saturate(sd + 0.77), 32.0));
        }
        else if (g_Modifier == MOD_CATEGORY_BEVEL) // softer +/-50% emboss (imgui_ps.hlsl:105-111)
        {
            float2 n = normalize(float3(ddx(sd), ddy(sd), 0.25)).xy;
            float  cosTheta = dot(n, normalize(float2(1.0, 1.0)));
            rgb = saturate(rgb * (1.0 + cosTheta * 0.5));
        }
        return float4(rgb, iTexCoord0.a * aa);
    }

    // ---- images / panels: antialiased sampling (same as csd_filter_ps) ----
    float2 uvTexspace = iTexCoord1.xy * dimensions;
    float2 seam = floor(uvTexspace + 0.5);
    uvTexspace = (uvTexspace - seam) / fwidth(uvTexspace) + seam;
    uvTexspace = clamp(uvTexspace, seam - 0.5, seam + 0.5);

    float4 color = texture.Sample(samplerState, uvTexspace / dimensions);
    color *= iTexCoord0;

    // ---- per-quad shader modifier (screen-space; matches the recomp's effects) ----
    int2 p = int2(iPosition.xy);
    if (g_Modifier == MOD_SCANLINE)                        // imgui_ps SCANLINE: even rows transparent
    {
        if ((p.y & 1) == 0) color.a = 0.0;
    }
    else if (g_Modifier == MOD_SCANLINE_BUTTON)            // imgui_ps SCANLINE_BUTTON: even rows half-alpha
    {
        if ((p.y & 1) == 0) color.a *= 0.5;
    }
    else if (g_Modifier == MOD_CHECKERBOARD)               // imgui_ps CHECKERBOARD: 9px grid
    {
        int rx = p.x % 9; int ry = p.y % 9;
        if (rx == 0 || ry == 0) color.a = 0.0;             // 1px grid lines punch through
        else if ((ry % 2) == 0) color.rgb *= 0.5;          // alternate rows dimmed 50%
    }
    else if (g_Modifier == MOD_GRAYSCALE)
    {
        color.rgb = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));   // luma desaturate
    }

    clip(color.a - g_AlphaThreshold);
    return color;
}
