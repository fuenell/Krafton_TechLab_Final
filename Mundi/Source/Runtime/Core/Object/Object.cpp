#include "pch.h"
#include "GarbageCollector.h"

FString UObject::GetName()
{
    return ObjectName.ToString();
}

FString UObject::GetComparisonName()
{
    return FString();
}

// 리플렉션 기반 자동 직렬화 (현재 클래스의 프로퍼티만 처리)
void UObject::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	const TArray<FProperty>& Properties = this->GetClass()->GetAllProperties();

	for (const FProperty& Prop : Properties)
	{
		switch (Prop.Type)
		{
		case EPropertyType::Bool:
		{
			bool* Value = Prop.GetValuePtr<bool>(this);
			if (bInIsLoading)
			{
				bool ReadValue;
				if (FJsonSerializer::ReadBool(InOutHandle, Prop.Name, ReadValue))
				{
					*Value = ReadValue;
				}
			}
			else
			{
				InOutHandle[Prop.Name] = *Value;
			}
			break;
		}
		case EPropertyType::Int32:
		{
			int32* Value = Prop.GetValuePtr<int32>(this);
			if (bInIsLoading)
			{
				int32 ReadValue;
				if (FJsonSerializer::ReadInt32(InOutHandle, Prop.Name, ReadValue))
				{
					*Value = ReadValue;
				}
			}
			else
			{
				InOutHandle[Prop.Name] = *Value;
			}
			break;
		}
		case EPropertyType::Float:
		{
			float* Value = Prop.GetValuePtr<float>(this);
			if (bInIsLoading)
			{
				float ReadValue;
				if (FJsonSerializer::ReadFloat(InOutHandle, Prop.Name, ReadValue))
				{
					*Value = ReadValue;
				}
			}
			else
			{
				InOutHandle[Prop.Name] = *Value;
			}
			break;
		}
		case EPropertyType::FVector:
		{
			FVector* Value = Prop.GetValuePtr<FVector>(this);
			if (bInIsLoading)
			{
				FVector ReadValue;
				if (FJsonSerializer::ReadVector(InOutHandle, Prop.Name, ReadValue))
				{
					*Value = ReadValue;
				}
			}
			else
			{
				InOutHandle[Prop.Name] = FJsonSerializer::VectorToJson(*Value);
			}
			break;
		}
		case EPropertyType::FLinearColor:
		{
			FLinearColor* Value = Prop.GetValuePtr<FLinearColor>(this);
			if (bInIsLoading)
			{
				FVector4 ReadValue;
				if (FJsonSerializer::ReadVector4(InOutHandle, Prop.Name, ReadValue))
				{
					*Value = FLinearColor(ReadValue);
				}
			}
			else
			{
				InOutHandle[Prop.Name] = FJsonSerializer::Vector4ToJson(Value->ToFVector4());
			}
			break;
		}
		case EPropertyType::FString:
		case EPropertyType::ScriptFile:
		{
			FString* Value = Prop.GetValuePtr<FString>(this);
			if (bInIsLoading)
			{
				FString ReadValue;
				if (FJsonSerializer::ReadString(InOutHandle, Prop.Name, ReadValue))
				{
					*Value = ReadValue;
				}
			}
			else
			{
				InOutHandle[Prop.Name] = Value->c_str();
			}
			break;
		}
		case EPropertyType::FName:
		{
			FName* Value = Prop.GetValuePtr<FName>(this);
			if (bInIsLoading)
			{
				FString ReadValue;
				if (FJsonSerializer::ReadString(InOutHandle, Prop.Name, ReadValue))
				{
					*Value = FName(ReadValue);
				}
			}
			else
			{
				InOutHandle[Prop.Name] = Value->ToString().c_str();
			}
			break;
		}
		case EPropertyType::Texture:
		{
			UTexture** Value = Prop.GetValuePtr<UTexture*>(this);
			if (bInIsLoading)
			{
				FString TexturePath;
				FJsonSerializer::ReadString(InOutHandle, Prop.Name, TexturePath);
				if (!TexturePath.empty())
				{
					*Value = UResourceManager::GetInstance().Load<UTexture>(TexturePath);
				}
				else
				{
					*Value = nullptr;
				}
			}
			else
			{
				if (*Value)
				{
					InOutHandle[Prop.Name] = (*Value)->GetFilePath().c_str();
				}
				else
				{
					InOutHandle[Prop.Name] = "";
				}
			}
			break;
		}
		case EPropertyType::StaticMesh:
		{
			UStaticMesh** Value = Prop.GetValuePtr<UStaticMesh*>(this);
			if (bInIsLoading)
			{
				FString MeshPath;
				FJsonSerializer::ReadString(InOutHandle, Prop.Name, MeshPath);
				if (!MeshPath.empty())
				{
					*Value = UResourceManager::GetInstance().Load<UStaticMesh>(MeshPath);
				}
				else
				{
					*Value = nullptr;
				}
			}
			else
			{
				if (*Value)
				{
					InOutHandle[Prop.Name] = (*Value)->GetAssetPathFileName().c_str();
				}
				else
				{
					InOutHandle[Prop.Name] = "";
				}
			}
			break;
		}
		case EPropertyType::SkeletalMesh:
		{
			USkeletalMesh** Value = Prop.GetValuePtr<USkeletalMesh*>(this);
			if (bInIsLoading)
			{
				FString MeshPath;
				FJsonSerializer::ReadString(InOutHandle, Prop.Name, MeshPath);
				if (!MeshPath.empty())
				{
					*Value = UResourceManager::GetInstance().Load<USkeletalMesh>(MeshPath);
				}
				else
				{
					*Value = nullptr;
				}
			}
			else
			{
				if (*Value)
				{
					InOutHandle[Prop.Name] = (*Value)->GetPathFileName().c_str();
				}
				else
				{
					InOutHandle[Prop.Name] = "";
				}
			}
			break;
		}
		case EPropertyType::Material:
		{
			UMaterial** Value = Prop.GetValuePtr<UMaterial*>(this);
			if (bInIsLoading)
			{
				FString MaterialPath;
				FJsonSerializer::ReadString(InOutHandle, Prop.Name, MaterialPath);
				if (!MaterialPath.empty())
				{
					*Value = UResourceManager::GetInstance().Load<UMaterial>(MaterialPath);
				}
				else
				{
					*Value = nullptr;
				}
			}
			else
			{
				if (*Value)
				{
					InOutHandle[Prop.Name] = (*Value)->GetFilePath().c_str();
				}
				else
				{
					InOutHandle[Prop.Name] = "";
				}
			}
			break;
		}
		case EPropertyType::ParticleSystem:
		{
			UParticleSystem** Value = Prop.GetValuePtr<UParticleSystem*>(this);
			if (bInIsLoading)
			{
				FString ParticlePath;
				FJsonSerializer::ReadString(InOutHandle, Prop.Name, ParticlePath);
				if (!ParticlePath.empty())
				{
					*Value = UResourceManager::GetInstance().Load<UParticleSystem>(ParticlePath);
				}
				else
				{
					*Value = nullptr;
				}
			}
			else
			{
				if (*Value)
				{
					InOutHandle[Prop.Name] = (*Value)->GetFilePath().c_str();
				}
				else
				{
					InOutHandle[Prop.Name] = "";
				}
			}
			break;
		}
		case EPropertyType::Curve:
		{
			// Curve 프로퍼티는 float[4] 배열입니다. 따라서 FVector4로 처리
			if (bInIsLoading)
			{
				FVector4 TempVec4;

				// 1. FJsonSerializer::ReadVector4를 사용해 JSON에서 4개 요소를 읽어옵니다.
				if (FJsonSerializer::ReadVector4(InOutHandle, Prop.Name, TempVec4))
				{
					// 2. 프로퍼티(float[4])의 실제 주소를 가져옵니다.
					float* PropData = Prop.GetValuePtr<float>(this);

					// 3. 읽어온 FVector4의 내용을 float[4] 메모리 위치에 복사합니다.
					memcpy(PropData, &TempVec4, sizeof(float) * 4);
				}
			}
			else // Saving
			{
				// 1. 프로퍼티(float[4])의 실제 주소를 가져옵니다.
				float* PropData = Prop.GetValuePtr<float>(this);

				// 2. float[4] 배열의 값으로 임시 FVector4 객체를 생성합니다.
				FVector4 TempVec4(PropData[0], PropData[1], PropData[2], PropData[3]);

				// 3. FJsonSerializer::Vector4ToJson을 사용해 JSON 배열로 변환하고 씁니다.
				InOutHandle[Prop.Name] = FJsonSerializer::Vector4ToJson(TempVec4);
			}
			break;
		}
		case EPropertyType::Array:
		{
			// Array 직렬화는 InnerType에 따라 처리
			if (Prop.InnerType == EPropertyType::Unknown)
			{
				UE_LOG("[AutoSerialize] Array property '%s' has Unknown InnerType, skipping.", Prop.Name);
				break;
			}

			JSON ArrayJson;
			if (bInIsLoading)
			{
				// JSON에서 배열 읽기
				if (!FJsonSerializer::ReadArray(InOutHandle, Prop.Name, ArrayJson))
				{
					break; // 배열이 없거나 유효하지 않음
				}
			}
			else
			{
				// 저장용 빈 배열 생성
				ArrayJson = JSON::Make(JSON::Class::Array);
			}

			// InnerType에 따라 처리
			switch (Prop.InnerType)
			{
			case EPropertyType::Int32:
			{
				TArray<int32>* ArrayPtr = Prop.GetValuePtr<TArray<int32>>(this);
				SerializePrimitiveArray<int32>(ArrayPtr, bInIsLoading, ArrayJson);
				break;
			}
			case EPropertyType::Float:
			{
				TArray<float>* ArrayPtr = Prop.GetValuePtr<TArray<float>>(this);
				SerializePrimitiveArray<float>(ArrayPtr, bInIsLoading, ArrayJson);
				break;
			}
			case EPropertyType::Bool:
			{
				TArray<bool>* ArrayPtr = Prop.GetValuePtr<TArray<bool>>(this);
				SerializePrimitiveArray<bool>(ArrayPtr, bInIsLoading, ArrayJson);
				break;
			}
			case EPropertyType::FString:
			{
				TArray<FString>* ArrayPtr = Prop.GetValuePtr<TArray<FString>>(this);
				SerializePrimitiveArray<FString>(ArrayPtr, bInIsLoading, ArrayJson);
				break;
			}
			
			case EPropertyType::Sound:
			{
				TArray<USound*>* ArrayPtr = Prop.GetValuePtr<TArray<USound*>>(this);
				if (bInIsLoading)
				{
					ArrayPtr->Empty();
					for (size_t i = 0; i < ArrayJson.size(); ++i)
					{
						const JSON& Elem = ArrayJson.at(i);
						if (Elem.JSONType() == JSON::Class::String)
						{
							FString Path = Elem.ToString();
							if (!Path.empty()) ArrayPtr->Add(UResourceManager::GetInstance().Load<USound>(Path));
							else ArrayPtr->Add(nullptr);
						}
					}
				}
				else
				{
					for (USound* Snd : *ArrayPtr)
					{
						ArrayJson.append((Snd) ? Snd->GetFilePath().c_str() : "");
					}
				}

				break;
			}
			default:
			{
				//UE_LOG("[AutoSerialize] Array property '%s' has unsupported InnerType: %d", Prop.Name, static_cast<int>(Prop.InnerType));
				break;
			}
			}

			// 저장 시 JSON에 배열 쓰기
			if (!bInIsLoading)
			{
				InOutHandle[Prop.Name] = ArrayJson;
			}
			break;
		}
		case EPropertyType::Map:
		{
			// Map 직렬화는 KeyType과 InnerType에 따라 처리
			if (Prop.KeyType == EPropertyType::Unknown || Prop.InnerType == EPropertyType::Unknown)
			{
				UE_LOG("[AutoSerialize] Map property '%s' has Unknown KeyType or InnerType, skipping.", Prop.Name);
				break;
			}

			JSON MapJson;
			if (bInIsLoading)
			{
				// JSON에서 Object 읽기
				if (!InOutHandle.hasKey(Prop.Name) || InOutHandle[Prop.Name].JSONType() != JSON::Class::Object)
				{
					break;
				}
				MapJson = InOutHandle[Prop.Name];
			}
			else
			{
				MapJson = JSON::Make(JSON::Class::Object);
			}

			// FString 키 Map 처리 (가장 일반적인 경우)
			if (Prop.KeyType == EPropertyType::FString)
			{
				switch (Prop.InnerType)
				{
				case EPropertyType::Int32:
				{
					TMap<FString, int32>* MapPtr = Prop.GetValuePtr<TMap<FString, int32>>(this);
					if (bInIsLoading)
					{
						MapPtr->clear();
						for (auto& [key, value] : MapJson.ObjectRange())
						{
							if (value.JSONType() == JSON::Class::Integral)
								(*MapPtr)[key] = static_cast<int32>(value.ToInt());
						}
					}
					else
					{
						for (auto& [key, value] : *MapPtr)
							MapJson[key] = value;
					}
					break;
				}
				case EPropertyType::Float:
				{
					TMap<FString, float>* MapPtr = Prop.GetValuePtr<TMap<FString, float>>(this);
					if (bInIsLoading)
					{
						MapPtr->clear();
						for (auto& [key, value] : MapJson.ObjectRange())
						{
							if (value.JSONType() == JSON::Class::Floating)
								(*MapPtr)[key] = static_cast<float>(value.ToFloat());
							else if (value.JSONType() == JSON::Class::Integral)
								(*MapPtr)[key] = static_cast<float>(value.ToInt());
						}
					}
					else
					{
						for (auto& [key, value] : *MapPtr)
							MapJson[key] = value;
					}
					break;
				}
				case EPropertyType::FString:
				{
					TMap<FString, FString>* MapPtr = Prop.GetValuePtr<TMap<FString, FString>>(this);
					if (bInIsLoading)
					{
						MapPtr->clear();
						for (auto& [key, value] : MapJson.ObjectRange())
						{
							if (value.JSONType() == JSON::Class::String)
								(*MapPtr)[key] = value.ToString();
						}
					}
					else
					{
						for (auto& [key, value] : *MapPtr)
							MapJson[key] = value.c_str();
					}
					break;
				}
				case EPropertyType::Bool:
				{
					TMap<FString, bool>* MapPtr = Prop.GetValuePtr<TMap<FString, bool>>(this);
					if (bInIsLoading)
					{
						MapPtr->clear();
						for (auto& [key, value] : MapJson.ObjectRange())
						{
							if (value.JSONType() == JSON::Class::Boolean)
								(*MapPtr)[key] = value.ToBool();
						}
					}
					else
					{
						for (auto& [key, value] : *MapPtr)
							MapJson[key] = value;
					}
					break;
				}
				default:
					UE_LOG("[AutoSerialize] Map property '%s' has unsupported InnerType: %d", Prop.Name, static_cast<int>(Prop.InnerType));
					break;
				}
			}
			// int32 키 Map 처리
			else if (Prop.KeyType == EPropertyType::Int32)
			{
				switch (Prop.InnerType)
				{
				case EPropertyType::FString:
				{
					TMap<int32, FString>* MapPtr = Prop.GetValuePtr<TMap<int32, FString>>(this);
					if (bInIsLoading)
					{
						MapPtr->clear();
						for (auto& [key, value] : MapJson.ObjectRange())
						{
							if (value.JSONType() == JSON::Class::String)
								(*MapPtr)[std::stoi(key)] = value.ToString();
						}
					}
					else
					{
						for (auto& [key, value] : *MapPtr)
							MapJson[std::to_string(key)] = value.c_str();
					}
					break;
				}
				default:
					break;
				}
			}

			// 저장 시 JSON에 Map 쓰기
			if (!bInIsLoading)
			{
				InOutHandle[Prop.Name] = MapJson;
			}
			break;
		}
		case EPropertyType::Enum:
		{
			// Enum은 int32로 직렬화
			int32* Value = Prop.GetValuePtr<int32>(this);
			if (bInIsLoading)
			{
				int32 ReadValue;
				if (FJsonSerializer::ReadInt32(InOutHandle, Prop.Name, ReadValue))
				{
					*Value = ReadValue;
				}
			}
			else
			{
				InOutHandle[Prop.Name] = *Value;
			}
			break;
		}
		case EPropertyType::Struct:
		{
			// Struct 직렬화 - TypeName으로 UStruct 찾아서 프로퍼티 순회
			UStruct* StructType = UStruct::FindStruct(Prop.TypeName);
			if (!StructType)
			{
				UE_LOG("[AutoSerialize] Struct property '%s' has unknown type: %s", Prop.Name, Prop.TypeName);
				break;
			}

			void* StructInstance = (char*)this + Prop.Offset;
			JSON StructJson;

			if (bInIsLoading)
			{
				if (!InOutHandle.hasKey(Prop.Name) || InOutHandle[Prop.Name].JSONType() != JSON::Class::Object)
				{
					break;
				}
				StructJson = InOutHandle[Prop.Name];
			}
			else
			{
				StructJson = JSON::Make(JSON::Class::Object);
			}

			// Struct의 모든 프로퍼티 순회
			for (const FProperty& StructProp : StructType->GetAllProperties())
			{
				switch (StructProp.Type)
				{
				case EPropertyType::Bool:
				{
					bool* Val = StructProp.GetValuePtr<bool>(StructInstance);
					if (bInIsLoading)
					{
						bool ReadVal;
						if (FJsonSerializer::ReadBool(StructJson, StructProp.Name, ReadVal))
							*Val = ReadVal;
					}
					else
					{
						StructJson[StructProp.Name] = *Val;
					}
					break;
				}
				case EPropertyType::Int32:
				{
					int32* Val = StructProp.GetValuePtr<int32>(StructInstance);
					if (bInIsLoading)
					{
						int32 ReadVal;
						if (FJsonSerializer::ReadInt32(StructJson, StructProp.Name, ReadVal))
							*Val = ReadVal;
					}
					else
					{
						StructJson[StructProp.Name] = *Val;
					}
					break;
				}
				case EPropertyType::Float:
				{
					float* Val = StructProp.GetValuePtr<float>(StructInstance);
					if (bInIsLoading)
					{
						float ReadVal;
						if (FJsonSerializer::ReadFloat(StructJson, StructProp.Name, ReadVal))
							*Val = ReadVal;
					}
					else
					{
						StructJson[StructProp.Name] = *Val;
					}
					break;
				}
				case EPropertyType::FString:
				{
					FString* Val = StructProp.GetValuePtr<FString>(StructInstance);
					if (bInIsLoading)
					{
						FString ReadVal;
						if (FJsonSerializer::ReadString(StructJson, StructProp.Name, ReadVal))
							*Val = ReadVal;
					}
					else
					{
						StructJson[StructProp.Name] = Val->c_str();
					}
					break;
				}
				case EPropertyType::FVector:
				{
					FVector* Val = StructProp.GetValuePtr<FVector>(StructInstance);
					if (bInIsLoading)
					{
						FVector ReadVal;
						if (FJsonSerializer::ReadVector(StructJson, StructProp.Name, ReadVal))
							*Val = ReadVal;
					}
					else
					{
						StructJson[StructProp.Name] = FJsonSerializer::VectorToJson(*Val);
					}
					break;
				}
				case EPropertyType::FLinearColor:
				{
					FLinearColor* Val = StructProp.GetValuePtr<FLinearColor>(StructInstance);
					if (bInIsLoading)
					{
						FVector4 ReadVal;
						if (FJsonSerializer::ReadVector4(StructJson, StructProp.Name, ReadVal))
							*Val = FLinearColor(ReadVal);
					}
					else
					{
						StructJson[StructProp.Name] = FJsonSerializer::Vector4ToJson(Val->ToFVector4());
					}
					break;
				}
				default:
					break;
				}
			}

			// 저장 시 JSON에 Struct 쓰기
			if (!bInIsLoading)
			{
				InOutHandle[Prop.Name] = StructJson;
			}
			break;
		}
		case EPropertyType::Sound:
		{
			USound** Value = Prop.GetValuePtr<USound*>(this);
			if (bInIsLoading)
			{
				FString SoundPath;
				FJsonSerializer::ReadString(InOutHandle, Prop.Name, SoundPath);
				if (!SoundPath.empty())
				{
					*Value = UResourceManager::GetInstance().Load<USound>(SoundPath);
				}
				else
				{
					*Value = nullptr;
				}
			}
			else
			{
				if (*Value)
				{
					InOutHandle[Prop.Name] = (*Value)->GetFilePath().c_str();
				}
				else
				{
					InOutHandle[Prop.Name] = "";
				}
			}
			break;
		}
		case EPropertyType::ObjectPtr:
		{
			// 일반 UObject* 포인터는 현재 지원하지 않음 (참조 기반이라 복잡)
			// 필요시 UUID 기반 참조 시스템 추가
			break;
		}
		// 나머지 타입들은 필요시 추가
		}
	}
}

