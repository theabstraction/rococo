#include <rococo.abstract.editor.h>

namespace ANON
{
	struct Properties : Rococo::Abedit::IUIPropertiesSupervisor
	{
		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Abedit::Internal
{
	IUIPropertiesSupervisor* CreateProperties()
	{
		return new ANON::Properties();
	}
}