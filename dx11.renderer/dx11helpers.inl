namespace
{
	using namespace Rococo;

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

		bool IsNull() const
		{
			return t == nullptr;
		}

		T* Detach() // Release reference and set internal pointer to null
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

	void ValidateDX11(HRESULT hr, const char* function, const char* file, int lineNumber)
	{
		if FAILED(hr)
		{
			Throw(hr, L"DX11 call failed. %S in %S line %d", function, file, lineNumber);
		}
	}
}

#define VALIDATEDX11(hr) ValidateDX11(hr, __FUNCTION__, __FILE__, __LINE__);