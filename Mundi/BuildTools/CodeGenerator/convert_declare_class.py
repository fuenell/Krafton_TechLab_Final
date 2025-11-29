#!/usr/bin/env python3
"""
DECLARE_CLASS를 GENERATED_REFLECTION_BODY로 변환하는 스크립트

변환 내용:
1. #include "ClassName.generated.h" 추가
2. UCLASS() 매크로 추가
3. DECLARE_CLASS(ClassName, ParentClass) → GENERATED_REFLECTION_BODY() 변경
4. DECLARE_DUPLICATE(ClassName) 제거 (GENERATED_REFLECTION_BODY에 포함됨)
"""

import os
import re
import sys
from pathlib import Path

# 변환할 파일 목록 (DECLARE_CLASS를 사용하는 헤더 파일들)
TARGET_FILES = [
    "Source/Slate/Windows/PropertyWindow.h",
    "Source/Runtime/Engine/GameFramework/Level.h",
    "Source/Runtime/Engine/GameFramework/World.h",
    "Source/Slate/Windows/UIWindow.h",
    "Source/Slate/Windows/SceneWindow.h",
    "Source/Slate/Windows/ExperimentalFeatureWindow.h",
    "Source/Slate/Windows/ContentBrowserWindow.h",
    "Source/Slate/Windows/ControlPanelWindow.h",
    "Source/Slate/Windows/ConsoleWindow.h",
    "Source/Slate/Widgets/Widget.h",
    "Source/Slate/Widgets/SceneManagerWidget.h",
    "Source/Slate/Widgets/TargetActorTransformWidget.h",
    "Source/Slate/Widgets/MainToolbarWidget.h",
    "Source/Slate/Widgets/InputInformationWidget.h",
    "Source/Slate/Widgets/CurveEditorWidget.h",
    "Source/Slate/Widgets/ConsoleWidget.h",
    "Source/Slate/UIManager.h",
    "Source/Slate/SlateManager.h",
    "Source/Slate/ImGui/ImGuiHelper.h",
    "Source/Slate/GlobalConsole.h",
    "Source/Slate/Factory/UIWindowFactory.h",
    "Source/Runtime/Renderer/Shader.h",
    "Source/Runtime/Renderer/RenderManager.h",
    "Source/Runtime/Renderer/QuadManager.h",
    "Source/Runtime/Renderer/Material.h",
    "Source/Runtime/RHI/PipelineStateManager.h",
    "Source/Runtime/InputCore/InputManager.h",
    "Source/Runtime/Engine/Viewer/EditorAssetPreviewContext.h",
    "Source/Runtime/Engine/Spatial/WorldPartitionManager.h",
    "Source/Runtime/Engine/GameFramework/Camera/CameraModifierBase.h",
    "Source/Runtime/Engine/GameFramework/Camera/CamMod_Vignette.h",
    "Source/Runtime/Engine/GameFramework/Camera/CamMod_Shake.h",
    "Source/Runtime/Engine/GameFramework/Camera/CamMod_Fade.h",
    "Source/Runtime/Engine/GameFramework/Camera/CamMod_Gamma.h",
    "Source/Runtime/Engine/GameFramework/Camera/CamMod_LetterBox.h",
    "Source/Runtime/Engine/Components/LineComponent.h",
    "Source/Runtime/Engine/Components/BoneAnchorComponent.h",
    "Source/Runtime/Engine/Collision/CollisionManager.h",
    "Source/Runtime/Engine/Audio/Sound.h",
    "Source/Runtime/AssetManagement/Texture.h",
    "Source/Runtime/AssetManagement/StaticMesh.h",
    "Source/Runtime/AssetManagement/ResourceManager.h",
    "Source/Runtime/AssetManagement/SkeletalMesh.h",
    "Source/Runtime/AssetManagement/Quad.h",
    "Source/Runtime/AssetManagement/ResourceBase.h",
    "Source/Runtime/AssetManagement/MeshLoader.h",
    "Source/Runtime/AssetManagement/Line.h",
    "Source/Runtime/AssetManagement/LineDynamicMesh.h",
    "Source/Runtime/AssetManagement/DynamicMesh.h",
    "Source/Editor/SelectionManager.h",
    "Source/Editor/Grid/GridActor.h",
    "Source/Editor/Gizmo/GizmoRotateComponent.h",
    "Source/Editor/Gizmo/GizmoScaleComponent.h",
    "Source/Editor/Gizmo/GizmoArrowComponent.h",
    "Source/Editor/Gizmo/GizmoActor.h",
    "Source/Editor/FBXLoader.h",
    "Source/Editor/Clipboard/ClipboardManager.h",
]

# 제외할 파일 (Object.h, ObjectMacros.h는 기본 매크로 정의 파일)
EXCLUDE_FILES = [
    "Source/Runtime/Core/Object/Object.h",
    "Source/Runtime/Core/Object/ObjectMacros.h",
]


