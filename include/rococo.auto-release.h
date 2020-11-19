#pragma once

// AutoRelease for use with Microsoft COM

namespace Rococo
{
	template<class T> class AutoRelease
	{
	private:
		T* t;

	public:
		AutoRelease() : t(nullptr)
		{
		}

		AutoRelease& operator = (T* _t)
		{
			if (t) t->Release();
			t = _t;
			return *this;
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