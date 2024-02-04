#include <rococo.abstract.editor.h>

namespace ANON
{
	struct Slate : Rococo::Abedit::IUIBlankSlateSupervisor
	{
		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Abedit::Internal
{
	IUIBlankSlateSupervisor* CreateBlankSlate()
	{
		return new ANON::Slate();
	}
}