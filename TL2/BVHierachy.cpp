#include "pch.h"
#include "BVHierachy.h"
#include "AABoundingBoxComponent.h" // FBound helpers
#include "Vector.h"
#include <algorithm>
#include <cfloat>

FBVHierachy::FBVHierachy(const FBound& InBounds, int InDepth, int InMaxDepth, int InMaxObjects)
    : Depth(InDepth)
    , MaxDepth(InMaxDepth)
    , MaxObjects(InMaxObjects)
    , Bounds(InBounds)
    , Left(nullptr)
    , Right(nullptr)
{
}

FBVHierachy::~FBVHierachy()
{
    Clear();
}

void FBVHierachy::Clear()
{
    // 액터/맵 비우기
    Actors = TArray<AActor*>();
    ActorLastBounds = TMap<AActor*, FBound>();

    // 자식 반환
    if (Left) { delete Left; Left = nullptr; }
    if (Right) { delete Right; Right = nullptr; }
}

void FBVHierachy::Insert(AActor* InActor, const FBound& ActorBounds)
{
    if (!InActor) return;

    // 항상 루트(또는 현재 노드)에 캐시 갱신
    ActorLastBounds.Add(InActor, ActorBounds);

    const bool isLeaf = (Left == nullptr && Right == nullptr);
    if (isLeaf)
    {
        Actors.Add(InActor);
        // 분할 조건: 용량 초과 + 깊이 여유
        if ((int)Actors.size() > MaxObjects && Depth < MaxDepth)
        {
            Split();
        }
        else
        {
            Refit();
        }
        return;
    }

    // 내부 노드: 자식 선택(간단한 볼륨 확장 비용)
    auto Union = [](const FBound& A, const FBound& B) { return UnionBounds(A, B); };
    auto Volume = [](const FBound& B)
    {
        FVector e = B.GetExtent() * 2.0f;
        return std::max(0.0f, e.X) * std::max(0.0f, e.Y) * std::max(0.0f, e.Z);
    };

    float costL = FLT_MAX, costR = FLT_MAX;
    if (Left) costL = Volume(Union(Left->Bounds, ActorBounds)) - Volume(Left->Bounds);
    if (Right) costR = Volume(Union(Right->Bounds, ActorBounds)) - Volume(Right->Bounds);

    if (!Left && Right)
    {
        Left = new FBVHierachy(ActorBounds, Depth + 1, MaxDepth, MaxObjects);
        Left->Actors.Add(InActor);
        Left->ActorLastBounds.Add(InActor, ActorBounds);
        Left->Refit();
    }
    else if (Left && !Right)
    {
        Right = new FBVHierachy(ActorBounds, Depth + 1, MaxDepth, MaxObjects);
        Right->Actors.Add(InActor);
        Right->ActorLastBounds.Add(InActor, ActorBounds);
        Right->Refit();
    }
    else
    {
        if (costL <= costR)
            Left->Insert(InActor, ActorBounds);
        else
            Right->Insert(InActor, ActorBounds);
    }

    Refit();
}

