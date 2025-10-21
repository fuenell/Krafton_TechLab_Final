#include "pch.h"
#include "PropertyRenderer.h"
#include "ImGui/imgui.h"
#include "Vector.h"
#include "Color.h"
#include "SceneComponent.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "StaticMesh.h"
#include "Material.h"
#include "BillboardComponent.h"
#include "DecalComponent.h"
#include "StaticMeshComponent.h"
#include "LightComponentBase.h"

TArray<FString> UPropertyRenderer::CachedMaterialPaths;
TArray<const char*> UPropertyRenderer::CachedMaterialItems;
TArray<FString> UPropertyRenderer::CachedShaderPaths;
TArray<const char*> UPropertyRenderer::CachedShaderItems;
TArray<FString> UPropertyRenderer::CachedTexturePaths;
TArray<const char*> UPropertyRenderer::CachedTextureItems;

bool UPropertyRenderer::RenderProperty(const FProperty& Property, void* ObjectInstance)
{
	bool bChanged = false;

	switch (Property.Type)
	{
	case EPropertyType::Bool:
		bChanged = RenderBoolProperty(Property, ObjectInstance);
		break;

	case EPropertyType::Int32:
		bChanged = RenderInt32Property(Property, ObjectInstance);
		break;

	case EPropertyType::Float:
		bChanged = RenderFloatProperty(Property, ObjectInstance);
		break;

	case EPropertyType::FVector:
		bChanged = RenderVectorProperty(Property, ObjectInstance);
		break;

	case EPropertyType::FLinearColor:
		bChanged = RenderColorProperty(Property, ObjectInstance);
		break;

	case EPropertyType::FString:
		bChanged = RenderStringProperty(Property, ObjectInstance);
		break;

	case EPropertyType::FName:
		bChanged = RenderNameProperty(Property, ObjectInstance);
		break;

	case EPropertyType::ObjectPtr:
		bChanged = RenderObjectPtrProperty(Property, ObjectInstance);
		break;

	case EPropertyType::Struct:
		bChanged = RenderStructProperty(Property, ObjectInstance);
		break;

	case EPropertyType::Texture:
		bChanged = RenderTextureProperty(Property, ObjectInstance);
		break;

	case EPropertyType::StaticMesh:
		bChanged = RenderStaticMeshProperty(Property, ObjectInstance);
		break;

	case EPropertyType::Material:
		bChanged = RenderMaterialProperty(Property, ObjectInstance);
		break;

	case EPropertyType::Array:
		switch (Property.InnerType)
		{
		case EPropertyType::Material:
			bChanged = RenderMaterialArrayProperty(Property, ObjectInstance);
			break;
		}
		break;

	default:
		ImGui::Text("%s: [Unknown Type]", Property.Name);
		break;
	}

	if (Property.Tooltip && Property.Tooltip[0] != '\0' && ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", Property.Tooltip);
	}

	// SceneComponent는 Transform 프로퍼티가 변경되면 Setter를 통해 동기화
	if (bChanged && ObjectInstance)
	{
		UObject* Obj = static_cast<UObject*>(ObjectInstance);
		if (USceneComponent* SceneComponent = Cast<USceneComponent>(Obj))
		{
			// 프로퍼티 이름으로 Transform 프로퍼티 판별 후 Setter 호출
			if (strcmp(Property.Name, "RelativeRotationEuler") == 0)
			{
				FVector* EulerValue = Property.GetValuePtr<FVector>(ObjectInstance);
				SceneComponent->SetRelativeRotationEuler(*EulerValue);
			}
			else if (strcmp(Property.Name, "RelativeLocation") == 0)
			{
				FVector* LocationValue = Property.GetValuePtr<FVector>(ObjectInstance);
				SceneComponent->SetRelativeLocation(*LocationValue);
			}
			else if (strcmp(Property.Name, "RelativeScale") == 0)
			{
				FVector* ScaleValue = Property.GetValuePtr<FVector>(ObjectInstance);
				SceneComponent->SetRelativeScale(*ScaleValue);
			}
		}

		// LightComponent는 Light 프로퍼티가 변경되면 UpdateLightData를 호출하여 동기화
		if (ULightComponentBase* LightComponent = Cast<ULightComponentBase>(Obj))
		{
			// Light 관련 프로퍼티가 변경되면 UpdateLightData 호출
			if (strcmp(Property.Name, "LightColor") == 0 ||
				strcmp(Property.Name, "Intensity") == 0 ||
				strcmp(Property.Name, "Temperature") == 0 ||
				strcmp(Property.Name, "bIsVisible") == 0)
			{
				LightComponent->UpdateLightData();
			}
		}
	}

	return bChanged;
}

