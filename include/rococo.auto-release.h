#pragma once

// AutoRelease for use with Microsoft COM

namespace Rococo
{
	template<class T> void AddRef(T* t)
	{
		if (t) t->AddRef();
	}

	template<class T> class AutoRelease
	{
	private:
		T* t;

	public:
		AutoRelease() : t(nullptr)
		{
		}

		AutoRelease(AutoRelease<T>&& other)
		{
			t = other.t;
			other.t = nullptr;
		}

		AutoRelease<T>& operator = (const T* _t)
		{
			if (t != _t)
			{
				if (t) t->Release();
				t = const_cast<T*>(_t);
			}
			return *this;
		}

		AutoRelease(const AutoRelease<T>& src): t(src.t)
		{
			AddRef(t);
		}

		AutoRelease(T* _t) : t(_t)
		{
		}

		~AutoRelease()
		{
			if (t) t->Release();
		}

		T** operator & ()
		{
			return &t;
		}

		T& operator * ()
		{
			return *t;
		}

		T* operator -> ()
		{
			return t;
		}

		[[nodiscard]] bool IsNull() const
		{
			return t == nullptr;
		}

		operator bool() const
		{
			return !IsNull();
		}

		// Release reference and set internal pointer to null. You may need to AddRef first if you 
		// are capturing the reference for later use
		T* Detach()
		{
			T* result = t;
			if (t)
			{
				t->Release();
				t = nullptr;
			}
			return result;
		}

		operator T* ()
		{
			return t;
		}
	};
}