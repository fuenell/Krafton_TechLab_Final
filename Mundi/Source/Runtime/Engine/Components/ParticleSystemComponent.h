#pragma once

#include "PrimitiveComponent.h"
#include "Source/Runtime/Engine/Particles/ParticleSystem.h"
#include "Source/Runtime/Engine/Particles/ParticleEmitterInstance.h"
#include "UParticleSystemComponent.generated.h"

struct FMeshBatchElement;
struct FSceneView;

UCLASS(DisplayName="파티클 시스템 컴포넌트", Description="파티클 시스템을 씬에 배치하는 컴포넌트입니다")
class UParticleSystemComponent : public UPrimitiveComponent
{
public:
	GENERATED_REFLECTION_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Particle System")
	UParticleSystem* Template = nullptr;

	UPROPERTY(EditAnywhere, Category="Particle System")
	bool bAutoActivate = true;

	// 이미터 인스턴스 (런타임)
	TArray<FParticleEmitterInstance*> EmitterInstances;

	// 렌더 데이터 (렌더링 스레드용)
	TArray<FDynamicEmitterDataBase*> EmitterRenderData;

	// 언리얼 엔진 호환: 인스턴스 파라미터 시스템
	// 게임플레이에서 파티클 속성을 동적으로 제어 가능
	struct FParticleParameter
	{
		FString Name;
		float FloatValue = 0.0f;
		FVector VectorValue = FVector(0.0f, 0.0f, 0.0f);
		FLinearColor ColorValue = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

		FParticleParameter() = default;
		explicit FParticleParameter(const FString& InName)
			: Name(InName)
		{
		}
	};

	TArray<FParticleParameter> InstanceParameters;

	// Dynamic Vertex / Index Buffer
	ID3D11Buffer* ParticleVertexBuffer = nullptr;
	ID3D11Buffer* ParticleIndexBuffer = nullptr;
	uint32 AllocatedVertexCount = 0;
	uint32 AllocatedIndexCount = 0;

	UParticleSystemComponent();
	virtual ~UParticleSystemComponent();

	// 초기화/정리
	virtual void OnRegister(UWorld* InWorld) override;    // 에디터/PIE 모두에서 호출
	virtual void OnUnregister() override;                 // 에디터/PIE 모두에서 호출

	// 틱
	virtual void TickComponent(float DeltaTime) override;

	// 활성화/비활성화
	void ActivateSystem();
	void DeactivateSystem();
	void ResetParticles();

	// 템플릿 설정
	void SetTemplate(UParticleSystem* NewTemplate);

	// 언리얼 엔진 호환: 인스턴스 파라미터 제어
	void SetFloatParameter(const FString& ParameterName, float Value);
	void SetVectorParameter(const FString& ParameterName, const FVector& Value);
	void SetColorParameter(const FString& ParameterName, const FLinearColor& Value);

	float GetFloatParameter(const FString& ParameterName, float DefaultValue = 0.0f) const;
	FVector GetVectorParameter(const FString& ParameterName, const FVector& DefaultValue = FVector(0.0f, 0.0f, 0.0f)) const;
	FLinearColor GetColorParameter(const FString& ParameterName, const FLinearColor& DefaultValue = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)) const;

	// 직렬화
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View) override;

	void EnsureBufferSize(uint32 RequiredVertexCount, uint32 RequiredIndexCount);

	void FillVertexBuffer(const FSceneView* View);

	void CreateMeshBatch(TArray<FMeshBatchElement>& OutMeshBatchElements, uint32 IndexCount);

private:
	void InitializeEmitterInstances();
	void ClearEmitterInstances();
	void UpdateRenderData();

	// 테스트용 디버그 파티클 시스템 생성
	void CreateDebugParticleSystem();
};