void UPropertyRenderer::RenderAllProperties(UObject* Object)
{
	if (!Object)
		return;

	UClass* Class = Object->GetClass();
	const TArray<FProperty>& Properties = Class->GetProperties();

	if (Properties.IsEmpty())
		return;

	// 카테고리별로 그룹화
	TMap<FString, TArray<const FProperty*>> CategorizedProps;

	for (const FProperty& Prop : Properties)
	{
		if (Prop.bIsEditAnywhere)
		{
			FString CategoryName = Prop.Category ? Prop.Category : "Default";
			if (!CategorizedProps.Contains(CategoryName))
			{
				CategorizedProps.Add(CategoryName, TArray<const FProperty*>());
			}
			CategorizedProps[CategoryName].Add(&Prop);
		}
	}

	// 카테고리별로 렌더링
	for (auto& Pair : CategorizedProps)
	{
		const FString& Category = Pair.first;
		const TArray<const FProperty*>& Props = Pair.second;

		ImGui::Separator();
		ImGui::Text("%s", Category.c_str());

		for (const FProperty* Prop : Props)
		{
			RenderProperty(*Prop, Object);
		}
	}
}

void UPropertyRenderer::RenderAllPropertiesWithInheritance(UObject* Object)
{
	if (!Object)
		return;

	UClass* Class = Object->GetClass();
	const TArray<FProperty>& AllProperties = Class->GetAllProperties();

	if (AllProperties.IsEmpty())
		return;

	// 카테고리별로 그룹화
	TMap<FString, TArray<const FProperty*>> CategorizedProps;

	for (const FProperty& Prop : AllProperties)
	{
		if (Prop.bIsEditAnywhere)
		{
			FString CategoryName = Prop.Category ? Prop.Category : "Default";
			if (!CategorizedProps.Contains(CategoryName))
			{
				CategorizedProps.Add(CategoryName, TArray<const FProperty*>());
			}
			CategorizedProps[CategoryName].Add(&Prop);
		}
	}

	// 카테고리별로 렌더링
	for (auto& Pair : CategorizedProps)
	{
		const FString& Category = Pair.first;
		const TArray<const FProperty*>& Props = Pair.second;

		ImGui::Separator();
		ImGui::Text("%s", Category.c_str());

		for (const FProperty* Prop : Props)
		{
			RenderProperty(*Prop, Object);
		}
	}
}

// ===== 리소스 캐싱 =====

void UPropertyRenderer::CacheMaterialResources()
{
	// 이미 캐시되어 있다면 다시 로드하지 않습니다.
	if (!CachedMaterialPaths.IsEmpty())
	{
		return;
	}

	UResourceManager& ResMgr = UResourceManager::GetInstance();

	// 1. 머티리얼
	CachedMaterialPaths = ResMgr.GetAllFilePaths<UMaterial>();
	CachedMaterialItems.Add("None");
	for (const FString& path : CachedMaterialPaths)
	{
		CachedMaterialItems.push_back(path.c_str());
	}

	// 2. 셰이더
	CachedShaderPaths = ResMgr.GetAllFilePaths<UShader>();
	CachedShaderItems.Add("None");
	for (const FString& path : CachedShaderPaths)
	{
		CachedShaderItems.push_back(path.c_str());
	}

	// 3. 텍스처
	CachedTexturePaths = ResMgr.GetAllFilePaths<UTexture>();
	CachedTextureItems.Add("None");
	for (const FString& path : CachedTexturePaths)
	{
		CachedTextureItems.push_back(path.c_str());
	}
}

