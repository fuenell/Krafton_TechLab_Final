#pragma once
#include "Actor.h"

class USpotLightComponent;

class ASpotLightActor : public AActor
{
public:
	DECLARE_CLASS(ASpotLightActor, AActor)
	GENERATED_REFLECTION_BODY()

	ASpotLightActor();
protected:
	~ASpotLightActor() override;

public:
	USpotLightComponent* GetLightComponent() const { return LightComponent; }

	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(ASpotLightActor)

	void OnSerialized() override;

protected:
	USpotLightComponent* LightComponent;
};