void FBVHierachy::BulkInsert(const TArray<std::pair<AActor*, FBound>>& ActorsAndBounds)
{
    // Smart path: if batch is large relative to current tree, rebuild for balance
    const int N = TotalActorCount();
    const int M = (int)ActorsAndBounds.size();

    // Heuristics (tunable):
    const int MinRebuildN = 2000;     // small trees rebuild cheaply
    const float RelRatio = 0.30f;     // if batch >= 30% of current size
    const int AbsMThreshold = 1500;   // absolute batch threshold

    const bool shouldRebuild =
        (N == 0) ||
        (N <= MinRebuildN) ||
        (M >= (int)(RelRatio * N)) ||
        (M >= AbsMThreshold);

    if (shouldRebuild)
    {
        // Build a combined list: existing cached bounds (prefer incoming overrides)
        TSet<AActor*> incoming;
        incoming.reserve(ActorsAndBounds.size());
        for (const auto& it : ActorsAndBounds)
        {
            incoming.insert(it.first);
        }

        TArray<std::pair<AActor*, FBound>> Combined;
        Combined.reserve((size_t)N + ActorsAndBounds.size());

        // Add existing ones that are NOT overridden by incoming batch
        for (const auto& kv : ActorLastBounds)
        {
            if (incoming.find(kv.first) == incoming.end())
            {
                Combined.push_back({ kv.first, kv.second });
            }
        }
        // Append incoming (new values take precedence)
        for (const auto& it : ActorsAndBounds)
        {
            Combined.push_back(it);
        }

        // Build a new balanced tree, then adopt its contents into this node
        FBVHierachy* NewRoot = FBVHierachy::Build(Combined, MaxDepth, MaxObjects);

        // Drop current subtree first to avoid leaks
        Clear();

        // Adopt fields from NewRoot (safe because this is a member function, we can access privates)
        Depth = NewRoot->Depth;
        MaxDepth = NewRoot->MaxDepth;
        MaxObjects = NewRoot->MaxObjects;
        Bounds = NewRoot->Bounds;
        Actors = std::move(NewRoot->Actors);
        Left = NewRoot->Left;
        Right = NewRoot->Right;
        ActorLastBounds = std::move(NewRoot->ActorLastBounds);

        // Null out NewRoot so its destructor doesn't delete our adopted children
        NewRoot->Left = nullptr;
        NewRoot->Right = nullptr;
        NewRoot->Actors = TArray<AActor*>();
        NewRoot->ActorLastBounds = TMap<AActor*, FBound>();
        delete NewRoot;

        return;
    }

    // Otherwise, do incremental online inserts
    for (const auto& kv : ActorsAndBounds)
    {
        Insert(kv.first, kv.second);
    }
}

bool FBVHierachy::Contains(const FBound& Box) const
{
    return Bounds.Contains(Box);
}

bool FBVHierachy::Remove(AActor* InActor, const FBound& ActorBounds)
{
    if (!InActor) return false;

    // 1) 리프에서 시도
    const bool isLeaf = (Left == nullptr && Right == nullptr);
    if (isLeaf)
    {
        auto it = std::find(Actors.begin(), Actors.end(), InActor);
        if (it == Actors.end()) return false;

        Actors.erase(it);
        ActorLastBounds.Remove(InActor);
        Refit();
        return true;
    }

    // 2) 내부 노드: 교차하는 자식으로 위임
    bool removed = false;
    if (Left && Left->Bounds.Intersects(ActorBounds))
        removed = Left->Remove(InActor, ActorBounds);
    if (!removed && Right && Right->Bounds.Intersects(ActorBounds))
        removed = Right->Remove(InActor, ActorBounds);

    // 3) 자식 병합 조건 확인(양쪽이 비면 해제)
    if (removed)
    {
        bool leftEmpty = (!Left) || (Left->Left == nullptr && Left->Right == nullptr && Left->Actors.empty());
        bool rightEmpty = (!Right) || (Right->Left == nullptr && Right->Right == nullptr && Right->Actors.empty());
        if (Left && leftEmpty) { delete Left; Left = nullptr; }
        if (Right && rightEmpty) { delete Right; Right = nullptr; }
        Refit();
        ActorLastBounds.Remove(InActor);
    }
    return removed;
}

void FBVHierachy::Update(AActor* InActor, const FBound& OldBounds, const FBound& NewBounds)
{
    if (!InActor) return;
    if (OldBounds.Min.X == NewBounds.Min.X && OldBounds.Min.Y == NewBounds.Min.Y && OldBounds.Min.Z == NewBounds.Min.Z &&
        OldBounds.Max.X == NewBounds.Max.X && OldBounds.Max.Y == NewBounds.Max.Y && OldBounds.Max.Z == NewBounds.Max.Z)
    {
        // 경계 동일 시 업데이트 불필요
        return;
    }

    Remove(InActor, OldBounds);
    Insert(InActor, NewBounds);
}

void FBVHierachy::Remove(AActor* InActor)
{
    if (!InActor) return;
    if (auto* Found = ActorLastBounds.Find(InActor))
    {
        Remove(InActor, *Found);
        ActorLastBounds.Remove(InActor);
    }
}

void FBVHierachy::Update(AActor* InActor)
{
    auto it = ActorLastBounds.find(InActor);
    if (it != ActorLastBounds.end())
    {
        Update(InActor, it->second, InActor->GetBounds());
    }
    else
    {
        Insert(InActor, InActor->GetBounds());
    }
}

