#pragma once

#include "ParticleModuleTypeDataBase.h"
#include "UParticleModuleTypeDataMesh.generated.h"

class UStaticMesh;

UCLASS(DisplayName="메시 타입 데이터", Description="메시 파티클을 위한 타입 데이터입니다")
class UParticleModuleTypeDataMesh : public UParticleModuleTypeDataBase
{
public:
	GENERATED_REFLECTION_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Mesh")
	UStaticMesh* Mesh = nullptr;

	// true면 RequiredModule->Material을 사용, false면 메시의 섹션별 Material 사용
	UPROPERTY(EditAnywhere, Category="Mesh")
	bool bOverrideMaterial = false;

	UParticleModuleTypeDataMesh() = default;
	virtual ~UParticleModuleTypeDataMesh() = default;

	virtual bool HasMesh() const override { return Mesh != nullptr; }

	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
