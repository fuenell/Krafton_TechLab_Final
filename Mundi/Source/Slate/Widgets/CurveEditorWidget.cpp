#include "pch.h"
#include "CurveEditorWidget.h"
#include "Source/Runtime/Engine/Viewer/ViewerState.h"
#include "ParticleModule.h"
#include "Distribution.h"

//IMPLEMENT_CLASS(SCurveEditorWidget)

SCurveEditorWidget::SCurveEditorWidget()
{
}

void SCurveEditorWidget::Initialize()
{
	UWidget::Initialize();
}

void SCurveEditorWidget::Update()
{
	UWidget::Update();
}

void SCurveEditorWidget::RenderWidget()
{
	// 렌더링할 영역 크기 가져오기
	ImVec2 TotalSize = ImGui::GetContentRegionAvail();
	TotalSize.y = FMath::Max(TotalSize.y, 150.0f);

	// 트랙 리스트 너비 계산
	float TrackListWidth = TotalSize.x * TrackListWidthRatio;
	float GraphWidth = TotalSize.x - TrackListWidth - 4.0f;

	// 좌측: 트랙 리스트
	ImGui::BeginChild("CurveTrackList", ImVec2(TrackListWidth, TotalSize.y), true);
	RenderTrackList();
	ImGui::EndChild();

	ImGui::SameLine();

	// 우측: 그래프 뷰
	ImGui::BeginChild("CurveGraphView", ImVec2(GraphWidth, TotalSize.y), true);
	RenderGraphView();
	ImGui::EndChild();
}

void SCurveEditorWidget::ToggleModuleTracks(UParticleModule* Module)
{
	if (!Module) return;

	if (CurveState.HasModule(Module))
	{
		// 이미 있으면 제거
		CurveState.RemoveModuleTracks(Module);
	}
	else
	{
		// 없으면 추가
		CurveState.AddModuleTracks(Module);
		AutoFitCurveView();
	}
}

bool SCurveEditorWidget::HasModule(UParticleModule* Module) const
{
	return CurveState.HasModule(Module);
}

bool SCurveEditorWidget::HasCurveProperties(UParticleModule* Module) const
{
	if (!Module) return false;

	UClass* ModuleClass = Module->GetClass();
	if (!ModuleClass) return false;

	// Distribution 프로퍼티가 있으면 true 반환 (커브 모드 여부 상관없이)
	const TArray<FProperty>& Properties = ModuleClass->GetProperties();
	for (const FProperty& Prop : Properties)
	{
		if (Prop.Type == EPropertyType::DistributionFloat ||
			Prop.Type == EPropertyType::DistributionVector ||
			Prop.Type == EPropertyType::DistributionColor)
		{
			return true;
		}
	}
	return false;
}

uint32 SCurveEditorWidget::GetModuleColor(UParticleModule* Module) const
{
	if (!Module || !CurveState.HasModule(Module))
	{
		return 0;  // 모듈이 커브 에디터에 없으면 0 반환
	}

	// 모듈별 색상 매핑 (RenderTrackList와 동일한 로직)
	TArray<UParticleModule*> UniqueModules;
	for (const FCurveTrack& Track : CurveState.Tracks)
	{
		if (!UniqueModules.Contains(Track.Module))
		{
			UniqueModules.Add(Track.Module);
		}
	}

	// 모듈 색상 팔레트
	static uint32 ModuleColors[] = {
		IM_COL32(255, 200, 0, 255),    // 노랑
		IM_COL32(0, 200, 255, 255),    // 시안
		IM_COL32(255, 100, 255, 255),  // 마젠타
		IM_COL32(100, 255, 100, 255),  // 라임
		IM_COL32(255, 150, 100, 255),  // 주황
		IM_COL32(150, 100, 255, 255),  // 보라
	};
	const int32 NumColors = sizeof(ModuleColors) / sizeof(ModuleColors[0]);

	int32 ModuleIndex = UniqueModules.Find(Module);
	if (ModuleIndex >= 0)
	{
		return ModuleColors[ModuleIndex % NumColors];
	}
	return 0;
}

// ============================================================
// FCurveEditorState 메서드
// ============================================================

bool FCurveEditorState::HasModule(UParticleModule* Module) const
{
	if (!Module) return false;

	for (const FCurveTrack& Track : Tracks)
	{
		if (Track.Module == Module)
			return true;
	}
	return false;
}

void FCurveEditorState::AddModuleTracks(UParticleModule* Module)
{
	if (!Module) return;

	UClass* ModuleClass = Module->GetClass();
	if (!ModuleClass) return;

	const TArray<FProperty>& Properties = ModuleClass->GetProperties();

	// 트랙 색상 배열
	static uint32 TrackColors[] = {
		IM_COL32(255, 200, 0, 255),    // 노랑
		IM_COL32(0, 200, 255, 255),    // 시안
		IM_COL32(255, 100, 255, 255),  // 마젠타
		IM_COL32(100, 255, 100, 255),  // 라임
		IM_COL32(255, 150, 100, 255),  // 주황
		IM_COL32(150, 100, 255, 255),  // 보라
	};
	const int32 NumColors = sizeof(TrackColors) / sizeof(TrackColors[0]);

	// 모듈의 모든 Distribution 프로퍼티를 트랙으로 추가
	for (const FProperty& Prop : Properties)
	{
		if (Prop.Type == EPropertyType::DistributionFloat ||
			Prop.Type == EPropertyType::DistributionVector ||
			Prop.Type == EPropertyType::DistributionColor)
		{
			FCurveTrack Track;
			Track.Module = Module;
			Track.PropertyName = Prop.Name;
			Track.DisplayName = Prop.Name;

			// 트랙 색상
			Track.TrackColor = TrackColors[Tracks.Num() % NumColors];

			// 커브 포인터 설정
			if (Prop.Type == EPropertyType::DistributionFloat)
			{
				Track.FloatCurve = Prop.GetValuePtr<FDistributionFloat>(Module);
			}
			else if (Prop.Type == EPropertyType::DistributionVector ||
					 Prop.Type == EPropertyType::DistributionColor)
			{
				Track.VectorCurve = Prop.GetValuePtr<FDistributionVector>(Module);
			}

			Tracks.Add(Track);
		}
	}
}

void FCurveEditorState::RemoveModuleTracks(UParticleModule* Module)
{
	if (!Module) return;

	// 선택된 트랙이 제거되는 모듈의 것이면 선택 해제
	if (SelectedTrackIndex >= 0 && SelectedTrackIndex < Tracks.Num())
	{
		if (Tracks[SelectedTrackIndex].Module == Module)
		{
			SelectedTrackIndex = -1;
			SelectedKeyIndex = -1;
			SelectedAxis = -1;
		}
	}

	// 모듈에 해당하는 모든 트랙 제거 (역순으로 제거)
	for (int32 i = Tracks.Num() - 1; i >= 0; --i)
	{
		if (Tracks[i].Module == Module)
		{
			Tracks.RemoveAt(i);

			// 선택 인덱스 조정
			if (SelectedTrackIndex > i)
			{
				SelectedTrackIndex--;
			}
		}
	}
}

// ============================================================
// 렌더링 함수
// ============================================================

void SCurveEditorWidget::RenderTrackList()
{
	if (CurveState.Tracks.Num() == 0)
	{
		return;
	}

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	ImVec2 WindowPos = ImGui::GetWindowPos();
	ImVec2 WindowSize = ImGui::GetWindowSize();
	float ScrollY = ImGui::GetScrollY();

	// 모듈별 색상 매핑 (같은 모듈 = 같은 색상)
	TArray<UParticleModule*> UniqueModules;
	for (const FCurveTrack& Track : CurveState.Tracks)
	{
		if (!UniqueModules.Contains(Track.Module))
		{
			UniqueModules.Add(Track.Module);
		}
	}

	// 모듈 색상 팔레트
	static ImU32 ModuleColors[] = {
		IM_COL32(255, 200, 0, 255),    // 노랑
		IM_COL32(0, 200, 255, 255),    // 시안
		IM_COL32(255, 100, 255, 255),  // 마젠타
		IM_COL32(100, 255, 100, 255),  // 라임
		IM_COL32(255, 150, 100, 255),  // 주황
		IM_COL32(150, 100, 255, 255),  // 보라
	};
	const int32 NumColors = sizeof(ModuleColors) / sizeof(ModuleColors[0]);

	// 레이아웃 상수
	const float HorizontalMargin = 6.0f;
	const float ColorBarWidth = 6.0f;
	const float ContentStartX = HorizontalMargin + ColorBarWidth + 4.0f;
	const float LineHeight = ImGui::GetTextLineHeight();
	const float RowHeight = LineHeight * 2.0f + 9.0f;
	const float ToggleBoxSize = 8.0f;

	for (int32 i = 0; i < CurveState.Tracks.Num(); ++i)
	{
		FCurveTrack& Track = CurveState.Tracks[i];
		ImGui::PushID(i);

		// 트랙 행 시작 위치
		ImVec2 CursorPos = ImGui::GetCursorPos();
		float RowStartY = WindowPos.y + CursorPos.y - ScrollY;

		// 모듈별 색상 가져오기
		int32 ModuleIndex = UniqueModules.Find(Track.Module);
		ImU32 ModuleColor = ModuleColors[ModuleIndex % NumColors];

		// 스크롤바 존재 여부에 따라 우측 여백 조정
		float ScrollbarReserve = (ImGui::GetScrollMaxY() > 0) ? ImGui::GetStyle().ScrollbarSize : 0;
		float RightEdge = WindowPos.x + WindowSize.x - HorizontalMargin - ScrollbarReserve;

		// 트랙 배경 (회색)
		DrawList->AddRectFilled(
			ImVec2(WindowPos.x + HorizontalMargin, RowStartY),
			ImVec2(RightEdge, RowStartY + RowHeight - 2),
			IM_COL32(50, 50, 50, 255));

		// 모듈별 색상 바 렌더링
		DrawList->AddRectFilled(
			ImVec2(WindowPos.x + HorizontalMargin, RowStartY),
			ImVec2(WindowPos.x + HorizontalMargin + ColorBarWidth, RowStartY + RowHeight - 2),
			ModuleColor);

		// 1행: 트랙 이름
		ImGui::SetCursorPosX(ContentStartX);
		ImGui::TextUnformatted(Track.DisplayName.c_str());

		// 클릭으로 트랙 선택
		ImVec2 TextMin = ImVec2(WindowPos.x + ContentStartX, RowStartY);
		ImVec2 TextMax = ImVec2(WindowPos.x + WindowSize.x - 20, RowStartY + LineHeight);
		ImVec2 MousePos = ImGui::GetMousePos();
		if (ImGui::IsMouseClicked(0) &&
			MousePos.x >= TextMin.x && MousePos.x <= TextMax.x &&
			MousePos.y >= TextMin.y && MousePos.y <= TextMax.y)
		{
			CurveState.SelectedTrackIndex = i;
			CurveState.SelectedKeyIndex = -1;
			CurveState.SelectedAxis = -1;
		}

		// 2행 Y 위치
		float SecondRowY = RowStartY + LineHeight + 15.0f;
		float BoxX = WindowPos.x + ContentStartX;
		float BoxY = SecondRowY;
		float BoxSpacing = ToggleBoxSize + 2.0f;

		// 채널 토글 박스 렌더링
		if (Track.VectorCurve || Track.FloatCurve)
		{
			// R 박스
			ImU32 RColor = Track.bShowX ? IM_COL32(200, 60, 60, 255) : IM_COL32(30, 30, 30, 255);
			DrawList->AddRectFilled(
				ImVec2(BoxX, BoxY),
				ImVec2(BoxX + ToggleBoxSize, BoxY + ToggleBoxSize),
				RColor);

			// R 클릭 감지
			MousePos = ImGui::GetMousePos();
			if (ImGui::IsMouseClicked(0) &&
				MousePos.x >= BoxX && MousePos.x <= BoxX + ToggleBoxSize &&
				MousePos.y >= BoxY && MousePos.y <= BoxY + ToggleBoxSize)
			{
				Track.bShowX = !Track.bShowX;
			}

			if (Track.VectorCurve)
			{
				// G 박스
				ImU32 GColor = Track.bShowY ? IM_COL32(60, 200, 60, 255) : IM_COL32(30, 30, 30, 255);
				DrawList->AddRectFilled(
					ImVec2(BoxX + BoxSpacing, BoxY),
					ImVec2(BoxX + BoxSpacing + ToggleBoxSize, BoxY + ToggleBoxSize),
					GColor);

				// G 클릭 감지
				if (ImGui::IsMouseClicked(0) &&
					MousePos.x >= BoxX + BoxSpacing && MousePos.x <= BoxX + BoxSpacing + ToggleBoxSize &&
					MousePos.y >= BoxY && MousePos.y <= BoxY + ToggleBoxSize)
				{
					Track.bShowY = !Track.bShowY;
				}

				// B 박스
				ImU32 BColor = Track.bShowZ ? IM_COL32(60, 60, 200, 255) : IM_COL32(30, 30, 30, 255);
				DrawList->AddRectFilled(
					ImVec2(BoxX + BoxSpacing * 2, BoxY),
					ImVec2(BoxX + BoxSpacing * 2 + ToggleBoxSize, BoxY + ToggleBoxSize),
					BColor);

				// B 클릭 감지
				if (ImGui::IsMouseClicked(0) &&
					MousePos.x >= BoxX + BoxSpacing * 2 && MousePos.x <= BoxX + BoxSpacing * 2 + ToggleBoxSize &&
					MousePos.y >= BoxY && MousePos.y <= BoxY + ToggleBoxSize)
				{
					Track.bShowZ = !Track.bShowZ;
				}
			}
		}

		// 가시성 토글 박스
		float VisBoxX = RightEdge - 4 - ToggleBoxSize;
		float VisBoxY = SecondRowY;
		ImU32 VisColor = Track.bVisible ? IM_COL32(255, 180, 0, 255) : IM_COL32(80, 80, 80, 255);
		DrawList->AddRectFilled(
			ImVec2(VisBoxX, VisBoxY),
			ImVec2(VisBoxX + ToggleBoxSize, VisBoxY + ToggleBoxSize),
			VisColor);

		// 가시성 클릭 감지
		MousePos = ImGui::GetMousePos();
		if (ImGui::IsMouseClicked(0) &&
			MousePos.x >= VisBoxX && MousePos.x <= VisBoxX + ToggleBoxSize &&
			MousePos.y >= VisBoxY && MousePos.y <= VisBoxY + ToggleBoxSize)
		{
			Track.bVisible = !Track.bVisible;
		}

		// 다음 트랙을 위해 커서 이동
		ImGui::SetCursorPosY(CursorPos.y + RowHeight);
		ImGui::Dummy(ImVec2(1, 1));

		// 구분선
		float SeparatorY = RowStartY + RowHeight - 2;
		DrawList->AddLine(
			ImVec2(WindowPos.x + HorizontalMargin, SeparatorY),
			ImVec2(RightEdge, SeparatorY),
			IM_COL32(80, 80, 80, 255), 1.0f);

		ImGui::PopID();
	}
}

