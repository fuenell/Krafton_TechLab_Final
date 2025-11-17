#pragma once
#include "SViewerWindow.h"
#include "Source/Runtime/Engine/Viewer/ViewerState.h"

class FViewport;
class FViewportClient;
class UWorld;
struct ID3D11Device;

class SAnimationViewerWindow : public SViewerWindow
{
public:
    SAnimationViewerWindow();
    virtual ~SAnimationViewerWindow();

    virtual void OnRender() override;
    virtual void OnUpdate(float DeltaSeconds) override;
    virtual void PreRenderViewportUpdate() override;

protected:
    virtual ViewerState* CreateViewerState(const char* Name, UEditorAssetPreviewContext* Context) override;
    virtual void DestroyViewerState(ViewerState*& State) override;
    virtual FString GetWindowTitle() const override { return "Animation Viewer"; }

private:
    // Load a skeletal mesh into the active tab
    void LoadSkeletalMesh(ViewerState* State, const FString& Path);
    void RenderAnimationBrowser();
};