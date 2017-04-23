namespace
{
	using namespace Rococo;

	struct DX11Exception : public IException
	{
		rchar msg[256];
		int32 errorCode;

		virtual cstr Message() const
		{
			return msg;
		}

		virtual int32 ErrorCode() const
		{
			return errorCode;
		}
	};
}

namespace Rococo
{
	void Throw(int32 errorCode, cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		DX11Exception ex;
		SafeVFormat(ex.msg, _TRUNCATE, format, args);

		ex.errorCode = errorCode;

		if (IsDebuggerPresent())
		{
			__debugbreak();
		}

		throw ex;
	}
}

namespace
{
	using namespace Rococo;

	struct RGBA
	{
		float red;
		float green;
		float blue;
		float alpha;

		RGBA(float _r, float _g, float _b, float _a = 1.0f) : red(_r), green(_g), blue(_b), alpha(_a) {}
	};

	template<class T> class AutoRelease
	{
	private:
		T* t;

	public:
		AutoRelease() : t(nullptr)
		{
		}

		AutoRelease(T* _t) : t(_t)
		{
		}

		~AutoRelease()
		{
			if (t) t->Release();
		}
		
		operator T** ()
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
