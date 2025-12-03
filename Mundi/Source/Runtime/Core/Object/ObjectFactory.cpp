#include "pch.h"
#include "ObjectFactory.h"
// 전역 오브젝트 맵 정의 (O(1) 삭제를 위해 TMap 사용)
TMap<uint32, UObject*> GUObjectArray;
uint32 GUObjectIndexCounter = 0;

namespace ObjectFactory
{
    TMap<UClass*, ConstructFunc>& GetRegistry()
    {
        static TMap<UClass*, ConstructFunc> Registry;
        return Registry;
    }

    void RegisterClassType(UClass* Class, ConstructFunc Func)
    {
        GetRegistry()[Class] = std::move(Func);
    }

    UObject* ConstructObject(UClass* Class)
    {
        auto& reg = GetRegistry();
        auto it = reg.find(Class);
        if (it == reg.end()) return nullptr;
        return it->second();
    }

    UObject* NewObject(UClass* Class)
    {
        UObject* Obj = ConstructObject(Class);
        if (!Obj) return nullptr;

        // 고유 인덱스 할당
        uint32 idx = ++GUObjectIndexCounter;
        GUObjectArray[idx] = Obj;
        Obj->InternalIndex = idx;

        static TMap<UClass*, int> NameCounters;
        int Count = ++NameCounters[Class];

        const std::string base = Class->Name; // FName -> string
        std::string unique;
        unique.reserve(base.size() + 1 + 12);            // "_" + 최대 10~12자리 여유
        unique.append(base);
        unique.push_back('_');
        unique.append(std::to_string(Count));

        Obj->ObjectName = FName(unique);

        return Obj;
    }

    UObject* AddToGUObjectArray(UClass* Class, UObject* Obj)
    {
        if (!Obj) return nullptr;

        // 고유 인덱스 할당
        uint32 idx = ++GUObjectIndexCounter;
        GUObjectArray[idx] = Obj;
        Obj->InternalIndex = idx;

        static TMap<UClass*, int> NameCounters;
        int Count = ++NameCounters[Class];

        return Obj;
    }

    void DeleteObject(UObject* Obj)
    {
        if (!Obj) return;

        // 안전하게 전체 순회하여 포인터 비교로 찾기 (dangling pointer 대응)
        for (auto it = GUObjectArray.begin(); it != GUObjectArray.end(); ++it)
        {
            if (it->second == Obj)
            {
                GUObjectArray.erase(it);  // 실제로 엔트리 제거 (sparse 유지)
                Obj->DestroyInternal();
                return;
            }
        }
        // Not managed or already deleted.
    }

    void DeleteObjectFast(UObject* Obj)
    {
        if (!Obj) return;

        // InternalIndex로 O(1) 조회
        uint32 idx = Obj->InternalIndex;
        auto it = GUObjectArray.find(idx);

        // 안전 검증: 맵에 있고, 같은 객체인지 확인 (dangling pointer 대응)
        if (it != GUObjectArray.end() && it->second == Obj)
        {
            GUObjectArray.erase(it);
            Obj->DestroyInternal();
        }
        // else: Not managed or already deleted (dangling pointer case)
    }

    void DeleteAll(bool bCallBeginDestroy)
    {
        // 복사본으로 순회 (삭제 중 맵 수정 방지)
        TArray<UObject*> ObjectsToDelete;
        for (auto& pair : GUObjectArray)
        {
            if (pair.second)
            {
                ObjectsToDelete.Add(pair.second);
            }
        }

        for (UObject* Obj : ObjectsToDelete)
        {
            DeleteObject(Obj);
        }

        GUObjectArray.clear();
        GUObjectIndexCounter = 0;
    }

    // TMap 사용 시 CompactNullSlots는 더 이상 필요 없음
    void CompactNullSlots()
    {
        // TMap은 삭제 시 자동으로 엔트리가 제거되므로 별도 압축 불필요
    }
}
