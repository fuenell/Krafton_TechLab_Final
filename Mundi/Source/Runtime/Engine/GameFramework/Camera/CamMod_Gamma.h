#pragma once

#include "CameraModifierBase.h"
#include "PostProcessing/PostProcessing.h"
#include "UCamMod_Gamma.generated.h"

UCLASS()
class UCamMod_Gamma : public UCameraModifierBase
{
public:
	GENERATED_REFLECTION_BODY()

	UCamMod_Gamma() = default;
	virtual ~UCamMod_Gamma() = default;

	/** gamma correction 변수*/
	float Gamma = 2.2f;

	virtual void ApplyToView(float DeltaTime, FMinimalViewInfo* ViewInfo) override {} ;

	virtual void CollectPostProcess(TArray<FPostProcessModifier>& Out) override
	{
		if (!bEnabled) return;

		FPostProcessModifier M;
		M.Type = EPostProcessEffectType::Gamma;
		M.bEnabled = true;
		M.Weight = Weight;
		M.SourceObject = this;

		M.Payload.Params0.X = Gamma;

		Out.Add(M);
	}
};