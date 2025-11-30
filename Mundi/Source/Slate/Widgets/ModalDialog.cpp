#include "pch.h"
#include "ModalDialog.h"
#include "ImGui/imgui.h"

FModalDialog& FModalDialog::Get()
{
	static FModalDialog Instance;
	return Instance;
}

void FModalDialog::Show(
	const FString& Title,
	const FString& Message,
	EModalType Type,
	std::function<void(EModalResult)> OnResult)
{
	CurrentTitle = Title;
	CurrentMessage = Message;
	CurrentType = Type;
	ResultCallback = OnResult;
	LastResult = EModalResult::None;
	bShouldOpen = true;
}

void FModalDialog::Close()
{
	if (bIsOpen)
	{
		bIsOpen = false;
		ResultCallback = nullptr;
	}
}

void FModalDialog::Render()
{
	// 열려있지 않고 열기 예약도 없으면 렌더링 스킵
	if (!bIsOpen && !bShouldOpen)
	{
		return;
	}

	// 다음 프레임에 모달 열기 예약
	if (bShouldOpen)
	{
		ImGui::OpenPopup(CurrentTitle.c_str());
		bShouldOpen = false;
		bIsOpen = true;
	}

	// 모달 중앙 정렬
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	// 최소 너비 설정
	ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(500, FLT_MAX));

	if (ImGui::BeginPopupModal(CurrentTitle.c_str(), nullptr,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
	{
		// 메시지 표시
		ImGui::TextWrapped("%s", CurrentMessage.c_str());
		ImGui::Dummy(ImVec2(0, 10));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0, 5));

		// 버튼 레이아웃 (오른쪽 정렬)
		float buttonWidth = 80.0f;
		float spacing = 8.0f;

		EModalResult result = EModalResult::None;

		switch (CurrentType)
		{
		case EModalType::OK:
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - buttonWidth - 10);
			if (ImGui::Button("확인", ImVec2(buttonWidth, 0)))
			{
				result = EModalResult::OK;
			}
			break;

		case EModalType::OKCancel:
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (buttonWidth * 2 + spacing + 10));
			if (ImGui::Button("확인", ImVec2(buttonWidth, 0)))
			{
				result = EModalResult::OK;
			}
			ImGui::SameLine(0, spacing);
			if (ImGui::Button("취소", ImVec2(buttonWidth, 0)))
			{
				result = EModalResult::Cancel;
			}
			break;

		case EModalType::YesNo:
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (buttonWidth * 2 + spacing + 10));
			if (ImGui::Button("예", ImVec2(buttonWidth, 0)))
			{
				result = EModalResult::Yes;
			}
			ImGui::SameLine(0, spacing);
			if (ImGui::Button("아니오", ImVec2(buttonWidth, 0)))
			{
				result = EModalResult::No;
			}
			break;

		case EModalType::YesNoCancel:
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (buttonWidth * 3 + spacing * 2 + 10));
			if (ImGui::Button("예", ImVec2(buttonWidth, 0)))
			{
				result = EModalResult::Yes;
			}
			ImGui::SameLine(0, spacing);
			if (ImGui::Button("아니오", ImVec2(buttonWidth, 0)))
			{
				result = EModalResult::No;
			}
			ImGui::SameLine(0, spacing);
			if (ImGui::Button("취소", ImVec2(buttonWidth, 0)))
			{
				result = EModalResult::Cancel;
			}
			break;
		}

		// 결과 처리
		if (result != EModalResult::None)
		{
			LastResult = result;
			bIsOpen = false;
			ImGui::CloseCurrentPopup();

			if (ResultCallback)
			{
				auto callback = ResultCallback;
				ResultCallback = nullptr;
				callback(result);
			}
		}

		ImGui::EndPopup();
	}
	else
	{
		// 팝업이 닫혔으면 상태 초기화
		if (bIsOpen)
		{
			bIsOpen = false;
			// ESC로 닫힌 경우 Cancel로 처리
			if (LastResult == EModalResult::None)
			{
				LastResult = EModalResult::Cancel;
				if (ResultCallback)
				{
					auto callback = ResultCallback;
					ResultCallback = nullptr;
					callback(EModalResult::Cancel);
				}
			}
		}
	}
}
