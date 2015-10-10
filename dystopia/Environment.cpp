#include <rococo.types.h>
#include "dystopia.h"
#include <rococo.io.h>

namespace Dystopia
{
	IOS& GetOS(Environment& e)
	{
		return e.installation.OS();
	}
}