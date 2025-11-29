# 리플렉션 기반 자동 GC 구현

## 개요

Mark-and-Sweep GC가 리플렉션 시스템을 활용하여 `UPROPERTY()`로 등록된 UObject* 참조를 자동으로 추적합니다. UPROPERTY로 등록되지 않은 멤버는 파생 클래스에서 수동으로 마킹해야 합니다.

---

## 구현 상태: 완료

### 지원하는 타입

| 타입 | 단일 포인터 | TArray |
|------|:-----------:|:------:|
| `UObject*` | O | O |
| `UTexture*` | O | O |
| `UStaticMesh*` | O | O |
| `USkeletalMesh*` | O | O |
| `UMaterial*` | O | O |
| `USound*` | O | O |
| `UParticleSystem*` | O | O |

### 미지원 타입 (수동 마킹 필요)

- `TSet<UObject*>` - EPropertyType에 Set 타입 없음
- `TMap<K, UObject*>` - 구현 복잡도로 인해 미지원
- 중첩 컨테이너 (`TArray<TArray<UObject*>>`)

---

## 사용법

### 1. UPROPERTY로 등록된 멤버 (자동 마킹)

```cpp
class UMyComponent : public UActorComponent
{
    UPROPERTY()
    UTexture* MyTexture;  // 자동으로 GC가 추적

    UPROPERTY()
    TArray<UMaterial*> Materials;  // 자동으로 GC가 추적
};
```

UPROPERTY로 등록된 UObject* 멤버는 별도 코드 없이 자동으로 GC에서 추적됩니다.

### 2. UPROPERTY 없는 멤버 (수동 마킹)

```cpp
class AActor : public UObject
{
protected:
    // UPROPERTY 없음 - 수동 마킹 필요
    USceneComponent* RootComponent;
    TSet<UActorComponent*> OwnedComponents;

public:
    void AddReferencedObjects(FGarbageCollector& Collector) override
    {
        // 부모의 리플렉션 기반 자동 마킹 호출
        Super::AddReferencedObjects(Collector);

        // UPROPERTY 없는 멤버는 수동 마킹
        Collector.MarkObject(RootComponent);
        for (UActorComponent* Comp : OwnedComponents)
        {
            Collector.MarkObject(Comp);
        }
    }
};
```

---

## 핵심 구현 코드

### Object.h

```cpp
class UObject
{
public:
    // GC가 호출하는 메서드 (파생 클래스에서 오버라이드 가능)
    virtual void AddReferencedObjects(FGarbageCollector& Collector);

protected:
    // 리플렉션 기반 자동 마킹
    void AddReferencedObjectsFromReflection(FGarbageCollector& Collector);

    // UObject 포인터 타입인지 확인
    static bool IsObjectPtrType(EPropertyType Type);

    // 프로퍼티 마킹 헬퍼
    void MarkPropertyReference(const FProperty& Prop, FGarbageCollector& Collector);
    void MarkArrayReferences(const FProperty& Prop, FGarbageCollector& Collector);
};
```

### Object.cpp

```cpp
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
    // 리플렉션 기반 자동 마킹
    AddReferencedObjectsFromReflection(Collector);
}

void UObject::AddReferencedObjectsFromReflection(FGarbageCollector& Collector)
{
    const TArray<FProperty>& Props = GetClass()->GetAllProperties();
    for (const FProperty& Prop : Props)
    {
        MarkPropertyReference(Prop, Collector);
    }
}
```

---

## 하이브리드 방식 설명

리플렉션 자동 마킹과 수동 마킹을 병행합니다:

```
[GC가 AddReferencedObjects 호출]
         │
         ▼
[UObject::AddReferencedObjects]
         │
         ├─ AddReferencedObjectsFromReflection()
         │      └─ UPROPERTY 멤버 자동 마킹
         │
         ▼
[파생 클래스::AddReferencedObjects]
         │
         ├─ Super::AddReferencedObjects() ← 부모 호출 (필수!)
         │
         └─ UPROPERTY 없는 멤버 수동 마킹
```

### 중요: Super 호출 필수!

파생 클래스에서 `AddReferencedObjects`를 오버라이드할 때 반드시 `Super::AddReferencedObjects(Collector)`를 호출해야 합니다. 그래야 부모 클래스의 UPROPERTY 멤버들도 자동 마킹됩니다.

---

## 장단점

### 장점

- UPROPERTY 멤버는 별도 코드 없이 자동 추적
- 새 UPROPERTY 추가 시 GC 코드 수정 불필요
- 참조 누락 실수 방지

### 단점

- UPROPERTY 미등록 멤버는 여전히 수동 마킹 필요
- TSet, TMap 등 복잡한 컨테이너 미지원
- 런타임 오버헤드 (프로퍼티 순회)

---

## 현재 수동 마킹이 필요한 클래스

| 클래스 | 수동 마킹 멤버 | 이유 |
|--------|---------------|------|
| `AActor` | `RootComponent`, `OwnedComponents` | UPROPERTY 미등록 |
| `ULevel` | `Actors` | UPROPERTY 미등록 |
| `UWorld` | `Level` (unique_ptr) | smart pointer |

---

## 향후 개선 사항

### 1. TSet 지원

```cpp
// Property.h에 추가 필요
enum class EPropertyType : uint8
{
    // ...
    Set,  // TSet<T> - InnerType으로 T 지정
};
```

### 2. 성능 최적화 (프로퍼티 캐싱)

```cpp
// UClass에 GC용 프로퍼티 캐시 추가
struct FGCPropertyCache
{
    TArray<const FProperty*> ObjectPtrProperties;
    TArray<const FProperty*> ArrayProperties;
    bool bInitialized = false;
};
```

### 3. TMap 지원

키/값 조합이 많아 복잡하지만, 필요시 구현 가능.

---

## 관련 파일

| 파일 | 내용 |
|------|------|
| `Source/Runtime/Core/Object/Object.h` | GC 메서드 선언 |
| `Source/Runtime/Core/Object/Object.cpp` | 리플렉션 기반 마킹 구현 |
| `Source/Runtime/Core/Object/Property.h` | EPropertyType 정의 |
| `Source/Runtime/Core/Memory/GarbageCollector.h` | GC 클래스 |
| `Source/Runtime/Core/Memory/GarbageCollector.cpp` | Mark-and-Sweep 구현 |

---

*작성일: 2025-11-29*
*상태: 구현 완료*
