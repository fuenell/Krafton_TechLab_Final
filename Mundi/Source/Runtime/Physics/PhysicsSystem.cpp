#include "pch.h"
#include "PhysicsSystem.h"
#include "PhysXConversion.h"
#include "BodyInstance.h"
#include "PhysicalMaterial.h"
#include "SphereElem.h"
#include "BoxElem.h"
#include "SphylElem.h"
#include "AggregateGeom.h"

// 싱글톤 인스턴스 반환
FPhysicsSystem& FPhysicsSystem::Get()
{
    static FPhysicsSystem instance;
    return instance;
}

FPhysicsSystem::FPhysicsSystem() = default;
FPhysicsSystem::~FPhysicsSystem() = default;

void FPhysicsSystem::CreateTestObjects()
{
    if (!mPhysics || !mScene) return;

    UE_LOG("========================================");
    UE_LOG("[PhysicsSystem] CreateTestObjects START");
    UE_LOG("========================================");

    // ============================================
    // 1. 좌표계 변환 테스트
    // ============================================
    UE_LOG("");
    UE_LOG("[TEST 1] Coordinate Conversion Test");
    UE_LOG("----------------------------------------");

    // 1-1. Vector 변환 테스트
    FVector TestPos(100.0f, 200.0f, 300.0f);
    PxVec3 PxPos = PhysXConvert::ToPx(TestPos);
    FVector BackPos = PhysXConvert::FromPx(PxPos);

    UE_LOG("Vector ToPx:   (%.1f, %.1f, %.1f) -> (%.1f, %.1f, %.1f)",
           TestPos.X, TestPos.Y, TestPos.Z, PxPos.x, PxPos.y, PxPos.z);
    UE_LOG("Vector FromPx: (%.1f, %.1f, %.1f) -> (%.1f, %.1f, %.1f)",
           PxPos.x, PxPos.y, PxPos.z, BackPos.X, BackPos.Y, BackPos.Z);
    UE_LOG("Roundtrip OK: %s", (TestPos == BackPos) ? "YES" : "NO");

    // 1-2. Quaternion 변환 테스트 (Z축 90도 회전)
    FQuat TestQuat = FQuat::FromAxisAngle(FVector(0, 0, 1), PI / 2.0f); // Z축 90도
    PxQuat PxQ = PhysXConvert::ToPx(TestQuat);
    FQuat BackQuat = PhysXConvert::FromPx(PxQ);

    UE_LOG("Quat ToPx:   (%.3f, %.3f, %.3f, %.3f) -> (%.3f, %.3f, %.3f, %.3f)",
           TestQuat.X, TestQuat.Y, TestQuat.Z, TestQuat.W,
           PxQ.x, PxQ.y, PxQ.z, PxQ.w);
    UE_LOG("Quat FromPx: (%.3f, %.3f, %.3f, %.3f) -> (%.3f, %.3f, %.3f, %.3f)",
           PxQ.x, PxQ.y, PxQ.z, PxQ.w,
           BackQuat.X, BackQuat.Y, BackQuat.Z, BackQuat.W);

    // ============================================
    // 2. 바닥 생성 (Ground Plane)
    // ============================================
    UE_LOG("");
    UE_LOG("[TEST 2] Ground Plane");
    UE_LOG("----------------------------------------");

    // PhysX Y-up 시스템에서 Y=0 평면 (프로젝트에서는 Z=0)
    PxRigidStatic* ground = PxCreatePlane(*mPhysics, PxPlane(0, 1, 0, 0), *mMaterial);
    mScene->addActor(*ground);
    UE_LOG("Ground plane created (PhysX Y=0, Project Z=0)");

    // ============================================
    // 3. FKShapeElem 테스트
    // ============================================
    UE_LOG("");
    UE_LOG("[TEST 3] FKShapeElem Classes");
    UE_LOG("----------------------------------------");

    // 3-1. Sphere
    FKSphereElem SphereElem(50.0f);  // 반지름 50
    SphereElem.Center = FVector(0, 0, 100);  // 프로젝트 좌표로 Z=100 위
    UE_LOG("SphereElem: Center=(%.1f, %.1f, %.1f), Radius=%.1f, Volume=%.1f",
           SphereElem.Center.X, SphereElem.Center.Y, SphereElem.Center.Z,
           SphereElem.Radius, SphereElem.GetVolume());

    // 3-2. Box
    FKBoxElem BoxElem(25.0f, 50.0f, 75.0f);  // Half extents
    BoxElem.Center = FVector(200, 0, 75);
    UE_LOG("BoxElem: Center=(%.1f, %.1f, %.1f), HalfExtent=(%.1f, %.1f, %.1f), Volume=%.1f",
           BoxElem.Center.X, BoxElem.Center.Y, BoxElem.Center.Z,
           BoxElem.X, BoxElem.Y, BoxElem.Z, BoxElem.GetVolume());

    // 3-3. Capsule (Sphyl)
    FKSphylElem SphylElem(30.0f, 100.0f);  // 반지름 30, 길이 100
    SphylElem.Center = FVector(-200, 0, 80);
    UE_LOG("SphylElem: Center=(%.1f, %.1f, %.1f), Radius=%.1f, Length=%.1f, TotalHeight=%.1f, Volume=%.1f",
           SphylElem.Center.X, SphylElem.Center.Y, SphylElem.Center.Z,
           SphylElem.Radius, SphylElem.Length, SphylElem.GetTotalHeight(), SphylElem.GetVolume());

    // ============================================
    // 4. FKAggregateGeom 테스트
    // ============================================
    UE_LOG("");
    UE_LOG("[TEST 4] FKAggregateGeom");
    UE_LOG("----------------------------------------");

    FKAggregateGeom AggGeom;
    AggGeom.AddSphereElem(SphereElem);
    AggGeom.AddBoxElem(BoxElem);
    AggGeom.AddSphylElem(SphylElem);

    UE_LOG("AggGeom: TotalElements=%d, Spheres=%d, Boxes=%d, Sphyls=%d",
           AggGeom.GetElementCount(),
           AggGeom.GetElementCount(EAggCollisionShape::Sphere),
           AggGeom.GetElementCount(EAggCollisionShape::Box),
           AggGeom.GetElementCount(EAggCollisionShape::Sphyl));
    UE_LOG("AggGeom: TotalVolume=%.1f", AggGeom.GetScaledVolume(FVector::One()));

    // ============================================
    // 5. FBodyInstance 테스트 - Dynamic Sphere
    // ============================================
    UE_LOG("");
    UE_LOG("[TEST 5] FBodyInstance - Dynamic Sphere");
    UE_LOG("----------------------------------------");

    // 동적 구체 생성 (낙하 테스트)
    FBodyInstance* DynamicSphere = new FBodyInstance();
    DynamicSphere->bSimulatePhysics = true;
    DynamicSphere->bEnableGravity = true;
    DynamicSphere->MassInKg = 10.0f;
    DynamicSphere->bOverrideMass = true;

    FTransform SphereTransform(FVector(0, 0, 300), FQuat::Identity(), FVector::One());
    PxSphereGeometry SphereGeom(50.0f);
    DynamicSphere->InitBody(SphereTransform, SphereGeom, nullptr);

    FVector SpawnPos = DynamicSphere->GetWorldLocation();
    UE_LOG("Dynamic Sphere spawned at Project coords: (%.1f, %.1f, %.1f)",
           SpawnPos.X, SpawnPos.Y, SpawnPos.Z);
    UE_LOG("Mass: %.1f kg, Gravity: %s",
           DynamicSphere->GetBodyMass(),
           DynamicSphere->bEnableGravity ? "ON" : "OFF");

    // ============================================
    // 6. FBodyInstance 테스트 - Static Box
    // ============================================
    UE_LOG("");
    UE_LOG("[TEST 6] FBodyInstance - Static Box");
    UE_LOG("----------------------------------------");

    FBodyInstance* StaticBox = new FBodyInstance();
    StaticBox->bSimulatePhysics = false;  // Static

    FTransform BoxTransform(FVector(0, 0, 25), FQuat::Identity(), FVector::One());
    PxBoxGeometry BoxGeom(PhysXConvert::BoxHalfExtentToPx(100.0f, 100.0f, 25.0f));
    StaticBox->InitBody(BoxTransform, BoxGeom, nullptr);

    FVector BoxPos = StaticBox->GetWorldLocation();
    UE_LOG("Static Box spawned at Project coords: (%.1f, %.1f, %.1f)",
           BoxPos.X, BoxPos.Y, BoxPos.Z);
    UE_LOG("IsStatic: %s", StaticBox->IsDynamic() ? "NO (Dynamic)" : "YES (Static)");

    // ============================================
    // 7. FBodyInstance 테스트 - Dynamic Box with DOF Lock
    // ============================================
    UE_LOG("");
    UE_LOG("[TEST 7] FBodyInstance - Dynamic Box with XY Plane Lock");
    UE_LOG("----------------------------------------");

    FBodyInstance* LockedBox = new FBodyInstance();
    LockedBox->bSimulatePhysics = true;
    LockedBox->bEnableGravity = true;
    LockedBox->DOFMode = EDOFMode::XYPlane;  // Z축 이동/회전 잠금

    FTransform LockedBoxTransform(FVector(150, 0, 200), FQuat::Identity(), FVector::One());
    PxBoxGeometry LockedBoxGeom(PhysXConvert::BoxHalfExtentToPx(30.0f, 30.0f, 30.0f));
    LockedBox->InitBody(LockedBoxTransform, LockedBoxGeom, nullptr);

    UE_LOG("Locked Box spawned at: (%.1f, %.1f, %.1f)",
           LockedBox->GetWorldLocation().X,
           LockedBox->GetWorldLocation().Y,
           LockedBox->GetWorldLocation().Z);
    UE_LOG("DOFMode: XYPlane (Z-axis locked in project coords)");

    // ============================================
    // 완료
    // ============================================
    UE_LOG("");
    UE_LOG("========================================");
    UE_LOG("[PhysicsSystem] CreateTestObjects END");
    UE_LOG("========================================");

    // NOTE: FBodyInstance들은 테스트용으로 메모리 누수 발생
    // 실제로는 Component가 소유하고 정리해야 함
}

