# Mundi Engine - Mark-and-Sweep 가비지 컬렉터

## 목차
1. [Mark-and-Sweep 알고리즘 개요](#mark-and-sweep-알고리즘-개요)
2. [구현 구조](#구현-구조)
3. [설계 결정 사항](#설계-결정-사항)
4. [사용법](#사용법)
5. [수정 시 주의사항](#수정-시-주의사항)
6. [파일 목록](#파일-목록)

---

## Mark-and-Sweep 알고리즘 개요

### 기본 원리

Mark-and-Sweep은 가장 기본적이고 널리 사용되는 가비지 컬렉션 알고리즘입니다.

```
[Root Objects]
     │
     ▼
   ┌───┐     ┌───┐     ┌───┐
   │ A │────▶│ B │────▶│ C │  ← 도달 가능 (Reachable)
   └───┘     └───┘     └───┘
                         │
                         ▼
                       ┌───┐
                       │ D │  ← 도달 가능
                       └───┘

   ┌───┐     ┌───┐
   │ E │────▶│ F │  ← 도달 불가능 (Unreachable) → 삭제 대상
   └───┘     └───┘
```

### 두 단계 동작

#### 1단계: Mark (마킹)
- Root 객체들부터 시작
- 참조를 따라가며 도달 가능한 모든 객체에 "마크" 표시
- BFS(너비 우선 탐색) 또는 DFS(깊이 우선 탐색) 사용

#### 2단계: Sweep (쓸기)
- 전체 객체 목록을 순회
- 마크되지 않은 객체 = 도달 불가능 = 쓰레기 → 삭제
- 마크된 객체의 마크 플래그 초기화

### 장단점

| 장점 | 단점 |
|------|------|
| 순환 참조 자동 해결 | GC 실행 중 일시 정지 (Stop-the-World) |
| 구현이 비교적 단순 | 힙 전체를 순회해야 함 |
| 메모리 단편화 없음 (Sweep만 할 경우) | 실시간 시스템에 부적합할 수 있음 |

---

## 구현 구조

### 클래스 다이어그램

```
┌─────────────────────────────────────────────────────────────┐
│                    FGarbageCollector                         │
│  (싱글톤)                                                    │
├─────────────────────────────────────────────────────────────┤
│  - RootObjects: TSet<UObject*>     // Root 객체 목록        │
│  - MarkQueue: TArray<UObject*>     // 마킹 대기 큐          │
│  - bIsEnabled: bool                // GC 활성화 여부        │
│  - bIsCollecting: bool             // 현재 GC 수행 중       │
├─────────────────────────────────────────────────────────────┤
│  + Get(): FGarbageCollector&       // 싱글톤 접근           │
│  + AddRoot(UObject*)               // Root 등록             │
│  + RemoveRoot(UObject*)            // Root 해제             │
│  + CollectGarbage()                // GC 실행               │
│  + MarkObject(UObject*)            // 객체 마킹             │
├─────────────────────────────────────────────────────────────┤
│  - MarkPhase()                     // 마킹 단계             │
│  - SweepPhase()                    // 쓸기 단계             │
│  - ResetMarks()                    // 마크 초기화           │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ uses
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                        UObject                               │
├─────────────────────────────────────────────────────────────┤
│  - bMarkedByGC: bool               // GC 마크 플래그        │
├─────────────────────────────────────────────────────────────┤
│  + IsMarkedByGC(): bool                                     │
│  + MarkByGC()                                               │
│  + UnmarkByGC()                                             │
│  + AddReferencedObjects(Collector) // 참조 객체 등록 (가상) │
└─────────────────────────────────────────────────────────────┘
                              △
                              │ 상속
          ┌───────────────────┼───────────────────┐
          │                   │                   │
    ┌─────┴─────┐      ┌─────┴─────┐      ┌─────┴─────┐
    │  AActor   │      │  UWorld   │      │  ULevel   │
    │           │      │           │      │           │
    │ 컴포넌트들│      │ 액터들    │      │ 액터들    │
    │ 마킹      │      │ 마킹      │      │ 마킹      │
    └───────────┘      └───────────┘      └───────────┘
```

### 실행 흐름

```
CollectGarbage()
       │
       ▼
┌──────────────┐
│ ResetMarks() │  ← 모든 객체의 bMarkedByGC = false
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ MarkPhase()  │
│              │
│  1. Root 객체들을 MarkQueue에 추가
│  2. Queue가 빌 때까지:
│     - 객체 꺼내기
│     - AddReferencedObjects() 호출
│       → 참조하는 객체들 MarkObject()
│       → 마킹 안 된 객체는 Queue에 추가
└──────┬───────┘
       │
       ▼
┌──────────────┐
│ SweepPhase() │
│              │
│  1. GUObjectArray 순회
│  2. 마킹 안 된 객체 수집
│  3. DeleteObject() 호출
│  4. CompactNullSlots() 호출
└──────────────┘
```

---

## 설계 결정 사항

### 1. 왜 Mark-and-Sweep인가?

**참조 카운팅 대신 Mark-and-Sweep을 선택한 이유:**

```cpp
// 참조 카운팅의 문제: 순환 참조
class AActorA {
    AActorB* RefToB;  // A → B
};
class AActorB {
    AActorA* RefToA;  // B → A (순환!)
};
// A와 B 모두 참조 카운트가 1 이상 → 영원히 삭제 안 됨
```

Mark-and-Sweep은 Root에서 도달 가능한지만 확인하므로 순환 참조 문제가 없습니다.

### 2. 왜 BFS 방식인가?

```cpp
// MarkPhase에서 BFS 사용
while (MarkQueue.Num() > 0)
{
    UObject* Current = MarkQueue[0];
    MarkQueue.RemoveAt(0);
    Current->AddReferencedObjects(*this);
}
```

**BFS 선택 이유:**
- 스택 오버플로우 방지 (DFS 재귀 호출 시 깊은 참조 체인에서 위험)
- 메모리 지역성이 좋음 (레벨별로 처리)
- 디버깅이 용이함

### 3. 왜 AddReferencedObjects() 가상 함수인가?

**언리얼 엔진 방식 채택:**

```cpp
// 각 클래스가 자신의 참조를 직접 등록
void AActor::AddReferencedObjects(FGarbageCollector& Collector)
{
    Super::AddReferencedObjects(Collector);

    Collector.MarkObject(RootComponent);
    for (UActorComponent* Comp : OwnedComponents)
        Collector.MarkObject(Comp);
}
```

**장점:**
- 클래스별로 참조 관계를 명확히 정의
- 리플렉션 시스템 없이도 동작
- 컴파일 타임에 타입 안전성 보장
- 새 클래스 추가 시 해당 클래스만 수정하면 됨

### 4. Root 객체 관리

```cpp
// Root = GC에서 절대 삭제되지 않는 시작점
FGarbageCollector::Get().AddRoot(GWorld);
FGarbageCollector::Get().AddRoot(&UResourceManager::GetInstance());
```

**Root로 등록해야 하는 객체:**
- `UWorld` - 게임 월드
- `UResourceManager` - 리소스 매니저 (싱글톤)
- 기타 전역 매니저들

### 5. GUObjectArray와의 연동

기존 `ObjectFactory` 시스템을 그대로 활용:

```cpp
// 기존 구조 유지
extern TArray<UObject*> GUObjectArray;

// Sweep에서 활용
for (UObject* Obj : GUObjectArray)
{
    if (Obj && !Obj->IsMarkedByGC())
        ObjectFactory::DeleteObject(Obj);
}
```

---

## 사용법

### 기본 사용

```cpp
#include "GarbageCollector.h"

// 엔진 초기화 시 Root 등록
void Engine::Initialize()
{
    FGarbageCollector::Get().AddRoot(GWorld);
    FGarbageCollector::Get().AddRoot(&UResourceManager::GetInstance());
}

// 매 프레임 또는 주기적으로 호출
void Engine::Tick()
{
    // ... 게임 로직 ...

    // GC 실행 (필요시)
    FGarbageCollector::Get().CollectGarbage();
}

// 엔진 종료 시
void Engine::Shutdown()
{
    FGarbageCollector::Get().RemoveRoot(GWorld);
}
```

### 새로운 클래스에서 참조 등록

```cpp
class UMyCustomObject : public UObject
{
    DECLARE_CLASS(UMyCustomObject, UObject)

private:
    UObject* DirectReference;
    TArray<UObject*> ObjectArray;
    TMap<FString, UObject*> ObjectMap;

public:
    void AddReferencedObjects(FGarbageCollector& Collector) override
    {
        // 반드시 Super 호출
        Super::AddReferencedObjects(Collector);

        // 직접 참조
        Collector.MarkObject(DirectReference);

        // 배열 참조
        for (UObject* Obj : ObjectArray)
        {
            Collector.MarkObject(Obj);
        }

        // 맵 참조
        for (auto& Pair : ObjectMap)
        {
            Collector.MarkObject(Pair.second);
        }
    }
};
```

### GC 제어

```cpp
// GC 비활성화 (성능 크리티컬 구간)
FGarbageCollector::Get().SetEnabled(false);

// 무거운 작업...

// GC 다시 활성화
FGarbageCollector::Get().SetEnabled(true);

// 통계 확인
uint32 LastCollected = FGarbageCollector::Get().GetLastCollectedCount();
uint32 TotalCollected = FGarbageCollector::Get().GetTotalCollectedCount();
```

---

## 수정 시 주의사항

### 1. 새로운 UObject 파생 클래스 추가 시

```cpp
// ❌ 잘못된 예: AddReferencedObjects 미구현
class UMyObject : public UObject
{
    UObject* ImportantRef;  // GC가 이 참조를 모름!
};

// ✅ 올바른 예
class UMyObject : public UObject
{
    UObject* ImportantRef;

    void AddReferencedObjects(FGarbageCollector& Collector) override
    {
        Super::AddReferencedObjects(Collector);
        Collector.MarkObject(ImportantRef);  // 명시적 등록
    }
};
```

### 2. Super 호출 필수

```cpp
void MyClass::AddReferencedObjects(FGarbageCollector& Collector)
{
    // ❌ Super 호출 안 함 → 부모 클래스의 참조가 마킹 안 됨
    Collector.MarkObject(MyRef);
}

void MyClass::AddReferencedObjects(FGarbageCollector& Collector)
{
    // ✅ 항상 Super 먼저 호출
    Super::AddReferencedObjects(Collector);
    Collector.MarkObject(MyRef);
}
```

### 3. 컨테이너 내 UObject* 처리

```cpp
void AddReferencedObjects(FGarbageCollector& Collector) override
{
    Super::AddReferencedObjects(Collector);

    // TArray
    for (UObject* Obj : MyArray)
        Collector.MarkObject(Obj);

    // TSet
    for (UObject* Obj : MySet)
        Collector.MarkObject(Obj);

    // TMap (값이 UObject*인 경우)
    for (auto& Pair : MyMap)
        Collector.MarkObject(Pair.second);

    // TMap (키가 UObject*인 경우)
    for (auto& Pair : MyMap)
        Collector.MarkObject(Pair.first);
}
```

### 4. 약한 참조 (Weak Reference)

GC 대상이 아닌 참조는 마킹하지 않아도 됩니다:

```cpp
class UMyObject : public UObject
{
    // 강한 참조 - 마킹 필요
    UObject* OwnedObject;

    // 약한 참조 - 마킹 불필요 (소유하지 않음)
    TWeakObjectPtr<UObject> WeakRef;

    void AddReferencedObjects(FGarbageCollector& Collector) override
    {
        Super::AddReferencedObjects(Collector);
        Collector.MarkObject(OwnedObject);
        // WeakRef는 마킹하지 않음
    }
};
```

### 5. Root 객체 관리

```cpp
// ❌ Root 등록 안 함 → 전체 객체 그래프가 삭제될 수 있음
UWorld* NewWorld = NewObject<UWorld>();

// ✅ 전역 객체는 Root로 등록
UWorld* NewWorld = NewObject<UWorld>();
FGarbageCollector::Get().AddRoot(NewWorld);

// 월드 교체 시
FGarbageCollector::Get().RemoveRoot(OldWorld);
FGarbageCollector::Get().AddRoot(NewWorld);
```

### 6. GC 실행 타이밍

```cpp
// ❌ 위험: 객체 순회 중 GC 실행
for (AActor* Actor : World->GetActors())
{
    FGarbageCollector::Get().CollectGarbage();  // 위험!
    Actor->DoSomething();  // Actor가 삭제됐을 수 있음
}

// ✅ 안전: 프레임 끝에서 실행
void Engine::Tick()
{
    // 게임 로직 처리
    World->Tick(DeltaTime);

    // 프레임 끝에서 GC
    FGarbageCollector::Get().CollectGarbage();
}
```

### 7. 순환 참조 확인

Mark-and-Sweep은 순환 참조를 자동 처리하지만, 의도치 않은 참조 체인이 있는지 확인하세요:

```cpp
// 의도치 않은 참조 체인 예시
class AEnemy : public AActor
{
    APlayer* TargetPlayer;  // Enemy → Player

    void AddReferencedObjects(FGarbageCollector& Collector) override
    {
        Super::AddReferencedObjects(Collector);
        // TargetPlayer를 마킹하면:
        // Enemy가 살아있는 한 Player도 삭제 안 됨
        // 이게 의도된 동작인지 확인 필요
        Collector.MarkObject(TargetPlayer);
    }
};
```

### 8. 성능 고려사항

```cpp
// GC 빈도 조절
static float GCTimer = 0.0f;
const float GCInterval = 1.0f;  // 1초마다

void Engine::Tick(float DeltaTime)
{
    GCTimer += DeltaTime;
    if (GCTimer >= GCInterval)
    {
        FGarbageCollector::Get().CollectGarbage();
        GCTimer = 0.0f;
    }
}

// 또는 객체 수 기반
if (GUObjectArray.Num() > 10000)
{
    FGarbageCollector::Get().CollectGarbage();
}
```

---

## 파일 목록

### 새로 생성된 파일

| 파일 | 설명 |
|------|------|
| `Source/Runtime/Core/Memory/GarbageCollector.h` | GC 클래스 헤더 |
| `Source/Runtime/Core/Memory/GarbageCollector.cpp` | GC 클래스 구현 |

### 수정된 파일

| 파일 | 수정 내용 |
|------|-----------|
| `Source/Runtime/Core/Object/Object.h` | `bMarkedByGC` 플래그, `AddReferencedObjects()` 가상 함수 추가 |
| `Source/Runtime/Core/Object/Actor.h` | `AddReferencedObjects()` 선언 추가 |
| `Source/Runtime/Core/Object/Actor.cpp` | `AddReferencedObjects()` 구현 (컴포넌트 마킹) |
| `Source/Runtime/Engine/GameFramework/World.h` | `AddReferencedObjects()` 선언 추가 |
| `Source/Runtime/Engine/GameFramework/World.cpp` | `AddReferencedObjects()` 구현 (레벨, 액터 마킹) |
| `Source/Runtime/Engine/GameFramework/Level.h` | `AddReferencedObjects()` 선언 추가 |
| `Source/Runtime/Engine/GameFramework/Level.cpp` | `AddReferencedObjects()` 구현 (액터 마킹) |

---

## 향후 개선 가능 사항

1. **증분 GC (Incremental GC)**: 여러 프레임에 걸쳐 GC 수행
2. **세대별 GC (Generational GC)**: 오래된 객체와 새 객체 분리 관리
3. **병렬 GC**: 백그라운드 스레드에서 마킹 수행
4. **메모리 풀링**: 자주 생성/삭제되는 객체 타입별 풀 관리

---

*마지막 업데이트: 2025-11-29*
