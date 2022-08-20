#pragma once

#include <rococo.types.h>

// Rococo namespace, implements fast-compile equivalent to std::forward and std::function, which are Rococo::Forward and Rococo::Function respectively

namespace Rococo
{
	template <class _Ty>
	struct RemoveReference<_Ty&>
	{
		using Type = _Ty;
	};

	template <class _Ty>
	struct RemoveReference<_Ty&&>
	{
		using Type = _Ty;
	};

	template <class _Ty>
	using TRemoveReference = typename RemoveReference<_Ty>::Type;

	template <class _Ty>
	[[nodiscard]] constexpr _Ty&& Forward(TRemoveReference<_Ty>& _Arg) noexcept
	{
		// forward an lvalue as either an lvalue or an rvalue
		return static_cast<_Ty&&>(_Arg);
	}

	template <class>
	inline constexpr bool IsLvalueReference = false; // determine whether type argument is an lvalue reference

	template <class _Ty>
	inline constexpr bool IsLvalueReference<_Ty&> = true;

	template <class _Ty>
	[[nodiscard]] constexpr _Ty&& Forward(TRemoveReference<_Ty>&& _Arg) noexcept
	{
		// forward an rvalue as an rvalue
		static_assert(!IsLvalueReference<_Ty>, "Not an L Value");
		return static_cast<_Ty&&>(_Arg);
	}

	// Comissary is a synonym for delegate, but not so frequently used, so we use this to avoid conflicts with other libraries
	template<typename RETURN_TYPE, typename ... ARGS>
	struct IComissary
	{
		virtual RETURN_TYPE Invoke(ARGS&& ...args) const = 0;
		virtual IComissary<RETURN_TYPE, ARGS...>* Clone() const = 0;

		virtual void CopyTo(char* destination) const = 0;
	};

	template<typename FUNCTOR, typename RETURN_TYPE, typename ... ARGS>
	struct ComissaryWithFunctor final : IComissary<RETURN_TYPE, ARGS...>
	{
		ComissaryWithFunctor(FUNCTOR f) : functor(f) {}

		RETURN_TYPE Invoke(ARGS&& ... args) const override
		{
			return functor(args...);
		}

		IComissary<RETURN_TYPE, ARGS...>* Clone() const override
		{
			ComissaryWithFunctor* clone = new ComissaryWithFunctor(*this);
			return clone;
		}

		void CopyTo(char* pDestinationBuffer) const override
		{
			new (pDestinationBuffer) ComissaryWithFunctor<FUNCTOR, RETURN_TYPE, ARGS...>(*this);
		}


		FUNCTOR functor;
	};

	template<int CAPACITY, typename TYPENAME>
	class StackComissary;

	template<int CAPACITY, class RETURN_TYPE, typename ... ARGS>
	class StackComissary<CAPACITY, RETURN_TYPE(ARGS...)>
	{
	private:
	public:
		alignas(8) char stackspace[CAPACITY];

		using IStackCommissary = IComissary<RETURN_TYPE, ARGS...>;

		IStackCommissary& Implementation() const { return *(IStackCommissary*)(stackspace); }

		StackComissary()
		{
			memset(stackspace, 0, CAPACITY);
		}

		template<typename FUNCTOR>
		StackComissary(FUNCTOR f)
		{
			static_assert(CAPACITY >= sizeof FUNCTOR, "StackComissary requires more stackspace to handle the delegate passed to it");
			new (stackspace) ComissaryWithFunctor<FUNCTOR, RETURN_TYPE, ARGS...>(f);
		}

		const bool IsBound() const
		{
			return *(void**)(stackspace) != nullptr;
		}

		RETURN_TYPE operator()(ARGS&&...args) const
		{
			return Invoke(Forward<ARGS>(args)...);
		}

		RETURN_TYPE Invoke(ARGS&&...args) const
		{
			return InvokeElseThrow(Forward<ARGS>(args)...);
		}

		// If the function is bound then invoke it, else return the default value of the RETURN_TYPE
		RETURN_TYPE InvokeElseDefault(ARGS&&...args) const
		{
			return IsBound() ? Implementation().Invoke(Forward<ARGS>(args)...) : RETURN_TYPE();
		}

		// If the function is bound then invoke it, else throw an exception
		RETURN_TYPE InvokeElseThrow(ARGS&&...args) const
		{
			if (IsBound())
			{
				return Implementation().Invoke(Forward<ARGS>(args)...);
			}
			else
			{
				Throw(0, "InvokeElseThrow: delegate not bound");
			} 
		}

