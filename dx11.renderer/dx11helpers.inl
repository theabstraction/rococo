#include <rococo.auto-release.h>

namespace
{
	using namespace Rococo;

	void ValidateDX11(HRESULT hr, const char* badcall, const char* function, const char* file, int lineNumber)
	{
		if FAILED(hr)
		{
			Throw(hr, "DX11 call failed: %s.\n%s in\n %s line %d.", badcall, function, file, lineNumber);
		}
	}
}

#define VALIDATEDX11(hr) ValidateDX11(hr, #hr, __FUNCTION__, __FILE__, __LINE__);
