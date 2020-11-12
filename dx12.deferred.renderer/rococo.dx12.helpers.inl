namespace ANON
{
	using namespace Rococo;

	void ValidateHR(HRESULT hr, const char* badcall, const char* function, const char* file, int lineNumber);
}

#define VALIDATE_HR(hr) ValidateHR(hr, #hr, __FUNCTION__, __FILE__, __LINE__);