#pragma once
#include <functional>

/**
 * 모달 다이얼로그 결과
 */
enum class EModalResult : uint8
{
	None,       // 아직 응답 없음
	Yes,
	No,
	Cancel,
	OK
};

/**
 * 모달 다이얼로그 타입
 */
enum class EModalType : uint8
{
	OK,             // [확인]
	OKCancel,       // [확인] [취소]
	YesNo,          // [예] [아니오]
	YesNoCancel     // [예] [아니오] [취소]
};

/**
 * FModalDialog
 *
 * 재사용 가능한 ImGui 모달 다이얼로그 시스템
 * 비동기 방식 - Show()로 열고, Render()에서 결과 확인
 *
 * 사용 예시:
 * @code
 * FModalDialog::Get().Show(
 *     "제목",
 *     "메시지 내용",
 *     EModalType::YesNoCancel,
 *     [](EModalResult Result) {
 *         if (Result == EModalResult::Yes) { ... }
 *     }
 * );
 * @endcode
 */
class FModalDialog
{
public:
	// 싱글톤 접근
	static FModalDialog& Get();

	/**
	 * 모달 다이얼로그 열기
	 * @param Title 다이얼로그 제목
	 * @param Message 표시할 메시지
	 * @param Type 버튼 타입
	 * @param OnResult 결과 콜백 (optional)
	 */
	void Show(
		const FString& Title,
		const FString& Message,
		EModalType Type = EModalType::OK,
		std::function<void(EModalResult)> OnResult = nullptr
	);

	/**
	 * 모달 렌더링 (매 프레임 호출 필요)
	 * 일반적으로 SlateManager에서 호출
	 */
	void Render();

	/**
	 * 현재 모달이 열려있는지 확인
	 */
	bool IsOpen() const { return bIsOpen; }

	/**
	 * 마지막 결과 가져오기 (폴링 방식용)
	 */
	EModalResult GetLastResult() const { return LastResult; }

	/**
	 * 모달 닫기 (강제)
	 */
	void Close();

private:
	FModalDialog() = default;
	~FModalDialog() = default;

	// 복사/이동 방지
	FModalDialog(const FModalDialog&) = delete;
	FModalDialog& operator=(const FModalDialog&) = delete;

	bool bIsOpen = false;
	bool bShouldOpen = false;  // 다음 프레임에 열기
	FString CurrentTitle;
	FString CurrentMessage;
	EModalType CurrentType = EModalType::OK;
	std::function<void(EModalResult)> ResultCallback;
	EModalResult LastResult = EModalResult::None;
};
