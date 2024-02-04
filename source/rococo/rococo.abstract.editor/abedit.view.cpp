#include <rococo.mvc.h>
#include <rococo.os.win32.h>
#include <rococo.abstract.editor.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Abedit;
using namespace Rococo::MVC;

namespace ANON
{
	struct AbstractEditor : Rococo::Abedit::IAbstractEditorSupervisor
	{
		IMVC_Host& host;

		AutoFree<IUIPaletteSupervisor> palette;
		AutoFree<IUIPropertiesSupervisor> properties;
		AutoFree<IUIBlankSlateSupervisor> slate;

		AbstractEditor(IMVC_Host& _host): host(_host)
		{
			palette = Internal::CreatePalette();
			properties = Internal::CreateProperties();
			slate = Internal::CreateBlankSlate();
		}

		void Free() override
		{
			delete this;
		}

		IUIPalette& Palette() override
		{
			return *palette;
		}

		IUIProperties& Properties() override
		{
			return *properties;
		}

		IUIBlankSlate& Slate() override
		{
			return *slate;
		}
	};

	struct AbstractEditor_MVC_View : IMVC_ViewSupervisor, IAbstractEditorFactory
	{
		IMVC_Host& host;

		AbstractEditor_MVC_View(IMVC_Host& _host, HINSTANCE _hInstance, cstr _commandLine): host(_host)
		{
			UNUSED(_commandLine);
			UNUSED(_hInstance);
		}

		void Free() override
		{
			delete this;
		}

		void Cast(void** ppInterface, cstr interfaceId) override
		{
			if (interfaceId == nullptr || *interfaceId == 0)
			{
				Throw(0, "%s: Cannot Cast. interfaceId was blank", __FUNCTION__);
			}

			cstr onlyKnownInterface = "Rococo::Abedit::IAbstractEditorFactory";

			if (ppInterface == nullptr)
			{
				Throw(0, "%s: Cannot Cast to %s. ppInterface was null. Only known interface is %s", __FUNCTION__, interfaceId, onlyKnownInterface);
			}

			if (Strings::Eq(interfaceId, onlyKnownInterface))
			{
				*ppInterface = static_cast<IAbstractEditorFactory*>(this);
				return;
			}

			Throw(0, "%s: Cannot Cast to %s. Only known interface is %s", __FUNCTION__, interfaceId, onlyKnownInterface);
		}

		IAbstractEditorSupervisor* CreateAbstractEditor() override
		{
			return new AbstractEditor(host);
		}
	};
}

// Abstract Editor
namespace Rococo::Abedit
{
	IMVC_ViewSupervisor* CreateAbstractEditor(IMVC_Host& host, HINSTANCE hInstance, cstr commandLine)
	{
		return new ANON::AbstractEditor_MVC_View(host, hInstance, commandLine);
	}
}