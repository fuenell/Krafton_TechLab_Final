#pragma once
#include "Object.h"
#include "UQuadManager.generated.h"

UCLASS()
class UQuadManager : public UObject
{
public:
	GENERATED_REFLECTION_BODY()

	UQuadManager();
	~UQuadManager();

	static UQuadManager* GetInstance()
	{
		static UQuadManager* Instance;
		if (!Instance)
		{
			Instance = NewObject<UQuadManager>();
		}
		return Instance;
	}

};

