#include "pch.h"
#include "SlateManager.h"
#include "SAnimationViewerWindow.h"
#include "Source/Runtime/Engine/Viewer/AnimationViewerBootstrap.h"
#include "Source/Runtime/Engine/GameFramework/SkeletalMeshActor.h"
#include "Source/Runtime/Engine/Viewer/EditorAssetPreviewContext.h"
#include "AnimSingleNodeInstance.h"

SAnimationViewerWindow::SAnimationViewerWindow()
{
    CenterRect = FRect(0, 0, 0, 0);
}

SAnimationViewerWindow::~SAnimationViewerWindow()
{
    // Clean up tabs if any
    for (int i = 0; i < Tabs.Num(); ++i)
    {
        ViewerState* State = Tabs[i];
        AnimationViewerBootstrap::DestroyViewerState(State);
    }
    Tabs.Empty();
    ActiveState = nullptr;
}

void SAnimationViewerWindow::OnRender()
{
    // If window is closed, don't render
    if (!bIsOpen)
    {
        return;
    }

    // Parent detachable window (movable, top-level) with solid background
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;

    if (!bInitialPlacementDone)
    {
        ImGui::SetNextWindowPos(ImVec2(Rect.Left, Rect.Top));
        ImGui::SetNextWindowSize(ImVec2(Rect.GetWidth(), Rect.GetHeight()));
        bInitialPlacementDone = true;
    }
    if (bRequestFocus)
    {
        ImGui::SetNextWindowFocus();
    }

    // Generate a unique window title to pass to ImGui
    // Format: "Skeletal Mesh Viewer - MyAsset.fbx###0x12345678"
    char UniqueTitle[256];
    sprintf_s(UniqueTitle, sizeof(UniqueTitle), "%s###%p", WindowTitle.c_str(), this);

    bool bViewerVisible = false;
    if (ImGui::Begin(UniqueTitle, &bIsOpen, flags))
    {
        bViewerVisible = true;

        //===============================
        // Tab Bar
        // : Render tab bar and switch active state
        //===============================
        if (ImGui::BeginTabBar("SkeletalViewerTabs", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable))
        {
            for (int i = 0; i < Tabs.Num(); ++i)
            {
                ViewerState* State = Tabs[i];
                bool open = true;
                if (ImGui::BeginTabItem(State->Name.ToString().c_str(), &open))
                {
                    ActiveTabIndex = i;
                    ActiveState = State;
                    ImGui::EndTabItem();
                }
                if (!open)
                {
                    CloseTab(i);
                    break;
                }
            }
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
            {
                char label[32]; sprintf_s(label, "Viewer %d", Tabs.Num() + 1);
                OpenNewTab(label);
            }
            ImGui::EndTabBar();
        }

        //===============================
        // Layout calculations
        //===============================
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();
        Rect.Left = pos.x; Rect.Top = pos.y; Rect.Right = pos.x + size.x; Rect.Bottom = pos.y + size.y; 
        Rect.UpdateMinMax();

        ImVec2 contentAvail = ImGui::GetContentRegionAvail();
        float totalWidth = contentAvail.x;
        float totalHeight = contentAvail.y;

        float leftWidth = totalWidth * LeftPanelRatio;
        float rightWidth = totalWidth * RightPanelRatio;
        float centerWidth = totalWidth - leftWidth - rightWidth;

        //===============================
        // Top panels (Left / Center / Right)
        //===============================

        // Remove spacing between panels
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        // Left panel - Asset Browser & Bone Hierarchy
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar();
        RenderLeftPanel(leftWidth);
        ImGui::EndChild();

        ImGui::SameLine(0, 0); // No spacing between panels

        // Center panel (viewport area) — draw with border to see the viewport area
        ImGui::BeginChild("CenterPanel", ImVec2(centerWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);

        float contentHeight = ImGui::GetContentRegionAvail().y;
        float itemSpacingY = ImGui::GetStyle().ItemSpacing.y;
        float viewportHeight = (contentHeight - itemSpacingY) * 0.75f;
        float timelineHeight = (contentHeight - itemSpacingY) * 0.25f;
        float innerWidth = ImGui::GetContentRegionAvail().x;
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            // Remove all padding, spacing, rounding
            ImGui::BeginChild("ViewportArea", ImVec2(innerWidth, viewportHeight), false,
                ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
            ImGui::PopStyleVar();
            ImVec2 childPos = ImGui::GetCursorScreenPos();      // inner top-left
            ImVec2 childSize = ImGui::GetContentRegionAvail();  // usable area
            ImVec2 rectMin = childPos;
            ImVec2 rectMax(childPos.x + childSize.x, childPos.y + childSize.y);
            CenterRect.Left = rectMin.x; CenterRect.Top = rectMin.y; CenterRect.Right = rectMax.x; CenterRect.Bottom = rectMax.y;
            CenterRect.UpdateMinMax();
            ImGui::EndChild();
        }
        {
            ImGui::BeginChild("TimelineArea", ImVec2(innerWidth, timelineHeight), true);
            //----------------------------------------------
            // TIMELINE PANEL (Inside CenterPanel)
            //----------------------------------------------
            ImGui::Text("Timeline");

            // -- Controls Row --
            ImGui::Spacing();

            if (ActiveState->bIsPlaying)
            {
                if (ImGui::Button("Pause", ImVec2(55, 24)))
                {
                    ActiveState->bIsPlaying = false;
                }
            }
            else
            {
                if (ImGui::Button("Play", ImVec2(55, 24)))
                {
                    ActiveState->bIsPlaying = true;
                }
            }

            ImGui::SameLine();
            ImGui::Checkbox("Loop", &ActiveState->bIsLooping);

            ImGui::SameLine();
            ImGui::Text("Speed:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            ImGui::SliderFloat("##AnimSpeed", &ActiveState->PlaybackSpeed, 0.1f, 3.0f, "%.1fx");

            ImGui::SameLine();
            ImGui::Text("Time:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            if (ImGui::SliderFloat("##AnimTime", &ActiveState->CurrentTime, 0.0f, ActiveState->TotalTime, "%.2f"))
            {
                ActiveState->bIsScrubbing = true;
            }

            ImGui::Separator();
            ImGui::Spacing();

            //----------------------------------------------
            // Track List + Timeline Grid Layout
            //----------------------------------------------
            ImGui::BeginChild("TrackLayoutInner", ImVec2(0, 0), false);

            float leftTrackWidth = 130.0f;
            float rightTimelineWidth = ImGui::GetContentRegionAvail().x - leftTrackWidth;

            //======== LEFT TRACK PANEL ========//
            ImGui::BeginChild("TrackListSmall", ImVec2(leftTrackWidth, 0), true);

            ImGui::Text("Notifies");
            ImGui::Separator();
            ImGui::Text("Curves");
            ImGui::Separator();
            ImGui::Text("Attributes");
            ImGui::Separator();

            ImGui::EndChild();

            ImGui::SameLine();

            //======== RIGHT TIMELINE GRID ========//
            ImGui::BeginChild("TimelineGridSmall", ImVec2(rightTimelineWidth, 0), true);

            ImGui::Text("Timeline:");
            ImGui::PushItemWidth(-1);
            if (ImGui::SliderFloat("##TimelineScrubSmall", &ActiveState->CurrentTime, 0.0f, ActiveState->TotalTime))
            {
                ActiveState->bIsScrubbing = true;
            }
            ImGui::PopItemWidth();

            ImGui::Separator();

            // Draw timeline grid
            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImVec2 sz = ImGui::GetContentRegionAvail();

            // background
            draw->AddRectFilled(p0, ImVec2(p0.x + sz.x, p0.y + sz.y), IM_COL32(40, 40, 40, 255));

            const int frameCount = 50;   // compact version
            for (int i = 0; i < frameCount; i++)
            {
                float x = p0.x + (float)i * (sz.x / frameCount);
                draw->AddLine(ImVec2(x, p0.y), ImVec2(x, p0.y + sz.y), IM_COL32(80, 80, 80, 180));
            }

            // Red playhead
            float playheadX = p0.x;
            if (ActiveState && ActiveState->TotalTime > 0.0f)
            {
                playheadX = p0.x + (ActiveState->CurrentTime / ActiveState->TotalTime) * sz.x;
            }
            draw->AddLine(ImVec2(playheadX, p0.y),
                ImVec2(playheadX, p0.y + sz.y),
                IM_COL32(255, 0, 0, 255),
                2.0f);

            ImGui::EndChild();  // TimelineGridSmall
            ImGui::EndChild();  // TrackLayoutInner
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 0); // No spacing between panels

        // Right panel - Bone Properties
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::BeginChild("RightPanel", ImVec2(rightWidth, totalHeight), true);
        ImGui::PopStyleVar();
        {
            float bonePropsHeight = ImGui::GetContentRegionAvail().y * 0.5f;
            float animBrowserHeight = ImGui::GetContentRegionAvail().y * 0.5f;

            ImGui::BeginChild("BonePropertiesArea", ImVec2(0, bonePropsHeight), false);
            RenderRightPanel();
            ImGui::EndChild();

            ImGui::BeginChild("AnimationBrowserArea", ImVec2(0, 0), false);
            RenderAnimationBrowser();
            ImGui::EndChild();
        }
        ImGui::EndChild(); // RightPanel

        // Pop the ItemSpacing style
        ImGui::PopStyleVar();
    }
    ImGui::End();

    // If collapsed or not visible, clear the center rect so we don't render a floating viewport
    if (!bViewerVisible)
    {
        CenterRect = FRect(0, 0, 0, 0);
        CenterRect.UpdateMinMax();
    }

    // If window was closed via X button, notify the manager to clean up
    if (!bIsOpen)
    {
        // Just request to close the window
        USlateManager::GetInstance().RequestCloseDetachedWindow(this);
    }

    bRequestFocus = false;
}

void SAnimationViewerWindow::OnUpdate(float DeltaSeconds)
{
    SViewerWindow::OnUpdate(DeltaSeconds);

    if (!ActiveState || !ActiveState->PreviewActor || !ActiveState->CurrentAnimation) return;

    USkeletalMeshComponent* MeshComp = ActiveState->PreviewActor->GetSkeletalMeshComponent();
    if (!MeshComp)  return;

    UAnimSingleNodeInstance* SingleAnimInstance = Cast<UAnimSingleNodeInstance>(MeshComp->GetAnimInstance());
    if (!SingleAnimInstance)    return;

    // Get current animation component's actual state
    bool bIsActuallyPlaying = MeshComp->IsPlayingAnimation();
    float CurrentAnimPosition = MeshComp->GetAnimationPosition();

    // Synchronize play/pause state
    if (ActiveState->bIsPlaying && !bIsActuallyPlaying)
    {
        MeshComp->PlayAnimation(ActiveState->CurrentAnimation, ActiveState->bIsLooping, ActiveState->PlaybackSpeed);
        MeshComp->SetAnimationPosition(ActiveState->CurrentTime);
    }
    else if (!ActiveState->bIsPlaying && bIsActuallyPlaying)
    {
        MeshComp->StopAnimation();
    }

    // Synchronize play rate and loop setting
    SingleAnimInstance->SetPlayRate(ActiveState->PlaybackSpeed);
    SingleAnimInstance->SetLooping(ActiveState->bIsLooping);

    // Control time slider (UI to Engine)
    // Update the component's animation position only when the user is dragging the slider
    if (ActiveState->bIsScrubbing)
    {
        MeshComp->SetAnimationPosition(ActiveState->CurrentTime);
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            ActiveState->bIsScrubbing = false;
        }
    }
    // Update time slider (Engine to UI)
    // Update the UI with the engine's time only when the user is not dragging the slider.
    else
    {
        ActiveState->CurrentTime = CurrentAnimPosition;
    }

    // Update total animation length
    if (ActiveState->CurrentAnimation)
    {
        ActiveState->TotalTime = ActiveState->CurrentAnimation->GetSequenceLength();
    }
}

void SAnimationViewerWindow::PreRenderViewportUpdate()
{
    // Reconstruct bone overlay
    if (ActiveState->bShowBones)
    {
        ActiveState->bBoneLinesDirty = true;
    }
    if (ActiveState->bShowBones && ActiveState->PreviewActor && ActiveState->CurrentMesh && ActiveState->bBoneLinesDirty)
    {
        if (ULineComponent* LineComp = ActiveState->PreviewActor->GetBoneLineComponent())
        {
            LineComp->SetLineVisible(true);
        }
        ActiveState->PreviewActor->RebuildBoneLines(ActiveState->SelectedBoneIndex);
        ActiveState->bBoneLinesDirty = false;
    }
}

ViewerState* SAnimationViewerWindow::CreateViewerState(const char* Name, UEditorAssetPreviewContext* Context)
{
    ViewerState* NewState = AnimationViewerBootstrap::CreateViewerState(Name, World, Device);
    if (!NewState) return nullptr;

    if (Context && !Context->AssetPath.empty())
    {
        LoadSkeletalMesh(NewState, Context->AssetPath);
    }
    return NewState;
}

void SAnimationViewerWindow::DestroyViewerState(ViewerState*& State)
{
    AnimationViewerBootstrap::DestroyViewerState(State);
}

void SAnimationViewerWindow::LoadSkeletalMesh(ViewerState* State, const FString& Path)
{
    if (!State || Path.empty())
        return;

    // Load the skeletal mesh using the resource manager
    USkeletalMesh* Mesh = UResourceManager::GetInstance().Load<USkeletalMesh>(Path);
    if (Mesh && State->PreviewActor)
    {
        // Set the mesh on the preview actor
        State->PreviewActor->SetSkeletalMesh(Path);
        State->CurrentMesh = Mesh;

        // Expand all bone nodes by default on mesh load
        State->ExpandedBoneIndices.clear();
        if (const FSkeleton* Skeleton = State->CurrentMesh->GetSkeleton())
        {
            for (int32 i = 0; i < Skeleton->Bones.size(); ++i)
            {
                State->ExpandedBoneIndices.insert(i);
            }
        }

        State->LoadedMeshPath = Path;  // Track for resource unloading

        // Update mesh path buffer for display in UI
        strncpy_s(State->MeshPathBuffer, Path.c_str(), sizeof(State->MeshPathBuffer) - 1);

        // Sync mesh visibility with checkbox state
        if (auto* Skeletal = State->PreviewActor->GetSkeletalMeshComponent())
        {
            Skeletal->SetVisibility(State->bShowMesh);
        }

        // Mark bone lines as dirty to rebuild on next frame
        State->bBoneLinesDirty = true;

        // Clear and sync bone line visibility
        if (auto* LineComp = State->PreviewActor->GetBoneLineComponent())
        {
            LineComp->ClearLines();
            LineComp->SetLineVisible(State->bShowBones);
        }

        UE_LOG("SAnimationViewerWindow: Loaded skeletal mesh from %s", Path.c_str());
    }
    else
    {
        UE_LOG("SAnimationViewerWindow: Failed to load skeletal mesh from %s", Path.c_str());
    }
}

void SAnimationViewerWindow::RenderAnimationBrowser()
{
    if (!ActiveState)   return;

    ImGui::Separator();
    ImGui::Text("Animation Browser");
    ImGui::Checkbox("Show Only Compatible", &ActiveState->bShowOnlyCompatible);
    ImGui::Separator();

    const TArray<UAnimSequence*>& AnimsToShow = ActiveState->bShowOnlyCompatible
        ? ActiveState->CompatibleAnimations
        : UResourceManager::GetInstance().GetAnimations();

    if (AnimsToShow.IsEmpty())
    {
        ImGui::Text(ActiveState->bShowOnlyCompatible
            ? "No compatible animations found."
            : "No animations loaded in ResourceManager.");
        return;
    }

    if (ImGui::BeginListBox("##AnimBrowser", ImVec2(-1, -1)))
    {
        for (UAnimSequence* Anim : AnimsToShow)
        {
            if (!Anim)  continue;

            bool isSelected = (ActiveState->CurrentAnimation == Anim);
            if (ImGui::Selectable(Anim->GetName().c_str(), isSelected))
            {
                if (ActiveState->PreviewActor)
                {
                    ActiveState->CurrentAnimation = Anim;
                    ActiveState->TotalTime = Anim->GetSequenceLength();
                    ActiveState->CurrentTime = 0.0f;
                    ActiveState->bIsPlaying = true;

                    USkeletalMeshComponent* MeshComp = ActiveState->PreviewActor->GetSkeletalMeshComponent();
                    if (MeshComp)
                    {
                        MeshComp->PlayAnimation(Anim, ActiveState->bIsLooping, ActiveState->PlaybackSpeed);
                    }
                }
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
}