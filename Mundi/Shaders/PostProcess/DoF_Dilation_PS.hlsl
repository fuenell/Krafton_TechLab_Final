#include "../Common/PostProcessCommon.hlsl"

Texture2D<float4> g_CoCTexture : register(t0);  // CoC Map (R=Far, G=Near, B=?, A=Depth)
SamplerState g_PointSampler : register(s0);

// Dilation: 주변 픽셀의 최소 CoC를 찾아서 전경 영역 확장
// (작은 CoC = 선명 = 전경이므로, Min Filter가 전경을 확장시킴)
float4 mainPS(float4 position : SV_Position, float2 texcoord : TEXCOORD0) : SV_Target
{
    // 픽셀 오프셋 (텍스처 공간)
    float2 pixelSize = ScreenSize.zw; // (1/width, 1/height)

    // 초기값: 충분히 큰 값 (min 연산에서 실제 값으로 대체됨)
    float minFarCoC = 1.0;
    float minNearCoC = 1.0;

    // 중심 픽셀의 b, a 채널 보존용 (루프 중 한 번만 샘플링)
    float4 centerCoC = g_CoCTexture.Sample(g_PointSampler, texcoord);

    // 3x3 영역에서 최소 CoC 값 찾기 (전경 확장)
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 offset = float2(x, y) * pixelSize;
            float4 sampleCoC = g_CoCTexture.Sample(g_PointSampler, texcoord + offset);

            // 최소값 찾기 (전경이 주변으로 확장됨)
            minFarCoC = min(minFarCoC, sampleCoC.r);
            minNearCoC = min(minNearCoC, sampleCoC.g);
        }
    }

    // Dilated CoC 반환
    return float4(minFarCoC, minNearCoC, centerCoC.b, centerCoC.a);
}
