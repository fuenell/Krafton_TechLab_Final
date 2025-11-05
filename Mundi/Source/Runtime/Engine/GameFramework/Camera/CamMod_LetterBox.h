#pragma once
#include "CameraModifierBase.h"
#include "PostProcessing/PostProcessing.h"

class UCamMod_LetterBox : public UCameraModifierBase
{
public:
    DECLARE_CLASS(UCamMod_LetterBox, UCameraModifierBase)
    UCamMod_LetterBox() = default;
    virtual ~UCamMod_LetterBox() = default;

    // 설정값
    FLinearColor BoxColor = FLinearColor::Zero();
    float AspectRatio = 2.39f;  // 목표 콘텐츠 AR (예: 2.39). 0이면 무시
    float HeightBarSize = 0.0f;   // 0이면 AR로 자동 계산, >0이면 화면 높이 비율(0~0.5)

private:
    bool          bInitialized = false;
    float         ElapsedTime = 0.f;
    FViewportRect BaseRect;

public:
    virtual void ApplyToView(float DeltaTime, FMinimalViewInfo* ViewInfo) override
    {
        if (!bEnabled || !ViewInfo) return;

        const FViewportRect Curr = ViewInfo->ViewRect;
        const uint32 VW = Curr.Width();
        const uint32 VH = Curr.Height();

        // 초기화 및 리사이즈 대응: 기준 사각형을 현재 뷰로 고정
        if (!bInitialized || VW != BaseRect.Width() || VH != BaseRect.Height())
        {
            bInitialized = true;
            BaseRect = Curr;
            ElapsedTime = 0.f; // 리사이즈 때 애니메이션 처음부터
        }

        // 진행도(애니메이션). Duration<0 이면 즉시 적용.
        ElapsedTime += DeltaTime;
        const float t = (Duration > 0.f) ? FMath::Clamp(ElapsedTime / Duration, 0.f, 1.f) : 1.f;
        const float s = t * t * (3.f - 2.f * t); // smoothstep

        const float W = float(BaseRect.Width());
        const float H = float(BaseRect.Height());

        // 목표 상/하 바 두께(한쪽 기준) 계산: 오직 위/아래만
        float targetBar = 0.f;
        if (HeightBarSize > 0.f)
        {
            // 고정 비율로 바 두께
            targetBar = FMath::Clamp(HeightBarSize, 0.f, 0.5f) * H;
        }
        else if (AspectRatio > 0.f)
        {
            // AR 기반 자동: 좌/우 바는 금지. 필요한 경우에만 상/하 바 생성.
            const float contentH = FMath::Min(H, W / AspectRatio); // 더 넓은 AR일 때만 줄어듦
            targetBar = 0.5f * FMath::Max(0.f, H - contentH);
        }

        const uint32 top = (uint32)(targetBar * s);
        const uint32 bottom = (uint32)(targetBar * s);

        FViewportRect R;
        R.MinX = BaseRect.MinX;            // 좌/우는 그대로!
        R.MaxX = BaseRect.MaxX;
        R.MinY = BaseRect.MinY + top;      // 위로만 내림
        R.MaxY = BaseRect.MaxY - bottom;   // 아래로만 올림
        ViewInfo->ViewRect = R;

        // 콘텐츠 영역 AR을 카메라 행렬 빌드에 반영(왜곡 방지)
        const uint32 CW = R.Width();
        const uint32 CH = R.Height();
        ViewInfo->AspectRatio = float(CW) / float(FMath::Max(1u, CH));
    }
};