def extract_class_info(content):
    """
    DECLARE_CLASS(ClassName, ParentClass)에서 클래스명과 부모 클래스명 추출
    """
    pattern = r'DECLARE_CLASS\s*\(\s*(\w+)\s*,\s*(\w+)\s*\)'
    matches = re.findall(pattern, content)
    return matches  # [(ClassName, ParentClass), ...]


def get_class_definition_line(content, class_name):
    """
    클래스 정의 시작 줄을 찾음 (class ClassName : public ParentClass)
    """
    # class ClassName : public ParentClass 또는 class ClassName final : public ParentClass
    pattern = rf'(class\s+{class_name}\s*(?:final\s*)?\s*:\s*public\s+\w+)'
    match = re.search(pattern, content)
    return match


def convert_file(filepath, root_dir):
    """
    단일 파일 변환
    """
    full_path = os.path.join(root_dir, filepath)

    if not os.path.exists(full_path):
        print(f"[SKIP] File not found: {filepath}")
        return False

    with open(full_path, 'r', encoding='utf-8-sig') as f:
        content = f.read()

    # 이미 GENERATED_REFLECTION_BODY가 있으면 스킵
    if 'GENERATED_REFLECTION_BODY' in content:
        print(f"[SKIP] Already converted: {filepath}")
        return False

    # DECLARE_CLASS 정보 추출
    class_infos = extract_class_info(content)
    if not class_infos:
        print(f"[SKIP] No DECLARE_CLASS found: {filepath}")
        return False

    original_content = content

    for class_name, parent_class in class_infos:
        # 1. .generated.h include 추가 (클래스 정의 전에)
        generated_include = f'#include "{class_name}.generated.h"'

        # 이미 include가 있는지 확인
        if generated_include not in content:
            # 클래스 정의 찾기
            class_def_match = get_class_definition_line(content, class_name)
            if class_def_match:
                # UCLASS() 매크로와 함께 include 추가
                class_def = class_def_match.group(1)

                # 클래스 정의 바로 앞에 UCLASS()와 include 추가
                # 먼저 include를 파일 상단의 다른 include들 근처에 추가

                # 마지막 #include 찾기
                last_include_match = None
                for match in re.finditer(r'#include\s+[<"].*?[>"]', content):
                    last_include_match = match

                if last_include_match:
                    insert_pos = last_include_match.end()
                    content = content[:insert_pos] + f'\n{generated_include}' + content[insert_pos:]

                # 클래스 정의 앞에 UCLASS() 추가
                # 다시 클래스 정의 위치 찾기 (include 추가로 위치가 변경됨)
                class_def_match = get_class_definition_line(content, class_name)
                if class_def_match:
                    class_def_start = class_def_match.start()
                    # 앞에 공백/줄바꿈 확인
                    uclass_macro = f'UCLASS()\n'
                    content = content[:class_def_start] + uclass_macro + content[class_def_start:]

        # 2. DECLARE_CLASS → GENERATED_REFLECTION_BODY 변경
        declare_pattern = rf'DECLARE_CLASS\s*\(\s*{class_name}\s*,\s*{parent_class}\s*\)'
        content = re.sub(declare_pattern, 'GENERATED_REFLECTION_BODY()', content)

        # 3. DECLARE_DUPLICATE 제거 (GENERATED_REFLECTION_BODY에 포함됨)
        duplicate_pattern = rf'\s*DECLARE_DUPLICATE\s*\(\s*{class_name}\s*\)\s*'
        content = re.sub(duplicate_pattern, '\n', content)

    # 변경사항이 있으면 저장
    if content != original_content:
        with open(full_path, 'w', encoding='utf-8-sig') as f:
            f.write(content)
        print(f"[CONVERTED] {filepath}")
        for class_name, parent_class in class_infos:
            print(f"    - {class_name} : {parent_class}")
        return True

    return False


def main():
    # 프로젝트 루트 디렉토리 찾기
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(os.path.dirname(script_dir))  # BuildTools/CodeGenerator -> Mundi

    print(f"Project root: {root_dir}")
    print(f"Converting {len(TARGET_FILES)} files...")
    print("=" * 60)

    converted_count = 0
    skipped_count = 0

    for filepath in TARGET_FILES:
        if filepath in EXCLUDE_FILES:
            print(f"[EXCLUDE] {filepath}")
            skipped_count += 1
            continue

        if convert_file(filepath, root_dir):
            converted_count += 1
        else:
            skipped_count += 1

    print("=" * 60)
    print(f"Conversion complete!")
    print(f"  Converted: {converted_count}")
    print(f"  Skipped: {skipped_count}")
    print()
    print("Next steps:")
    print("  1. Run generate.py to create .generated.h files")
    print("  2. Build the project to verify changes")


if __name__ == "__main__":
    main()
