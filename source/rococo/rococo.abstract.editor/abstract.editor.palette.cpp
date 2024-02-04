#include <rococo.abstract.editor.h>

namespace ANON
{
	struct Palette : Rococo::Abedit::IUIPaletteSupervisor
	{
		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Abedit::Internal
{
	IUIPaletteSupervisor* CreatePalette()
	{
		return new ANON::Palette();
	}
}