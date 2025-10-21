#pragma once
#include "Actor.h"

class UAmbientLightComponent;

class AAmbientLightActor : public AActor
{
public:
	DECLARE_CLASS(AAmbientLightActor, AActor)
	GENERATED_REFLECTION_BODY()

	AAmbientLightActor();
protected:
	~AAmbientLightActor() override;

public:
	UAmbientLightComponent* GetLightComponent() const { return LightComponent; }

	void DuplicateSubObjects() override;
	DECLARE_DUPLICATE(AAmbientLightActor)

	void OnSerialized() override;

protected:
	UAmbientLightComponent* LightComponent;
};
