#pragma once
#include "Object.h"
#include "UEditorAssetPreviewContext.generated.h"

class USkeletalMesh;
class SWindow;
UCLASS()
class UEditorAssetPreviewContext : public UObject
{
public:
	GENERATED_REFLECTION_BODY()

	UEditorAssetPreviewContext();

	USkeletalMesh* SkeletalMesh;
	TArray<SWindow*> ListeningWindows;
	EViewerType ViewerType = EViewerType::None;
	FString AssetPath;
};