void UObject::DuplicateSubObjects()
{
    UUID = GenerateUUID(); // UUID는 고유값이므로 새로 생성
}

UObject* UObject::Duplicate() const
{
    UObject* NewObject = ObjectFactory::DuplicateObject<UObject>(this); // 모든 멤버 얕은 복사

    NewObject->DuplicateSubObjects(); // 얕은 복사한 멤버들에 대해 메뉴얼하게 깊은 복사 수행

	NewObject->PostDuplicate();
    return NewObject;
}

// ───── 리플렉션 기반 가비지 컬렉션 ────────────────────────────

bool UObject::IsObjectPtrType(EPropertyType Type)
{
    switch (Type)
    {
        case EPropertyType::ObjectPtr:
        case EPropertyType::Texture:
        case EPropertyType::SkeletalMesh:
        case EPropertyType::StaticMesh:
        case EPropertyType::Material:
        case EPropertyType::Sound:
        case EPropertyType::ParticleSystem:
            return true;
        default:
            return false;
    }
}

void UObject::AddReferencedObjects(FGarbageCollector& Collector)
{
    // 리플렉션 기반 자동 마킹 (UPROPERTY로 등록된 UObject* 멤버들)
    AddReferencedObjectsFromReflection(Collector);
}

void UObject::AddReferencedObjectsFromReflection(FGarbageCollector& Collector)
{
    // 리플렉션으로 모든 프로퍼티 순회
    const TArray<FProperty>& Props = GetClass()->GetAllProperties();

    for (const FProperty& Prop : Props)
    {
        MarkPropertyReference(Prop, Collector);
    }
}

