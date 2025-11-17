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

    LeftPanelRatio = 0.2f;   // 25% of width
    RightPanelRatio = 0.2f;  // 25% of width
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
        return;

    // Parent detachable window (movable, top-level) with solid background
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;

    // Initial placement (first frame only)
    if (!bInitialPlacementDone)
    {
        ImGui::SetNextWindowPos(ImVec2(Rect.Left, Rect.Top));
        ImGui::SetNextWindowSize(ImVec2(Rect.GetWidth(), Rect.GetHeight()));
        bInitialPlacementDone = true;
    }
    // Request focus for this frame
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

        //=====================================================
        // Tab Bar : Render tab bar and switch active state
        //=====================================================
        RenderTabBar();

        //===============================
        // Update window rect
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

        //======================================
        // Top panels (Left / Center / Right)
        //======================================

        // Remove spacing between panels
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        // +-+-+ Left panel +-+-+
        // : Asset Browser & Bone Hierarchy
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar();
        RenderLeftPanel(leftWidth);
        ImGui::EndChild();

        ImGui::SameLine(0, 0); // No spacing between panels

        // +-+-+ Center panel +-+-+
        // : draw with border to see the viewport area
        ImGui::BeginChild("CenterPanel", ImVec2(centerWidth, totalHeight), true, ImGuiWindowFlags_NoScrollbar);
        RenderCenterPanel();
        ImGui::EndChild();

        ImGui::SameLine(0, 0); // No spacing between panels

        // +-+-+ Right panel +-+-+
        // : Bone Properties
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
        ImGui::EndChild();
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
    float FinalPlayRate = ActiveState->bReversePlay ? -ActiveState->PlaybackSpeed : ActiveState->PlaybackSpeed;
    SingleAnimInstance->SetPlayRate(FinalPlayRate);
    SingleAnimInstance->SetLooping(ActiveState->bIsLooping);

    // Control time slider (UI to Engine)
    // Update the component's animation position only when the user is dragging the slider
    if (ActiveState->bIsScrubbing)
    {
        MeshComp->PlayAnimation(ActiveState->CurrentAnimation, ActiveState->bIsLooping, 0.0f);
        MeshComp->SetAnimationPosition(ActiveState->CurrentTime);
        ActiveState->bBoneLinesDirty = true;
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

void SAnimationViewerWindow::RenderRightPanel()
{
    if (!ActiveState)   return;

    if (ActiveState->SelectedNotify.IsValid())
    {
        RenderNotifyProperties();
    }
    else
    {
        SViewerWindow::RenderRightPanel();
    }
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

void SAnimationViewerWindow::RenderNotifyProperties()
{
    // Panel header
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.35f, 0.25f, 0.50f, 0.8f));
    ImGui::Text("Notify Properties");
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    ImGui::Indent(8.0f);
    ImGui::Text("Notify Properties");
    ImGui::Unindent(8.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // 선택된 노티파이에 대한 참조 가져오기
    FAnimNotifyEvent& notify = ActiveState->NotifyTracks[ActiveState->SelectedNotify.TrackIndex].Notifies[ActiveState->SelectedNotify.NotifyIndex];

    // 이름 편집 (FName은 직접 수정이 어려우므로 char 버퍼 사용)
    char nameBuffer[128];
    strcpy_s(nameBuffer, sizeof(nameBuffer), notify.NotifyName.ToString().c_str());
    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        notify.NotifyName = FName(nameBuffer);
    }

    // 시간 및 기간 편집
    ImGui::DragFloat("Trigger Time", &notify.TriggerTime, 0.01f, 0.0f, ActiveState->TotalTime, "%.2f s");
    ImGui::DragFloat("Duration", &notify.Duration, 0.01f, 0.0f, ActiveState->TotalTime, "%.2f s");

    // 색상 편집
    ImGui::ColorEdit4("Color", (float*)&notify.Color);
}

void SAnimationViewerWindow::AnimJumpToStart()
{
    if (!ActiveState || !ActiveState->PreviewActor) return;

    ActiveState->bIsPlaying = false;
    ActiveState->CurrentTime = 0.0f;

    if (USkeletalMeshComponent* MeshComp = ActiveState->PreviewActor->GetSkeletalMeshComponent())
    {
        MeshComp->PlayAnimation(ActiveState->CurrentAnimation, ActiveState->bIsLooping, 0.0f);
        MeshComp->SetAnimationPosition(ActiveState->CurrentTime);
        MeshComp->TickComponent(0.0f);
    }
    ActiveState->bBoneLinesDirty = true;
}

