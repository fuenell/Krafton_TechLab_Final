#include "pch.h"
#include "PhysicsAsset.h"
#include "JsonSerializer.h"

IMPLEMENT_CLASS(UPhysicsAsset)

// ────────────────────────────────────────────────────────────────
// Body 관리 메서드
// ────────────────────────────────────────────────────────────────

int32 UPhysicsAsset::AddBody(const FName& BoneName, int32 BoneIndex)
{
	// 이미 존재하는지 확인
	int32 ExistingIndex = FindBodyIndex(BoneName);
	if (ExistingIndex != -1)
	{
		return ExistingIndex;
	}

	// 새 바디 생성 (UObject이므로 NewObject 사용)
	UBodySetup* NewBody = NewObject<UBodySetup>();
	NewBody->BoneName = BoneName;
	NewBody->BoneIndex = BoneIndex;

	int32 NewIndex = static_cast<int32>(BodySetups.size());
	BodySetups.Add(NewBody);

	// 캐시 갱신
	BodySetupIndexMap.Add(BoneName, NewIndex);

	return NewIndex;
}

bool UPhysicsAsset::RemoveBody(int32 BodyIndex)
{
	if (BodyIndex < 0 || BodyIndex >= static_cast<int32>(BodySetups.size()))
	{
		return false;
	}

	// 연결된 제약 조건도 제거
	for (int32 i = static_cast<int32>(ConstraintSetups.size()) - 1; i >= 0; --i)
	{
		FConstraintSetup& Constraint = ConstraintSetups[i];
		if (Constraint.ParentBodyIndex == BodyIndex || Constraint.ChildBodyIndex == BodyIndex)
		{
			ConstraintSetups.RemoveAt(i);
		}
		else
		{
			// 인덱스 조정
			if (Constraint.ParentBodyIndex > BodyIndex)
			{
				Constraint.ParentBodyIndex--;
			}
			if (Constraint.ChildBodyIndex > BodyIndex)
			{
				Constraint.ChildBodyIndex--;
			}
		}
	}

	// 바디 제거
	BodySetups.RemoveAt(BodyIndex);

	// 캐시 재구성
	RebuildIndexMap();

	return true;
}

int32 UPhysicsAsset::FindBodyIndex(const FName& BoneName) const
{
	const int32* Found = BodySetupIndexMap.Find(BoneName);
	return Found ? *Found : -1;
}

int32 UPhysicsAsset::FindBodyIndexByBone(int32 BoneIndex) const
{
	for (int32 i = 0; i < static_cast<int32>(BodySetups.size()); ++i)
	{
		if (BodySetups[i] && BodySetups[i]->BoneIndex == BoneIndex)
		{
			return i;
		}
	}
	return -1;
}

// ────────────────────────────────────────────────────────────────
// Constraint 관리 메서드
// ────────────────────────────────────────────────────────────────

int32 UPhysicsAsset::AddConstraint(const FName& JointName, int32 ParentBodyIndex, int32 ChildBodyIndex)
{
	// 유효성 검사
	if (ParentBodyIndex < 0 || ParentBodyIndex >= static_cast<int32>(BodySetups.size()) ||
		ChildBodyIndex < 0 || ChildBodyIndex >= static_cast<int32>(BodySetups.size()))
	{
		return -1;
	}

	// 이미 존재하는지 확인
	int32 ExistingIndex = FindConstraintIndex(ParentBodyIndex, ChildBodyIndex);
	if (ExistingIndex != -1)
	{
		return ExistingIndex;
	}

	// 새 제약 조건 생성
	FConstraintSetup NewConstraint(JointName, ParentBodyIndex, ChildBodyIndex);
	int32 NewIndex = static_cast<int32>(ConstraintSetups.size());
	ConstraintSetups.Add(NewConstraint);

	return NewIndex;
}

bool UPhysicsAsset::RemoveConstraint(int32 ConstraintIndex)
{
	if (ConstraintIndex < 0 || ConstraintIndex >= static_cast<int32>(ConstraintSetups.size()))
	{
		return false;
	}

	ConstraintSetups.RemoveAt(ConstraintIndex);
	return true;
}

int32 UPhysicsAsset::FindConstraintIndex(int32 ParentBodyIndex, int32 ChildBodyIndex) const
{
	for (int32 i = 0; i < static_cast<int32>(ConstraintSetups.size()); ++i)
	{
		const FConstraintSetup& Constraint = ConstraintSetups[i];
		if (Constraint.ParentBodyIndex == ParentBodyIndex && Constraint.ChildBodyIndex == ChildBodyIndex)
		{
			return i;
		}
	}
	return -1;
}