void FBVHierachy::DebugDraw(URenderer* Renderer) const
{
    if (!Renderer) return;

    // 깊이에 따른 색상(간단한 변화)
    const float t = 1.0f - std::min(1.0f, Depth / float(std::max(1, MaxDepth)));
    const FVector4 LineColor(1.0f, t, 0.0f, 1.0f);

    TArray<FVector> Start;
    TArray<FVector> End;
    TArray<FVector4> Color;

    const FVector Min = Bounds.Min;
    const FVector Max = Bounds.Max;

    const FVector v0(Min.X, Min.Y, Min.Z);
    const FVector v1(Max.X, Min.Y, Min.Z);
    const FVector v2(Max.X, Max.Y, Min.Z);
    const FVector v3(Min.X, Max.Y, Min.Z);
    const FVector v4(Min.X, Min.Y, Max.Z);
    const FVector v5(Max.X, Min.Y, Max.Z);
    const FVector v6(Max.X, Max.Y, Max.Z);
    const FVector v7(Min.X, Max.Y, Max.Z);

    // 아래쪽 면
    Start.Add(v0); End.Add(v1); Color.Add(LineColor);
    Start.Add(v1); End.Add(v2); Color.Add(LineColor);
    Start.Add(v2); End.Add(v3); Color.Add(LineColor);
    Start.Add(v3); End.Add(v0); Color.Add(LineColor);

    // 위쪽 면
    Start.Add(v4); End.Add(v5); Color.Add(LineColor);
    Start.Add(v5); End.Add(v6); Color.Add(LineColor);
    Start.Add(v6); End.Add(v7); Color.Add(LineColor);
    Start.Add(v7); End.Add(v4); Color.Add(LineColor);

    // 기둥
    Start.Add(v0); End.Add(v4); Color.Add(LineColor);
    Start.Add(v1); End.Add(v5); Color.Add(LineColor);
    Start.Add(v2); End.Add(v6); Color.Add(LineColor);
    Start.Add(v3); End.Add(v7); Color.Add(LineColor);


    Renderer->AddLines(Start, End, Color);

    if (Left) Left->DebugDraw(Renderer);
    if (Right) Right->DebugDraw(Renderer);
}

int FBVHierachy::TotalNodeCount() const
{
    int count = 1; // self
    if (Left) count += Left->TotalNodeCount();
    if (Right) count += Right->TotalNodeCount();
    return count;
}

int FBVHierachy::TotalActorCount() const
{
    int count = 0;
    for (auto* a : Actors) { (void)a; ++count; }
    if (Left) count += Left->TotalActorCount();
    if (Right) count += Right->TotalActorCount();
    return count;
}

int FBVHierachy::MaxOccupiedDepth() const
{
    int maxDepth = Actors.empty() ? -1 : Depth;
    if (Left)
    {
        int childMax = Left->MaxOccupiedDepth();
        if (childMax > maxDepth) maxDepth = childMax;
    }
    if (Right)
    {
        int childMax = Right->MaxOccupiedDepth();
        if (childMax > maxDepth) maxDepth = childMax;
    }
    return std::max(maxDepth, Depth);
}

void FBVHierachy::DebugDump() const
{
    UE_LOG("===== BVHierachy DUMP BEGIN =====\r\n");

    struct StackItem { const FBVHierachy* Node; int D; };
    TArray<StackItem> stack;
    stack.push_back({ this, Depth });

    while (!stack.empty())
    {
        StackItem it = stack.back();
        stack.pop_back();
        const FBVHierachy* N = it.Node;

        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "[BVH] depth=%d, actors=%zu, bounds=[(%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f)]\r\n",
            it.D,
            N->Actors.size(),
            N->Bounds.Min.X, N->Bounds.Min.Y, N->Bounds.Min.Z,
            N->Bounds.Max.X, N->Bounds.Max.Y, N->Bounds.Max.Z);
        UE_LOG(buf);

        if (N->Right) stack.push_back({ N->Right, it.D + 1 });
        if (N->Left)  stack.push_back({ N->Left,  it.D + 1 });
    }

    UE_LOG("===== BVHierachy DUMP END =====\r\n");
}

