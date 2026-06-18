// mesh_ps.hlsl — the 3D viewport pixel shader: bindless texture sample x base
// color, single directional light + ambient (computed from the VS normal).
cbuffer MeshCB : register(b0, space4)
{
    row_major float4x4 gMVP;
    row_major float4x4 gModel;
    float4 gLightDir;
    float4 gBaseColor;
    uint4  gMisc;
};

Texture2D<float4> gTextures[] : register(t0, space0);
SamplerState      gSamplers[] : register(s0, space3);

struct VSOut { float4 pos : SV_Position; float3 nrm : NORMAL; float2 uv : TEXCOORD0; };

float4 main(VSOut i) : SV_Target
{
    float4 tex = gTextures[NonUniformResourceIndex(gMisc.x)].Sample(gSamplers[0], i.uv);
    float ndl = saturate(dot(normalize(i.nrm), normalize(gLightDir.xyz)));
    float l = gLightDir.w + (1.0 - gLightDir.w) * ndl;
    float4 c = tex * gBaseColor;
    return float4(c.rgb * l, c.a);
}