		~StackComissary()
		{
			Implementation().~IStackCommissary();
		}
	};

	template<typename TYPENAME>
	using TinyStackFunction = StackComissary<32, TYPENAME>;

	template<typename TYPENAME>
	using SmallStackFunction = StackComissary<64, TYPENAME>;

	template<typename TYPENAME>
	using BigStackFunction = StackComissary<128, TYPENAME>;

	template<int OPTIMAL_SIZE, typename TYPENAME, typename ... ARGS>
	class ArbitraryFunction;

	// A delegate for a functor of arbitrary size. If the size is not greater than the supplied OPTIMIAL_SIZE argument, then the delegate is handled without heap allocation
	template<int OPTIMAL_SIZE, class RETURN_TYPE, typename ... ARGS>
	class ArbitraryFunction<OPTIMAL_SIZE, RETURN_TYPE(ARGS...)>
	{
	private:
	public:
		using IProxyCommissary = IComissary<RETURN_TYPE, ARGS...>;

		IProxyCommissary* implementation = nullptr;

		alignas(8) char stackspace[OPTIMAL_SIZE];

		template<typename FUNCTOR>
		ArbitraryFunction(FUNCTOR f)
		{
			if (sizeof ComissaryWithFunctor<FUNCTOR, RETURN_TYPE, ARGS...> > sizeof stackspace)
			{
				implementation = new ComissaryWithFunctor<FUNCTOR, RETURN_TYPE, ARGS...>(f);
			}
			else
			{
				new (stackspace) ComissaryWithFunctor<FUNCTOR, RETURN_TYPE, ARGS...>(f);
				implementation = (IProxyCommissary*)stackspace;
			}
		}

		ArbitraryFunction()
		{
		}

		const bool IsBound() const
		{
			return implementation != nullptr;
		}

		ArbitraryFunction& operator = (const ArbitraryFunction& source)
		{
			if (&source == this) return *this;

			if ((char*)(implementation) == stackspace)
			{
				implementation->~IProxyCommissary();
			}
			else
			{
				delete implementation;
			}

			if ((char*)source.implementation == source.stackspace)
			{
				source.implementation->CopyTo(stackspace);
				implementation = (IProxyCommissary*)stackspace;
			}
			else
			{
				implementation = source.implementation ? source.implementation->Clone() : nullptr;
			}

			return *this;
		}

		ArbitraryFunction(ArbitraryFunction&& source)
		{
			if (source.implementation == source.stackspace)
			{
				source.implementation->CopyTo(stackspace);
				implementation = (IProxyCommissary*)stackspace;
			}
			else
			{
				implementation = source.implementation;
				source.implementation = nullptr;
			}
		}

		ArbitraryFunction(const ArbitraryFunction& source)
		{
			if ((char*)source.implementation == source.stackspace)
			{
				source.implementation->CopyTo(stackspace);
				implementation = (IProxyCommissary*)stackspace;
			}
			else
			{
				implementation = source.implementation ? source.implementation->Clone() : nullptr;
			}
		}

		RETURN_TYPE operator()(ARGS&&...args) const
		{
			return implementation->Invoke(Forward<ARGS>(args)...);
		}

		RETURN_TYPE Invoke(ARGS...args) const
		{
			return InvokeElseThrow(Forward<ARGS>(args)...);
		}

		RETURN_TYPE InvokeElseDefault(ARGS&&...args) const
		{
			return implementation ? implementation->Invoke(Forward<ARGS>(args)...) : RETURN_TYPE();
		}

		RETURN_TYPE InvokeElseThrow(ARGS&&...args) const
		{
			if (implementation)
			{
				return implementation->Invoke(Forward<ARGS>(args)...);
			}
			else
			{
				Throw(0, "InvokeElseThrow: no implementation for delegate");
			}
		}

		~ArbitraryFunction()
		{
			if ((char*)(implementation) == stackspace)
			{
				implementation->~IProxyCommissary();
			}
			else
			{
				delete implementation;
			}
		}
	};

	// Provides a delegate for callbacks. If the size of the callback object is sufficiently small the callback is implemented on the stack, rather than the heap
	template<typename RETURNTYPE, typename ... ARGS>
	using Function = ArbitraryFunction<64, RETURNTYPE, ARGS ...>;
}

namespace Rococo::OS
{
	// Open a file and invokes a callback with a pointer to content. The file is closed at the point that the callback is invoked. 
	void LoadAsciiTextFile(Function<void(cstr)> callback, const wchar_t* filename);
}