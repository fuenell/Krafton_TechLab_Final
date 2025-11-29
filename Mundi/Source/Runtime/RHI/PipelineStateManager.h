#pragma once
#include "Object.h"
#include "UPipelineStateManager.generated.h"

UCLASS()
class UPipelineStateManager : public UObject
{
public:
	GENERATED_REFLECTION_BODY()
		UPipelineStateManager() {};
	static UPipelineStateManager& GetInstance();
	void Initialize();
};
