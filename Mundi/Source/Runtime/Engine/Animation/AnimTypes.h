#pragma once
#include "Vector.h"
#include "Archive.h"
#include "UEContainer.h"

/**
 * 단일 본의 키프레임을 포함하는 Raw 애니메이션 시퀀스 트랙
 * Position, Rotation, Scale 키프레임을 각각 저장
 */
struct FRawAnimSequenceTrack
{
public:
	/** 위치 키프레임 (Translation) */
	TArray<FVector> PosKeys;

	/** 회전 키프레임 (Quaternion) */
	TArray<FQuat> RotKeys;

	/** 스케일 키프레임 */
	TArray<FVector> ScaleKeys;

	/** 기본 생성자 */
	FRawAnimSequenceTrack()
	{
	}

	/** 위치 키프레임 개수 반환 */
	int32 GetNumPosKeys() const { return PosKeys.Num(); }

	/** 회전 키프레임 개수 반환 */
	int32 GetNumRotKeys() const { return RotKeys.Num(); }

	/** 스케일 키프레임 개수 반환 */
	int32 GetNumScaleKeys() const { return ScaleKeys.Num(); }

	/** 트랙이 비어있는지 확인 */
	bool IsEmpty() const
	{
		return PosKeys.Num() == 0 && RotKeys.Num() == 0 && ScaleKeys.Num() == 0;
	}

	/** 아카이브로 직렬화 */
	friend FArchive& operator<<(FArchive& Ar, FRawAnimSequenceTrack& Track)
	{
		// 위치 키 직렬화
		int32 NumPosKeys = Track.PosKeys.Num();
		Ar << NumPosKeys;
		if (Ar.IsLoading())
		{
			Track.PosKeys.SetNum(NumPosKeys);
		}
		for (int32 i = 0; i < NumPosKeys; ++i)
		{
			Ar << Track.PosKeys[i];
		}

		// 회전 키 직렬화
		int32 NumRotKeys = Track.RotKeys.Num();
		Ar << NumRotKeys;
		if (Ar.IsLoading())
		{
			Track.RotKeys.SetNum(NumRotKeys);
		}
		for (int32 i = 0; i < NumRotKeys; ++i)
		{
			Ar << Track.RotKeys[i];
		}

		// 스케일 키 직렬화
		int32 NumScaleKeys = Track.ScaleKeys.Num();
		Ar << NumScaleKeys;
		if (Ar.IsLoading())
		{
			Track.ScaleKeys.SetNum(NumScaleKeys);
		}
		for (int32 i = 0; i < NumScaleKeys; ++i)
		{
			Ar << Track.ScaleKeys[i];
		}

		return Ar;
	}
};

/**
 * 특정 본에 대한 애니메이션 트랙
 * 본 인덱스와 해당 본의 Raw 키프레임 데이터를 포함
 */
struct FBoneAnimationTrack
{
public:
	/** 스켈레톤의 본 배열에서의 인덱스 */
	int32 BoneIndex;

	/** 참조 및 디버깅을 위한 본 이름 */
	FString BoneName;

	/** 이 본의 Raw 키프레임 데이터 */
	FRawAnimSequenceTrack InternalTrack;

	/** 기본 생성자 */
	FBoneAnimationTrack()
		: BoneIndex(-1)
	{
	}

	/** 본 인덱스를 받는 생성자 */
	explicit FBoneAnimationTrack(int32 InBoneIndex)
		: BoneIndex(InBoneIndex)
	{
	}

	/** 본 인덱스와 이름을 받는 생성자 */
	FBoneAnimationTrack(int32 InBoneIndex, const FString& InBoneName)
		: BoneIndex(InBoneIndex)
		, BoneName(InBoneName)
	{
	}

	/** 트랙이 유효한지 확인 */
	bool IsValid() const
	{
		return BoneIndex >= 0 && !InternalTrack.IsEmpty();
	}

	/** 아카이브로 직렬화 */
	friend FArchive& operator<<(FArchive& Ar, FBoneAnimationTrack& Track)
	{
		Ar << Track.BoneIndex;
		Ar << Track.BoneName;
		Ar << Track.InternalTrack;
		return Ar;
	}
};
