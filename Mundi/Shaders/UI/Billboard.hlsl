// Generic textured billboard shader
cbuffer CameraInfo : register(b0)
{
    float3 WorldPos;
    row_major matrix viewMatrix;
    row_major matrix projectionMatrix;
    row_major matrix viewInverse;
};

cbuffer ColorId : register(b3)
{
    float4 Color;
    uint UUID;
}
struct VS_INPUT
{
    float3 localPos : POSITION;   // quad local offset (-0.5~0.5)
    float2 uv       : TEXCOORD0;  // per-vertex UV
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
    uint UUID : SV_Target1;
};

Texture2D BillboardTex : register(t0);
SamplerState LinearSamp : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT o;
    // Face the camera by transforming local offsets using inverse view rotation
    float3 posAligned = mul(float4(input.localPos, 0.0f), viewInverse).xyz;
    float3 worldPos   = WorldPos + posAligned;

    o.pos = mul(float4(worldPos, 1.0f), mul(viewMatrix, projectionMatrix));
    o.uv  = input.uv;
    return o;
}

PS_OUTPUT mainPS(PS_INPUT i) 
{
    PS_OUTPUT Output;
    
    float4 c = BillboardTex.Sample(LinearSamp, i.uv);
    if (c.a < 0.1f)
        discard;
    Output.Color = c;
    Output.UUID = UUID;
    return Output;
}