void FBVHierachy::Split()
{
    // 이미 내부 노드면 패스
    if (Left || Right) return;

    // 액터가 2개 미만이면 분할 의미 없음
    if ((int)Actors.size() < 2)
    {
        Refit();
        return;
    }

    // 분할 축 선정(가장 긴 축)
    int axis = ChooseSplitAxis();

    // 중앙값 기반 분할로 변경 (더 균등한 분할 보장)
    struct ActorSort { AActor* Actor; float Key; };
    TArray<ActorSort> sortData;
    
    for (auto* a : Actors)
    {
        if (auto* pb = ActorLastBounds.Find(a))
        {
            FVector c = pb->GetCenter();
            float key = (axis == 0 ? c.X : (axis == 1 ? c.Y : c.Z));
            sortData.Add({ a, key });
        }
    }
    
    if (sortData.empty()) { Refit(); return; }
    
    // 중앙값으로 정렬
    std::sort(sortData.begin(), sortData.end(), 
        [](const ActorSort& a, const ActorSort& b) { return a.Key < b.Key; });
    
    TArray<AActor*> leftActors;
    TArray<AActor*> rightActors;
    
    int half = (int)sortData.size() / 2;
    for (int i = 0; i < (int)sortData.size(); ++i)
    {
        if (i < half) 
            leftActors.Add(sortData[i].Actor);
        else 
            rightActors.Add(sortData[i].Actor);
    }

    // 퇴화 방지: 한쪽이 비면 균등 분할
    if (leftActors.empty() || rightActors.empty())
    {
        leftActors = TArray<AActor*>();
        rightActors = TArray<AActor*>();
        int half = (int)Actors.size() / 2;
        int idx = 0;
        for (auto* a : Actors)
        {
            if (idx++ < half) leftActors.Add(a); else rightActors.Add(a);
        }
    }

    // 자식 생성
    Left = new FBVHierachy(Bounds, Depth + 1, MaxDepth, MaxObjects);
    Right = new FBVHierachy(Bounds, Depth + 1, MaxDepth, MaxObjects);

    // 재분배
    for (auto* a : leftActors)
    {
        const FBound* pb = ActorLastBounds.Find(a);
        if (pb) { Left->Actors.Add(a); Left->ActorLastBounds.Add(a, *pb); }
    }
    for (auto* a : rightActors)
    {
        const FBound* pb = ActorLastBounds.Find(a);
        if (pb) { Right->Actors.Add(a); Right->ActorLastBounds.Add(a, *pb); }
    }

    // 부모의 액터는 비움(내부 노드)
    Actors = TArray<AActor*>();

    // 자식 경계 재적합 후, 부모도 갱신
    Left->Refit();
    Right->Refit();
    Refit();
}

void FBVHierachy::Refit()
{
    // 내부 노드: 자식 Bounds 합집합
    if (Left || Right)
    {
        bool inited = false;
        FBound acc;
        if (Left) { acc = Left->Bounds; inited = true; }
        if (Right)
        {
            if (!inited) acc = Right->Bounds;
            else acc = UnionBounds(acc, Right->Bounds);
        }
        Bounds = acc;
        return;
    }

    // 리프: 보유 액터들의 Bounds 합집합
    bool inited = false;
    FBound acc;
    for (auto* a : Actors)
    {
        if (auto* pb = ActorLastBounds.Find(a))
        {
            if (!inited) { acc = *pb; inited = true; }
            else acc = UnionBounds(acc, *pb);
        }
    }
    if (inited) Bounds = acc;
}

FBound FBVHierachy::UnionBounds(const FBound& A, const FBound& B)
{
    FBound out;
    out.Min = FVector(
        std::min(A.Min.X, B.Min.X),
        std::min(A.Min.Y, B.Min.Y),
        std::min(A.Min.Z, B.Min.Z));
    out.Max = FVector(
        std::max(A.Max.X, B.Max.X),
        std::max(A.Max.Y, B.Max.Y),
        std::max(A.Max.Z, B.Max.Z));
    return out;
}