void SCurveEditorWidget::RenderChannelButtons(FCurveTrack& Track)
{
	// R 버튼
	ImGui::PushStyleColor(ImGuiCol_Button, Track.bShowX ? ImVec4(0.8f, 0.2f, 0.2f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	if (ImGui::SmallButton("R"))
	{
		Track.bShowX = !Track.bShowX;
	}
	ImGui::PopStyleColor();

	ImGui::SameLine(0, 2);

	// G 버튼
	ImGui::PushStyleColor(ImGuiCol_Button, Track.bShowY ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	if (ImGui::SmallButton("G"))
	{
		Track.bShowY = !Track.bShowY;
	}
	ImGui::PopStyleColor();

	ImGui::SameLine(0, 2);

	// B 버튼
	ImGui::PushStyleColor(ImGuiCol_Button, Track.bShowZ ? ImVec4(0.2f, 0.2f, 0.8f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	if (ImGui::SmallButton("B"))
	{
		Track.bShowZ = !Track.bShowZ;
	}
	ImGui::PopStyleColor();
}

void SCurveEditorWidget::RenderGraphView()
{
	ImVec2 FullSize = ImGui::GetContentRegionAvail();
	FullSize.y = FMath::Max(FullSize.y, 150.0f);

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	ImVec2 FullPos = ImGui::GetCursorScreenPos();

	// 라벨 여백 확보
	const float LeftMargin = 35.0f;
	const float BottomMargin = 20.0f;
	ImVec2 CanvasPos = ImVec2(FullPos.x + LeftMargin, FullPos.y);
	ImVec2 CanvasSize = ImVec2(FullSize.x - LeftMargin, FullSize.y - BottomMargin);

	// 배경
	DrawList->AddRectFilled(CanvasPos,
		ImVec2(CanvasPos.x + CanvasSize.x, CanvasPos.y + CanvasSize.y),
		IM_COL32(20, 20, 20, 255));

	// 그리드
	RenderCurveGrid(DrawList, CanvasPos, CanvasSize);

	// 모든 visible 트랙의 커브 렌더링
	for (int32 i = 0; i < CurveState.Tracks.Num(); ++i)
	{
		FCurveTrack& Track = CurveState.Tracks[i];
		if (!Track.bVisible)
		{
			continue;
		}

		RenderTrackCurve(DrawList, CanvasPos, CanvasSize, Track);
	}

	// 모든 visible 트랙의 키 렌더링
	RenderCurveKeys(DrawList, CanvasPos, CanvasSize);

	// 인터랙션 영역
	ImGui::InvisibleButton("CurveCanvas", CanvasSize);
	HandleCurveInteraction(CanvasPos, CanvasSize);
}

void SCurveEditorWidget::RenderTrackCurve(ImDrawList* DrawList, ImVec2 CanvasPos, ImVec2 CanvasSize, FCurveTrack& Track)
{
	// 값을 Y 좌표로 변환하는 헬퍼 람다
	auto ValueToY = [&](float Value) -> float {
		float y = CanvasPos.y + CanvasSize.y -
			((Value - CurveState.ViewMinValue) /
			 (CurveState.ViewMaxValue - CurveState.ViewMinValue)) * CanvasSize.y;
		return FMath::Clamp(y, CanvasPos.y, CanvasPos.y + CanvasSize.y);
	};

	if (Track.FloatCurve)
	{
		// 채널 색상: R=빨강(값/Min), G=초록(Max)
		ImU32 RColor = IM_COL32(255, 80, 80, 255);
		ImU32 GColor = IM_COL32(80, 255, 80, 255);

		EDistributionType Type = Track.FloatCurve->Type;

		if (Type == EDistributionType::Constant)
		{
			// Constant: 단일 수평선 (R 채널)
			if (Track.bShowX)
			{
				float Value = Track.FloatCurve->ConstantValue;
				float y = ValueToY(Value);
				DrawList->AddLine(
					ImVec2(CanvasPos.x, y),
					ImVec2(CanvasPos.x + CanvasSize.x, y),
					RColor, 2.0f);
			}
		}
		else if (Type == EDistributionType::Uniform)
		{
			// Uniform: min(R)/max(G) 두 개의 수평선
			float MinVal = Track.FloatCurve->MinValue;
			float MaxVal = Track.FloatCurve->MaxValue;

			// Min 수평선 (R 채널)
			if (Track.bShowX)
			{
				float yMin = ValueToY(MinVal);
				DrawList->AddLine(
					ImVec2(CanvasPos.x, yMin),
					ImVec2(CanvasPos.x + CanvasSize.x, yMin),
					RColor, 2.0f);
			}

			// Max 수평선 (G 채널)
			if (Track.bShowY)
			{
				float yMax = ValueToY(MaxVal);
				DrawList->AddLine(
					ImVec2(CanvasPos.x, yMax),
					ImVec2(CanvasPos.x + CanvasSize.x, yMax),
					GColor, 2.0f);
			}
		}
		else if (Type == EDistributionType::ConstantCurve)
		{
			// ConstantCurve: 단일 커브 (R 채널)
			if (Track.bShowX)
			{
				FInterpCurveFloat& Curve = Track.FloatCurve->ConstantCurve;
				if (Curve.Points.Num() == 0) return;

				const int NumSamples = 100;
				ImVec2 PrevPoint;

				for (int i = 0; i <= NumSamples; ++i)
				{
					float t = (float)i / NumSamples;
					float Time = FMath::Lerp(CurveState.ViewMinTime, CurveState.ViewMaxTime, t);
					float Value = Curve.Eval(Time);

					float x = CanvasPos.x + t * CanvasSize.x;
					float y = ValueToY(Value);

					ImVec2 Point(x, y);
					if (i > 0)
					{
						DrawList->AddLine(PrevPoint, Point, RColor, 2.0f);
					}
					PrevPoint = Point;
				}
			}
		}
		else if (Type == EDistributionType::UniformCurve)
		{
			// UniformCurve: Min 커브(R) + Max 커브(G)
			const int NumSamples = 100;

			// Min 커브 (R 채널)
			if (Track.bShowX)
			{
				FInterpCurveFloat& MinCurve = Track.FloatCurve->MinCurve;
				if (MinCurve.Points.Num() > 0)
				{
					ImVec2 PrevPoint;
					for (int i = 0; i <= NumSamples; ++i)
					{
						float t = (float)i / NumSamples;
						float Time = FMath::Lerp(CurveState.ViewMinTime, CurveState.ViewMaxTime, t);
						float Value = MinCurve.Eval(Time);

						float x = CanvasPos.x + t * CanvasSize.x;
						float y = ValueToY(Value);

						ImVec2 Point(x, y);
						if (i > 0)
						{
							DrawList->AddLine(PrevPoint, Point, RColor, 2.0f);
						}
						PrevPoint = Point;
					}
				}
			}

			// Max 커브 (G 채널)
			if (Track.bShowY)
			{
				FInterpCurveFloat& MaxCurve = Track.FloatCurve->MaxCurve;
				if (MaxCurve.Points.Num() > 0)
				{
					ImVec2 PrevPoint;
					for (int i = 0; i <= NumSamples; ++i)
					{
						float t = (float)i / NumSamples;
						float Time = FMath::Lerp(CurveState.ViewMinTime, CurveState.ViewMaxTime, t);
						float Value = MaxCurve.Eval(Time);

						float x = CanvasPos.x + t * CanvasSize.x;
						float y = ValueToY(Value);

						ImVec2 Point(x, y);
						if (i > 0)
						{
							DrawList->AddLine(PrevPoint, Point, GColor, 2.0f);
						}
						PrevPoint = Point;
					}
				}
			}
		}
	}
	else if (Track.VectorCurve)
	{
		// 채널별 색상
		ImU32 Colors[3] = {
			IM_COL32(255, 80, 80, 255),   // X - 빨강
			IM_COL32(80, 255, 80, 255),   // Y - 초록
			IM_COL32(80, 80, 255, 255)    // Z - 파랑
		};
		bool ShowChannel[3] = { Track.bShowX, Track.bShowY, Track.bShowZ };

		EDistributionType Type = Track.VectorCurve->Type;

		if (Type == EDistributionType::Constant)
		{
			// Constant: 각 채널별 수평선
			FVector Value = Track.VectorCurve->ConstantValue;
			float Values[3] = { Value.X, Value.Y, Value.Z };

			for (int axis = 0; axis < 3; ++axis)
			{
				if (!ShowChannel[axis]) continue;
				float y = ValueToY(Values[axis]);
				DrawList->AddLine(
					ImVec2(CanvasPos.x, y),
					ImVec2(CanvasPos.x + CanvasSize.x, y),
					Colors[axis], 2.0f);
			}
		}
		else if (Type == EDistributionType::Uniform)
		{
			// Uniform: 각 채널별 min/max 수평선
			FVector MinVal = Track.VectorCurve->MinValue;
			FVector MaxVal = Track.VectorCurve->MaxValue;
			float MinValues[3] = { MinVal.X, MinVal.Y, MinVal.Z };
			float MaxValues[3] = { MaxVal.X, MaxVal.Y, MaxVal.Z };

			for (int axis = 0; axis < 3; ++axis)
			{
				if (!ShowChannel[axis]) continue;
				ImU32 LineColor = (Colors[axis] & 0x00FFFFFF) | 0xAA000000;
				float yMin = ValueToY(MinValues[axis]);
				float yMax = ValueToY(MaxValues[axis]);
				DrawList->AddLine(
					ImVec2(CanvasPos.x, yMin),
					ImVec2(CanvasPos.x + CanvasSize.x, yMin),
					LineColor, 1.5f);
				DrawList->AddLine(
					ImVec2(CanvasPos.x, yMax),
					ImVec2(CanvasPos.x + CanvasSize.x, yMax),
					LineColor, 1.5f);
			}
		}
		else if (Type == EDistributionType::ConstantCurve)
		{
			// ConstantCurve: 단일 커브
			FInterpCurveVector& Curve = Track.VectorCurve->ConstantCurve;
			if (Curve.Points.Num() == 0) return;

			for (int axis = 0; axis < 3; ++axis)
			{
				if (!ShowChannel[axis]) continue;

				const int NumSamples = 100;
				ImVec2 PrevPoint;

				for (int i = 0; i <= NumSamples; ++i)
				{
					float t = (float)i / NumSamples;
					float Time = FMath::Lerp(CurveState.ViewMinTime, CurveState.ViewMaxTime, t);
					FVector Value = Curve.Eval(Time);

					float AxisValue = (axis == 0) ? Value.X : (axis == 1) ? Value.Y : Value.Z;

					float x = CanvasPos.x + t * CanvasSize.x;
					float y = ValueToY(AxisValue);

					ImVec2 Point(x, y);
					if (i > 0)
					{
						DrawList->AddLine(PrevPoint, Point, Colors[axis], 2.0f);
					}
					PrevPoint = Point;
				}
			}
		}
		else if (Type == EDistributionType::UniformCurve)
		{
			// UniformCurve: Min 커브 + Max 커브 (각 축별)
			FInterpCurveVector& MinCurve = Track.VectorCurve->MinCurve;
			FInterpCurveVector& MaxCurve = Track.VectorCurve->MaxCurve;

			const int NumSamples = 100;

			for (int axis = 0; axis < 3; ++axis)
			{
				if (!ShowChannel[axis]) continue;

				// Min 커브 (실선)
				if (MinCurve.Points.Num() > 0)
				{
					ImVec2 PrevPoint;
					for (int i = 0; i <= NumSamples; ++i)
					{
						float t = (float)i / NumSamples;
						float Time = FMath::Lerp(CurveState.ViewMinTime, CurveState.ViewMaxTime, t);
						FVector Value = MinCurve.Eval(Time);

						float AxisValue = (axis == 0) ? Value.X : (axis == 1) ? Value.Y : Value.Z;

						float x = CanvasPos.x + t * CanvasSize.x;
						float y = ValueToY(AxisValue);

						ImVec2 Point(x, y);
						if (i > 0)
						{
							DrawList->AddLine(PrevPoint, Point, Colors[axis], 2.0f);
						}
						PrevPoint = Point;
					}
				}

				// Max 커브 (점선 스타일 - 약간 투명)
				if (MaxCurve.Points.Num() > 0)
				{
					ImU32 MaxColor = (Colors[axis] & 0x00FFFFFF) | 0x99000000;
					ImVec2 PrevPoint;
					for (int i = 0; i <= NumSamples; ++i)
					{
						float t = (float)i / NumSamples;
						float Time = FMath::Lerp(CurveState.ViewMinTime, CurveState.ViewMaxTime, t);
						FVector Value = MaxCurve.Eval(Time);

						float AxisValue = (axis == 0) ? Value.X : (axis == 1) ? Value.Y : Value.Z;

						float x = CanvasPos.x + t * CanvasSize.x;
						float y = ValueToY(AxisValue);

						ImVec2 Point(x, y);
						if (i > 0)
						{
							DrawList->AddLine(PrevPoint, Point, MaxColor, 1.5f);
						}
						PrevPoint = Point;
					}
				}
			}
		}
	}
}

void SCurveEditorWidget::RenderCurveGrid(ImDrawList* DrawList, ImVec2 CanvasPos, ImVec2 CanvasSize)
{
	ImU32 GridColor = IM_COL32(50, 50, 50, 255);
	ImU32 SubGridColor = IM_COL32(35, 35, 35, 255);
	ImU32 AxisColor = IM_COL32(80, 80, 80, 255);
	ImU32 TextColor = IM_COL32(150, 150, 150, 255);

	// "Nice number" 계산 헬퍼 - 1, 2, 5, 10, 20, 50... 패턴
	auto GetNiceStep = [](float Range, float TargetSteps) -> float
	{
		float RoughStep = Range / TargetSteps;
		float Magnitude = powf(10.0f, floorf(log10f(RoughStep)));
		float Residual = RoughStep / Magnitude;

		float NiceResidual;
		if (Residual <= 1.0f) NiceResidual = 1.0f;
		else if (Residual <= 2.0f) NiceResidual = 2.0f;
		else if (Residual <= 5.0f) NiceResidual = 5.0f;
		else NiceResidual = 10.0f;

		return NiceResidual * Magnitude;
	};

	// 라벨 포맷 결정 헬퍼
	auto GetLabelFormat = [](float Step) -> const char*
	{
		if (Step >= 1.0f) return "%.0f";
		else if (Step >= 0.1f) return "%.1f";
		else if (Step >= 0.01f) return "%.2f";
		else return "%.3f";
	};

	float TimeRange = CurveState.ViewMaxTime - CurveState.ViewMinTime;
	float ValueRange = CurveState.ViewMaxValue - CurveState.ViewMinValue;

	// 적응형 그리드 간격 계산 (약 4-8개의 라인이 보이도록)
	float TimeStep = GetNiceStep(TimeRange, 5.0f);
	float ValueStep = GetNiceStep(ValueRange, 5.0f);

	const char* TimeFmt = GetLabelFormat(TimeStep);
	const char* ValueFmt = GetLabelFormat(ValueStep);

	// 서브 그리드 (메인 그리드의 1/5)
	float TimeSubStep = TimeStep / 5.0f;
	float ValueSubStep = ValueStep / 5.0f;

	// 서브 그리드 - 수직선 (시간 축)
	float TimeStart = floorf(CurveState.ViewMinTime / TimeSubStep) * TimeSubStep;
	for (float t = TimeStart; t <= CurveState.ViewMaxTime; t += TimeSubStep)
	{
		float x = CanvasPos.x + ((t - CurveState.ViewMinTime) / TimeRange) * CanvasSize.x;
		if (x >= CanvasPos.x && x <= CanvasPos.x + CanvasSize.x)
		{
			DrawList->AddLine(ImVec2(x, CanvasPos.y), ImVec2(x, CanvasPos.y + CanvasSize.y), SubGridColor);
		}
	}

	// 서브 그리드 - 수평선 (값 축)
	float ValueStart = floorf(CurveState.ViewMinValue / ValueSubStep) * ValueSubStep;
	for (float v = ValueStart; v <= CurveState.ViewMaxValue; v += ValueSubStep)
	{
		float y = CanvasPos.y + CanvasSize.y - ((v - CurveState.ViewMinValue) / ValueRange) * CanvasSize.y;
		if (y >= CanvasPos.y && y <= CanvasPos.y + CanvasSize.y)
		{
			DrawList->AddLine(ImVec2(CanvasPos.x, y), ImVec2(CanvasPos.x + CanvasSize.x, y), SubGridColor);
		}
	}

	// 메인 그리드 - 수직선 + 시간 라벨 (X축)
	TimeStart = floorf(CurveState.ViewMinTime / TimeStep) * TimeStep;
	for (float t = TimeStart; t <= CurveState.ViewMaxTime; t += TimeStep)
	{
		float x = CanvasPos.x + ((t - CurveState.ViewMinTime) / TimeRange) * CanvasSize.x;
		if (x >= CanvasPos.x && x <= CanvasPos.x + CanvasSize.x)
		{
			DrawList->AddLine(ImVec2(x, CanvasPos.y), ImVec2(x, CanvasPos.y + CanvasSize.y), GridColor);

			// 시간 라벨 (하단)
			char TimeLabel[16];
			snprintf(TimeLabel, sizeof(TimeLabel), TimeFmt, t);
			DrawList->AddText(ImVec2(x - 12, CanvasPos.y + CanvasSize.y + 4), TextColor, TimeLabel);
		}
	}

	// 메인 그리드 - 수평선 + 값 라벨 (Y축)
	ValueStart = floorf(CurveState.ViewMinValue / ValueStep) * ValueStep;
	for (float v = ValueStart; v <= CurveState.ViewMaxValue; v += ValueStep)
	{
		float y = CanvasPos.y + CanvasSize.y - ((v - CurveState.ViewMinValue) / ValueRange) * CanvasSize.y;
		if (y >= CanvasPos.y && y <= CanvasPos.y + CanvasSize.y)
		{
			DrawList->AddLine(ImVec2(CanvasPos.x, y), ImVec2(CanvasPos.x + CanvasSize.x, y), GridColor);

			// 값 라벨 (좌측)
			char ValueLabel[16];
			snprintf(ValueLabel, sizeof(ValueLabel), ValueFmt, v);
			DrawList->AddText(ImVec2(CanvasPos.x - 32, y - 6), TextColor, ValueLabel);
		}
	}

	// 0 축 강조 (값이 0인 수평선)
	if (CurveState.ViewMinValue < 0 && CurveState.ViewMaxValue > 0)
	{
		float zeroY = CanvasPos.y + CanvasSize.y -
			((0 - CurveState.ViewMinValue) / ValueRange) * CanvasSize.y;
		DrawList->AddLine(ImVec2(CanvasPos.x, zeroY), ImVec2(CanvasPos.x + CanvasSize.x, zeroY), AxisColor, 2.0f);
	}

	// 0 축 강조 (시간이 0인 수직선)
	if (CurveState.ViewMinTime < 0 && CurveState.ViewMaxTime > 0)
	{
		float zeroX = CanvasPos.x +
			((0 - CurveState.ViewMinTime) / TimeRange) * CanvasSize.x;
		DrawList->AddLine(ImVec2(zeroX, CanvasPos.y), ImVec2(zeroX, CanvasPos.y + CanvasSize.y), AxisColor, 2.0f);
	}
}

void SCurveEditorWidget::RenderCurveKeys(ImDrawList* DrawList, ImVec2 CanvasPos, ImVec2 CanvasSize)
{
	// 모든 visible 트랙의 키 렌더링
	for (int32 TrackIdx = 0; TrackIdx < CurveState.Tracks.Num(); ++TrackIdx)
	{
		FCurveTrack& Track = CurveState.Tracks[TrackIdx];
		if (!Track.bVisible)
		{
			continue;
		}

		bool bIsSelectedTrack = (CurveState.SelectedTrackIndex == TrackIdx);

		if (Track.FloatCurve)
		{
			// 람다: 단일 커브의 키 렌더링
			auto RenderFloatCurveKeys = [&](FInterpCurveFloat& Curve, ImU32 BaseColor, int32 CurveIndex)
			{
				for (int32 i = 0; i < Curve.Points.Num(); ++i)
				{
					FInterpCurvePointFloat& Point = Curve.Points[i];

					// 화면 좌표
					float x = CanvasPos.x +
						((Point.InVal - CurveState.ViewMinTime) /
						 (CurveState.ViewMaxTime - CurveState.ViewMinTime)) * CanvasSize.x;
					float y = CanvasPos.y + CanvasSize.y -
						((Point.OutVal - CurveState.ViewMinValue) /
						 (CurveState.ViewMaxValue - CurveState.ViewMinValue)) * CanvasSize.y;

					// 클램핑
					x = FMath::Clamp(x, CanvasPos.x, CanvasPos.x + CanvasSize.x);
					y = FMath::Clamp(y, CanvasPos.y, CanvasPos.y + CanvasSize.y);

					// 키 사각형
					float KeySize = 8.0f;
					// 선택 상태: 선택된 트랙의 선택된 커브의 선택된 키면 노랑
					bool bIsSelected = bIsSelectedTrack &&
						(CurveState.SelectedAxis == CurveIndex) &&
						(i == CurveState.SelectedKeyIndex);
					ImU32 KeyColor = bIsSelected ? IM_COL32(255, 255, 0, 255) : BaseColor;

					DrawList->AddRectFilled(
						ImVec2(x - KeySize / 2, y - KeySize / 2),
						ImVec2(x + KeySize / 2, y + KeySize / 2),
						KeyColor);
					DrawList->AddRect(
						ImVec2(x - KeySize / 2, y - KeySize / 2),
						ImVec2(x + KeySize / 2, y + KeySize / 2),
						IM_COL32(0, 0, 0, 255));

					// 탄젠트 핸들 (선택된 키만)
					if (bIsSelected)
					{
						RenderTangentHandles(DrawList, Point, x, y, CanvasSize, CurveIndex);
					}
				}
			};

			// 람다: 단일 값(비커브)의 키 렌더링 (다이아몬드 형태)
			auto RenderFloatConstantKey = [&](float Value, ImU32 BaseColor, int32 CurveIndex)
			{
				// 시간 중앙에 표시
				float CenterTime = (CurveState.ViewMinTime + CurveState.ViewMaxTime) * 0.5f;
				float x = CanvasPos.x +
					((CenterTime - CurveState.ViewMinTime) /
					 (CurveState.ViewMaxTime - CurveState.ViewMinTime)) * CanvasSize.x;
				float y = CanvasPos.y + CanvasSize.y -
					((Value - CurveState.ViewMinValue) /
					 (CurveState.ViewMaxValue - CurveState.ViewMinValue)) * CanvasSize.y;

				// 클램핑
				y = FMath::Clamp(y, CanvasPos.y, CanvasPos.y + CanvasSize.y);

				// 다이아몬드 형태로 비커브 키 표시
				float KeySize = 5.0f;
				bool bIsSelected = bIsSelectedTrack &&
					(CurveState.SelectedAxis == CurveIndex) &&
					(CurveState.SelectedKeyIndex == 0);  // Constant는 항상 인덱스 0
				ImU32 KeyColor = bIsSelected ? IM_COL32(255, 255, 0, 255) : BaseColor;

				// 다이아몬드 꼭짓점
				ImVec2 DiamondPoints[4] = {
					ImVec2(x, y - KeySize),          // 상단
					ImVec2(x + KeySize, y),          // 우측
					ImVec2(x, y + KeySize),          // 하단
					ImVec2(x - KeySize, y)           // 좌측
				};
				DrawList->AddConvexPolyFilled(DiamondPoints, 4, KeyColor);
				DrawList->AddPolyline(DiamondPoints, 4, IM_COL32(0, 0, 0, 255), true, 1.5f);
			};

			if (Track.FloatCurve->Type == EDistributionType::Constant)
			{
				// Constant: 단일 값 (흰색 다이아몬드)
				if (Track.bShowX)
				{
					RenderFloatConstantKey(Track.FloatCurve->ConstantValue, IM_COL32(255, 255, 255, 255), 0);
				}
			}
			else if (Track.FloatCurve->Type == EDistributionType::Uniform)
			{
				// Uniform: MinValue(빨강), MaxValue(초록) 다이아몬드
				if (Track.bShowX)
				{
					RenderFloatConstantKey(Track.FloatCurve->MinValue, IM_COL32(255, 100, 100, 255), 0);
				}
				if (Track.bShowY)
				{
					RenderFloatConstantKey(Track.FloatCurve->MaxValue, IM_COL32(100, 255, 100, 255), 1);
				}
			}
			else if (Track.FloatCurve->Type == EDistributionType::ConstantCurve)
			{
				// ConstantCurve: 단일 커브 (흰색)
				if (Track.bShowX)
				{
					RenderFloatCurveKeys(Track.FloatCurve->ConstantCurve, IM_COL32(255, 255, 255, 255), 0);
				}
			}
			else if (Track.FloatCurve->Type == EDistributionType::UniformCurve)
			{
				// UniformCurve: MinCurve(빨강), MaxCurve(초록)
				if (Track.bShowX)
				{
					RenderFloatCurveKeys(Track.FloatCurve->MinCurve, IM_COL32(255, 100, 100, 255), 0);
				}
				if (Track.bShowY)
				{
					RenderFloatCurveKeys(Track.FloatCurve->MaxCurve, IM_COL32(100, 255, 100, 255), 1);
				}
			}
		}
		else if (Track.VectorCurve)
		{
			// 색상: X=빨강, Y=초록, Z=파랑
			ImU32 Colors[3] = {
				IM_COL32(255, 80, 80, 255),
				IM_COL32(80, 255, 80, 255),
				IM_COL32(80, 80, 255, 255)
			};
			// Uniform용 어두운 색상 (Min 값)
			ImU32 DarkColors[3] = {
				IM_COL32(180, 50, 50, 255),
				IM_COL32(50, 180, 50, 255),
				IM_COL32(50, 50, 180, 255)
			};
			bool ShowChannel[3] = { Track.bShowX, Track.bShowY, Track.bShowZ };

			// 람다: 단일 VectorCurve의 키 렌더링
			auto RenderVectorCurveKeys = [&](FInterpCurveVector& Curve, ImU32* ChannelColors, bool bIsMinCurve)
			{
				for (int32 i = 0; i < Curve.Points.Num(); ++i)
				{
					FInterpCurvePointVector& Point = Curve.Points[i];

					float x = CanvasPos.x +
						((Point.InVal - CurveState.ViewMinTime) /
						 (CurveState.ViewMaxTime - CurveState.ViewMinTime)) * CanvasSize.x;

					// 각 축별로 키 렌더링 (활성화된 채널만)
					for (int axis = 0; axis < 3; ++axis)
					{
						if (!ShowChannel[axis]) continue;

						int32 CurveIndex = bIsMinCurve ? (3 + axis) : axis;
						float AxisValue = (axis == 0) ? Point.OutVal.X : (axis == 1) ? Point.OutVal.Y : Point.OutVal.Z;
						float y = CanvasPos.y + CanvasSize.y -
							((AxisValue - CurveState.ViewMinValue) /
							 (CurveState.ViewMaxValue - CurveState.ViewMinValue)) * CanvasSize.y;

						y = FMath::Clamp(y, CanvasPos.y, CanvasPos.y + CanvasSize.y);

						// 선택 상태 확인
						bool bIsSelected = bIsSelectedTrack &&
							(CurveState.SelectedAxis == CurveIndex) &&
							(i == CurveState.SelectedKeyIndex);

						float KeySize = 6.0f;
						ImU32 KeyColor = bIsSelected ? IM_COL32(255, 255, 0, 255) : ChannelColors[axis];
						DrawList->AddRectFilled(
							ImVec2(x - KeySize / 2, y - KeySize / 2),
							ImVec2(x + KeySize / 2, y + KeySize / 2),
							KeyColor);
						DrawList->AddRect(
							ImVec2(x - KeySize / 2, y - KeySize / 2),
							ImVec2(x + KeySize / 2, y + KeySize / 2),
							IM_COL32(0, 0, 0, 255));

						// 탄젠트 핸들 (선택된 키만)
						if (bIsSelected)
						{
							RenderTangentHandlesVector(DrawList, Point, axis, x, y, CanvasSize, CurveIndex);
						}
					}
				}
			};

			// 람다: 단일 Vector 값(비커브)의 키 렌더링 (다이아몬드 형태)
			auto RenderVectorConstantKey = [&](const FVector& Value, ImU32* ChannelColors, bool bIsMinValue)
			{
				// 시간 중앙에 표시
				float CenterTime = (CurveState.ViewMinTime + CurveState.ViewMaxTime) * 0.5f;
				float x = CanvasPos.x +
					((CenterTime - CurveState.ViewMinTime) /
					 (CurveState.ViewMaxTime - CurveState.ViewMinTime)) * CanvasSize.x;

				// 각 축별로 키 렌더링 (활성화된 채널만)
				for (int axis = 0; axis < 3; ++axis)
				{
					if (!ShowChannel[axis]) continue;

					float AxisValue = (axis == 0) ? Value.X : (axis == 1) ? Value.Y : Value.Z;
					float y = CanvasPos.y + CanvasSize.y -
						((AxisValue - CurveState.ViewMinValue) /
						 (CurveState.ViewMaxValue - CurveState.ViewMinValue)) * CanvasSize.y;

					y = FMath::Clamp(y, CanvasPos.y, CanvasPos.y + CanvasSize.y);

					// 선택 상태 확인
					int32 ExpectedAxis = bIsMinValue ? (3 + axis) : axis;
					bool bIsSelected = bIsSelectedTrack &&
						(CurveState.SelectedAxis == ExpectedAxis) &&
						(CurveState.SelectedKeyIndex == 0);  // Constant는 항상 인덱스 0
					ImU32 KeyColor = bIsSelected ? IM_COL32(255, 255, 0, 255) : ChannelColors[axis];

					// 다이아몬드 형태로 비커브 키 표시
					float KeySize = 5.0f;
					ImVec2 DiamondPoints[4] = {
						ImVec2(x, y - KeySize),          // 상단
						ImVec2(x + KeySize, y),          // 우측
						ImVec2(x, y + KeySize),          // 하단
						ImVec2(x - KeySize, y)           // 좌측
					};
					DrawList->AddConvexPolyFilled(DiamondPoints, 4, KeyColor);
					DrawList->AddPolyline(DiamondPoints, 4, IM_COL32(0, 0, 0, 255), true, 1.5f);
				}
			};

			if (Track.VectorCurve->Type == EDistributionType::Constant)
			{
				// Constant: 단일 벡터 값 (다이아몬드)
				RenderVectorConstantKey(Track.VectorCurve->ConstantValue, Colors, false);
			}
			else if (Track.VectorCurve->Type == EDistributionType::Uniform)
			{
				// Uniform: MinValue(어두운색), MaxValue(밝은색) 다이아몬드
				RenderVectorConstantKey(Track.VectorCurve->MinValue, DarkColors, true);
				RenderVectorConstantKey(Track.VectorCurve->MaxValue, Colors, false);
			}
			else if (Track.VectorCurve->Type == EDistributionType::ParticleParameter)
			{
				// ParticleParameter: DefaultValue (보라색 다이아몬드)
				ImU32 ParamColors[3] = {
					IM_COL32(200, 100, 200, 255),  // 보라-빨강
					IM_COL32(100, 200, 100, 255),  // 보라-초록
					IM_COL32(100, 100, 200, 255)   // 보라-파랑
				};
				RenderVectorConstantKey(Track.VectorCurve->ParameterDefaultValue, ParamColors, false);
			}
			else if (Track.VectorCurve->Type == EDistributionType::ConstantCurve)
			{
				// ConstantCurve: 단일 커브
				RenderVectorCurveKeys(Track.VectorCurve->ConstantCurve, Colors, false);
			}
			else if (Track.VectorCurve->Type == EDistributionType::UniformCurve)
			{
				// UniformCurve: MinCurve(어두운색), MaxCurve(밝은색)
				RenderVectorCurveKeys(Track.VectorCurve->MinCurve, DarkColors, true);
				RenderVectorCurveKeys(Track.VectorCurve->MaxCurve, Colors, false);
			}
		}
	}
}

void SCurveEditorWidget::RenderTangentHandles(ImDrawList* DrawList, FInterpCurvePointFloat& Point,
	float KeyX, float KeyY, ImVec2 CanvasSize, int32 CurveIndex)
{
	float HandleLength = 40.0f;
	ImU32 HandleColor = IM_COL32(255, 255, 255, 255);
	ImU32 SelectedColor = IM_COL32(255, 255, 0, 255);  // 선택된 핸들은 노랑

	float TimeScale = CanvasSize.x / (CurveState.ViewMaxTime - CurveState.ViewMinTime);
	float ValueScale = CanvasSize.y / (CurveState.ViewMaxValue - CurveState.ViewMinValue);

	// 도착 탄젠트 (왼쪽) - SelectedTangentHandle == 1
	// Arrive 핸들은 핸들→키 방향이 접선 방향이므로 Y를 반대로 (KeyY + ArriveDy)
	float ArriveDx = -HandleLength;
	float ArriveDy = Point.ArriveTangent * HandleLength * (ValueScale / TimeScale);
	bool bArriveSelected = (CurveState.SelectedTangentHandle == 1) &&
		(CurveState.SelectedAxis == CurveIndex);
	DrawList->AddLine(ImVec2(KeyX, KeyY),
		ImVec2(KeyX + ArriveDx, KeyY + ArriveDy), HandleColor, 1.5f);
	DrawList->AddCircleFilled(ImVec2(KeyX + ArriveDx, KeyY + ArriveDy), 5.0f,
		bArriveSelected ? SelectedColor : HandleColor);

	// 출발 탄젠트 (오른쪽) - SelectedTangentHandle == 2
	float LeaveDx = HandleLength;
	float LeaveDy = Point.LeaveTangent * HandleLength * (ValueScale / TimeScale);
	bool bLeaveSelected = (CurveState.SelectedTangentHandle == 2) &&
		(CurveState.SelectedAxis == CurveIndex);
	DrawList->AddLine(ImVec2(KeyX, KeyY),
		ImVec2(KeyX + LeaveDx, KeyY - LeaveDy), HandleColor, 1.5f);
	DrawList->AddCircleFilled(ImVec2(KeyX + LeaveDx, KeyY - LeaveDy), 5.0f,
		bLeaveSelected ? SelectedColor : HandleColor);
}

void SCurveEditorWidget::RenderTangentHandlesVector(ImDrawList* DrawList, FInterpCurvePointVector& Point,
	int32 Axis, float KeyX, float KeyY, ImVec2 CanvasSize, int32 CurveIndex)
{
	float HandleLength = 40.0f;
	ImU32 HandleColor = IM_COL32(255, 255, 255, 255);
	ImU32 SelectedColor = IM_COL32(255, 255, 0, 255);

	float TimeScale = CanvasSize.x / (CurveState.ViewMaxTime - CurveState.ViewMinTime);
	float ValueScale = CanvasSize.y / (CurveState.ViewMaxValue - CurveState.ViewMinValue);

	// 해당 축의 탄젠트 가져오기
	float ArriveTangent = (Axis == 0) ? Point.ArriveTangent.X :
						  (Axis == 1) ? Point.ArriveTangent.Y : Point.ArriveTangent.Z;
	float LeaveTangent = (Axis == 0) ? Point.LeaveTangent.X :
						 (Axis == 1) ? Point.LeaveTangent.Y : Point.LeaveTangent.Z;

	// 도착 탄젠트 (왼쪽)
	// Arrive 핸들은 핸들→키 방향이 접선 방향이므로 Y를 반대로 (KeyY + ArriveDy)
	float ArriveDx = -HandleLength;
	float ArriveDy = ArriveTangent * HandleLength * (ValueScale / TimeScale);
	bool bArriveSelected = (CurveState.SelectedTangentHandle == 1) &&
		(CurveState.SelectedAxis == CurveIndex);
	DrawList->AddLine(ImVec2(KeyX, KeyY),
		ImVec2(KeyX + ArriveDx, KeyY + ArriveDy), HandleColor, 1.5f);
	DrawList->AddCircleFilled(ImVec2(KeyX + ArriveDx, KeyY + ArriveDy), 5.0f,
		bArriveSelected ? SelectedColor : HandleColor);

	// 출발 탄젠트 (오른쪽)
	float LeaveDx = HandleLength;
	float LeaveDy = LeaveTangent * HandleLength * (ValueScale / TimeScale);
	bool bLeaveSelected = (CurveState.SelectedTangentHandle == 2) &&
		(CurveState.SelectedAxis == CurveIndex);
	DrawList->AddLine(ImVec2(KeyX, KeyY),
		ImVec2(KeyX + LeaveDx, KeyY - LeaveDy), HandleColor, 1.5f);
	DrawList->AddCircleFilled(ImVec2(KeyX + LeaveDx, KeyY - LeaveDy), 5.0f,
		bLeaveSelected ? SelectedColor : HandleColor);
}

void SCurveEditorWidget::HandleCurveInteraction(ImVec2 CanvasPos, ImVec2 CanvasSize)
{
	if (!ImGui::IsItemHovered()) return;

	ImVec2 MousePos = ImGui::GetMousePos();

	// 탄젠트 핸들 또는 키 선택 - 모든 visible 트랙에서 검사
	if (ImGui::IsMouseClicked(0))
	{
		bool bKeyFound = false;
		bool bTangentHandleFound = false;
		float HandleLength = 40.0f;
		float TimeScale = CanvasSize.x / (CurveState.ViewMaxTime - CurveState.ViewMinTime);
		float ValueScale = CanvasSize.y / (CurveState.ViewMaxValue - CurveState.ViewMinValue);

		// 먼저 현재 선택된 키의 탄젠트 핸들 검사 (선택된 키가 있을 때만)
		if (CurveState.SelectedKeyIndex >= 0 && CurveState.SelectedAxis >= 0)
		{
			FCurveTrack* SelectedTrack = CurveState.GetSelectedTrack();
			if (SelectedTrack)
			{
				// Float 커브 탄젠트 핸들 검사
				if (SelectedTrack->FloatCurve &&
					(SelectedTrack->FloatCurve->Type == EDistributionType::ConstantCurve ||
					 SelectedTrack->FloatCurve->Type == EDistributionType::UniformCurve))
				{
					FInterpCurveFloat* CurvePtr = nullptr;
					if (SelectedTrack->FloatCurve->Type == EDistributionType::ConstantCurve)
					{
						CurvePtr = &SelectedTrack->FloatCurve->ConstantCurve;
					}
					else
					{
						CurvePtr = (CurveState.SelectedAxis == 0)
							? &SelectedTrack->FloatCurve->MinCurve
							: &SelectedTrack->FloatCurve->MaxCurve;
					}

					if (CurvePtr && CurveState.SelectedKeyIndex < CurvePtr->Points.Num())
					{
						FInterpCurvePointFloat& Point = CurvePtr->Points[CurveState.SelectedKeyIndex];
						float KeyX = CanvasPos.x +
							((Point.InVal - CurveState.ViewMinTime) /
							 (CurveState.ViewMaxTime - CurveState.ViewMinTime)) * CanvasSize.x;
						float KeyY = CanvasPos.y + CanvasSize.y -
							((Point.OutVal - CurveState.ViewMinValue) /
							 (CurveState.ViewMaxValue - CurveState.ViewMinValue)) * CanvasSize.y;

						// Arrive 탄젠트 핸들 (왼쪽)
						// Arrive 핸들은 핸들→키 방향이 접선 방향이므로 Y를 반대로 (KeyY + ArriveDy)
						float ArriveDx = -HandleLength;
						float ArriveDy = Point.ArriveTangent * HandleLength * (ValueScale / TimeScale);
						float ArriveX = KeyX + ArriveDx;
						float ArriveY = KeyY + ArriveDy;
						float distArrive = FMath::Sqrt((MousePos.x - ArriveX) * (MousePos.x - ArriveX) +
							(MousePos.y - ArriveY) * (MousePos.y - ArriveY));
						if (distArrive < 8.0f)
						{
							CurveState.SelectedTangentHandle = 1;  // Arrive
							bTangentHandleFound = true;
						}

						// Leave 탄젠트 핸들 (오른쪽)
						if (!bTangentHandleFound)
						{
							float LeaveDx = HandleLength;
							float LeaveDy = Point.LeaveTangent * HandleLength * (ValueScale / TimeScale);
							float LeaveX = KeyX + LeaveDx;
							float LeaveY = KeyY - LeaveDy;
							float distLeave = FMath::Sqrt((MousePos.x - LeaveX) * (MousePos.x - LeaveX) +
								(MousePos.y - LeaveY) * (MousePos.y - LeaveY));
							if (distLeave < 8.0f)
							{
								CurveState.SelectedTangentHandle = 2;  // Leave
								bTangentHandleFound = true;
							}
						}
					}
				}
				// Vector 커브 탄젠트 핸들 검사
				else if (SelectedTrack->VectorCurve &&
					(SelectedTrack->VectorCurve->Type == EDistributionType::ConstantCurve ||
					 SelectedTrack->VectorCurve->Type == EDistributionType::UniformCurve))
				{
					FInterpCurveVector* CurvePtr = nullptr;
					int32 AxisIndex = CurveState.SelectedAxis % 3;
					bool bIsMinCurve = (CurveState.SelectedAxis >= 3);

					if (SelectedTrack->VectorCurve->Type == EDistributionType::ConstantCurve)
					{
						CurvePtr = &SelectedTrack->VectorCurve->ConstantCurve;
					}
					else
					{
						CurvePtr = bIsMinCurve
							? &SelectedTrack->VectorCurve->MinCurve
							: &SelectedTrack->VectorCurve->MaxCurve;
					}

					if (CurvePtr && CurveState.SelectedKeyIndex < CurvePtr->Points.Num())
					{
						FInterpCurvePointVector& Point = CurvePtr->Points[CurveState.SelectedKeyIndex];
						float KeyX = CanvasPos.x +
							((Point.InVal - CurveState.ViewMinTime) /
							 (CurveState.ViewMaxTime - CurveState.ViewMinTime)) * CanvasSize.x;
						float AxisValue = (AxisIndex == 0) ? Point.OutVal.X :
							(AxisIndex == 1) ? Point.OutVal.Y : Point.OutVal.Z;
						float KeyY = CanvasPos.y + CanvasSize.y -
							((AxisValue - CurveState.ViewMinValue) /
							 (CurveState.ViewMaxValue - CurveState.ViewMinValue)) * CanvasSize.y;

						float ArriveTangent = (AxisIndex == 0) ? Point.ArriveTangent.X :
							(AxisIndex == 1) ? Point.ArriveTangent.Y : Point.ArriveTangent.Z;
						float LeaveTangent = (AxisIndex == 0) ? Point.LeaveTangent.X :
							(AxisIndex == 1) ? Point.LeaveTangent.Y : Point.LeaveTangent.Z;

						// Arrive 탄젠트 핸들
						// Arrive 핸들은 핸들→키 방향이 접선 방향이므로 Y를 반대로 (KeyY + ArriveDy)
						float ArriveDx = -HandleLength;
						float ArriveDy = ArriveTangent * HandleLength * (ValueScale / TimeScale);
						float ArriveX = KeyX + ArriveDx;
						float ArriveY = KeyY + ArriveDy;
						float distArrive = FMath::Sqrt((MousePos.x - ArriveX) * (MousePos.x - ArriveX) +
							(MousePos.y - ArriveY) * (MousePos.y - ArriveY));
						if (distArrive < 8.0f)
						{
							CurveState.SelectedTangentHandle = 1;
							bTangentHandleFound = true;
						}

						// Leave 탄젠트 핸들
						if (!bTangentHandleFound)
						{
							float LeaveDx = HandleLength;
							float LeaveDy = LeaveTangent * HandleLength * (ValueScale / TimeScale);
							float LeaveX = KeyX + LeaveDx;
							float LeaveY = KeyY - LeaveDy;
							float distLeave = FMath::Sqrt((MousePos.x - LeaveX) * (MousePos.x - LeaveX) +
								(MousePos.y - LeaveY) * (MousePos.y - LeaveY));
							if (distLeave < 8.0f)
							{
								CurveState.SelectedTangentHandle = 2;
								bTangentHandleFound = true;
							}
						}
					}
				}
			}
		}

		// 탄젠트 핸들을 찾지 못했으면 키 선택 검사
		if (!bTangentHandleFound)
		{
			CurveState.SelectedTangentHandle = 0;  // 탄젠트 핸들 선택 해제
		}

		// 람다: Float 커브에서 키 선택 검사
		auto CheckFloatCurveKey = [&](FInterpCurveFloat& Curve, int32 CurveIndex) -> bool
		{
			for (int32 i = 0; i < Curve.Points.Num(); ++i)
			{
				FInterpCurvePointFloat& Point = Curve.Points[i];

				float x = CanvasPos.x +
					((Point.InVal - CurveState.ViewMinTime) /
					 (CurveState.ViewMaxTime - CurveState.ViewMinTime)) * CanvasSize.x;
				float y = CanvasPos.y + CanvasSize.y -
					((Point.OutVal - CurveState.ViewMinValue) /
					 (CurveState.ViewMaxValue - CurveState.ViewMinValue)) * CanvasSize.y;

				float dist = FMath::Sqrt((MousePos.x - x) * (MousePos.x - x) + (MousePos.y - y) * (MousePos.y - y));
				if (dist < 10.0f)
				{
					CurveState.SelectedKeyIndex = i;
					CurveState.SelectedAxis = CurveIndex;
					return true;
				}
			}
			return false;
		};

		// 람다: Float 단일 값(비커브)에서 키 선택 검사
		auto CheckFloatConstantKey = [&](float Value, int32 CurveIndex) -> bool
		{
			// 시간 중앙에 표시
			float CenterTime = (CurveState.ViewMinTime + CurveState.ViewMaxTime) * 0.5f;
			float x = CanvasPos.x +
				((CenterTime - CurveState.ViewMinTime) /
				 (CurveState.ViewMaxTime - CurveState.ViewMinTime)) * CanvasSize.x;
			float y = CanvasPos.y + CanvasSize.y -
				((Value - CurveState.ViewMinValue) /
				 (CurveState.ViewMaxValue - CurveState.ViewMinValue)) * CanvasSize.y;

			float dist = FMath::Sqrt((MousePos.x - x) * (MousePos.x - x) + (MousePos.y - y) * (MousePos.y - y));
			if (dist < 12.0f)  // 다이아몬드가 약간 더 크므로 범위 확대
			{
				CurveState.SelectedKeyIndex = 0;  // Constant는 항상 인덱스 0
				CurveState.SelectedAxis = CurveIndex;
				return true;
			}
			return false;
		};

		// 람다: Vector 커브에서 키 선택 검사
		auto CheckVectorCurveKey = [&](FInterpCurveVector& Curve, bool ShowChannel[3], bool bIsMinCurve) -> bool
		{
			for (int32 i = 0; i < Curve.Points.Num(); ++i)
			{
				FInterpCurvePointVector& Point = Curve.Points[i];

				float x = CanvasPos.x +
					((Point.InVal - CurveState.ViewMinTime) /
					 (CurveState.ViewMaxTime - CurveState.ViewMinTime)) * CanvasSize.x;

				for (int axis = 0; axis < 3; ++axis)
				{
					if (!ShowChannel[axis]) continue;

					float AxisValue = (axis == 0) ? Point.OutVal.X : (axis == 1) ? Point.OutVal.Y : Point.OutVal.Z;
					float y = CanvasPos.y + CanvasSize.y -
						((AxisValue - CurveState.ViewMinValue) /
						 (CurveState.ViewMaxValue - CurveState.ViewMinValue)) * CanvasSize.y;

					float dist = FMath::Sqrt((MousePos.x - x) * (MousePos.x - x) + (MousePos.y - y) * (MousePos.y - y));
					if (dist < 10.0f)
					{
						CurveState.SelectedKeyIndex = i;
						CurveState.SelectedAxis = bIsMinCurve ? (3 + axis) : axis;
						return true;
					}
				}
			}
			return false;
		};

		// 람다: Vector 단일 값(비커브)에서 키 선택 검사
		auto CheckVectorConstantKey = [&](const FVector& Value, bool ShowChannel[3], bool bIsMinValue) -> bool
		{
			// 시간 중앙에 표시
			float CenterTime = (CurveState.ViewMinTime + CurveState.ViewMaxTime) * 0.5f;
			float x = CanvasPos.x +
				((CenterTime - CurveState.ViewMinTime) /
				 (CurveState.ViewMaxTime - CurveState.ViewMinTime)) * CanvasSize.x;

			for (int axis = 0; axis < 3; ++axis)
			{
				if (!ShowChannel[axis]) continue;

				float AxisValue = (axis == 0) ? Value.X : (axis == 1) ? Value.Y : Value.Z;
				float y = CanvasPos.y + CanvasSize.y -
					((AxisValue - CurveState.ViewMinValue) /
					 (CurveState.ViewMaxValue - CurveState.ViewMinValue)) * CanvasSize.y;

				float dist = FMath::Sqrt((MousePos.x - x) * (MousePos.x - x) + (MousePos.y - y) * (MousePos.y - y));
				if (dist < 10.0f)
				{
					CurveState.SelectedKeyIndex = 0;  // Constant는 항상 인덱스 0
					CurveState.SelectedAxis = bIsMinValue ? (3 + axis) : axis;
					return true;
				}
			}
			return false;
		};

		// 모든 visible 트랙 검사 (탄젠트 핸들을 찾지 못했을 때만)
		for (int32 TrackIdx = 0; TrackIdx < CurveState.Tracks.Num() && !bKeyFound && !bTangentHandleFound; ++TrackIdx)
		{
			FCurveTrack& Track = CurveState.Tracks[TrackIdx];
			if (!Track.bVisible) continue;

			if (Track.FloatCurve)
			{
				if (Track.FloatCurve->Type == EDistributionType::Constant)
				{
					// Constant: 단일 값
					if (Track.bShowX && CheckFloatConstantKey(Track.FloatCurve->ConstantValue, 0))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
				}
				else if (Track.FloatCurve->Type == EDistributionType::Uniform)
				{
					// Uniform: Min/Max 값
					if (Track.bShowX && CheckFloatConstantKey(Track.FloatCurve->MinValue, 0))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
					else if (Track.bShowY && CheckFloatConstantKey(Track.FloatCurve->MaxValue, 1))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
				}
				else if (Track.FloatCurve->Type == EDistributionType::ConstantCurve)
				{
					if (Track.bShowX && CheckFloatCurveKey(Track.FloatCurve->ConstantCurve, 0))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
				}
				else if (Track.FloatCurve->Type == EDistributionType::UniformCurve)
				{
					if (Track.bShowX && CheckFloatCurveKey(Track.FloatCurve->MinCurve, 0))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
					else if (Track.bShowY && CheckFloatCurveKey(Track.FloatCurve->MaxCurve, 1))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
				}
			}
			else if (Track.VectorCurve)
			{
				bool ShowChannel[3] = { Track.bShowX, Track.bShowY, Track.bShowZ };

				if (Track.VectorCurve->Type == EDistributionType::Constant)
				{
					// Constant: 단일 벡터 값
					if (CheckVectorConstantKey(Track.VectorCurve->ConstantValue, ShowChannel, false))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
				}
				else if (Track.VectorCurve->Type == EDistributionType::Uniform)
				{
					// Uniform: Min/Max 벡터 값
					if (CheckVectorConstantKey(Track.VectorCurve->MaxValue, ShowChannel, false))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
					else if (CheckVectorConstantKey(Track.VectorCurve->MinValue, ShowChannel, true))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
				}
				else if (Track.VectorCurve->Type == EDistributionType::ParticleParameter)
				{
					// ParticleParameter: Default 벡터 값
					if (CheckVectorConstantKey(Track.VectorCurve->ParameterDefaultValue, ShowChannel, false))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
				}
				else if (Track.VectorCurve->Type == EDistributionType::ConstantCurve)
				{
					if (CheckVectorCurveKey(Track.VectorCurve->ConstantCurve, ShowChannel, false))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
				}
				else if (Track.VectorCurve->Type == EDistributionType::UniformCurve)
				{
					if (CheckVectorCurveKey(Track.VectorCurve->MaxCurve, ShowChannel, false))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
					else if (CheckVectorCurveKey(Track.VectorCurve->MinCurve, ShowChannel, true))
					{
						CurveState.SelectedTrackIndex = TrackIdx;
						bKeyFound = true;
					}
				}
			}
		}

		// 키를 못 찾았고 탄젠트 핸들도 못 찾았으면 선택 해제
		if (!bKeyFound && !bTangentHandleFound)
		{
			CurveState.SelectedKeyIndex = -1;
			CurveState.SelectedAxis = -1;
			CurveState.SelectedTangentHandle = 0;
		}
	}

	// 선택된 트랙 가져오기 (드래그용)
	FCurveTrack* SelectedTrack = CurveState.GetSelectedTrack();

	// 키 드래그 (Float) - 탄젠트 핸들이 선택되지 않았을 때만
	if (ImGui::IsMouseDragging(0) && CurveState.SelectedKeyIndex >= 0 &&
		CurveState.SelectedAxis >= 0 && CurveState.SelectedTangentHandle == 0 &&
		SelectedTrack && SelectedTrack->FloatCurve)
	{
		// 마우스 위치를 값으로 변환 (Y축만 사용)
		float v = 1.0f - (MousePos.y - CanvasPos.y) / CanvasSize.y;
		float NewValue = FMath::Lerp(CurveState.ViewMinValue, CurveState.ViewMaxValue, v);

		if (SelectedTrack->FloatCurve->Type == EDistributionType::Constant)
		{
			// Constant: 단일 값 직접 수정 (Y축만)
			SelectedTrack->FloatCurve->ConstantValue = NewValue;
			if (EditorState) EditorState->bIsDirty = true;
		}
		else if (SelectedTrack->FloatCurve->Type == EDistributionType::Uniform)
		{
			// Uniform: Min/Max 값 직접 수정 (Y축만)
			if (CurveState.SelectedAxis == 0)
			{
				SelectedTrack->FloatCurve->MinValue = NewValue;
			}
			else
			{
				SelectedTrack->FloatCurve->MaxValue = NewValue;
			}
			if (EditorState) EditorState->bIsDirty = true;
		}
		else if (SelectedTrack->FloatCurve->Type == EDistributionType::ConstantCurve ||
				 SelectedTrack->FloatCurve->Type == EDistributionType::UniformCurve)
		{
			// SelectedAxis에 따라 올바른 커브 선택
			FInterpCurveFloat* CurvePtr = nullptr;
			if (SelectedTrack->FloatCurve->Type == EDistributionType::ConstantCurve)
			{
				CurvePtr = &SelectedTrack->FloatCurve->ConstantCurve;
			}
			else // UniformCurve
			{
				CurvePtr = (CurveState.SelectedAxis == 0)
					? &SelectedTrack->FloatCurve->MinCurve
					: &SelectedTrack->FloatCurve->MaxCurve;
			}

			if (CurvePtr && CurveState.SelectedKeyIndex < CurvePtr->Points.Num())
			{
				FInterpCurvePointFloat& Point = CurvePtr->Points[CurveState.SelectedKeyIndex];

				// 마우스 위치를 커브 값으로 변환
				float t = (MousePos.x - CanvasPos.x) / CanvasSize.x;

				Point.InVal = FMath::Lerp(CurveState.ViewMinTime, CurveState.ViewMaxTime, t);
				Point.OutVal = NewValue;

				// CurveAuto/CurveAutoClamped 모드면 탄젠트 재계산
				CurvePtr->AutoCalculateTangents();

				if (EditorState) EditorState->bIsDirty = true;
			}
		}
	}

	// 키 드래그 (Vector) - 탄젠트 핸들이 선택되지 않았을 때만
	if (ImGui::IsMouseDragging(0) && CurveState.SelectedKeyIndex >= 0 &&
		CurveState.SelectedAxis >= 0 && CurveState.SelectedTangentHandle == 0 &&
		SelectedTrack && SelectedTrack->VectorCurve)
	{
		// 마우스 위치를 값으로 변환 (Y축만 사용)
		float v = 1.0f - (MousePos.y - CanvasPos.y) / CanvasSize.y;
		float NewValue = FMath::Lerp(CurveState.ViewMinValue, CurveState.ViewMaxValue, v);

		// SelectedAxis에 따라 축 결정
		// 0-2: ConstantValue/MaxValue의 X/Y/Z
		// 3-5: MinValue의 X/Y/Z
		int32 AxisIndex = CurveState.SelectedAxis % 3;
		bool bIsMinValue = (CurveState.SelectedAxis >= 3);

		if (SelectedTrack->VectorCurve->Type == EDistributionType::Constant)
		{
			// Constant: 단일 벡터 값 직접 수정
			FVector& Value = SelectedTrack->VectorCurve->ConstantValue;
			if (AxisIndex == 0) Value.X = NewValue;
			else if (AxisIndex == 1) Value.Y = NewValue;
			else Value.Z = NewValue;
			if (EditorState) EditorState->bIsDirty = true;
		}
		else if (SelectedTrack->VectorCurve->Type == EDistributionType::Uniform)
		{
			// Uniform: Min/Max 벡터 값 직접 수정
			FVector& Value = bIsMinValue
				? SelectedTrack->VectorCurve->MinValue
				: SelectedTrack->VectorCurve->MaxValue;
			if (AxisIndex == 0) Value.X = NewValue;
			else if (AxisIndex == 1) Value.Y = NewValue;
			else Value.Z = NewValue;
			if (EditorState) EditorState->bIsDirty = true;
		}
		else if (SelectedTrack->VectorCurve->Type == EDistributionType::ParticleParameter)
		{
			// ParticleParameter: Default 벡터 값 직접 수정
			FVector& Value = SelectedTrack->VectorCurve->ParameterDefaultValue;
			if (AxisIndex == 0) Value.X = NewValue;
			else if (AxisIndex == 1) Value.Y = NewValue;
			else Value.Z = NewValue;
			if (EditorState) EditorState->bIsDirty = true;
		}
		else if (SelectedTrack->VectorCurve->Type == EDistributionType::ConstantCurve ||
				 SelectedTrack->VectorCurve->Type == EDistributionType::UniformCurve)
		{
			FInterpCurveVector* CurvePtr = nullptr;

			if (SelectedTrack->VectorCurve->Type == EDistributionType::ConstantCurve)
			{
				CurvePtr = &SelectedTrack->VectorCurve->ConstantCurve;
			}
			else // UniformCurve
			{
				CurvePtr = bIsMinValue
					? &SelectedTrack->VectorCurve->MinCurve
					: &SelectedTrack->VectorCurve->MaxCurve;
			}

			if (CurvePtr && CurveState.SelectedKeyIndex < CurvePtr->Points.Num())
			{
				FInterpCurvePointVector& Point = CurvePtr->Points[CurveState.SelectedKeyIndex];

				// 마우스 위치를 커브 값으로 변환
				float t = (MousePos.x - CanvasPos.x) / CanvasSize.x;
				Point.InVal = FMath::Lerp(CurveState.ViewMinTime, CurveState.ViewMaxTime, t);

				// 해당 축만 업데이트
				if (AxisIndex == 0) Point.OutVal.X = NewValue;
				else if (AxisIndex == 1) Point.OutVal.Y = NewValue;
				else Point.OutVal.Z = NewValue;

				// CurveAuto/CurveAutoClamped 모드면 탄젠트 재계산
				CurvePtr->AutoCalculateTangents();

				if (EditorState) EditorState->bIsDirty = true;
			}
		}
	}

	// 탄젠트 핸들 드래그 (탄젠트 핸들이 선택된 상태에서)
	if (ImGui::IsMouseDragging(0) && CurveState.SelectedTangentHandle > 0 &&
		CurveState.SelectedKeyIndex >= 0 && CurveState.SelectedAxis >= 0 && SelectedTrack)
	{
		ImVec2 Delta = ImGui::GetMouseDragDelta(0);
		ImGui::ResetMouseDragDelta(0);

		float TimeScale = CanvasSize.x / (CurveState.ViewMaxTime - CurveState.ViewMinTime);
		float ValueScale = CanvasSize.y / (CurveState.ViewMaxValue - CurveState.ViewMinValue);

		// 탄젠트 변화량 계산 (마우스 Y 이동을 기울기로 변환)
		// Leave: KeyY - LeaveDy → 위로 드래그(DeltaY<0) → 탄젠트 증가 → -DeltaY
		// Arrive: KeyY + ArriveDy → 위로 드래그(DeltaY<0) → 탄젠트 감소 → +DeltaY
		float HandleLength = 40.0f;
		float Sign = (CurveState.SelectedTangentHandle == 1) ? 1.0f : -1.0f;  // Arrive: +, Leave: -
		float TangentDelta = Sign * Delta.y * TimeScale / (ValueScale * HandleLength);

		// Float 커브
		if (SelectedTrack->FloatCurve &&
			(SelectedTrack->FloatCurve->Type == EDistributionType::ConstantCurve ||
			 SelectedTrack->FloatCurve->Type == EDistributionType::UniformCurve))
		{
			FInterpCurveFloat* CurvePtr = nullptr;
			if (SelectedTrack->FloatCurve->Type == EDistributionType::ConstantCurve)
			{
				CurvePtr = &SelectedTrack->FloatCurve->ConstantCurve;
			}
			else
			{
				CurvePtr = (CurveState.SelectedAxis == 0)
					? &SelectedTrack->FloatCurve->MinCurve
					: &SelectedTrack->FloatCurve->MaxCurve;
			}

			if (CurvePtr && CurveState.SelectedKeyIndex < CurvePtr->Points.Num())
			{
				FInterpCurvePointFloat& Point = CurvePtr->Points[CurveState.SelectedKeyIndex];

				// Curve 모드가 아니면 Curve 모드로 전환
				if (Point.InterpMode != EInterpCurveMode::Curve &&
					Point.InterpMode != EInterpCurveMode::CurveAuto &&
					Point.InterpMode != EInterpCurveMode::CurveAutoClamped)
				{
					Point.InterpMode = EInterpCurveMode::Curve;
				}

				// 선택된 핸들만 조정
				if (CurveState.SelectedTangentHandle == 1)
				{
					Point.ArriveTangent += TangentDelta;
				}
				else
				{
					Point.LeaveTangent += TangentDelta;
				}

				if (EditorState) EditorState->bIsDirty = true;
			}
		}
		// Vector 커브
		else if (SelectedTrack->VectorCurve &&
			(SelectedTrack->VectorCurve->Type == EDistributionType::ConstantCurve ||
			 SelectedTrack->VectorCurve->Type == EDistributionType::UniformCurve))
		{
			FInterpCurveVector* CurvePtr = nullptr;
			int32 AxisIndex = CurveState.SelectedAxis % 3;
			bool bIsMinCurve = (CurveState.SelectedAxis >= 3);

			if (SelectedTrack->VectorCurve->Type == EDistributionType::ConstantCurve)
			{
				CurvePtr = &SelectedTrack->VectorCurve->ConstantCurve;
			}
			else
			{
				CurvePtr = bIsMinCurve
					? &SelectedTrack->VectorCurve->MinCurve
					: &SelectedTrack->VectorCurve->MaxCurve;
			}

			if (CurvePtr && CurveState.SelectedKeyIndex < CurvePtr->Points.Num())
			{
				FInterpCurvePointVector& Point = CurvePtr->Points[CurveState.SelectedKeyIndex];

				// Curve 모드가 아니면 Curve 모드로 전환
				if (Point.InterpMode != EInterpCurveMode::Curve &&
					Point.InterpMode != EInterpCurveMode::CurveAuto &&
					Point.InterpMode != EInterpCurveMode::CurveAutoClamped)
				{
					Point.InterpMode = EInterpCurveMode::Curve;
				}

				// 선택된 핸들과 축에 따라 조정
				if (CurveState.SelectedTangentHandle == 1)
				{
					// Arrive 탄젠트
					if (AxisIndex == 0) Point.ArriveTangent.X += TangentDelta;
					else if (AxisIndex == 1) Point.ArriveTangent.Y += TangentDelta;
					else Point.ArriveTangent.Z += TangentDelta;
				}
				else
				{
					// Leave 탄젠트
					if (AxisIndex == 0) Point.LeaveTangent.X += TangentDelta;
					else if (AxisIndex == 1) Point.LeaveTangent.Y += TangentDelta;
					else Point.LeaveTangent.Z += TangentDelta;
				}

				if (EditorState) EditorState->bIsDirty = true;
			}
		}
	}

	// 휠로 줌
	float Wheel = ImGui::GetIO().MouseWheel;
	if (Wheel != 0.0f)
	{
		float ZoomFactor = 1.0f - Wheel * 0.1f;

		float CenterTime = (CurveState.ViewMinTime + CurveState.ViewMaxTime) * 0.5f;
		float CenterValue = (CurveState.ViewMinValue + CurveState.ViewMaxValue) * 0.5f;
		float TimeRange = (CurveState.ViewMaxTime - CurveState.ViewMinTime) * ZoomFactor;
		float ValueRange = (CurveState.ViewMaxValue - CurveState.ViewMinValue) * ZoomFactor;

		CurveState.ViewMinTime = CenterTime - TimeRange * 0.5f;
		CurveState.ViewMaxTime = CenterTime + TimeRange * 0.5f;
		CurveState.ViewMinValue = CenterValue - ValueRange * 0.5f;
		CurveState.ViewMaxValue = CenterValue + ValueRange * 0.5f;
	}

	// 중앙 클릭 드래그로 줌
	if (ImGui::IsMouseDragging(2))
	{
		ImVec2 Delta = ImGui::GetMouseDragDelta(2);
		ImGui::ResetMouseDragDelta(2);

		// Y 드래그로 줌 (아래로 드래그 = 확대, 위로 드래그 = 축소)
		float ZoomFactor = 1.0f + Delta.y * 0.005f;

		float CenterTime = (CurveState.ViewMinTime + CurveState.ViewMaxTime) * 0.5f;
		float CenterValue = (CurveState.ViewMinValue + CurveState.ViewMaxValue) * 0.5f;
		float TimeRange = (CurveState.ViewMaxTime - CurveState.ViewMinTime) * ZoomFactor;
		float ValueRange = (CurveState.ViewMaxValue - CurveState.ViewMinValue) * ZoomFactor;

		CurveState.ViewMinTime = CenterTime - TimeRange * 0.5f;
		CurveState.ViewMaxTime = CenterTime + TimeRange * 0.5f;
		CurveState.ViewMinValue = CenterValue - ValueRange * 0.5f;
		CurveState.ViewMaxValue = CenterValue + ValueRange * 0.5f;
	}

	// 우클릭 드래그로 패닝
	if (ImGui::IsMouseDragging(1))
	{
		ImVec2 Delta = ImGui::GetMouseDragDelta(1);
		ImGui::ResetMouseDragDelta(1);

		float TimePerPixel = (CurveState.ViewMaxTime - CurveState.ViewMinTime) / CanvasSize.x;
		float ValuePerPixel = (CurveState.ViewMaxValue - CurveState.ViewMinValue) / CanvasSize.y;

		CurveState.ViewMinTime -= Delta.x * TimePerPixel;
		CurveState.ViewMaxTime -= Delta.x * TimePerPixel;
		CurveState.ViewMinValue += Delta.y * ValuePerPixel;
		CurveState.ViewMaxValue += Delta.y * ValuePerPixel;
	}
}

void SCurveEditorWidget::AutoFitCurveView()
{
	if (CurveState.Tracks.Num() == 0)
	{
		CurveState.ViewMinTime = 0.0f;
		CurveState.ViewMaxTime = 1.0f;
		CurveState.ViewMinValue = 0.0f;
		CurveState.ViewMaxValue = 1.0f;
		return;
	}

	float MinTime = FLT_MAX;
	float MaxTime = -FLT_MAX;
	float MinValue = FLT_MAX;
	float MaxValue = -FLT_MAX;
	bool bFoundValidCurve = false;

	for (const FCurveTrack& Track : CurveState.Tracks)
	{
		if (Track.FloatCurve)
		{
			EDistributionType Type = Track.FloatCurve->Type;
			if (Type == EDistributionType::Constant)
			{
				float Value = Track.FloatCurve->ConstantValue;
				MinValue = FMath::Min(MinValue, Value);
				MaxValue = FMath::Max(MaxValue, Value);
				MinTime = FMath::Min(MinTime, 0.0f);
				MaxTime = FMath::Max(MaxTime, 1.0f);
				bFoundValidCurve = true;
			}
			else if (Type == EDistributionType::Uniform)
			{
				MinValue = FMath::Min(MinValue, Track.FloatCurve->MinValue);
				MaxValue = FMath::Max(MaxValue, Track.FloatCurve->MaxValue);
				MinTime = FMath::Min(MinTime, 0.0f);
				MaxTime = FMath::Max(MaxTime, 1.0f);
				bFoundValidCurve = true;
			}
			else if (Type == EDistributionType::ConstantCurve)
			{
				FInterpCurveFloat& Curve = Track.FloatCurve->ConstantCurve;
				for (int32 i = 0; i < Curve.Points.Num(); ++i)
				{
					MinTime = FMath::Min(MinTime, Curve.Points[i].InVal);
					MaxTime = FMath::Max(MaxTime, Curve.Points[i].InVal);
					MinValue = FMath::Min(MinValue, Curve.Points[i].OutVal);
					MaxValue = FMath::Max(MaxValue, Curve.Points[i].OutVal);
					bFoundValidCurve = true;
				}
			}
			else if (Type == EDistributionType::UniformCurve)
			{
				FInterpCurveFloat& MinCurve = Track.FloatCurve->MinCurve;
				FInterpCurveFloat& MaxCurve = Track.FloatCurve->MaxCurve;
				for (int32 i = 0; i < MinCurve.Points.Num(); ++i)
				{
					MinTime = FMath::Min(MinTime, MinCurve.Points[i].InVal);
					MaxTime = FMath::Max(MaxTime, MinCurve.Points[i].InVal);
					MinValue = FMath::Min(MinValue, MinCurve.Points[i].OutVal);
					MaxValue = FMath::Max(MaxValue, MinCurve.Points[i].OutVal);
					bFoundValidCurve = true;
				}
				for (int32 i = 0; i < MaxCurve.Points.Num(); ++i)
				{
					MinTime = FMath::Min(MinTime, MaxCurve.Points[i].InVal);
					MaxTime = FMath::Max(MaxTime, MaxCurve.Points[i].InVal);
					MinValue = FMath::Min(MinValue, MaxCurve.Points[i].OutVal);
					MaxValue = FMath::Max(MaxValue, MaxCurve.Points[i].OutVal);
					bFoundValidCurve = true;
				}
			}
		}
		else if (Track.VectorCurve)
		{
			EDistributionType Type = Track.VectorCurve->Type;
			if (Type == EDistributionType::Constant)
			{
				FVector Value = Track.VectorCurve->ConstantValue;
				MinValue = FMath::Min(MinValue, FMath::Min(Value.X, FMath::Min(Value.Y, Value.Z)));
				MaxValue = FMath::Max(MaxValue, FMath::Max(Value.X, FMath::Max(Value.Y, Value.Z)));
				MinTime = FMath::Min(MinTime, 0.0f);
				MaxTime = FMath::Max(MaxTime, 1.0f);
				bFoundValidCurve = true;
			}
			else if (Type == EDistributionType::Uniform)
			{
				FVector MinVal = Track.VectorCurve->MinValue;
				FVector MaxVal = Track.VectorCurve->MaxValue;
				MinValue = FMath::Min(MinValue, FMath::Min(MinVal.X, FMath::Min(MinVal.Y, MinVal.Z)));
				MaxValue = FMath::Max(MaxValue, FMath::Max(MaxVal.X, FMath::Max(MaxVal.Y, MaxVal.Z)));
				MinTime = FMath::Min(MinTime, 0.0f);
				MaxTime = FMath::Max(MaxTime, 1.0f);
				bFoundValidCurve = true;
			}
			else if (Type == EDistributionType::ConstantCurve)
			{
				FInterpCurveVector& Curve = Track.VectorCurve->ConstantCurve;
				for (int32 i = 0; i < Curve.Points.Num(); ++i)
				{
					MinTime = FMath::Min(MinTime, Curve.Points[i].InVal);
					MaxTime = FMath::Max(MaxTime, Curve.Points[i].InVal);
					FVector& Val = Curve.Points[i].OutVal;
					MinValue = FMath::Min(MinValue, FMath::Min(Val.X, FMath::Min(Val.Y, Val.Z)));
					MaxValue = FMath::Max(MaxValue, FMath::Max(Val.X, FMath::Max(Val.Y, Val.Z)));
					bFoundValidCurve = true;
				}
			}
			else if (Type == EDistributionType::UniformCurve)
			{
				FInterpCurveVector& MinCurve = Track.VectorCurve->MinCurve;
				FInterpCurveVector& MaxCurve = Track.VectorCurve->MaxCurve;
				for (int32 i = 0; i < MinCurve.Points.Num(); ++i)
				{
					MinTime = FMath::Min(MinTime, MinCurve.Points[i].InVal);
					MaxTime = FMath::Max(MaxTime, MinCurve.Points[i].InVal);
					FVector& Val = MinCurve.Points[i].OutVal;
					MinValue = FMath::Min(MinValue, FMath::Min(Val.X, FMath::Min(Val.Y, Val.Z)));
					MaxValue = FMath::Max(MaxValue, FMath::Max(Val.X, FMath::Max(Val.Y, Val.Z)));
					bFoundValidCurve = true;
				}
				for (int32 i = 0; i < MaxCurve.Points.Num(); ++i)
				{
					MinTime = FMath::Min(MinTime, MaxCurve.Points[i].InVal);
					MaxTime = FMath::Max(MaxTime, MaxCurve.Points[i].InVal);
					FVector& Val = MaxCurve.Points[i].OutVal;
					MinValue = FMath::Min(MinValue, FMath::Min(Val.X, FMath::Min(Val.Y, Val.Z)));
					MaxValue = FMath::Max(MaxValue, FMath::Max(Val.X, FMath::Max(Val.Y, Val.Z)));
					bFoundValidCurve = true;
				}
			}
		}
	}

	if (!bFoundValidCurve)
	{
		MinTime = 0.0f;
		MaxTime = 1.0f;
		MinValue = 0.0f;
		MaxValue = 1.0f;
	}

	float TimeRange = MaxTime - MinTime;
	float ValueRange = MaxValue - MinValue;
	if (TimeRange < 0.001f) TimeRange = 1.0f;
	if (ValueRange < 0.001f) ValueRange = 1.0f;

	CurveState.ViewMinTime = MinTime - TimeRange * 0.1f;
	CurveState.ViewMaxTime = MaxTime + TimeRange * 0.1f;
	CurveState.ViewMinValue = MinValue - ValueRange * 0.1f;
	CurveState.ViewMaxValue = MaxValue + ValueRange * 0.1f;
}
