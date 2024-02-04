#include <rococo.mvc.h>
#include <rococo.abstract.editor.h>

using namespace Rococo;
using namespace Rococo::MVC;

namespace ANON
{
	struct CFGS_Controller: IMVC_ControllerSupervisor
	{
		AutoFree<Rococo::Abedit::IAbstractEditorSupervisor> editor;

		CFGS_Controller(IMVC_Host& _host, IMVC_View& view, cstr _commandLine)
		{
			UNUSED(_commandLine);
			UNUSED(_host);

			Abedit::IAbstractEditorFactory* editorFactory = nullptr;
			view.Cast((void**)&editorFactory, "Rococo::Abedit::IAbstractEditorFactory");
			if (!editorFactory)
			{
				Throw(0, "%s: Expected an IAbstractEditorFactory to be non-NULL", __FUNCTION__);
			}

			editor = editorFactory->CreateAbstractEditor();
			if (!editor)
			{
				Throw(0, "%s: Expected editorFactory->CreateAbstractEditor() to return a non-NULL pointer", __FUNCTION__);
			}
		}

		void Free() override
		{
			delete this;
		}

		bool IsRunning() const override
		{
			return false;
		}
	};
}

// Control-Flow Graph System
namespace Rococo::CFGS
{
	IMVC_ControllerSupervisor* CreateMVCControllerInternal(IMVC_Host& host, IMVC_View& view, cstr commandLine)
	{
		return new ANON::CFGS_Controller(host, view, commandLine);
	}
}