int FBVHierachy::ChooseSplitAxis() const
{
    // 리프에 담긴 액터들의 합집합 Bounds로 가장 긴 축 선택
    bool inited = false;
    FBound acc;
    for (auto* a : Actors)
    {
        if (auto* pb = ActorLastBounds.Find(a))
        {
            if (!inited) { acc = *pb; inited = true; }
            else acc = UnionBounds(acc, *pb);
        }
    }
    if (!inited) return 0;
    FVector ext = acc.GetExtent() * 2.0f;
    if (ext.X >= ext.Y && ext.X >= ext.Z) return 0;
    if (ext.Y >= ext.X && ext.Y >= ext.Z) return 1;
    return 2;
}

FBVHierachy* FBVHierachy::Build(const TArray<std::pair<AActor*, FBound>>& Items, int InMaxDepth, int InMaxObjects)
{
    // 입력을 빌더 아이템으로 변환(센트로이드 계산)
    TArray<FBuildItem> Work;
    Work.reserve(Items.size());
    for (const auto& kv : Items)
    {
        FBuildItem it;
        it.Actor = kv.first;
        it.Box = kv.second;
        it.Centroid = it.Box.GetCenter();
        Work.Add(it);
    }

    if (Work.empty())
    {
        return new FBVHierachy(FBound{}, 0, InMaxDepth, InMaxObjects);
    }

    return BuildRecursive(Work, 0, InMaxDepth, InMaxObjects);
}

FBVHierachy* FBVHierachy::BuildRecursive(TArray<FBuildItem>& Items, int Depth, int InMaxDepth, int InMaxObjects)
{
    // 현재 노드 Bounds 계산
    FBound bounds = Items[0].Box;
    for (size_t i = 1; i < Items.size(); ++i)
    {
        bounds = UnionBounds(bounds, Items[i].Box);
    }

    FBVHierachy* node = new FBVHierachy(bounds, Depth, InMaxDepth, InMaxObjects);

    // 리프 조건
    if ((int)Items.size() <= InMaxObjects || Depth >= InMaxDepth)
    {
        for (auto& it : Items)
        {
            node->Actors.Add(it.Actor);
            node->ActorLastBounds.Add(it.Actor, it.Box);
        }
        node->Refit();
        return node;
    }

    // 가장 긴 축
    FVector ext = (bounds.GetExtent() * 2.0f);
    int axis = 0;
    if (ext.X >= ext.Y && ext.X >= ext.Z) axis = 0;
    else if (ext.Y >= ext.X && ext.Y >= ext.Z) axis = 1;
    else axis = 2;

    // 중앙값 위치
    auto begin = Items.begin();
    auto mid = begin + Items.size() / 2;
    auto end = Items.end();

    std::nth_element(begin, mid, end, [axis](const FBuildItem& a, const FBuildItem& b)
    {
        const float ca = (axis == 0 ? a.Centroid.X : (axis == 1 ? a.Centroid.Y : a.Centroid.Z));
        const float cb = (axis == 0 ? b.Centroid.X : (axis == 1 ? b.Centroid.Y : b.Centroid.Z));
        return ca < cb;
    });

    // 양쪽 그룹 복사
    TArray<FBuildItem> LeftItems;
    TArray<FBuildItem> RightItems;
    LeftItems.reserve(Items.size()/2 + 1);
    RightItems.reserve(Items.size()/2 + 1);
    for (auto it = begin; it != mid; ++it) LeftItems.Add(*it);
    for (auto it = mid; it != end; ++it) RightItems.Add(*it);

    // 퇴화 방지: 비어있으면 균등 분할
    if (LeftItems.empty() || RightItems.empty())
    {
        LeftItems = TArray<FBuildItem>();
        RightItems = TArray<FBuildItem>();
        size_t half = Items.size() / 2;
        for (size_t i = 0; i < Items.size(); ++i)
        {
            if (i < half) LeftItems.Add(Items[i]); else RightItems.Add(Items[i]);
        }
    }

    node->Left = BuildRecursive(LeftItems, Depth + 1, InMaxDepth, InMaxObjects);
    node->Right = BuildRecursive(RightItems, Depth + 1, InMaxDepth, InMaxObjects);
    node->Refit();
    return node;
}