// ────────────────────────────────────────────────────────────────
// 유틸리티
// ────────────────────────────────────────────────────────────────

void UPhysicsAsset::ClearAll()
{
	BodySetups.Empty();
	ConstraintSetups.Empty();
	BodySetupIndexMap.Empty();
}

void UPhysicsAsset::ClearBodySelection()
{
	// 선택 상태는 에디터(PhysicsAssetEditorState)에서 관리
	// UBodySetup은 데이터만 저장
}

void UPhysicsAsset::ClearConstraintSelection()
{
	for (FConstraintSetup& Constraint : ConstraintSetups)
	{
		Constraint.bSelected = false;
	}
}

bool UPhysicsAsset::IsValid() const
{
	// 최소한 하나의 바디가 있어야 함
	return !BodySetups.IsEmpty();
}

void UPhysicsAsset::RebuildIndexMap()
{
	BodySetupIndexMap.Empty();
	for (int32 i = 0; i < static_cast<int32>(BodySetups.size()); ++i)
	{
		if (BodySetups[i])
		{
			BodySetupIndexMap.Add(BodySetups[i]->BoneName, i);
		}
	}
}

// ────────────────────────────────────────────────────────────────
// 직렬화
// ────────────────────────────────────────────────────────────────

void UPhysicsAsset::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
	FWideString WidePath(InFilePath.begin(), InFilePath.end());

	JSON JsonHandle;
	if (!FJsonSerializer::LoadJsonFromFile(JsonHandle, WidePath))
	{
		UE_LOG("[UPhysicsAsset] Load 실패: %s", InFilePath.c_str());
		return;
	}

	Serialize(true, JsonHandle);
	UE_LOG("[UPhysicsAsset] Load 성공: %s", InFilePath.c_str());
}

