#pragma once
#include "Widget.h"
#include "SCurveEditorWidget.generated.h"

class UParticleModule;
struct FDistributionFloat;
struct FDistributionVector;
struct FInterpCurveFloat;
struct FInterpCurvePointFloat;
struct FInterpCurveVector;
struct FInterpCurvePointVector;
struct ParticleEditorState;
struct ImVec2;
struct ImDrawList;

// 개별 커브 트랙 (하나의 Distribution 프로퍼티)
struct FCurveTrack
{
	UParticleModule* Module = nullptr;
	FString PropertyName;
	FString DisplayName;
	FDistributionFloat* FloatCurve = nullptr;
	FDistributionVector* VectorCurve = nullptr;

	bool bVisible = true;  // 노란 체크박스 (표시 토글)
	bool bShowX = true;    // R/X 채널 토글
	bool bShowY = true;    // G/Y 채널 토글
	bool bShowZ = true;    // B/Z 채널 토글

	uint32 TrackColor = 0;  // 트랙 고유 색상 (ImU32)

	bool IsFloatCurve() const { return FloatCurve != nullptr; }
	bool IsVectorCurve() const { return VectorCurve != nullptr; }
};

// 커브 에디터 상태
struct FCurveEditorState
{
	// 다중 트랙 관리
	TArray<FCurveTrack> Tracks;
	int32 SelectedTrackIndex = -1;

	// 뷰 설정 (공유)
	float ViewMinTime = 0.0f;
	float ViewMaxTime = 1.0f;
	float ViewMinValue = -1.0f;
	float ViewMaxValue = 1.0f;

	// 선택된 키
	int32 SelectedKeyIndex = -1;
	int32 SelectedAxis = -1;  // Vector 커브용: 0=X, 1=Y, 2=Z, -1=전체

	// 선택된 탄젠트 핸들 (0=없음, 1=Arrive(왼쪽), 2=Leave(오른쪽))
	int32 SelectedTangentHandle = 0;

	// 헬퍼 함수
	bool HasModule(UParticleModule* Module) const;
	void AddModuleTracks(UParticleModule* Module);
	void RemoveModuleTracks(UParticleModule* Module);

	// 선택된 트랙 가져오기
	FCurveTrack* GetSelectedTrack()
	{
		if (SelectedTrackIndex >= 0 && SelectedTrackIndex < Tracks.Num())
		{
			return &Tracks[SelectedTrackIndex];
		}
		return nullptr;
	}

	void Reset()
	{
		Tracks.Empty();
		SelectedTrackIndex = -1;
		SelectedKeyIndex = -1;
		SelectedAxis = -1;
		SelectedTangentHandle = 0;
		ViewMinTime = 0.0f;
		ViewMaxTime = 1.0f;
		ViewMinValue = -1.0f;
		ViewMaxValue = 1.0f;
	}
};

// 커브 에디터 위젯
UCLASS()
class SCurveEditorWidget : public UWidget
{
public:
	GENERATED_REFLECTION_BODY()

	SCurveEditorWidget();
	virtual ~SCurveEditorWidget() = default;

	// UWidget 인터페이스
	virtual void Initialize() override;
	virtual void Update() override;
	virtual void RenderWidget() override;

	// 외부 상태 연결
	void SetEditorState(ParticleEditorState* InState) { EditorState = InState; }

	// 모듈 트랙 관리
	void ToggleModuleTracks(UParticleModule* Module);
	bool HasModule(UParticleModule* Module) const;
	uint32 GetModuleColor(UParticleModule* Module) const;
	bool HasCurveProperties(UParticleModule* Module) const;

	// 상태 접근
	FCurveEditorState& GetCurveState() { return CurveState; }
	const FCurveEditorState& GetCurveState() const { return CurveState; }

private:
	// 커브 에디터 내부 상태
	FCurveEditorState CurveState;

	// 외부 참조 (소유하지 않음)
	ParticleEditorState* EditorState = nullptr;

	// 레이아웃 상수
	static constexpr float TrackListWidthRatio = 0.25f;

	// 렌더링 함수
	void RenderTrackList();
	void RenderChannelButtons(FCurveTrack& Track);
	void RenderGraphView();
	void RenderTrackCurve(ImDrawList* DrawList, ImVec2 CanvasPos, ImVec2 CanvasSize, FCurveTrack& Track);
	void RenderCurveGrid(ImDrawList* DrawList, ImVec2 CanvasPos, ImVec2 CanvasSize);
	void RenderCurveKeys(ImDrawList* DrawList, ImVec2 CanvasPos, ImVec2 CanvasSize);
	void RenderTangentHandles(ImDrawList* DrawList, FInterpCurvePointFloat& Point, float KeyX, float KeyY, ImVec2 CanvasSize, int32 CurveIndex);
	void RenderTangentHandlesVector(ImDrawList* DrawList, FInterpCurvePointVector& Point, int32 Axis, float KeyX, float KeyY, ImVec2 CanvasSize, int32 CurveIndex);

	// 인터랙션
	void HandleCurveInteraction(ImVec2 CanvasPos, ImVec2 CanvasSize);
	void AutoFitCurveView();
};
