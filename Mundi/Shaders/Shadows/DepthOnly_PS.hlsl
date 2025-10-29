// b1: ViewProjBuffer (VS) - ViewProjBufferType과 일치
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;   // 0행 광원의 월드 좌표 + 스포트라이트 반경
    row_major float4x4 InverseProjectionMatrix;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPosition : TEXCOORD0;
};

float2 mainPS(PS_INPUT Input) : SV_TARGET
{
    float3 LightWorldPosition = float3(InverseViewMatrix[0][0], InverseViewMatrix[0][1], InverseViewMatrix[0][2]);
    float LightRadius = InverseViewMatrix[0][3];
    float Distance = length(Input.WorldPosition - LightWorldPosition);
    float NormalizedDepth = saturate(Distance / LightRadius);
    float Moment1 = NormalizedDepth;
    float Moment2 = NormalizedDepth * NormalizedDepth;
    // float A = ProjectionMatrix[2][2];
    // float B = ProjectionMatrix[3][2];
    // float Near = -B / A;
    // float Far = A / (A - 1.0f) * Near;
    //
    // float NDCDepth = Input.Position.z / Input.Position.w;
    // NDCDepth = NDCDepth * 2.0f - 1.0f;
    //
    // // 선형화
    // float4 ClipPos = float4(0.0f, 0.0f, NDCDepth, 1.0f);
    // float4 ViewPos = mul(ClipPos, InverseProjectionMatrix);
    // float LinearDepth = ViewPos.z / ViewPos.w;
    //
    // float NormalizedLinearDepth = (LinearDepth - Near) / (Far - Near);
    // NormalizedLinearDepth = saturate(NormalizedLinearDepth);
    //
    // // float Moment1 = LinearDepth;
    // // float Moment2 = LinearDepth * LinearDepth;
    // float Moment1 = NormalizedLinearDepth;
    // float Moment2 = NormalizedLinearDepth * NormalizedLinearDepth;
    //
    // // float Moment1 = NDCDepth;
    // // float Moment2 = NDCDepth * NDCDepth;

    return float2(Moment1, Moment2);
}