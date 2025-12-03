#include "pch.h"
#include "AnimSequenceBase.h"
#include "Vector.h"
#include "VertexData.h"
#include "JsonSerializer.h"

// 기본 구현: 바인드 포즈(로컬)로 채웁니다. 파생(UAnimSequence)에서 실제 트랙 기반 추출을 제공합니다.
void UAnimSequenceBase::ExtractBonePose(const FSkeleton& Skeleton, float Time, bool /*bLooping*/, bool /*bInterpolate*/, TArray<FTransform>& OutLocalPose) const
{
    const int32 NumBones = static_cast<int32>(Skeleton.Bones.Num());
    OutLocalPose.SetNum(NumBones);

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        const FBone& ThisBone = Skeleton.Bones[BoneIndex];
        const int32 ParentIndex = ThisBone.ParentIndex;
        FMatrix LocalBindMatrix;
        if (ParentIndex == -1)
        {
            LocalBindMatrix = ThisBone.BindPose;
        }
        else
        {
            const FMatrix& ParentInverseBindPose = Skeleton.Bones[ParentIndex].InverseBindPose;
            LocalBindMatrix = ThisBone.BindPose * ParentInverseBindPose;
        }
        OutLocalPose[BoneIndex] = FTransform(LocalBindMatrix);
    }
}

void UAnimSequenceBase::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    // 부모 클래스 직렬화 (UAnimationAsset - 기본 UPROPERTY만)
    UAnimationAsset::Serialize(bInIsLoading, InOutHandle);

    // Note: SequenceLength, SkeletonName, SkeletonSignature, SkeletonBoneCount는
    // FBX 로드 시 설정되므로 .animsequence 파일에 저장하지 않음
    // .animsequence는 NotifyTracks + SourceFilePath + 메타데이터만 저장
}