void SAnimationViewerWindow::AnimJumpToEnd()
{
    if (!ActiveState || !ActiveState->PreviewActor) return;

    ActiveState->bIsPlaying = false;
    ActiveState->CurrentTime = ActiveState->TotalTime;

    if (USkeletalMeshComponent* MeshComp = ActiveState->PreviewActor->GetSkeletalMeshComponent())
    {
        MeshComp->PlayAnimation(ActiveState->CurrentAnimation, ActiveState->bIsLooping, 0.0f);
        MeshComp->SetAnimationPosition(ActiveState->CurrentTime);
        MeshComp->TickComponent(0.0f);
    }
    ActiveState->bBoneLinesDirty = true;
}

void SAnimationViewerWindow::AnimStep(bool bForward)
{
    if (!ActiveState || !ActiveState->PreviewActor) return;

    ActiveState->bIsPlaying = false;
    const float FrameTime = 1.0f / 30.0f;

    if (bForward)
    {
        ActiveState->CurrentTime = FMath::Min(ActiveState->TotalTime, ActiveState->CurrentTime + FrameTime);
    }
    else
    {
        ActiveState->CurrentTime = FMath::Max(0.0f, ActiveState->CurrentTime - FrameTime);
    }

    if (USkeletalMeshComponent* MeshComp = ActiveState->PreviewActor->GetSkeletalMeshComponent())
    {
        MeshComp->PlayAnimation(ActiveState->CurrentAnimation, ActiveState->bIsLooping, 0.0f);
        MeshComp->SetAnimationPosition(ActiveState->CurrentTime);
    }
}