void UPropertyRenderer::ClearMaterialResourcesCache()
{
	CachedMaterialPaths.Empty();
	CachedMaterialItems.Empty();
	CachedShaderPaths.Empty();
	CachedShaderItems.Empty();
	CachedTexturePaths.Empty();
	CachedTextureItems.Empty();
}

// ===== 타입별 렌더링 구현 =====

bool UPropertyRenderer::RenderBoolProperty(const FProperty& Prop, void* Instance)
{
	bool* Value = Prop.GetValuePtr<bool>(Instance);
	return ImGui::Checkbox(Prop.Name, Value);
}

bool UPropertyRenderer::RenderInt32Property(const FProperty& Prop, void* Instance)
{
	int32* Value = Prop.GetValuePtr<int32>(Instance);
	return ImGui::DragInt(Prop.Name, Value, 1.0f, (int)Prop.MinValue, (int)Prop.MaxValue);
}

bool UPropertyRenderer::RenderFloatProperty(const FProperty& Prop, void* Instance)
{
	float* Value = Prop.GetValuePtr<float>(Instance);

	// Min과 Max가 둘 다 0이면 범위 제한 없음
	if (Prop.MinValue == 0.0f && Prop.MaxValue == 0.0f)
	{
		return ImGui::DragFloat(Prop.Name, Value, 0.01f);
	}
	else
	{
		return ImGui::DragFloat(Prop.Name, Value, 0.01f, Prop.MinValue, Prop.MaxValue);
	}
}

bool UPropertyRenderer::RenderVectorProperty(const FProperty& Prop, void* Instance)
{
	FVector* Value = Prop.GetValuePtr<FVector>(Instance);

	return ImGui::DragFloat3(Prop.Name, &Value->X, 0.1f);
}

bool UPropertyRenderer::RenderColorProperty(const FProperty& Prop, void* Instance)
{
	FLinearColor* Value = Prop.GetValuePtr<FLinearColor>(Instance);
	return ImGui::ColorEdit4(Prop.Name, &Value->R);
}

bool UPropertyRenderer::RenderStringProperty(const FProperty& Prop, void* Instance)
{
	FString* Value = Prop.GetValuePtr<FString>(Instance);

	// ImGui::InputText는 char 버퍼를 사용하므로 변환 필요
	char Buffer[256];
	strncpy_s(Buffer, Value->c_str(), sizeof(Buffer) - 1);
	Buffer[sizeof(Buffer) - 1] = '\0';

	if (ImGui::InputText(Prop.Name, Buffer, sizeof(Buffer)))
	{
		*Value = Buffer;
		return true;
	}

	return false;
}

bool UPropertyRenderer::RenderNameProperty(const FProperty& Prop, void* Instance)
{
	FName* Value = Prop.GetValuePtr<FName>(Instance);

	// FName은 읽기 전용으로 표시
	FString NameStr = Value->ToString();
	ImGui::Text("%s: %s", Prop.Name, NameStr.c_str());

	return false;
}

bool UPropertyRenderer::RenderObjectPtrProperty(const FProperty& Prop, void* Instance)
{
	UObject** ObjPtr = Prop.GetValuePtr<UObject*>(Instance);

	if (*ObjPtr)
	{
		// 객체가 있으면 클래스 이름과 객체 이름 표시
		UClass* ObjClass = (*ObjPtr)->GetClass();
		FString ObjName = (*ObjPtr)->GetName();
		ImGui::Text("%s: %s (%s)", Prop.Name, ObjName.c_str(), ObjClass->Name);
	}
	else
	{
		// nullptr이면 None 표시
		ImGui::Text("%s: None", Prop.Name);
	}

	return false; // 읽기 전용
}

bool UPropertyRenderer::RenderStructProperty(const FProperty& Prop, void* Instance)
{
	// Struct는 읽기 전용으로 표시
	// FVector, FLinearColor 등 주요 타입은 이미 별도로 처리됨
	ImGui::Text("%s: [Struct]", Prop.Name);
	return false;
}