void UObject::MarkPropertyReference(const FProperty& Prop, FGarbageCollector& Collector)
{
    switch (Prop.Type)
    {
        // === 단일 UObject* 타입들 ===
        case EPropertyType::ObjectPtr:
        case EPropertyType::Texture:
        case EPropertyType::SkeletalMesh:
        case EPropertyType::StaticMesh:
        case EPropertyType::Material:
        case EPropertyType::Sound:
        case EPropertyType::ParticleSystem:
        {
            UObject** ObjPtr = Prop.GetValuePtr<UObject*>(this);
            if (ObjPtr && *ObjPtr)
            {
                Collector.MarkObject(*ObjPtr);
            }
            break;
        }

        // === TArray<UObject*> ===
        case EPropertyType::Array:
        {
            if (IsObjectPtrType(Prop.InnerType))
            {
                MarkArrayReferences(Prop, Collector);
            }
            break;
        }

        default:
            break;
    }
}

void UObject::MarkArrayReferences(const FProperty& Prop, FGarbageCollector& Collector)
{
    // TArray의 원시 포인터를 가져와서 UObject* 배열로 처리
    // 모든 UObject* 파생 타입은 동일한 포인터 크기를 가지므로 안전하게 캐스팅 가능

    switch (Prop.InnerType)
    {
        case EPropertyType::ObjectPtr:
        {
            TArray<UObject*>* ArrayPtr = Prop.GetValuePtr<TArray<UObject*>>(this);
            if (ArrayPtr)
            {
                for (UObject* Obj : *ArrayPtr)
                {
                    if (Obj)
                    {
                        Collector.MarkObject(Obj);
                    }
                }
            }
            break;
        }

        case EPropertyType::Texture:
        {
            TArray<UTexture*>* ArrayPtr = Prop.GetValuePtr<TArray<UTexture*>>(this);
            if (ArrayPtr)
            {
                for (UTexture* Obj : *ArrayPtr)
                {
                    if (Obj)
                    {
                        Collector.MarkObject(Obj);
                    }
                }
            }
            break;
        }

        case EPropertyType::StaticMesh:
        {
            TArray<UStaticMesh*>* ArrayPtr = Prop.GetValuePtr<TArray<UStaticMesh*>>(this);
            if (ArrayPtr)
            {
                for (UStaticMesh* Obj : *ArrayPtr)
                {
                    if (Obj)
                    {
                        Collector.MarkObject(Obj);
                    }
                }
            }
            break;
        }

        case EPropertyType::SkeletalMesh:
        {
            TArray<USkeletalMesh*>* ArrayPtr = Prop.GetValuePtr<TArray<USkeletalMesh*>>(this);
            if (ArrayPtr)
            {
                for (USkeletalMesh* Obj : *ArrayPtr)
                {
                    if (Obj)
                    {
                        Collector.MarkObject(Obj);
                    }
                }
            }
            break;
        }

        case EPropertyType::Material:
        {
            TArray<UMaterial*>* ArrayPtr = Prop.GetValuePtr<TArray<UMaterial*>>(this);
            if (ArrayPtr)
            {
                for (UMaterial* Obj : *ArrayPtr)
                {
                    if (Obj)
                    {
                        Collector.MarkObject(Obj);
                    }
                }
            }
            break;
        }

        case EPropertyType::Sound:
        {
            TArray<USound*>* ArrayPtr = Prop.GetValuePtr<TArray<USound*>>(this);
            if (ArrayPtr)
            {
                for (USound* Obj : *ArrayPtr)
                {
                    if (Obj)
                    {
                        Collector.MarkObject(Obj);
                    }
                }
            }
            break;
        }

        case EPropertyType::ParticleSystem:
        {
            TArray<UParticleSystem*>* ArrayPtr = Prop.GetValuePtr<TArray<UParticleSystem*>>(this);
            if (ArrayPtr)
            {
                for (UParticleSystem* Obj : *ArrayPtr)
                {
                    if (Obj)
                    {
                        Collector.MarkObject(Obj);
                    }
                }
            }
            break;
        }

        default:
            break;
    }
}





