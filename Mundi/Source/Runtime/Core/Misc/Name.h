#pragma once
// Name.h
#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include"UEContainer.h"
// ──────────────────────────────
// FNameEntry & Pool
// ──────────────────────────────
struct FNameEntry
{
    FString Display;    // 원문
    FString Comparison; // lower-case
};

class FNamePool
{
public:
    static uint32 Add(const FString& InStr);
    static const FNameEntry& Get(uint32 Index);

private:
    static TArray<FNameEntry> Entries; // 정의는 Name.cpp
    static std::unordered_map<FString, uint32> NameMap; // 빠른 검색용
};

// ──────────────────────────────
// FName
// ──────────────────────────────
struct FName
{
    uint32 DisplayIndex = -1;
    uint32 ComparisonIndex = -1;

    FName() = default;
    FName(const char* InStr) { Init(FString(InStr)); }
    FName(const FString& InStr) { Init(InStr); }

    void Init(const FString& InStr)
    {
        int32_t Index = FNamePool::Add(InStr);
        DisplayIndex = Index;
        ComparisonIndex = Index; // 필요시 다른 규칙 적용 가능
    }

    bool operator==(const FName& Other) const { return ComparisonIndex == Other.ComparisonIndex; }
    FString ToString() const { return FNamePool::Get(DisplayIndex).Display; }

    friend FName operator+(const FName& A, const FName& B)
    {
        return FName(A.ToString() + B.ToString());
    }

    friend FName operator+(const FName& A, const FString& B)
    {
        return FName(A.ToString() + B);
    }

    friend FName operator+(const FString& A, const FName& B)
    {
        return FName(A + B.ToString());
    }
};

// ============================================================================
// 경로 정규화 유틸리티 함수
// ============================================================================

/**
 * @brief 경로 문자열의 디렉토리 구분자를 모두 '/'로 통일합니다.
 * @details Windows/Unix 간 경로 구분자 차이로 인한 중복 리소스 로드를 방지합니다.
 *          예: "Data\\Textures\\image.png" -> "Data/Textures/image.png"
 * @param InPath 정규화할 경로 문자열
 * @return 정규화된 경로 문자열 ('/' 구분자 사용)
 */
inline FString NormalizePath(const FString& InPath)
{
	FString Result = InPath;
	std::replace(Result.begin(), Result.end(), '\\', '/');
	return Result;
}