void FPhysicsSystem::Initialize()
{
    // Foundation 생성
    mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mAllocator, mErrorCallback);
    if (!mFoundation) {
        UE_LOG("[FPhysicsSystem]: PxCreateFoundation Error");
        return;
    }

    // PVD 연결
    mPvd = PxCreatePvd(*mFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    mPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    // Physics 메인 객체 생성
    mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true, mPvd);

    // Scene(물리 월드) 설정
    PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f); // 중력: PhysX Y축 아래로 (프로젝트 Z축 아래)

    // CPU 쓰레드 디스패처 (2개의 작업 스레드 사용)
    mDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = mDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader; // 기본 충돌 필터

    // 성능 및 디버깅 플래그
    sceneDesc.flags |= PxSceneFlag::eENABLE_PCM; // 정확도 향상
    sceneDesc.flags |= PxSceneFlag::eENABLE_CCD; // 총알 같은 빠른 물체 뚫림 방지

    // Scene 생성
    mScene = mPhysics->createScene(sceneDesc);

    // PVD에 씬 정보 보내기
    PxPvdSceneClient* pvdClient = mScene->getScenePvdClient();
    if (pvdClient)
    {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
    }

    // 기본 재질 생성 (마찰력 0.5, 반발력 0.5)
    mMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    if (mScene->getScenePvdClient()) {
        mScene->getScenePvdClient()->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        mScene->getScenePvdClient()->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        mScene->getScenePvdClient()->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
    }

    // FOR TEST
    CreateTestObjects();
}

void FPhysicsSystem::UpdateSimulation(float DeltaTime)
{
    if (!mScene) return;

    // 시뮬레이션 시작
    mScene->simulate(DeltaTime);

    // 결과 대기 (true = 끝날 때까지 멈춤. 간단한 구현용)
    // 나중에는 이걸 false로 하고 렌더링 중에 물리 계산 돌리는 최적화
    mScene->fetchResults(true);
}

void FPhysicsSystem::Shutdown()
{
    // 생성의 역순으로 해제
    if (mScene)         mScene->release();
    if (mDispatcher)    mDispatcher->release();
    if (mPhysics)       mPhysics->release();

    if (mPvd)
    {
        PxPvdTransport* transport = mPvd->getTransport();
        mPvd->release();
        transport->release();
    }

    if (mFoundation)    mFoundation->release();
}
