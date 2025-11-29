#pragma once
#include "UEContainer.h"

class UObject;

/**
 * FGarbageCollector - Mark-and-Sweep 가비지 컬렉터
 *
 * 사용법:
 * 1. Root 객체 등록: AddRoot(World), AddRoot(ResourceManager) 등
 * 2. 주기적으로 CollectGarbage() 호출 (매 프레임 또는 특정 주기)
 * 3. 각 UObject 파생 클래스에서 AddReferencedObjects() 오버라이드하여 참조 객체 등록
 *
 * 동작 원리:
 * - Mark 단계: Root 객체들부터 시작하여 참조되는 모든 객체에 마킹
 * - Sweep 단계: 마킹되지 않은 객체들을 삭제
 */
class FGarbageCollector
{
public:
    // 싱글톤 접근
    static FGarbageCollector& Get();

    // Root 객체 관리 (GC에서 절대 삭제되지 않는 객체들)
    void AddRoot(UObject* Object);
    void RemoveRoot(UObject* Object);
    bool IsRoot(UObject* Object) const;

    // 가비지 컬렉션 실행
    void CollectGarbage();

    // 통계
    uint32 GetLastCollectedCount() const { return LastCollectedCount; }
    uint32 GetTotalCollectedCount() const { return TotalCollectedCount; }

    // GC 활성화/비활성화
    void SetEnabled(bool bEnabled) { bIsEnabled = bEnabled; }
    bool IsEnabled() const { return bIsEnabled; }

    // Mark 단계에서 사용 - 객체를 마킹 큐에 추가
    void MarkObject(UObject* Object);

    // 현재 GC 수행 중인지 확인
    bool IsCollecting() const { return bIsCollecting; }

private:
    FGarbageCollector() = default;
    ~FGarbageCollector() = default;
    FGarbageCollector(const FGarbageCollector&) = delete;
    FGarbageCollector& operator=(const FGarbageCollector&) = delete;

    // Mark 단계: Root부터 시작하여 도달 가능한 모든 객체 마킹
    void MarkPhase();

    // Sweep 단계: 마킹되지 않은 객체 삭제
    void SweepPhase();

    // 모든 객체의 마크 플래그 초기화
    void ResetMarks();

private:
    // Root 객체 목록 (World, ResourceManager 등)
    TSet<UObject*> RootObjects;

    // Mark 단계에서 처리할 객체 큐
    TArray<UObject*> MarkQueue;

    // 통계
    uint32 LastCollectedCount = 0;
    uint32 TotalCollectedCount = 0;

    // 상태
    bool bIsEnabled = true;
    bool bIsCollecting = false;
};