void SAnimationViewerWindow::RenderTabBar()
{
    if (!ImGui::BeginTabBar("SkeletalViewerTabs",
        ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable))
        return;

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

void SAnimationViewerWindow::RenderCenterPanel()
{
    float contentHeight = ImGui::GetContentRegionAvail().y;
    float itemSpacingY = ImGui::GetStyle().ItemSpacing.y;
    float viewportHeight = (contentHeight - itemSpacingY) * 0.75f;
    float timelineHeight = (contentHeight - itemSpacingY) * 0.25f;
    float innerWidth = ImGui::GetContentRegionAvail().x;
    
    RenderViewportArea(innerWidth, viewportHeight);
    RenderTimelineArea(innerWidth, timelineHeight);
}

void SAnimationViewerWindow::RenderViewportArea(float width, float height)
{
    // Remove all padding, spacing, rounding
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::BeginChild("ViewportArea", ImVec2(width, height), false,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();

    ImVec2 Pos = ImGui::GetCursorScreenPos();      // inner top-left
    ImVec2 Size = ImGui::GetContentRegionAvail();  // usable area

    CenterRect.Left   = Pos.x;
    CenterRect.Top    = Pos.y;
    CenterRect.Right  = Pos.x + Size.x;
    CenterRect.Bottom = Pos.y + Size.y;
    CenterRect.UpdateMinMax();

    ImGui::EndChild();
}

void SAnimationViewerWindow::RenderTimelineArea(float width, float height)
{
    ImGui::BeginChild("TimelineArea", ImVec2(width, height), true);

    RenderTimelineControls();
    ImGui::Separator();
    ImGui::Spacing();

    RenderTrackAndGridLayout();
    
    ImGui::EndChild();  // TimelineGridSmall
}

void SAnimationViewerWindow::RenderTimelineControls()
{
    ImGui::Spacing();

    if (ImGui::Button("|<<")) { AnimJumpToStart(); }
    ImGui::SameLine();
    if (ImGui::Button("<")) { AnimStep(false); }
    ImGui::SameLine();

    if (ActiveState->bIsPlaying)
    {
        if (ImGui::Button("Pause")) { ActiveState->bIsPlaying = false; }
    }
    else
    {
        if (ImGui::Button("Play")) { ActiveState->bIsPlaying = true; }
    }

    ImGui::SameLine();
    if (ImGui::Button(">")) { AnimStep(true); }
    ImGui::SameLine();
    if (ImGui::Button(">>|")) { AnimJumpToEnd(); }
    ImGui::SameLine();

    bool bHighlighted = ActiveState->bReversePlay;
    if (bHighlighted)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
    }
    if (ImGui::Button("Reverse"))
    {
        ActiveState->bReversePlay = !ActiveState->bReversePlay;
    }
    if (bHighlighted)
    {
        ImGui::PopStyleColor(2);
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
}

void SAnimationViewerWindow::RenderTrackAndGridLayout()
{
    ImGui::BeginChild("TrackLayout", ImVec2(0, 0), false);

    float leftWidth = 130.0f;
    float rightWidth = ImGui::GetContentRegionAvail().x - leftWidth;

    const float RowHeight = 24.0f;
    const float HeaderHeight = 24.0f;

    TArray<FString> LeftRows;
    BuildLeftRows(LeftRows);     // Notifies(Tracks) / Curves / Attributes

    TArray<int> RowToNotifyIndex;
    BuildRowToNotifyIndex(LeftRows, RowToNotifyIndex);

    RenderLeftTrackList(leftWidth, RowHeight, HeaderHeight, LeftRows, RowToNotifyIndex);
    ImGui::SameLine();

    RenderRightTimeline(rightWidth, RowHeight, HeaderHeight, LeftRows, RowToNotifyIndex);
    ImGui::EndChild();
}

void SAnimationViewerWindow::RenderLeftTrackList(float width, float RowHeight, float HeaderHeight, const TArray<FString>& LeftRows, const TArray<int>& RowToNotifyIndex)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("TrackListSmall", ImVec2(width, 0), true);
    ImGui::Dummy(ImVec2(0, HeaderHeight));
    ImGui::PopStyleVar();

    int RowCount = LeftRows.size();
    ImDrawList* DL = ImGui::GetWindowDrawList();

    // Row Rendering Loop
    for (int i = 0; i < RowCount; i++)
    {
        ImVec2 rowPos = ImGui::GetCursorScreenPos();

        ImGui::InvisibleButton(
            ("TrackListRowBtn_" + std::to_string(i)).c_str(),
            ImVec2(width - 2, RowHeight)
        );

        // ============================
        // 0번 Row (Notifies) — 폴더블 UI
        // ============================
        if (i == 0)
        {
            const char* arrow = ActiveState->bFoldNotifies ? "▶" : "▼";

            DL->AddText(
                ImVec2(rowPos.x + 4, rowPos.y + 4),
                IM_COL32(255, 255, 255, 255),
                arrow
            );

            DL->AddText(
                ImVec2(rowPos.x + 20, rowPos.y + 4),
                IM_COL32(230, 230, 230, 255),
                "Notifies"
            );

            if (ImGui::IsItemClicked())
            {
                ActiveState->bFoldNotifies = !ActiveState->bFoldNotifies;
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                ImGui::OpenPopup("NotifiesContextMenu");
        }
        else
        {
            float indent = (RowToNotifyIndex[i] != -1) ? 16.0f : 0.0f;

            // General Row (Track, Curves, Attributes)
            DL->AddText(
                ImVec2(rowPos.x + 4 + indent, rowPos.y + 4),
                IM_COL32(230, 230, 230, 255),
                LeftRows[i].c_str()
            );
        }

        // Divider
        DL->AddLine(
            ImVec2(rowPos.x, rowPos.y + RowHeight),
            ImVec2(rowPos.x + width, rowPos.y + RowHeight),
            IM_COL32(80, 80, 80, 200)
        );
    }

    // Popup must be OUTSIDE the loop
    if (ImGui::BeginPopup("NotifiesContextMenu"))
    {
        if (ImGui::MenuItem("Add Notify Track"))
        {
            int idx = ActiveState->NotifyTracks.size();
            ActiveState->NotifyTracks.push_back(FNotifyTrack("Track " + std::to_string(idx)));
        }
        ImGui::EndPopup();
    }

    ImGui::EndChild();
}

void SAnimationViewerWindow::RenderRightTimeline(float width, float RowHeight, float HeaderHeight, const TArray<FString>& LeftRows, const TArray<int>& RowToNotifyIndex)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("TimelineGridSmall", ImVec2(width, 0), true);
    ImGui::PopStyleVar();

    RenderTimelineHeader(HeaderHeight);
    RenderTimelineGridBody(RowHeight, LeftRows, RowToNotifyIndex);

    ImGui::EndChild();
}

void SAnimationViewerWindow::RenderTimelineHeader(float height)
{
    ImGui::BeginChild("TimelineHeader", ImVec2(0, height), false, ImGuiWindowFlags_NoScrollbar);

    ImDrawList* DL = ImGui::GetWindowDrawList();
    ImVec2 headerPos = ImGui::GetCursorScreenPos();
    float headerWidth = ImGui::GetContentRegionAvail().x;

    // Background
    DL->AddRectFilled(headerPos,
        ImVec2(headerPos.x + headerWidth, headerPos.y + height),
        IM_COL32(30, 30, 30, 255));

    // Frame markers
    const int frameCount = 50;
    for (int f = 0; f < frameCount; f++)
    {
        float x = headerPos.x + (headerWidth / frameCount) * f;
        DL->AddLine(ImVec2(x, headerPos.y), ImVec2(x, headerPos.y + height),
            IM_COL32(80, 80, 80, 180));

        char buf[16];
        sprintf_s(buf, "%d", f);
        DL->AddText(ImVec2(x + 2, headerPos.y + 4), IM_COL32(200, 200, 200, 255), buf);
    }

    // Playhead handle
    float norm = ActiveState->TotalTime > 0 ? ActiveState->CurrentTime / ActiveState->TotalTime : 0.0f;
    float playheadX = headerPos.x + norm * headerWidth;
    DL->AddTriangleFilled(
        ImVec2(playheadX - 6, headerPos.y),
        ImVec2(playheadX + 6, headerPos.y),
        ImVec2(playheadX, headerPos.y + 10),
        IM_COL32(255, 80, 80, 255)
    );

    // Hitbox
    ImGui::SetCursorScreenPos(ImVec2(playheadX - 8, headerPos.y));
    ImGui::InvisibleButton("UnifiedPlayheadHandle", ImVec2(16, height));

    bool draggingHeader = ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left);
    if (draggingHeader)
    {
        float mx = std::clamp(ImGui::GetIO().MousePos.x, headerPos.x, headerPos.x + headerWidth);
        float newNorm = (mx - headerPos.x) / headerWidth;
        ActiveState->CurrentTime = newNorm * ActiveState->TotalTime;

        ActiveState->bIsScrubbing = true;

        if (auto* mesh = ActiveState->PreviewActor->GetSkeletalMeshComponent())
        {
            mesh->SetAnimationPosition(ActiveState->CurrentTime);
            mesh->TickComponent(0.f);
        }
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        ActiveState->bIsScrubbing = false;

    ImGui::EndChild(); // TimelineHeader
}

void SAnimationViewerWindow::RenderTimelineGridBody(float RowHeight, const TArray<FString>& LeftRows, const TArray<int>& RowToNotifyIndex)
{
    ImGui::BeginChild("TimelineGridBody", ImVec2(0, 0), false);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 gridOrigin = ImGui::GetCursorScreenPos();
    ImVec2 gridAvail = ImGui::GetContentRegionAvail();

    const int RowCount = LeftRows.size();
    float TotalHeight = RowCount * RowHeight;

    draw->AddRectFilled(
        gridOrigin,
        ImVec2(gridOrigin.x + gridAvail.x, gridOrigin.y + TotalHeight),
        IM_COL32(40, 40, 40, 255)
    );

    // Frame columns
    const int frameCount = 50;
    for (int i = 0; i < frameCount; i++)
    {
        float x = gridOrigin.x + (float)i * (gridAvail.x / frameCount);
        draw->AddLine(
            ImVec2(x, gridOrigin.y),
            ImVec2(x, gridOrigin.y + TotalHeight),
            IM_COL32(60, 60, 60, 160)
        );
    }

    //-----------------------------
    // Track Rows + Right-Click Menu
    //-----------------------------
    for (int row = 0; row < RowCount; row++)
    {
        float y0 = gridOrigin.y + row * RowHeight;
        float y1 = y0 + RowHeight;

        // Row bottom line
        draw->AddLine(
            ImVec2(gridOrigin.x, y1),
            ImVec2(gridOrigin.x + gridAvail.x, y1),
            IM_COL32(90, 90, 90, 160)
        );

        // ============================
        //  ROW → NotifyTrack 매핑 사용
        // ============================
        int NotifyIndex = RowToNotifyIndex[row];

        // Draw Notify Marker
        if (NotifyIndex != -1 && ActiveState->TotalTime > 0.0f)
        {
            FNotifyTrack& Track = ActiveState->NotifyTracks[NotifyIndex];

            for (int i = 0; i < Track.Notifies.size(); ++i)
            {
                const FAnimNotifyEvent& Notify = Track.Notifies[i];

                float NotifyStartX = gridOrigin.x +
                    (Notify.TriggerTime / ActiveState->TotalTime) * gridAvail.x;

                float NotifyEndX = NotifyStartX;
                if (Notify.Duration > 0.0f)
                {
                    float endNorm = (Notify.TriggerTime + Notify.Duration) / ActiveState->TotalTime;
                    NotifyEndX = gridOrigin.x + endNorm * gridAvail.x;
                }

                // Y center
                float midY = (y0 + y1) * 0.5f;

                //--- Diamond Marker ---
                float s = 5.0f;
                ImVec2 d0(NotifyStartX, midY - s);
                ImVec2 d1(NotifyStartX + s, midY);
                ImVec2 d2(NotifyStartX, midY + s);
                ImVec2 d3(NotifyStartX - s, midY);

                ImU32 fillColor = IM_COL32(160, 130, 220, 255);  // 보라 계열
                ImU32 fillHover = IM_COL32(200, 170, 255, 255);  // hover brighter
                ImU32 barColor = IM_COL32(110, 90, 190, 180);

                //--- Hitbox ---
                ImVec2 hbMin(NotifyStartX - 8, midY - 8);
                ImVec2 hbSize(16, 16);
                ImGui::SetCursorScreenPos(hbMin);
                ImGui::InvisibleButton(("NotifyHit_" + std::to_string(row) + "_" + std::to_string(i)).c_str(), hbSize);

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    ActiveState->SelectedNotify.TrackIndex = NotifyIndex;
                    ActiveState->SelectedNotify.NotifyIndex = i;
                    ActiveState->SelectedBoneIndex = -1;
                }

                bool hovered = ImGui::IsItemHovered();

                //--- Duration Bar (if duration > 0) ---
                if (NotifyEndX > NotifyStartX)
                {
                    draw->AddRectFilled(
                        ImVec2(NotifyStartX, midY - 4),
                        ImVec2(NotifyEndX, midY + 4),
                        barColor,
                        2.0f
                    );
                }

                //--- Draw Diamond ---
                draw->AddQuadFilled(
                    d0, d1, d2, d3,
                    hovered ? fillHover : fillColor
                );

                //--- Text (aligned inside the box or right of diamond) ---
                std::string label = Notify.NotifyName.ToString();
                float textX = (NotifyEndX > NotifyStartX) ? (NotifyStartX + 6) : (NotifyStartX + 10);

                draw->AddText(
                    ImVec2(textX, midY - 7),
                    IM_COL32(230, 230, 230, 255),
                    label.c_str()
                );

                //-----------------------------------------
                // Notify Popup
                //-----------------------------------------
                if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                    ImGui::OpenPopup(("NotifyPopup_" + std::to_string(row) + "_" + std::to_string(i)).c_str());

                if (ImGui::BeginPopup(("NotifyPopup_" + std::to_string(row) + "_" + std::to_string(i)).c_str()))
                {
                    if (ImGui::MenuItem("Delete"))
                        Track.Notifies.erase(Track.Notifies.begin() + i);

                    if (ImGui::MenuItem("Add Duration +0.2s"))
                        Track.Notifies[i].Duration += 0.2f;

                    ImGui::EndPopup();
                }

                //-----------------------------------------
                // Drag to move Notify
                //-----------------------------------------
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    float mouseX = ImGui::GetIO().MousePos.x;
                    mouseX = std::clamp(mouseX, gridOrigin.x, gridOrigin.x + gridAvail.x);

                    float norm = (mouseX - gridOrigin.x) / gridAvail.x;
                    Track.Notifies[i].TriggerTime = norm * ActiveState->TotalTime;
                }
            }
        }

        // Hitbox
        ImGui::SetCursorScreenPos(ImVec2(gridOrigin.x, y0));
        ImGui::InvisibleButton(
            ("GridRowBtn_" + std::to_string(row)).c_str(),
            ImVec2(gridAvail.x, RowHeight)
        );

        // If right-clicked anywhere in the row → Add Notify popup
        if (ImGui::BeginPopupContextItem(("RowPopup_" + std::to_string(row)).c_str()))
        {
            if (ImGui::MenuItem("Add Notify"))
            {
                float mouseX = ImGui::GetMousePos().x - gridOrigin.x;
                float newTime = (mouseX / gridAvail.x) * ActiveState->TotalTime;
                newTime = std::clamp(newTime, 0.0f, ActiveState->TotalTime);

                FAnimNotifyEvent newNotify;
                newNotify.TriggerTime = newTime;
                newNotify.Duration = 0.0f;

                int notifyCount = 0;
                for (const auto& track : ActiveState->NotifyTracks)
                    notifyCount += track.Notifies.size();

                newNotify.NotifyName = FName("NewNotify_" + std::to_string(notifyCount + 1));

                ActiveState->NotifyTracks[NotifyIndex].Notifies.Add(newNotify);
            }

            ImGui::EndPopup();
        }
    }

    //=========================
    // Playhead / Scrubbing
    //=========================

    float xMin = gridOrigin.x;
    float xMax = gridOrigin.x + gridAvail.x;

    float playheadX_Grid = xMin;
    if (ActiveState->TotalTime > 0)
        playheadX_Grid = xMin + (ActiveState->CurrentTime / ActiveState->TotalTime) * (xMax - xMin);

    // red line
    draw->AddLine(
        ImVec2(playheadX_Grid, gridOrigin.y),
        ImVec2(playheadX_Grid, gridOrigin.y + TotalHeight),
        IM_COL32(255, 40, 40, 255), 2.0f
    );

    // handle triangle
    float handleY = gridOrigin.y - 8;
    draw->AddTriangleFilled(
        ImVec2(playheadX_Grid - 6, handleY),
        ImVec2(playheadX_Grid + 6, handleY),
        ImVec2(playheadX_Grid, handleY + 10),
        IM_COL32(255, 40, 40, 255)
    );

    // hitbox
    ImGui::SetCursorScreenPos(ImVec2(playheadX_Grid - 8, gridOrigin.y - 12));
    ImGui::InvisibleButton("UnifiedPlayheadHandle", ImVec2(16, TotalHeight + 12));

    bool dragging = ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left);

    if (dragging)
    {
        ActiveState->bIsScrubbing = true;

        float mouseX = ImGui::GetIO().MousePos.x;
        mouseX = std::clamp(mouseX, xMin, xMax);

        float norm = (mouseX - xMin) / (xMax - xMin);
        ActiveState->CurrentTime = norm * ActiveState->TotalTime;

        if (auto* MeshComp = ActiveState->PreviewActor->GetSkeletalMeshComponent())
        {
            MeshComp->SetAnimationPosition(ActiveState->CurrentTime);
            MeshComp->TickComponent(0.f);
        }
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        ActiveState->bIsScrubbing = false;
    }

    ImGui::EndChild(); // TimelineHeader
}