void UPhysicsAsset::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	if (bInIsLoading)
	{
		UObject::Serialize(bInIsLoading, InOutHandle);
		BodySetups.Empty();
		JSON BodySetupsJson;
		if (FJsonSerializer::ReadArray(InOutHandle, "BodySetups", BodySetupsJson, JSON(), false))
		{
			for (int32 i = 0; i < static_cast<int32>(BodySetupsJson.size()); ++i)
			{
				const JSON& BodyJson = BodySetupsJson.at(i);
				UBodySetup* NewBody = NewObject<UBodySetup>();
				FString BoneNameStr;
				FJsonSerializer::ReadString(BodyJson, "BoneName", BoneNameStr, "", false);
				NewBody->BoneName = FName(BoneNameStr);
				FJsonSerializer::ReadInt32(BodyJson, "BoneIndex", NewBody->BoneIndex, -1, false);
				JSON AggGeomJson;
				if (FJsonSerializer::ReadObject(BodyJson, "AggGeom", AggGeomJson, JSON(), false))
				{
					JSON SphereElemsJson;
					if (FJsonSerializer::ReadArray(AggGeomJson, "SphereElems", SphereElemsJson, JSON(), false))
						for (int32 j = 0; j < static_cast<int32>(SphereElemsJson.size()); ++j)
						{
							const JSON& SphereJson = SphereElemsJson.at(j);
							FKSphereElem Sphere;
							FJsonSerializer::ReadVector(SphereJson, "Center", Sphere.Center, FVector::Zero(), false);
							FJsonSerializer::ReadFloat(SphereJson, "Radius", Sphere.Radius, 0.5f, false);
							NewBody->AggGeom.SphereElems.Add(Sphere);
						}
					JSON BoxElemsJson;
					if (FJsonSerializer::ReadArray(AggGeomJson, "BoxElems", BoxElemsJson, JSON(), false))
						for (int32 j = 0; j < static_cast<int32>(BoxElemsJson.size()); ++j)
						{
							const JSON& BoxJson = BoxElemsJson.at(j);
							FKBoxElem Box;
							FJsonSerializer::ReadVector(BoxJson, "Center", Box.Center, FVector::Zero(), false);
							FVector4 RotVec;
							FJsonSerializer::ReadVector4(BoxJson, "Rotation", RotVec, FVector4(0,0,0,1), false);
							Box.Rotation = FQuat(RotVec.X, RotVec.Y, RotVec.Z, RotVec.W);
							FJsonSerializer::ReadFloat(BoxJson, "X", Box.X, 1.f, false);
							FJsonSerializer::ReadFloat(BoxJson, "Y", Box.Y, 1.f, false);
							FJsonSerializer::ReadFloat(BoxJson, "Z", Box.Z, 1.f, false);
							NewBody->AggGeom.BoxElems.Add(Box);
						}
					JSON SphylElemsJson;
					if (FJsonSerializer::ReadArray(AggGeomJson, "SphylElems", SphylElemsJson, JSON(), false))
						for (int32 j = 0; j < static_cast<int32>(SphylElemsJson.size()); ++j)
						{
							const JSON& SphylJson = SphylElemsJson.at(j);
							FKSphylElem Sphyl;
							FJsonSerializer::ReadVector(SphylJson, "Center", Sphyl.Center, FVector::Zero(), false);
							FVector4 RotVec;
							FJsonSerializer::ReadVector4(SphylJson, "Rotation", RotVec, FVector4(0,0,0,1), false);
							Sphyl.Rotation = FQuat(RotVec.X, RotVec.Y, RotVec.Z, RotVec.W);
							FJsonSerializer::ReadFloat(SphylJson, "Radius", Sphyl.Radius, 0.5f, false);
							FJsonSerializer::ReadFloat(SphylJson, "Length", Sphyl.Length, 1.f, false);
							NewBody->AggGeom.SphylElems.Add(Sphyl);
						}
				}
				BodySetups.Add(NewBody);
			}
		}
		RebuildIndexMap();
	}
	else
	{
		UObject::Serialize(bInIsLoading, InOutHandle);
		JSON BodySetupsJson = JSON::Make(JSON::Class::Array);
		for (UBodySetup* Body : BodySetups)
		{
			if (!Body) continue;
			JSON BodyJson = JSON::Make(JSON::Class::Object);
			BodyJson["BoneName"] = Body->BoneName.ToString();
			BodyJson["BoneIndex"] = Body->BoneIndex;
			JSON AggGeomJson = JSON::Make(JSON::Class::Object);
			JSON SphereElemsJson = JSON::Make(JSON::Class::Array);
			for (const FKSphereElem& Sphere : Body->AggGeom.SphereElems)
			{
				JSON SphereJson = JSON::Make(JSON::Class::Object);
				SphereJson["Center"] = FJsonSerializer::VectorToJson(Sphere.Center);
				SphereJson["Radius"] = Sphere.Radius;
				SphereElemsJson.append(SphereJson);
			}
			AggGeomJson["SphereElems"] = SphereElemsJson;
			JSON BoxElemsJson = JSON::Make(JSON::Class::Array);
			for (const FKBoxElem& Box : Body->AggGeom.BoxElems)
			{
				JSON BoxJson = JSON::Make(JSON::Class::Object);
				BoxJson["Center"] = FJsonSerializer::VectorToJson(Box.Center);
				BoxJson["Rotation"] = FJsonSerializer::Vector4ToJson(FVector4(Box.Rotation.X, Box.Rotation.Y, Box.Rotation.Z, Box.Rotation.W));
				BoxJson["X"] = Box.X;
				BoxJson["Y"] = Box.Y;
				BoxJson["Z"] = Box.Z;
				BoxElemsJson.append(BoxJson);
			}
			AggGeomJson["BoxElems"] = BoxElemsJson;
			JSON SphylElemsJson = JSON::Make(JSON::Class::Array);
			for (const FKSphylElem& Sphyl : Body->AggGeom.SphylElems)
			{
				JSON SphylJson = JSON::Make(JSON::Class::Object);
				SphylJson["Center"] = FJsonSerializer::VectorToJson(Sphyl.Center);
				SphylJson["Rotation"] = FJsonSerializer::Vector4ToJson(FVector4(Sphyl.Rotation.X, Sphyl.Rotation.Y, Sphyl.Rotation.Z, Sphyl.Rotation.W));
				SphylJson["Radius"] = Sphyl.Radius;
				SphylJson["Length"] = Sphyl.Length;
				SphylElemsJson.append(SphylJson);
			}
			AggGeomJson["SphylElems"] = SphylElemsJson;
			BodyJson["AggGeom"] = AggGeomJson;
			BodySetupsJson.append(BodyJson);
		}
		InOutHandle["BodySetups"] = BodySetupsJson;
	}
}

void UPhysicsAsset::DuplicateSubObjects()
{
	UObject::DuplicateSubObjects();

	// UBodySetup*은 포인터이므로 복제 필요
	TArray<UBodySetup*> DuplicatedSetups;
	for (UBodySetup* BodySetup : BodySetups)
	{
		if (BodySetup)
		{
			UBodySetup* Duplicate = NewObject<UBodySetup>();
			Duplicate->BoneName = BodySetup->BoneName;
			Duplicate->BoneIndex = BodySetup->BoneIndex;
			Duplicate->AggGeom = BodySetup->AggGeom;
			DuplicatedSetups.Add(Duplicate);
		}
	}
	BodySetups = DuplicatedSetups;

	// FConstraintSetup은 값 타입이므로 자동 복제됨
}
