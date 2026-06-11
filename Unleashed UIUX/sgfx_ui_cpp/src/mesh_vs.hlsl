// mesh_vs.hlsl — the 3D viewport vertex shader (plain SM6, NOT XenosRecomp ABI).
// Row-vector convention: position * row_major matrix (classic D3D).
cbuffer MeshCB : register(b0, space4)
{
    row_major float4x4 gMVP;
    row_major float4x4 gModel;
    float4 gLightDir;    // xyz = direction TOWARDS the light (normalized), w = ambient
    float4 gBaseColor;
    uint4  gMisc;        // x = bindless texture index
};

struct VSIn  { float3 pos : POSITION; float3 nrm : NORMAL; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : SV_Position; float3 nrm : NORMAL; float2 uv : TEXCOORD0; };

VSOut main(VSIn i)
{
    VSOut o;
    o.pos = mul(float4(i.pos, 1.0), gMVP);
    o.nrm = normalize(mul(float4(i.nrm, 0.0), gModel).xyz);
    o.uv  = i.uv;
    return o;
}
