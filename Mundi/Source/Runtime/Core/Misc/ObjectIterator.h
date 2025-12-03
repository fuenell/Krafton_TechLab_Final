#pragma once

#include "ObjectFactory.h"

template<typename TObject>
class TObjectIterator
{
public:
	TObjectIterator()
	{
		// TMap의 begin()으로 시작
		MapIterator = GUObjectArray.begin();
		AdvanceToNextValidObject();
	}

	// 다음 객체로 이동
	TObjectIterator& operator++()
	{
		if (MapIterator != GUObjectArray.end())
		{
			++MapIterator;
			AdvanceToNextValidObject();
		}
		return *this;
	}

	// 현재 객체에 접근
	TObject* operator*() const
	{
		if (MapIterator != GUObjectArray.end())
		{
			return static_cast<TObject*>(MapIterator->second);
		}
		return nullptr;
	}

	// 현재 객체에 접근 (포인터 연산자)
	TObject* operator->() const
	{
		return **this;
	}

	// 비교 연산자
	bool operator!=(const TObjectIterator& Other) const
	{
		return MapIterator != Other.MapIterator;
	}

	// bool 변환 연산자
	explicit operator bool() const
	{
		return MapIterator != GUObjectArray.end();
	}

private:
	// 현재 위치부터 다음 유효 객체를 찾는 헬퍼 함수
	void AdvanceToNextValidObject()
	{
		while (MapIterator != GUObjectArray.end())
		{
			UObject* Object = MapIterator->second;
			// 현재 객체가 유효하고, TObject 타입이면 검색 종료
			if (Object && Object->IsA<TObject>())
			{
				break;
			}
			++MapIterator;
		}
	}

private:
	typename TMap<uint32, UObject*>::iterator MapIterator;
};