void SAnimationViewerWindow::BuildLeftRows(TArray<FString>& OutRows)
{
    OutRows.clear();
    OutRows.push_back("Notifies");

    // Notify Tracks only if unfolded
    if (!ActiveState->bFoldNotifies)
    {
        for (auto& Track : ActiveState->NotifyTracks)
            OutRows.push_back(Track.Name);
    }

    OutRows.push_back("Curves");
    OutRows.push_back("Attributes");
}

void SAnimationViewerWindow::BuildRowToNotifyIndex(const TArray<FString>& InRows, TArray<int>& OutMapping)
{
    OutMapping.clear();
    OutMapping.reserve(InRows.size());

    // row 0 = Notifies header
    OutMapping.push_back(-1);

    // The order of NotifyTrack rows is immediately after the "Notifies" header.
    // i.e., LeftRows:
    // [0] Notifies
    // [1..N] NotifyTrack (only when expanded)
    // [... ] Curves
    // [... ] Attributes

    // Notify Tracks only if unfolded
    if (!ActiveState->bFoldNotifies) {
        for (int i = 0; i < ActiveState->NotifyTracks.size(); ++i)
            OutMapping.push_back(i);
    }

    OutMapping.push_back(-1);     // Curves
    OutMapping.push_back(-1);     // Attributes
}
