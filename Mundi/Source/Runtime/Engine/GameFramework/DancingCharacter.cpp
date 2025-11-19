#include "pch.h"
#include "DancingCharacter.h"
#include "SkeletalMeshComponent.h"

ADancingCharacter::ADancingCharacter()
{
	if (MeshComponent)
	{
		MeshComponent->SetSkeletalMesh(GDataDir + "/SillyDancing.fbx");
	}
}

ADancingCharacter::~ADancingCharacter()
{
}

void ADancingCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAnimSequence* AnimToPlay = UResourceManager::GetInstance().Get<UAnimSequence>(GDataDir + "/SillyDancing_mixamo.com");

	if (AnimToPlay && GetMesh())
	{
		MeshComponent->PlayAnimation(AnimToPlay, true);
		UE_LOG("ADancingCharacter: Started playing animation.");
	}
	else
	{
		UE_LOG("ADancingCharacter: Failedd to find animation to play");
	}
}