bool UPropertyRenderer::RenderTextureProperty(const FProperty& Prop, void* Instance)
{
	UTexture** TexturePtr = Prop.GetValuePtr<UTexture*>(Instance);

	FString CurrentPath;
	if (*TexturePtr)
	{
		CurrentPath = (*TexturePtr)->GetTextureName();
	}

	const TArray<FString> TexturePaths = UResourceManager::GetInstance().GetAllFilePaths<UTexture>();

	if (TexturePaths.empty())
	{
		ImGui::Text("%s: <No Textures>", Prop.Name);
		return false;
	}

	TArray<const char*> Items;
	Items.reserve(TexturePaths.size());
	for (const FString& path : TexturePaths)
		Items.push_back(path.c_str());

	int SelectedIdx = -1;
	for (int i = 0; i < static_cast<int>(TexturePaths.size()); ++i)
	{
		if (TexturePaths[i] == CurrentPath)
		{
			SelectedIdx = i;
			break;
		}
	}

	ImGui::SetNextItemWidth(240);
	if (ImGui::Combo(Prop.Name, &SelectedIdx, Items.data(), static_cast<int>(Items.size())))
	{
		if (SelectedIdx >= 0 && SelectedIdx < static_cast<int>(TexturePaths.size()))
		{
			*TexturePtr = UResourceManager::GetInstance().Load<UTexture>(TexturePaths[SelectedIdx]);
			if (*TexturePtr)
			{
				(*TexturePtr)->SetTextureName(TexturePaths[SelectedIdx]);
			}

			// 컴포넌트별 Setter 호출
			UObject* Obj = static_cast<UObject*>(Instance);
			if (UBillboardComponent* Billboard = Cast<UBillboardComponent>(Obj))
			{
				Billboard->SetTextureName(TexturePaths[SelectedIdx]);
			}
			else if (UDecalComponent* Decal = Cast<UDecalComponent>(Obj))
			{
				Decal->SetDecalTexture(TexturePaths[SelectedIdx]);
			}

			return true;
		}
	}

	return false;
}

bool UPropertyRenderer::RenderStaticMeshProperty(const FProperty& Prop, void* Instance)
{
	UStaticMesh** MeshPtr = Prop.GetValuePtr<UStaticMesh*>(Instance);

	FString CurrentPath;
	if (*MeshPtr)
	{
		CurrentPath = (*MeshPtr)->GetFilePath();
	}

	const TArray<FString> MeshPaths = UResourceManager::GetInstance().GetAllFilePaths<UStaticMesh>();

	if (MeshPaths.empty())
	{
		ImGui::Text("%s: <No Meshes>", Prop.Name);
		return false;
	}

	TArray<const char*> Items;
	Items.reserve(MeshPaths.size());
	for (const FString& path : MeshPaths)
		Items.push_back(path.c_str());

	int SelectedIdx = -1;
	for (int i = 0; i < static_cast<int>(MeshPaths.size()); ++i)
	{
		if (MeshPaths[i] == CurrentPath)
		{
			SelectedIdx = i;
			break;
		}
	}

	ImGui::SetNextItemWidth(240);
	if (ImGui::Combo(Prop.Name, &SelectedIdx, Items.data(), static_cast<int>(Items.size())))
	{
		if (SelectedIdx >= 0 && SelectedIdx < static_cast<int>(MeshPaths.size()))
		{
			// 컴포넌트별 Setter 호출
			UObject* Object = static_cast<UObject*>(Instance);
			if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Object))
			{
				StaticMeshComponent->SetStaticMesh(MeshPaths[SelectedIdx]);
			}
			else
			{
				*MeshPtr = UResourceManager::GetInstance().Load<UStaticMesh>(MeshPaths[SelectedIdx]);
			}
			return true;
		}
	}

	return false;
}

bool UPropertyRenderer::RenderMaterialProperty(const FProperty& Prop, void* Instance)
{
	// 렌더링에 필요한 리소스가 캐시되었는지 확인하고, 없으면 로드합니다.
	CacheMaterialResources();

	UMaterialInterface** MaterialPtr = Prop.GetValuePtr<UMaterialInterface*>(Instance);
	if (!MaterialPtr)
	{
		ImGui::Text("%s: [Invalid Material Ptr]", Prop.Name);
		return false;
	}

	UObject* OwningObject = static_cast<UObject*>(Instance);

	// Material 렌더 함수 호출
	return RenderSingleMaterialSlot(Prop.Name, MaterialPtr, OwningObject, 0);	// 단일 슬롯은 0번 인덱스
}

