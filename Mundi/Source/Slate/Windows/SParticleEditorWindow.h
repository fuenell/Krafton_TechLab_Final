#pragma once
#include "SViewerWindow.h"
#include "Source/Runtime/Engine/Viewer/ViewerState.h"

class FViewport;
class FViewportClient;
class UWorld;
struct ID3D11Device;
class UParticleEmitter;
class UParticleModule;
class ULineComponent;

class SParticleEditorWindow : public SViewerWindow
{
public:
	SParticleEditorWindow();
	virtual ~SParticleEditorWindow();

	virtual void OnRender() override;
	virtual void OnUpdate(float DeltaSeconds) override;
	virtual void PreRenderViewportUpdate() override;

protected:
	virtual ViewerState* CreateViewerState(const char* Name, UEditorAssetPreviewContext* Context) override;
	virtual void DestroyViewerState(ViewerState*& State) override;
	virtual FString GetWindowTitle() const override { return "Particle Editor"; }
	virtual void RenderTabsAndToolbar(EViewerType CurrentViewerType) override;

	// 패널 렌더링
	virtual void RenderLeftPanel(float PanelWidth) override;   // 뷰포트 + 디테일
	virtual void RenderRightPanel() override;                  // 이미터 패널
	virtual void RenderBottomPanel() override;                 // 커브 에디터

private:
	// 툴바
	void RenderToolbar();

	// 디테일 패널
	void RenderDetailsPanel(float PanelWidth);

	// 이미터 패널 하위 렌더링
	void RenderEmitterColumn(int32 EmitterIndex, UParticleEmitter* Emitter);
	void RenderModuleBlock(int32 EmitterIdx, int32 ModuleIdx, UParticleModule* Module);

	// 뷰포트
	void RenderViewportArea(float width, float height);

	// 4개 영역 렌더링 함수
	void RenderLeftViewportArea(float totalHeight);
	void RenderLeftDetailsArea();
	void RenderRightEmitterArea(float totalHeight);
	void RenderRightCurveArea();

	// 파일 작업
	void CreateNewParticleSystem();
	void SaveParticleSystem();
	void SaveParticleSystemAs();
	void LoadParticleSystem();

	// 레이아웃 비율 및 크기
	float LeftViewportHeight = 250.f;   // 좌측 상단 뷰포트 높이
	float RightEmitterHeight = 400.f;   // 우측 상단 이미터 패널 높이

	// 툴바 아이콘
	UTexture* IconSave = nullptr;
	UTexture* IconLoad = nullptr;
	UTexture* IconRestart = nullptr;
	UTexture* IconBounds = nullptr;
	UTexture* IconOriginAxis = nullptr;
	UTexture* IconBackgroundColor = nullptr;
	UTexture* IconLODFirst = nullptr;
	UTexture* IconLODPrev = nullptr;
	UTexture* IconLODInsertBefore = nullptr;
	UTexture* IconLODInsertAfter = nullptr;
	UTexture* IconLODDelete = nullptr;
	UTexture* IconLODNext = nullptr;
	UTexture* IconLODLast = nullptr;

	// 원점축 렌더링
	ULineComponent* OriginAxisLineComponent = nullptr;

	// 툴바 상태
	void LoadToolbarIcons();

	// 헬퍼 함수
	ParticleEditorState* GetActiveParticleState() const
	{
		return static_cast<ParticleEditorState*>(ActiveState);
	}

	void CreateOriginAxisLines();
};
