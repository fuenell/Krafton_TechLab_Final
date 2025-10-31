#pragma once

#include <vector>

using FDelegateHandle = size_t;

template<typename... Args>
class TDelegate
{
public:
	using HandlerType = std::function<void(Args...)>;

	TDelegate() : NextHandle(1) {}

	FDelegateHandle Add(const HandlerType& Handler)
	{
		FDelegateHandle Handle = NextHandle++;
		Handlers.push_back({ Handle, Handler });
		return Handle;
	}

	template<typename T>
	FDelegateHandle AddDynamic(T* Instance, void(T::*Func)(Args...))
	{
		FDelegateHandle Handle = NextHandle++;
		Handlers.push_back({
			Handle,
			[=](Args... args) {
			(Instance->*Func)(args...); }
		});
		return Handle;
	}

	void Remove(FDelegateHandle Handle)
	{
		auto it = std::remove_if(Handlers.begin(), Handlers.end(),
		[&](const Entry& e)
		{
			return e.Handle == Handle;
		});
		Handlers.erase(it, Handlers.end());
	}

	void Broadcast(Args... args) {
		for (auto& Entry : Handlers) {
			if (Entry.Handler)
			{
				Entry.Handler(args...);
			}
		}
	}

private:
	struct Entry
	{
		FDelegateHandle Handle;
		HandlerType Handler;
	};

	std::vector<Entry> Handlers;
	FDelegateHandle NextHandle;
};

#define DECLARE_DELEGATE(Name, ...)				TDelegate<__VA_ARGS__> Name
#define DECLARE_DELEGATE_OneParam(Name, T1)		TDelegate<T1> Name
#define DECLARE_DELEGATE_TwoParam(Name, T1, T2)	TDelegate<T1, T2> Name
#define DECLARE_DYNAMIC_DELEGATE(Name, ...)		std::shared_ptr<TDelegate<__VA_ARGS__>> Name = std::make_shared<TDelegate<__VA_ARGS__>>();