bool UPropertyRenderer::RenderMaterialArrayProperty(const FProperty& Prop, void* Instance)
{
	// 렌더링에 필요한 리소스가 캐시되었는지 확인하고, 없으면 로드합니다.
	CacheMaterialResources();

	TArray<UMaterialInterface*>* MaterialSlots = Prop.GetValuePtr<TArray<UMaterialInterface*>>(Instance);
	if (!MaterialSlots)
	{
		ImGui::Text("%s: [Invalid Array Ptr]", Prop.Name);
		return false;
	}

	UObject* OwningObject = static_cast<UObject*>(Instance);
	bool bArrayChanged = false;

	// 배열의 각 요소를 순회하며 헬퍼 함수 호출 (매개변수 없음)
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialSlots->Num(); ++MaterialIndex)
	{
		FString Label = FString(Prop.Name) + " [" + std::to_string(MaterialIndex) + "]";
		UMaterialInterface** MaterialPtr = &(*MaterialSlots)[MaterialIndex];

		if (RenderSingleMaterialSlot(Label.c_str(), MaterialPtr, OwningObject, MaterialIndex))
		{
			bArrayChanged = true;
		}
	}

	return bArrayChanged;
}

bool UPropertyRenderer::RenderSingleMaterialSlot(const char* Label, UMaterialInterface** MaterialPtr, UObject* OwningObject, uint32 MaterialIndex)
{
	bool bElementChanged = false;
	UMaterialInterface* CurrentMaterial = *MaterialPtr;

	// 캐시가 비어있으면 아무것도 렌더링하지 않음 (필수)
	if (CachedMaterialItems.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No materials available in cache.");
		return false;
	}

	// --- ImGui::BeginCombo / EndCombo 사용 ---

	// 1. 콤보박스에 현재 표시될 "미리보기 값"을 설정합니다.
	// CurrentMaterial이 유효하면 그 경로를, 아니면 "None"을 사용합니다.
	FString CurrentMaterialPath = (CurrentMaterial) ? CurrentMaterial->GetFilePath() : "None";

	ImGui::SetNextItemWidth(240);
	// 2. BeginCombo를 호출합니다. PreviewValue가 캐시 목록에 없어도 그대로 표시됩니다.
	if (ImGui::BeginCombo(Label, CurrentMaterialPath.c_str()))
	{
		// 3. 드롭다운 목록을 수동으로 렌더링합니다.
		// 기존 로직과 동일하게 CachedMaterialItems[0]은 "None"이라고 가정합니다.

		// "None" 옵션
		bool bIsNoneSelected = (CurrentMaterial == nullptr);
		if (ImGui::Selectable(CachedMaterialItems[0], bIsNoneSelected))
		{
			bElementChanged = true;
			if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(OwningObject))
			{
				PrimitiveComponent->SetMaterial(MaterialIndex, nullptr);
			}
			else
			{
				*MaterialPtr = nullptr;
			}
		}

		// "None"을 선택했을 때 포커스를 줍니다.
		if (bIsNoneSelected)
		{
			ImGui::SetItemDefaultFocus();
		}

		// 캐시된 머티리얼 목록 (CachedMaterialPaths[j]가 CachedMaterialItems[j+1]에 해당)
		for (int j = 0; j < (int)CachedMaterialPaths.size(); ++j)
		{
			const FString& Path = CachedMaterialPaths[j];
			// CachedMaterialItems[0]은 "None"이므로 j+1 인덱스를 사용합니다.
			const char* DisplayName = CachedMaterialItems[j + 1];

			bool bIsSelected = (CurrentMaterialPath == Path);

			if (ImGui::Selectable(DisplayName, bIsSelected))
			{
				bElementChanged = true;
				if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(OwningObject))
				{
					PrimitiveComponent->SetMaterialByName(MaterialIndex, Path);
				}
				else
				{
					// UPrimitiveComponent가 아닌 경우 리소스를 로드하여 직접 할당
					*MaterialPtr = UResourceManager::GetInstance().Load<UMaterial>(Path);
				}
			}

			// 현재 선택된 항목이 뷰에 보이도록 스크롤합니다.
			if (bIsSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}

		// 4. EndCombo를 호출하여 콤보박스를 닫습니다.
		ImGui::EndCombo();

		bElementChanged = true;
		CurrentMaterial = *MaterialPtr;
	}

	// --- UMaterialInterface 내부 프로퍼티 렌더링 (셰이더, 텍스처) ---
	if (CurrentMaterial)
	{
		ImGui::Indent();

		// --- 셰이더 렌더링 (읽기 전용, 비활성화된 InputText 사용) ---
		UShader* CurrentShader = CurrentMaterial->GetShader();
		FString CurrentShaderPath = (CurrentShader) ? CurrentShader->GetFilePath() : "None";

		// ImGui::InputText에 사용하기 위해 경로를 복사할 버퍼
		char ShaderPathBuffer[512]; // 경로 길이에 맞춰 버퍼 크기 조정
		strncpy_s(ShaderPathBuffer, sizeof(ShaderPathBuffer), CurrentShaderPath.c_str(), _TRUNCATE);

		FString ShaderLabel = "Shader##" + FString(Label);
		ImGui::SetNextItemWidth(400);

		// ImGui::BeginDisabled(true)로 감싸 위젯을 시각적으로 비활성화합니다.
		ImGui::BeginDisabled(true);

		// InputText는 계속 사용하되, 비활성화된 상태로 둡니다.
		ImGui::InputText(ShaderLabel.c_str(), ShaderPathBuffer, sizeof(ShaderPathBuffer), ImGuiInputTextFlags_ReadOnly);

		// 비활성화 상태를 해제합니다.
		ImGui::EndDisabled();

		// --- 텍스처 슬롯 렌더링 ---

		// 썸네일 미리보기 크기를 상수로 정의
		const float ThumbnailSize = 24.0f;
		const ImVec2 ThumbnailImVec(ThumbnailSize, ThumbnailSize);

		for (uint8 TexSlotIndex = 0; TexSlotIndex < (uint8)EMaterialTextureSlot::Max; ++TexSlotIndex)
		{
			EMaterialTextureSlot Slot = static_cast<EMaterialTextureSlot>(TexSlotIndex);
			UTexture* CurrentTexture = CurrentMaterial->GetTexture(Slot);
			FString CurrentTexturePath = (CurrentTexture) ? CurrentTexture->GetTextureName() : "None";

			// 1. 현재 선택된 텍스처의 인덱스 찾기
			int SelectedTextureIdx = 0; // "None"
			for (int j = 0; j < (int)CachedTexturePaths.size(); ++j)
			{
				if (CachedTexturePaths[j] == CurrentTexturePath)
				{
					SelectedTextureIdx = j + 1;
					break;
				}
			}

			// 2. 슬롯 이름 (Label) 설정
			const char* SlotName = "Unknown Slot";
			switch (Slot)
			{
			case EMaterialTextureSlot::Diffuse:
				SlotName = "Diffuse Texture";
				break;
			case EMaterialTextureSlot::Normal:
				SlotName = "Normal Texture";
				break;
				// ... (다른 텍스처 슬롯이 EMaterialTextureSlot에 추가되면 여기에도 case 추가) ...
			}
			FString TextureLabel = FString(SlotName) + "##" + Label;

			// 3. 콤보박스 왼쪽에 썸네일 표시
			if (CurrentTexture && CurrentTexture->GetShaderResourceView())
			{
				ImGui::Image(reinterpret_cast<ImTextureID>(CurrentTexture->GetShaderResourceView()), ThumbnailImVec);

				// 썸네일 이미지에 마우스를 올렸을 때 이미지/크기 툴팁
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(CurrentTexture->GetTextureName().c_str());
					ImGui::Text("Size: %u x %u", CurrentTexture->GetWidth(), CurrentTexture->GetHeight());

					// 썸네일 크기 계산 (가로/세로 비율 유지, 최대 128x128)
					const float MAX_THUMBNAIL_SIZE = 128.0f;
					float width = (float)CurrentTexture->GetWidth();
					float height = (float)CurrentTexture->GetHeight();
					float ratio = width / height;

					ImVec2 thumbSize;
					if (ratio > 1.0f) // 가로가 더 긴 이미지
					{
						thumbSize = { MAX_THUMBNAIL_SIZE, MAX_THUMBNAIL_SIZE / ratio };
					}
					else // 세로가 더 길거나 같은 이미지
					{
						thumbSize = { MAX_THUMBNAIL_SIZE * ratio, MAX_THUMBNAIL_SIZE };
					}

					ImGui::Image(reinterpret_cast<ImTextureID>(CurrentTexture->GetShaderResourceView()), thumbSize);
					ImGui::EndTooltip();
				}
			}
			else
			{
				// 리소스가 없을 때도 레이아웃 유지를 위해 Dummy 사용
				ImGui::Dummy(ThumbnailImVec);
			}

			ImGui::SameLine();

			// 4. 커스텀 콤보박스 시작
			// 콤보박스에 표시될 텍스트 (현재 선택된 항목의 텍스트)
			const char* PreviewText = CachedTextureItems[SelectedTextureIdx];

			// SetNextItemWidth를 썸네일 크기만큼 보정
			ImGui::SetNextItemWidth(220.0f - ThumbnailSize - ImGui::GetStyle().ItemSpacing.x);

			if (ImGui::BeginCombo(TextureLabel.c_str(), PreviewText))
			{
				// --- 콤보박스 드롭다운 리스트 렌더링 ---

				// 드롭다운 리스트 내부의 아이템 간 수직 간격 설정
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 1.0f));

				// 텍스처 리스트 (미리보기 포함) - "None" 옵션(i=0)을 루프에 포함
				for (int i = 0; i < (int)CachedTextureItems.size(); ++i)
				{
					bool is_selected = (SelectedTextureIdx == i);
					const char* ItemText = CachedTextureItems[i];

					UTexture* previewTexture = nullptr;
					FString TexturePath = "None";

					if (i > 0)
					{
						TexturePath = CachedTexturePaths[i - 1];
						previewTexture = UResourceManager::GetInstance().Load<UTexture>(TexturePath);
					}

					FString SelectableID = FString("##Selectable_") + TexturePath + std::to_string(i);

					// 1. Selectable을 먼저 호출
					if (ImGui::Selectable(SelectableID.c_str(), is_selected, 0, ImVec2(0, ThumbnailSize - 1.0f)))
					{
						// 1. 선택된 텍스처 리소스를 경로로부터 로드합니다.
						UTexture* SelectedTexture = (i == 0)
							? nullptr
							: UResourceManager::GetInstance().Load<UTexture>(TexturePath);

						// 2. 현재 머티리얼이 MID인지 확인합니다.
						UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(CurrentMaterial);

						// 3. MID가 아니라면 (즉, UMaterial 원본을 사용 중이라면)
						if (MID == nullptr)
						{
							// 3-1. OwningObject를 UStaticMeshComponent로 캐스팅합니다.
							if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(OwningObject))
							{
								// 3-2. 컴포넌트의 헬퍼 함수를 호출하여 MID를 생성하고 슬롯에 할당합니다.
								// 이 함수는 내부적으로 MaterialSlots[MaterialIndex]를 새 MID로 교체하고
								// DynamicMaterialInstances 배열에도 추가하여 메모리 관리를 보장합니다.
								MID = StaticMeshComp->CreateAndSetMaterialInstanceDynamic(MaterialIndex);

								// 3-3. 이 UI가 참조하는 로컬 변수도 새 MID로 업데이트합니다.
								CurrentMaterial = MID;
								// (*MaterialPtr = MID; 줄은 CreateAndSet... 함수가 이미 처리했으므로 필요 없습니다)
							}
							else
							{
								// UStaticMeshComponent가 아닌 다른 컴포넌트가 이 속성을 소유한 경우
								// (경고: 이 경우 MID의 소유권 추적이 어려워 메모리 릭이 발생할 수 있습니다.)
								MID = UMaterialInstanceDynamic::Create(CurrentMaterial);
								*MaterialPtr = MID; // 포인터가 가리키는 곳(배열)의 값을 직접 변경
								CurrentMaterial = MID;

								if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(OwningObject))
								{
									PrimitiveComponent->SetMaterial(MaterialIndex, MID);
								}
							}
						}

						// 4. 이제 CurrentMaterial은 반드시 MID이므로 (기존 MID 또는 방금 생성한 MID),
						//    UTexture* 포인터를 사용하여 파라미터를 설정합니다.
						if (MID) // (안전 검사)
						{
							MID->SetTextureParameterValue(Slot, SelectedTexture);
						}

						bElementChanged = true;
					}
					ImVec2 NextItemCursorPos = ImGui::GetCursorPos(); // 다음 아이템 위치 저장

					// 2. 툴팁 표시
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						if (previewTexture && previewTexture->GetTexture2D() && previewTexture->GetShaderResourceView())
						{
							ImGui::TextUnformatted(previewTexture->GetTextureName().c_str());
							ImGui::Text("Size: %u x %u", previewTexture->GetWidth(), previewTexture->GetHeight());
							float width = (float)previewTexture->GetWidth();
							float height = (float)previewTexture->GetHeight();
							float ratio = width / height;
							ImVec2 thumbSize = (ratio > 1.0f) ? ImVec2(128.0f, 128.0f / ratio) : ImVec2(128.0f * ratio, 128.0f);
							ImGui::Image(reinterpret_cast<ImTextureID>(previewTexture->GetShaderResourceView()), thumbSize);
						}
						else
						{
							ImGui::TextUnformatted("텍스처 없음");
						}
						ImGui::EndTooltip();
					}

					// 3. Selectable 위에 겹쳐 그리기 위해 커서를 되돌림
					ImVec2 ItemPos = ImGui::GetItemRectMin();

					// 4. 내용물(썸네일) 그리기
					ImVec2 ImagePos = ImVec2(ItemPos.x, ItemPos.y);
					ImGui::SetCursorScreenPos(ImagePos);

					if (previewTexture && previewTexture->GetShaderResourceView())
					{
						ImGui::Image(reinterpret_cast<ImTextureID>(previewTexture->GetShaderResourceView()), ThumbnailImVec);
					}
					else
					{
						ImGui::Dummy(ThumbnailImVec);
					}

					// 5. 내용물(텍스트) 그리기 (수정: SetCursorScreenPos 사용)
					// SameLine() + SetCursorPosY() 대신 절대 위치로 그림
					ImVec2 TextPos = ImVec2(
						ItemPos.x + ThumbnailSize + ImGui::GetStyle().ItemSpacing.x,
						ItemPos.y
					);
					ImGui::SetCursorScreenPos(TextPos);
					ImGui::TextUnformatted(ItemText);

					if (is_selected)
						ImGui::SetItemDefaultFocus();

					// 6. 커서 위치 복원
					ImGui::SetCursorPos(NextItemCursorPos);

					// --- 어설션 해결을 위해 Dummy 위젯 추가 ---
					// SetCursorPos로 커서를 강제 이동시킨 후,
					// ImGui가 레이아웃 경계를 인지하도록 더미 위젯을 추가합니다.
					ImGui::Dummy(ImVec2(0.0f, 0.0f));
				}

				// PushStyleVar로 변경했던 스타일을 원래대로 복원합니다.
				ImGui::PopStyleVar();

				ImGui::EndCombo();
			}

			// 닫힌 콤보박스 '텍스트' 부분에 마우스를 올렸을 때 전체 경로 툴팁
			// ImGui::IsItemHovered()는 마지막 아이템인 BeginCombo 위젯을 참조합니다.
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				// CurrentTexturePath는 CurrentTexture가 null일 때 "None"을 표시합니다.
				ImGui::TextUnformatted(CurrentTexturePath.c_str());
				ImGui::EndTooltip();
			}
		}

		ImGui::Unindent();
	}

	return bElementChanged;
}
