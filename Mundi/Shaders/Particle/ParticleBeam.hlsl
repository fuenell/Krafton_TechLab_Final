// ------------------------------------------------------------
// Global constants
// ------------------------------------------------------------

// b1: ViewProjBuffer (VS)
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
};

// ------------------------------------------------------------
// Vertex Shader
// ------------------------------------------------------------

struct VS_INPUT
{
    float3 Position : POSITION; // Beam quad world position
    float2 UV : TEXCOORD0; // Can be kept for procedural effects
    float4 Color : COLOR0; // Vertex color
    float Width : TEXCOORD1; // Optional (not used here)
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR0;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    float4 worldPos = float4(input.Position, 1.0f);

    // World → View → Projection
    float4 viewPos = mul(worldPos, ViewMatrix);
    output.Position = mul(viewPos, ProjectionMatrix);

    output.UV = input.UV;
    output.Color = input.Color;

    return output;
}

// ------------------------------------------------------------
// Pixel Shader
// ------------------------------------------------------------

float4 mainPS(PS_INPUT input) : SV_Target
{
    // Pure color beam: ignore UV, ignore textures
    return input.Color;
}